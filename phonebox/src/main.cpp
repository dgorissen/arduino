#include <Wire.h>
#include <VL53L1X.h>
#include <ESP32Servo.h>
#include <Adafruit_DotStar.h>
#include <SPI.h>
#include "pitches.h"
#include <WiFi.h>
#include "time.h"

const char* ssid       = "YOUR_SSID";
const char* password   = "YOUR_PASS";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0*60*60;
const int   daylightOffset_sec = 3600;

const int SERVO_PIN = 5;
const int LED_DIN_PIN = 11;
const int LED_CIN_PIN = 12;
const int PIEZZO_PIN = 5;
const int MIC_SWITCH_PIN = 6;
const int CONT_SWITCH_PIN = 9;

VL53L1X dist_sensor;
//Adafruit_NeoPixel status_led = Adafruit_NeoPixel(1, PIN_NEOPIXEL, NEO_GRB);
Adafruit_DotStar strip = Adafruit_DotStar(20, LED_DIN_PIN, LED_CIN_PIN, DOTSTAR_BRG);
Servo lock_servo; 

const uint32_t orange = strip.Color(255, 165, 0);
const uint32_t red = strip.Color(255, 0, 0);
const uint32_t green = strip.Color(0, 255, 0);
const uint32_t blue = strip.Color(0, 0, 255);

bool locked = false;
long minss_since_start = 0;

void printLocalTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void show_colour(const uint32_t c){
  strip.fill(c);
  strip.show();
}

void show_colour(const int r, const int g, const int b){
  strip.fill(r, g, b);
  strip.show();
}

void connect_to_wifi(){
  //connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" CONNECTED");
  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void setup() {
  // Allow allocation of all timers
	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);

  strip.begin();
  strip.setBrightness(50);
  strip.fill(orange);
  strip.show();

  while(!Serial) {}
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000); // use 400 kHz I2C

  dist_sensor.setTimeout(500);
  while (!dist_sensor.init()) {
    Serial.println("Failed to detect and initialize sensor!");
    delay(1000);
  }

  dist_sensor.setDistanceMode(VL53L1X::Short);
  dist_sensor.setMeasurementTimingBudget(20000);
  dist_sensor.startContinuous(50);

  lock_servo.attach(SERVO_PIN, 500, 2500);
  lock_servo.setPeriodHertz(50);

  pinMode(PIEZZO_PIN, OUTPUT);
  pinMode(MIC_SWITCH_PIN, INPUT);
  pinMode(CONT_SWITCH_PIN, INPUT);

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
  const int n = 5;
  const int thres = 5;
  const int pad = 5;
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
    Serial.println("Warning: phone not detected");
  }

  return false;
}

bool is_phone_present(){
  return is_phone_present_dist() && is_phone_present_switch;
}

void play_melody(int melody[], int durations[], int len){
  for (int thisNote = 0; thisNote < len; thisNote++) {
    // to calculate the note duration, take one second divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / durations[thisNote];
    tone(8, melody[thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(8);
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
  if(locked) return;

  for (int pos = 0; pos <= 180; pos += 1){
    lock_servo.write(pos);
    delay(10);
  }

  locked = true;
}

void unlock_lid(){
  if(locked == false) return;

  for (int pos = 180; pos >= 0; pos -= 1){
    lock_servo.write(pos);
    delay(10);
  }

  locked = false;
}

bool is_locked(){
  return locked;
}

bool is_unlocked(){
  return !is_locked();
}

bool is_time_to_lock(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return false;
  }

  // dummy impl
  if (timeinfo.tm_sec < 30){
    return true;
  }else{
    return false;
  }
}

bool is_time_to_unlock(){
  // dummy impl
  return !is_time_to_lock();
}

void loop() {
  if(is_unlocked() && is_time_to_lock()){
    if (is_lid_closed() && is_phone_present()) {
      lock_lid();
      lock_tone();
      show_colour(red);
      Serial.println("Box locked");
    } else {
      Serial.println("ERROR: Time to lock but lid is open or phone not detected");
    }
  } else if(is_locked() && is_time_to_unlock()){
    unlock_lid();
    unlock_tone();
    show_colour(green);
  } else {
    // nothing to do
  }
  
  delay(1000);
}