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

const char* ssid       = "dummy";
const char* password   = "dummy";
const char* CAL_SECRET = "dummy";

const char* ntpServer = "pool.ntp.org";
const char* timezone = "GMT0BST,M3.5.0/1,M10.5.0/2";

const int LOCK_HR = 19;
const int LOCK_MIN = 30;
const int UNLOCK_HR = 8;
const int UNLOCK_MIN = 10;
const int WE_UNLOCK_HR = 10;
const int WE_UNLOCK_MIN = 00;

const int CAL_CHECK_INTERVAL = 1000*60*15;
const int ALARM_DURATION = 1000*60*3;
const int PHONE_DIST = 48.5;

const int SERVO_PIN = 10;
const int LED_DIN_PIN = 11;
const int LED_CIN_PIN = 12;
const int PIEZZO_PIN = 5;
const int MIC_SWITCH_PIN = 6;
const int CONT_SWITCH_PIN = 9;

VL53L1X dist_sensor;
//Adafruit_NeoPixel status_led = Adafruit_NeoPixel(1, PIN_NEOPIXEL, NEO_GRB);
Adafruit_DotStar strip = Adafruit_DotStar(20, LED_DIN_PIN, LED_CIN_PIN, DOTSTAR_BGR);

const uint32_t orange = strip.Color(255, 165, 0);
const uint32_t red = strip.Color(255, 0, 0);
const uint32_t green = strip.Color(0, 255, 0);
const uint32_t blue = strip.Color(0, 0, 255);

bool locked = false;
int servoIndex1 = -1;
int cal_res = -1;
int last_cal_check = -1;
long alarm_start = -1;
bool ble_override = false;

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

void lock_lid();
void unlock_lid();

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

      if (rxValue.length() > 0) {
        Serial.print("Received BLE Value: ");
        Serial.println(rxValue);

        if(rxValue.equals("U\n")){
          ble_override = true;
          unlock_lid();
        }else if(rxValue.equals("L\n")){
          ble_override = true;
          lock_lid();
        }else if(rxValue.equals("C\n")){
          ble_override = false;
        }else{
          Serial.print("Invalid BLE command, ignoring: '");
          Serial.print(rxValue);
          Serial.println("'");
        }
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
    if (deviceConnected) {
        pTxCharacteristic->setValue(&txValue, 1);
        pTxCharacteristic->notify();
        txValue++;
		delay(10); // bluetooth stack will go into congestion, if too many packets are sent
	}

    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("BLE start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
		  // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}

bool check_calendar(){
  if(WiFi.status() == WL_CONNECTED){
      HTTPClient http;

      String serverPath = "https://script.google.com/macros/s/" + String(CAL_SECRET) + "/exec";
      
      http.begin(serverPath.c_str());
      http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
      
      Serial.print("Making calendar request...");
      int httpResponseCode = http.GET();
      String payload = "";

      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        payload = http.getString();
        Serial.println("Response: '" + payload + "'");
      } else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      http.end();

      if(payload.equals("true")){
        Serial.println("TRUE");
        return true;
      }

    } else {
      Serial.println("WiFi Disconnected");
    }

    return false;
}

void printLocalTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.print("Local time: ");
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void show_colour(const uint32_t c){
  strip.fill(c);
  strip.show();
}


void connect_to_wifi(){
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" CONNECTED");

  //init and get the time
  configTzTime(timezone, ntpServer);
}


void setup_dist_sensor(){
  dist_sensor.setTimeout(500);
  while (!dist_sensor.init()) {
    Serial.println("Failed to detect and initialize sensor!");
    delay(1000);
  }

  dist_sensor.setDistanceMode(VL53L1X::Short);
  dist_sensor.setMeasurementTimingBudget(20000);
  dist_sensor.startContinuous(50);
}

void setup_servo(){
  ESP32_ISR_Servos.useTimer(3);
  servoIndex1 = ESP32_ISR_Servos.setupServo(SERVO_PIN, 500, 2500);
  
  if (servoIndex1 != -1){
    Serial.println(F("Servo setup OK"));
  }  else {
    Serial.println(F("Servo setup failed"));
  }
}


void setup() {
  strip.begin();
  strip.setBrightness(50);
  strip.fill(orange);
  strip.show();

  int max_wait = 8;
  int i = 0;
  while(!Serial && i < max_wait) {
    delay(1000);
    ++i;
  }

  Serial.begin(115200);
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
  const int n = 20;
  const int thres = PHONE_DIST;
  const int pad = 2.5;
  float dist = 0;
  
  // Take an average over n
  for(int i=0; i < n; ++i){
    dist += dist_sensor.read();

    if (dist_sensor.timeoutOccurred()) {
      Serial.println("Warning: Distance sensor timeout");
    }
  }
  
  dist = dist / n;

  Serial.println("Distance is " + String(dist));

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

  for (int pos = 5; pos <= 180; pos += 1){
    ESP32_ISR_Servos.setPosition(servoIndex1,pos);
    delay(10);
  }

  locked = true;
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

  for (int pos = 180; pos >= 5; pos -= 1){
    ESP32_ISR_Servos.setPosition(servoIndex1,pos);
    delay(10);
  }

  locked = false;
  unlock_tone();
  show_colour(green);
}

bool is_locked(){
  return locked;
}

bool is_unlocked(){
  return !is_locked();
}

bool is_lock_time(const int now_h, const int now_m, const bool is_weekend){
  bool res = false;

  // Assumes never locking more than 24h

  const int lock_hr = LOCK_HR;
  const int lock_min = LOCK_MIN;
  const int unlock_hr = (is_weekend) ? WE_UNLOCK_HR : UNLOCK_HR;
  const int unlock_min = (is_weekend) ? WE_UNLOCK_MIN : UNLOCK_MIN;

  // Are we locking for less than an hour
  if ((now_h == lock_hr) && (lock_hr == unlock_hr)){
    res = ((lock_min <= now_m) && (now_m <= unlock_min));
  
  // We span midnight
  } else if(lock_hr > unlock_hr){
    // cur is after lock but before midnight
    if((now_h > lock_hr) && (now_h > unlock_hr)){
      res = true;
    // cur is after lock but before midnight, mins matter
    }else if (((now_h == lock_hr) && (now_m >= lock_min)) && (now_h > unlock_hr)){
      res = true;
    // cur is after lock and after midnight
    }else if((now_h < lock_hr) && (now_h < unlock_hr)){
      res = true;
    // cur is after lock and after midnight, mins matter
    }else if ((now_h < lock_hr) && ((now_h == unlock_hr) && (now_m <= unlock_min))){
      res = true;
    } else {
      // Stay unlocked
      res = false;
    }
  // We dont span midnight
  } else {
    res = ((now_h >= lock_hr) && (now_m >= lock_min)) && ((now_h <= unlock_hr) && (now_m <= unlock_min));
  }

  Serial.print("Time locking calc: now_h=" + String(now_h) + " now_m=" + String(now_m) + " res=" + String(res));

  return res;
}

bool is_time_to_lock(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Error: Failed to obtain time");
    return false;
  }

  // Dont hammer the calendar
  if((last_cal_check < 0) || ((millis() - last_cal_check) > CAL_CHECK_INTERVAL)){
    cal_res = check_calendar();
    last_cal_check = millis();
  }

  if (cal_res){
    Serial.println("Today is a phone locking day!");
  } else {
    // Today is not a phone locking day, rejoice
    return false;
  }

  // Check if today is a weekend (Saturday or Sunday)
  bool isWeekend = (timeinfo.tm_wday == 0 || timeinfo.tm_wday == 6);
  bool res = is_lock_time(timeinfo.tm_hour, timeinfo.tm_min, isWeekend);

  if(res){
    Serial.println("Time to lock!");
  }else{
    Serial.println("Not time to lock yet");
  }

  return res;
}

bool is_time_to_unlock(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Error: Failed to obtain time");
    return false;
  }

  // Check if today is a weekend (Saturday or Sunday)
  bool isWeekend = (timeinfo.tm_wday == 0 || timeinfo.tm_wday == 6);
  bool res = ! is_lock_time(timeinfo.tm_hour, timeinfo.tm_min, isWeekend);

  if(res){
    Serial.println("Time to unlock");
  }else{
    Serial.println("Not time to unlock yet");
  }

  return res;
}

void alarm(){
  show_colour(red);
  alarm_tone();
}

void loop_main() {
  printLocalTime();

  iter_ble();
  delay(5000);

  // User (phone) override
  if(ble_override){
    Serial.println("Somebody using BLE to control box, ignoring std logic");
    return;
  }

  if(is_unlocked() && is_time_to_lock()){
    if (is_lid_closed() && is_phone_present_switch()) {
      lock_lid();
      alarm_start = -1;
      Serial.println("Box locked !");
    } else {
      Serial.println("ERROR: Time to lock but lid is open or phone not detected");
    
      // Check if the alarm needs to be started or is currently active within ALARM_DURATION.
      if (alarm_start < 0) {
          // Alarm has not started yet, initialize it.
          Serial.println("Sounding alarm");
          alarm_start = millis();
          alarm();  // Function to trigger the alarm
      } else if ((millis() - alarm_start) < ALARM_DURATION) {
          // Alarm is currently active
          Serial.println("Sounding alarm");
          alarm();  // Continue sounding the alarm
      } else {
          // Alarm duration has passed, give up on the alarm.
          Serial.println("Alarm not listened to, giving up");
      }
  
    }
  } else if(is_locked() && is_time_to_unlock()){
    unlock_lid();
    Serial.println("Box unlocked !");
  } else {
    // nothing to do
  }
}

void loop_test(){
  Serial.print("is_unlocked: ");
  Serial.println(is_unlocked());
  
  Serial.print("is time to lock: ");
  Serial.println(is_time_to_lock());

  Serial.print("is lid closed: ");
  Serial.println(is_lid_closed());

  Serial.print("is phone present: ");
  Serial.println(is_phone_present());

  Serial.print("is phone present dist: ");
  Serial.println(is_phone_present_dist());

  Serial.print("is phone present switch: ");
  Serial.println(is_phone_present_switch());

  Serial.println("--------------------------");

  lock_lid();
  delay(2000);   
  unlock_lid();

  iter_ble();
}

void loop_ble(){
  iter_ble();
}

void loop_caltest(){
  check_calendar();
  delay(3000);
}

void loop(){
  loop_main();
  //loop_test();
  //loop_ble();
  //loop_caltest();
}
