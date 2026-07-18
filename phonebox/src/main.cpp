#include <Wire.h>
#include <VL53L1X.h>
#include <Adafruit_DotStar.h>
#include <SPI.h>
#include "pitches.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "time.h"
#include "ESP32_ISR_Servo.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Preferences.h>

Preferences prefs;

const char* ssid       = "";
const char* password   = "";
const char* CAL_SECRET = "";

const char* ntpServer = "pool.ntp.org";
const char* timezone = "GMT0BST,M3.5.0/1,M10.5.0/2";

const int LOCK_HR = 19;
const int LOCK_MIN = 30;
const int UNLOCK_HR = 8;
const int UNLOCK_MIN = 10;
const int WE_UNLOCK_HR = 10;
const int WE_UNLOCK_MIN = 00;

const int CAL_CHECK_INTERVAL = 1000*60*15;
const int CAL_RETRY_INTERVAL = 1000*60;       // retry sooner after a failed/unknown check
const int ALARM_DURATION = 1000*60*3;
constexpr float PHONE_DIST = 48.5f;

const int SERVO_PIN = 10;
const int LED_DIN_PIN = 11;
const int LED_CIN_PIN = 12;
const int PIEZZO_PIN = 5;
const int MIC_SWITCH_PIN = 6;
const int CONT_SWITCH_PIN = 9;

VL53L1X dist_sensor;
bool dist_sensor_ok = false;
//Adafruit_NeoPixel status_led = Adafruit_NeoPixel(1, PIN_NEOPIXEL, NEO_GRB);
Adafruit_DotStar strip = Adafruit_DotStar(20, LED_DIN_PIN, LED_CIN_PIN, DOTSTAR_BGR);

const uint32_t orange = strip.Color(255, 165, 0);
const uint32_t red = strip.Color(255, 0, 0);
const uint32_t green = strip.Color(0, 255, 0);
const uint32_t blue = strip.Color(0, 0, 255);

bool locked = false;
int servoIndex1 = -1;

// Tri-state calendar result. CAL_UNKNOWN means we couldn't reach the calendar
// (e.g. WiFi down, HTTP failure, malformed response) - distinct from a real "no".
enum CalResult { CAL_UNKNOWN = -1, CAL_NO = 0, CAL_YES = 1 };

// Last *successful* calendar answer, scoped to a specific (year, day-of-year)
// so we don't accidentally reuse yesterday's answer - or, more subtly, an
// answer from the same yday in a previous year after a long power-off or
// across new-year. Persisted to flash so a reboot during a WiFi outage
// doesn't lose the schedule for today.
bool cached_cal_res = false;
int  cached_cal_yday = -1;
int  cached_cal_year = -1;
bool cal_cache_loaded = false;

unsigned long last_cal_check = 0;
bool last_cal_check_valid = false;

unsigned long alarm_start = 0;
bool alarm_active = false;
bool alarm_gave_up = false;

unsigned long tamper_alarm_start = 0;
bool tamper_alarm_active = false;
bool tamper_gave_up = false;

bool ble_override = false;

// BLE callbacks run on the BLE stack task. Doing slow work there (servo
// sweeps with delay(), NVS writes) starves BLE and races with the main loop
// which also touches `locked`, the servo, and `prefs`. Instead, the callback
// just records the requested command and the main loop acts on it on its own
// task. Single-byte writes are atomic on the ESP32 so a plain `volatile char`
// is sufficient; 0 means "no pending command".
volatile char pending_ble_cmd = 0;

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

void lock_lid();
void unlock_lid();
void reset_lock_alarm_state();
void printLocalTime();
void printLocalTime(const struct tm& timeinfo);

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String rxValue = pCharacteristic->getValue().c_str();

      if (rxValue.length() == 0) return;

      Serial.print("Received BLE Value: ");
      Serial.println(rxValue);

      rxValue.trim();
      rxValue.toUpperCase();

      // Don't do real work on the BLE task: just hand a single-char request
      // to the main loop. Anything else (servo, NVS, alarm) racing with
      // loop_main() is asking for trouble.
      if (rxValue.equals("L") || rxValue.equals("U") || rxValue.equals("C")) {
        pending_ble_cmd = rxValue.charAt(0);
      } else {
        Serial.print("Invalid BLE command, ignoring: '");
        Serial.print(rxValue);
        Serial.println("'");
      }
    }
};


void setup_ble() {
  // Create the BLE Device
  BLEDevice::init("UART Service");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
										CHARACTERISTIC_UUID_TX,
										BLECharacteristic::PROPERTY_NOTIFY
									);
                      
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
											 CHARACTERISTIC_UUID_RX,
											BLECharacteristic::PROPERTY_WRITE
										);

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void iter_ble(){
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        // No need to wait here; startAdvertising() does not require a
        // post-disconnect grace period. The 500 ms delay copy-pasted from
        // the Nordic UART example just delays tamper detection.
        pServer->startAdvertising(); // restart advertising
        Serial.println("BLE start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }
}

// Process any BLE command queued by MyCallbacks::onWrite. Runs on the main
// task so servo/NVS/locked are only ever touched by one task.
// Only writes ble_override to NVS when its value actually changes (avoids
// pointless flash wear when the user hammers the same command).
void process_pending_ble_cmd(){
  char cmd = pending_ble_cmd;
  if (cmd == 0) return;
  pending_ble_cmd = 0;

  bool prev_override = ble_override;

  switch (cmd) {
    case 'L':
      ble_override = true;   // enter manual override: stay locked until C
      lock_lid();
      break;
    case 'U':
      ble_override = true;   // enter manual override: stay unlocked until C
      unlock_lid();
      break;
    case 'C':
      ble_override = false;  // exit override only; keep current state
      // Reset stale alarm timers: if the lock-time alarm had started before
      // the override was set, alarm_start is now hours old. Leaving it
      // means the next slow_tick immediately "gives up" without ever
      // sounding the alarm again. Fresh state lets it restart cleanly.
      reset_lock_alarm_state();
      Serial.println("BLE override cleared; resuming time-based logic");
      break;
    default:
      // Should never happen - callback already filtered.
      return;
  }

  if (ble_override != prev_override) {
    prefs.putBool("ble_override", ble_override);
  }
}

CalResult check_calendar(){
  if(WiFi.status() != WL_CONNECTED){
    Serial.println("WiFi Disconnected; calendar result UNKNOWN");
    return CAL_UNKNOWN;
  }

  HTTPClient http;
  String serverPath = "https://script.google.com/macros/s/" + String(CAL_SECRET) + "/exec";

  http.begin(serverPath.c_str());
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  // Fail fast: a slow/down AP stalls the entire main loop (no tamper, no
  // BLE handling) for the duration of this call. CAL_RETRY_INTERVAL (1 min)
  // already retries cheaply, so a short timeout is strictly better than a
  // long stall. Healthy Apps Script responses complete well under this.
  http.setTimeout(4000);

  Serial.print("Making calendar request...");
  int httpResponseCode = http.GET();
  String payload = "";
  CalResult result = CAL_UNKNOWN;

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
    Serial.println("Response: '" + payload + "'");

    // Be liberal in what we accept: Apps Script responses can come back with
    // surrounding whitespace, BOMs, or capitalisation differences.
    payload.trim();
    if (payload.equalsIgnoreCase("true")) {
      result = CAL_YES;
    } else if (payload.equalsIgnoreCase("false")) {
      result = CAL_NO;
    } else {
      Serial.println("Calendar response malformed; treating as UNKNOWN");
      result = CAL_UNKNOWN;
    }
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
    // HTTP error -> we genuinely don't know, don't cache as "no"
    result = CAL_UNKNOWN;
  }
  http.end();

  return result;
}

void printLocalTime(const struct tm& timeinfo) {
  Serial.print("Local time: ");
  // Print's println(struct tm*, ...) overload isn't const-correct, but it
  // only reads from the struct, so the cast is safe.
  Serial.println(const_cast<struct tm*>(&timeinfo), "%A, %B %d %Y %H:%M:%S");
}

void printLocalTime() {
  struct tm timeinfo;
  // Non-blocking: don't waste 5s in the main loop just to print the time.
  if(!getLocalTime(&timeinfo, 0)){
    Serial.println("Failed to obtain time");
    return;
  }
  printLocalTime(timeinfo);
}

void show_colour(const uint32_t c){
  strip.fill(c);
  strip.show();
}


void connect_to_wifi(){
  // Fully non-blocking. Busy-waiting here used to block setup() for up to
  // 20s, during which loop() hadn't started yet, so tamper detection and
  // BLE override were dead. WiFi.setAutoReconnect(true) plus ensure_wifi()
  // finish the handshake in the background, and ensure_wifi() also calls
  // configTzTime() exactly once when WiFi first comes up.
  Serial.printf("Connecting to %s (non-blocking)\n", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  // We pass ssid/password explicitly on every boot, so persisting them to
  // flash just wears NVS for no benefit.
  WiFi.persistent(false);
  WiFi.begin(ssid, password);
}

// Fully non-blocking reconnect kick. Safe to call from the main loop;
// rate-limited so we don't hammer the radio while the AP is down.
// Also guarantees configTzTime() has run at least once after WiFi has
// ever been up (in case the boot-time connect timed out and auto-reconnect
// brought WiFi up later in the background).
void ensure_wifi(){
  static bool time_configured = false;

  if (WiFi.status() == WL_CONNECTED) {
    if (!time_configured) {
      configTzTime(timezone, ntpServer);
      time_configured = true;
    }
    return;
  }

  static unsigned long last_attempt = 0;
  static bool first_attempt = true;
  unsigned long now = millis();
  if (!first_attempt && (now - last_attempt) < 30000UL) return;
  first_attempt = false;
  last_attempt = now;

  // Trigger a reconnect attempt and return immediately. We rely on
  // WiFi.setAutoReconnect(true) (set in connect_to_wifi) to actually finish
  // the handshake in the background. Busy-waiting here used to block the
  // main loop for up to 5s, starving tamper detection and BLE handling.
  Serial.println("WiFi disconnected; kicking reconnect (non-blocking)");
  WiFi.reconnect();
}


void setup_dist_sensor(){
  dist_sensor.setTimeout(500);

  // Try a bounded number of times. If the sensor is missing or wedged at boot
  // we still want BLE/WiFi/time-based locking to come up; is_phone_present()
  // only uses the switch anyway, so the dist sensor is non-critical.
  const int max_attempts = 5;
  for (int i = 0; i < max_attempts; ++i) {
    if (dist_sensor.init()) {
      dist_sensor_ok = true;
      break;
    }
    Serial.println("Failed to detect and initialize sensor!");
    delay(1000);
  }

  if (!dist_sensor_ok) {
    Serial.println("WARNING: distance sensor unavailable; continuing without it");
    return;
  }

  dist_sensor.setDistanceMode(VL53L1X::Short);
  dist_sensor.setMeasurementTimingBudget(20000);
  dist_sensor.startContinuous(50);
}

void unlock_servo(){
  for (int pos = 180; pos >= 5; pos -= 1){
    ESP32_ISR_Servos.setPosition(servoIndex1,pos);
    delay(10);
  }
  Serial.println("Servo unlocked");
}

void lock_servo(){
  for (int pos = 5; pos <= 180; pos += 1){
    ESP32_ISR_Servos.setPosition(servoIndex1,pos);
    delay(10);
  }
  Serial.println("Servo locked");
}

void setup_servo(){
  ESP32_ISR_Servos.useTimer(3);
  servoIndex1 = ESP32_ISR_Servos.setupServo(SERVO_PIN, 500, 2500);
  
  if (servoIndex1 != -1){
    Serial.println(F("Servo setup OK"));
    delay(20);
    // Restore servo position based on saved state
    if (locked) {
      lock_servo();
      Serial.println("Restored servo to locked position (180)");
    } else {
      unlock_servo();
      Serial.println("Restored servo to unlocked position (5)");
    }
  }  else {
    Serial.println(F("Servo setup failed"));
  }
}


void setup() {
  strip.begin();
  strip.setBrightness(50);
  strip.fill(orange);
  strip.show();

  // Bring up Serial FIRST so subsequent prints actually land on the wire.
  Serial.begin(115200);
  unsigned long serial_wait_start = millis();
  while (!Serial && (millis() - serial_wait_start) < 2000UL) {
    delay(50);
  }

  prefs.begin("phonebox", false);
  locked = prefs.getBool("locked_state", locked);
  ble_override = prefs.getBool("ble_override", ble_override);

  Serial.println("Locked state read from prefs at startup: " + String(locked));
  Serial.println("BLE override read from prefs at startup: " + String(ble_override));

  Wire.begin();
  Wire.setClock(400000); // use 400 kHz I2C

  setup_dist_sensor();
  setup_servo();

  pinMode(PIEZZO_PIN, OUTPUT);
  pinMode(MIC_SWITCH_PIN, INPUT);
  pinMode(CONT_SWITCH_PIN, INPUT);

  setup_ble();
  connect_to_wifi();

  printLocalTime();

  strip.fill(blue);
  strip.show();
}

bool is_lid_closed(){
  return digitalRead(CONT_SWITCH_PIN) > 0;
}

bool is_phone_present_switch(){
  return digitalRead(MIC_SWITCH_PIN) > 0;
}

bool is_phone_present_dist(){
  if (!dist_sensor_ok) {
    Serial.println("Distance sensor unavailable; skipping distance check");
    return false;
  }

  const int n = 20;
  const float thres = PHONE_DIST;
  const float pad = 2.5f;
  float dist_sum = 0;
  int valid = 0;

  // Average over n samples, skipping any reads where the sensor reported a
  // timeout. read() returns a sentinel (~65535) on timeout that would otherwise
  // poison the mean and make a present phone read as "absent".
  // Each read can block up to ~50 ms (matches startContinuous(50)), so this
  // loop can take ~1s in the worst case.
  for(int i=0; i < n; ++i){
    uint16_t r = dist_sensor.read();
    if (dist_sensor.timeoutOccurred()) {
      Serial.println("Warning: Distance sensor timeout");
      continue;
    }
    dist_sum += r;
    valid++;
  }

  if (valid == 0) {
    Serial.println("Warning: Distance sensor returned no valid reads");
    return false;
  }

  float dist = dist_sum / valid;

  Serial.println("Distance is " + String(dist) + " (valid=" + String(valid) + "/" + String(n) + ")");

  if (((dist-pad) <= thres) && ((dist+pad) >= thres)){
    return true;
  }else{
    Serial.println("Warning: phone not (distance) detected");
  }

  return false;
}

bool is_phone_present(){
  //return is_phone_present_dist() && is_phone_present_switch();
  return is_phone_present_switch();
}

void play_melody(int melody[], int durations[], int len){
  for (int thisNote = 0; thisNote < len; thisNote++) {
    // to calculate the note duration, take one second divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / durations[thisNote];
    tone(PIEZZO_PIN, melody[thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(PIEZZO_PIN);
  }
}

void unlock_tone(){
  int melody[] = {
    NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
  };

  // note durations: 4 = quarter note, 8 = eighth note, etc.:
  int durations[] = {
    4, 8, 8, 4, 4, 4, 4, 4
  };

  play_melody(melody, durations, 8);
}

void lock_tone(){
  int melody[] = {
    NOTE_C4, NOTE_B3, 0, NOTE_G3, NOTE_A3, NOTE_G3, NOTE_G3, NOTE_C4
  };

  // note durations: 4 = quarter note, 8 = eighth note, etc.:
  int durations[] = {
    4, 4, 4, 4, 4, 8, 8, 4
  };

  play_melody(melody, durations, 8);
}

void alarm_tone(){
  int melody[] = {
    NOTE_C4, NOTE_C4, NOTE_C4, NOTE_C4, NOTE_C4, NOTE_C4, NOTE_C4, NOTE_C4
  };

  // note durations: 4 = quarter note, 8 = eighth note, etc.:
  int durations[] = {
    4, 4, 4, 4, 4, 4, 4, 4
  };

  play_melody(melody, durations, 8);
}

void lock_lid(){
  if(locked){
    Serial.println("Asked to lock, but locked is true, ignoring");
    return;
  } else {
    Serial.println("Locking lid");
  }

  lock_servo();

  locked = true;
  prefs.putBool("locked_state", locked);
  lock_tone();
  show_colour(red);
}

void unlock_lid(){
  if(!locked){
    Serial.println("Asked to unlock, but locked is false, ignoring");
    return;
  } else {
    Serial.println("Unlocking lid");
  }

  unlock_servo();

  locked = false;
  prefs.putBool("locked_state", locked);
  unlock_tone();
  show_colour(green);
}

bool is_locked(){
  return locked;
}

bool is_unlocked(){
  return !is_locked();
}

// Pure modular-arithmetic test: are we currently inside the daily lock window?
// Does NOT consult the calendar; callers combine this with is_locking_day()
// when calendar gating is required.
bool in_lock_window(const int now_h, const int now_m, const bool is_weekend){
  // Assumes never locking more than 24h.
  //
  // Treat each time as minutes-since-midnight, then collapse onto a single
  // "minutes since the lock instant" axis modulo 24h. The lock window is
  // simply [0, window_len). This handles all four cases in one expression:
  // span-midnight, no-span, same-hour, and the [0, 1)-minute boundary at
  // unlock_min, which the previous branchy version got subtly wrong.
  const int lock_hr   = LOCK_HR;
  const int lock_min  = LOCK_MIN;
  const int unlock_hr = (is_weekend) ? WE_UNLOCK_HR : UNLOCK_HR;
  const int unlock_min = (is_weekend) ? WE_UNLOCK_MIN : UNLOCK_MIN;

  const int day_min     = 24 * 60;
  const int lock_total  = lock_hr   * 60 + lock_min;
  const int unlock_total = unlock_hr * 60 + unlock_min;
  const int now_total   = now_h     * 60 + now_m;

  const int since_lock = ((now_total - lock_total) % day_min + day_min) % day_min;
  const int window_len = ((unlock_total - lock_total) % day_min + day_min) % day_min;

  // window_len == 0 means lock_time == unlock_time, which we interpret as
  // "never locked" rather than "locked for 24h".
  return (window_len > 0) && (since_lock < window_len);
}

bool cache_matches_day_key(const int day_year, const int day_yday) {
  return (cached_cal_yday == day_yday) && (cached_cal_year == day_year);
}

void ensure_calendar_cache_loaded() {
  if (cal_cache_loaded) return;
  cached_cal_yday = prefs.getInt("cal_yday", -1);
  cached_cal_year = prefs.getInt("cal_year", -1);
  cached_cal_res  = prefs.getBool("cal_res", false);
  cal_cache_loaded = true;
}

bool is_leap_tm_year(const int tm_year) {
  const int year = tm_year + 1900;
  return ((year % 4) == 0 && (year % 100) != 0) || ((year % 400) == 0);
}

int days_in_tm_year(const int tm_year) {
  return is_leap_tm_year(tm_year) ? 366 : 365;
}

void get_prev_day_key(const struct tm& timeinfo, int& day_year, int& day_yday) {
  if (timeinfo.tm_yday > 0) {
    day_year = timeinfo.tm_year;
    day_yday = timeinfo.tm_yday - 1;
    return;
  }

  day_year = timeinfo.tm_year - 1;
  day_yday = days_in_tm_year(day_year) - 1;
}

bool is_before_unlock_time(const struct tm& timeinfo, const bool is_weekend) {
  const int unlock_hr = is_weekend ? WE_UNLOCK_HR : UNLOCK_HR;
  const int unlock_min = is_weekend ? WE_UNLOCK_MIN : UNLOCK_MIN;
  if (timeinfo.tm_hour < unlock_hr) return true;
  if (timeinfo.tm_hour > unlock_hr) return false;
  return timeinfo.tm_min < unlock_min;
}

// Refresh the persisted calendar cache if (a) we've never checked,
// (b) we already have today's answer but it's getting stale, or
// (c) we DON'T have today's answer and the retry throttle has expired.
// Transient HTTP/WiFi failures must NOT poison the cache with a false
// "no" - only definitive CAL_YES/CAL_NO answers update it.
void update_calendar_cache(const struct tm& timeinfo){
  // Lazy-load the persisted calendar cache once (survives reboots so a
  // power-cycle during a WiFi outage doesn't lose today's schedule).
  ensure_calendar_cache_loaded();

  // Match BOTH year and yday so we don't reuse e.g. day 100 from last year
  // after a long power-off.
  const bool today_known = cache_matches_day_key(timeinfo.tm_year, timeinfo.tm_yday);

  // Decide whether to issue a new calendar request:
  //  - We've never checked since boot, OR
  //  - We have a confirmed answer for today and it's getting stale (15 min), OR
  //  - We have NO confirmed answer for today, but we throttle these retries
  //    to once a minute so a WiFi outage doesn't trigger an HTTP call every
  //    loop iteration.
  const unsigned long since_last = millis() - last_cal_check;
  bool need_check;
  if (!last_cal_check_valid) {
    need_check = true;
  } else if (today_known) {
    need_check = since_last > (unsigned long)CAL_CHECK_INTERVAL;
  } else {
    need_check = since_last > (unsigned long)CAL_RETRY_INTERVAL;
  }

  if (!need_check) return;

  ensure_wifi();
  CalResult r = check_calendar();
  last_cal_check = millis();
  last_cal_check_valid = true;

  // Only update the cache on a definitive answer; transient WiFi/HTTP
  // failures must NOT poison the cache with a false "no".
  if (r == CAL_UNKNOWN) return;

  const bool new_res  = (r == CAL_YES);
  const int  new_yday = timeinfo.tm_yday;
  const int  new_year = timeinfo.tm_year;
  // Only hit NVS when the value actually changes, to avoid pointless
  // flash wear from the periodic 15-min refresh.
  if (new_yday != cached_cal_yday ||
      new_year != cached_cal_year ||
      new_res  != cached_cal_res) {
    cached_cal_yday = new_yday;
    cached_cal_year = new_year;
    cached_cal_res  = new_res;
    prefs.putInt("cal_yday", cached_cal_yday);
    prefs.putInt("cal_year", cached_cal_year);
    prefs.putBool("cal_res", cached_cal_res);
  }
}

// Pure cache lookup for a specific lock-window day. When we have
// no confirmed answer for that day (calendar unreachable since boot, etc.),
// fail SAFE and assume yes so the alarm can still fire. Flip the fallback
// to `false` if you'd rather fail open.
bool is_locking_day(const int day_year, const int day_yday){
  ensure_calendar_cache_loaded();
  const bool day_known = cache_matches_day_key(day_year, day_yday);
  if (day_known) {
    Serial.println(cached_cal_res
      ? "Lock-window day is a phone locking day!"
      : "Lock-window day is not a phone locking day");
    return cached_cal_res;
  }
  Serial.println("WARNING: Calendar unknown for lock-window day; defaulting to LOCKING DAY");
  return true;
}

void reset_lock_alarm_state() {
  alarm_active = false;
  alarm_gave_up = false;
}

void alarm(){
  show_colour(red);
  alarm_tone();
}

void detect_tamper_lid_open(){
  // If the box is logically locked but the lid sensor reports open, trigger alarm
  if (is_locked() && !is_lid_closed()) {
    if (!tamper_alarm_active) {
      Serial.println("TAMPER: Lid opened while locked");
      tamper_alarm_start = millis();
      tamper_alarm_active = true;
      tamper_gave_up = false;
      alarm();
    } else if ((millis() - tamper_alarm_start) < (unsigned long)ALARM_DURATION) {
      alarm();
    } else if (!tamper_gave_up) {
      // Timeout expired; log the transition once then suppress until cleared.
      Serial.println("TAMPER alarm not listened to, giving up");
      tamper_gave_up = true;
    }
  } else {
    // Reset once tamper condition is no longer present
    tamper_alarm_active = false;
    tamper_gave_up = false;
  }
}

// Slow path: time printing, calendar check, lock/unlock decisions. Runs
// every SLOW_TICK_MS, NOT every loop iteration, so the fast path (tamper,
// BLE) stays responsive even when this work blocks (HTTP up to 10s).
void slow_tick() {
  // Must run before getLocalTime(): configTzTime() lives in ensure_wifi(),
  // and we used to only reach it via update_calendar_cache() after time
  // was already valid — a chicken-and-egg that prevented lock/alarm logic.
  ensure_wifi();

  struct tm timeinfo;
  // Non-blocking: rely on cached RTC time. After the first successful NTP
  // sync, getLocalTime() returns the RTC value instantly. We must NOT block
  // the loop for 5s every iteration when NTP is briefly unavailable.
  if(!getLocalTime(&timeinfo, 0)){
    Serial.println("Error: Failed to obtain time");
    return;
  }

  printLocalTime(timeinfo);

  // User (phone) override
  if(ble_override){
    Serial.println("Somebody using BLE to control box, ignoring std logic");
    return;
  }

  const bool isWeekend   = (timeinfo.tm_wday == 0 || timeinfo.tm_wday == 6);
  const bool in_window   = in_lock_window(timeinfo.tm_hour, timeinfo.tm_min, isWeekend);
  const bool in_morning_window = in_window && is_before_unlock_time(timeinfo, isWeekend);
  int lock_day_year = timeinfo.tm_year;
  int lock_day_yday = timeinfo.tm_yday;
  if (in_morning_window) {
    // During the overnight tail, the lock window still belongs to yesterday's
    // evening schedule, not the current civil date after midnight.
    get_prev_day_key(timeinfo, lock_day_year, lock_day_yday);
  }

  // Preserve yesterday's cached answer during the overnight segment so we
  // don't overwrite it with today's value before morning unlock.
  if (!in_morning_window) {
    update_calendar_cache(timeinfo);
  }

  // INTENTIONAL ASYMMETRY: locking requires being inside the daily window
  // AND it being a calendar-marked locking day; unlocking only requires
  // being outside the window (calendar is NOT consulted). This avoids
  // unlocking at midnight on a "free" day mid-overnight-lock, which the
  // user explicitly does not want.
  const bool want_locked = in_window && is_locking_day(lock_day_year, lock_day_yday);

  if(is_unlocked() && want_locked){
    if (is_lid_closed() && is_phone_present_switch()) {
      lock_lid();
      reset_lock_alarm_state();
      Serial.println("Box locked !");
    } else {
      Serial.println("ERROR: Time to lock but lid is open or phone not detected");

      if (!alarm_active) {
          Serial.println("Sounding alarm");
          alarm_start = millis();
          alarm_active = true;
          alarm_gave_up = false;
          alarm();
      } else if ((millis() - alarm_start) < (unsigned long)ALARM_DURATION) {
          Serial.println("Sounding alarm");
          alarm();
      } else if (!alarm_gave_up) {
          Serial.println("Alarm not listened to, giving up");
          alarm_gave_up = true;
          // Restore LED: alarm() set it red, but the box is still unlocked.
          // Without this the LED stays red indefinitely (unlock_lid() is
          // never called if locking never succeeded).
          show_colour(green);
      }
    }
  } else if(is_locked() && !in_window){
    unlock_lid();
    reset_lock_alarm_state();
    Serial.println("Box unlocked !");
  } else {
    // Outside the lock-transition window (or already locked).
    // Re-arm the alarm so it can fire fresh on the next lock window;
    // without this, once the alarm "gives up" it would never sound again
    // until a successful lock_lid() reset the sentinel.
    reset_lock_alarm_state();
  }
}

void loop_main() {
  static unsigned long last_slow_tick = 0;
  static bool slow_tick_primed = false;
  const unsigned long SLOW_TICK_MS = 5000UL;

  // Fast path: must run every iteration so tamper alarm fires within ~50ms
  // of the lid being opened, and BLE commands are handled promptly.
  ensure_wifi();
  detect_tamper_lid_open();
  iter_ble();
  process_pending_ble_cmd();

  unsigned long now = millis();
  if (!slow_tick_primed || (now - last_slow_tick) >= SLOW_TICK_MS) {
    last_slow_tick = now;
    slow_tick_primed = true;
    slow_tick();
  }

  // Yield briefly so we don't peg the CPU / starve other tasks. Short enough
  // that tamper detection latency stays well under 100 ms.
  delay(50);
}

void loop(){
  loop_main();
}
