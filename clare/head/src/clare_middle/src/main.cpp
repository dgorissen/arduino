#include <Arduino.h>
#include <ros.h>
#include <std_msgs/Bool.h>
#include <std_msgs/Float32.h>
#include <std_msgs/Empty.h>
#include <clare_middle/GasMessage.h>
#include <clare_middle/UltrasoundMessage.h>

// Ref https://www.maxbotix.com/Arduino-Ultrasonic-Sensors-085/

// Baud rate
const long BAUD = 115200;

ros::NodeHandle nh;
std_msgs::Bool pir_msg;
clare_middle::GasMessage gas_msg;
clare_middle::UltrasoundMessage ultra_msg;
std_msgs::Float32 voltage_msg;

ros::Publisher pir_pub("clare/pir", &pir_msg);
ros::Publisher gas_pub("clare/gas", &gas_msg);
ros::Publisher ultra_pub("clare/ultra", &ultra_msg);
ros::Publisher voltage_pub("clare/voltage", &voltage_msg);

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
int prevPirVal = -1;

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

  // Setup timers to facilitate publishing at a regular interval
  // http://www.8bit-era.cz/arduino-timer-interrupts-calculator.html
  // TIMER 1 for interrupt frequency 0.5 Hz:
  cli(); // stop interrupts
  TCCR1A = 0; // set entire TCCR1A register to 0
  TCCR1B = 0; // same for TCCR1B
  TCNT1  = 0; // initialize counter value to 0
  // set compare match register for 0.5 Hz increments
  OCR1A = 31249; // = 16000000 / (1024 * 0.5) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12, CS11 and CS10 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (0 << CS11) | (1 << CS10);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei(); // allow interrupts
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

ISR(TIMER1_COMPA_vect){
  //interrupt commands for TIMER 1
  read_voltage();
  read_gasses();
  
  voltage_msg.data = voltVal;
  voltage_pub.publish(&voltage_msg);
  
  gas_msg.methane = mqVals[0];
  gas_msg.h2s = mqVals[1];
  gas_pub.publish(&gas_msg);
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

    ultra_msg.left = ranges[0];
    ultra_msg.middle = ranges[1];
    ultra_msg.right = ranges[2];
    ultra_pub.publish(&ultra_msg);

    delay(150);
  } else {
    digitalWrite(triggerPin,LOW);    
  }

  read_pir();

  if(pirVal != prevPirVal){
    pir_msg.data = pirVal > 0;
    pir_pub.publish(&pir_msg);
    pirVal = prevPirVal;
  }

  //print_vals();
  nh.spinOnce();
}
