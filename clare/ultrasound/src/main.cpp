#include <Arduino.h>

// Ref https://www.maxbotix.com/Arduino-Ultrasonic-Sensors-085/

// Baud rate
const long BAUD = 115200;

//--- Pin aliases for convenience
const int D2 = 2;
const int D3 = 3;
const int D4 = 4;
const int D5 = 5;
const int D6 = 6;

//--- Ultrasonics
// If this pin is low then the sensors are turned off
const int rangingEnablePin = D2;
// Set to low/high depending on the state of the rangingEnablePin
const int triggerPin = D3;
// Number of sensors
const int numUltraSensors = 3;
// Pin IDs
const int ultraPins[numUltraSensors] = {A7, A6, A5};
// Range values
int ranges[numUltraSensors];

//--- PIR Sensor
const int pirPin = D4;
int pirVal = -1;

//--- Voltage Sensor
const int voltPin = A2;
float voltVal = -1.0;

//-- Gas sensors
const int mqPins[2] = {A4, A3};
int mqVals[2] = {-1, -1};

void setup () {
  Serial.begin(BAUD);

  for(int i=0; i < numUltraSensors; ++i){
    pinMode(ultraPins[i], INPUT);
  }

  for(int i=0; i < 2; ++i){
    pinMode(mqPins[i], INPUT);
  }

  pinMode(pirPin, INPUT_PULLUP);
  pinMode(voltPin, INPUT);
}

void read_ranges(){
  for(int i=0; i < numUltraSensors; ++i){
    ranges[i] = analogRead(ultraPins[i]) * 5;
  }
}

void read_gasses(){
  for(int i=0; i < 2; ++i){
    mqVals[i] = analogRead(mqPins[i]);
  }
}

void read_voltage(){
  int val = analogRead(voltPin);
  const float R1 = 30000.0;
  const float R2 = 7500.0;
  float vOUT = (val * 5.0) / 1023.0;
  voltVal = vOUT / (R2/(R1+R2));
}

void read_pir(){
  pirVal = digitalRead(pirPin);
}

void print_vals(){
  char s[50];
  for(int i=0; i < numUltraSensors; ++i){
    sprintf(s,"U%d %d", i, ranges[i]);
    Serial.print(s);
    Serial.print(" ");
  }

  for(int i=0; i < 2; ++i){
    sprintf(s,"MQ%d %d", i, mqVals[i]);
    Serial.print(s);
    Serial.print(" ");
  }

  Serial.print("P ");
  Serial.print(pirVal);
  Serial.print(" V ");
  Serial.print(voltVal);
  Serial.println("");
}

void loop_test () {
  // float v = pulseIn(A7, HIGH);
  float v = analogRead(A7)*5;
  Serial.print("val=");
  Serial.print(v);
  Serial.println();
  delay(100);
}


void loop() {
  bool enableUltras = digitalRead(rangingEnablePin) == HIGH;
  // TODO: for testing
  enableUltras = true;

  if(enableUltras){
    digitalWrite(triggerPin,HIGH);
    delay(1);
    // digitalWrite(triggerPin,LOW);

    read_ranges();
    delay(133);
  } else {
    digitalWrite(triggerPin,LOW);    
  }

  read_voltage();
  read_pir();
  read_gasses();

  print_vals();
}
