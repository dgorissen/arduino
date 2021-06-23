#include <Arduino.h>

// Inspired from https://www.maxbotix.com/documents/Arduino%20Codes/HR-MaxSonars/HR_MaxSonar_PW_Chain.ino
// https://www.maxbotix.com/Arduino-Ultrasonic-Sensors-085/

// Baud rate
const int BAUD = 115200;
// If this pin is low then the sensors are turned off
const int rangingEnablePin = A9;
// Set to low/high depending on the state of the rangingEnablePin
const int triggerPin = A10;

const int numSensors = 3;
const int pwPins[numSensors] = {A2, A3, A4};
long pulses[numSensors];

void setup () {
  Serial.begin(BAUD);
  for(int i=0; i < numSensors; ++i){
    pinMode(pwPins[i], INPUT);
  }
}

void read_sensors(){
  // This pin outputs a pulse width representation of the distance
  // with a scale factor of 1uS per mm. Output range is 300uS for 300-mm to 5000uS for
  // 5000-mm. Pulse width output is +/- 1% of the serial data sent.
  for(int i=0; i < numSensors; ++i){
    pulses[i] = pulseIn(pwPins[i], HIGH);
  }
}

void print_ranges(){     
  for(int i=0; i < numSensors; ++i){
    Serial.print(pulses[i]);
    Serial.print(" ");
  }
  Serial.println(" ");
}

void loop () {
  bool enable = digitalRead(rangingEnablePin) == HIGH;
  // TODO: for testing
  enable = true;

  if(enable){
    digitalWrite(triggerPin,HIGH);
    read_sensors();
    print_ranges();
  } else {
    digitalWrite(triggerPin,LOW);    
  }

  delay(133);
}
