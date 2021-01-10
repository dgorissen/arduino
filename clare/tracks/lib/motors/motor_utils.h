#include <Arduino.h>
#include "config.h"

enum MotorPos { left, right };
enum MotorDir { fwd, reverse };

void set_motor(const MotorPos p, const MotorDir d){
	if(p == left){
		if(d == fwd){
			digitalWrite(motor_in1, HIGH);
			digitalWrite(motor_in2, LOW);
		} else {
			digitalWrite(motor_in1, LOW);
			digitalWrite(motor_in2, HIGH);
		}
	} else {
		if(d == fwd){
			digitalWrite(motor_in3, HIGH);
			digitalWrite(motor_in4, LOW);
		} else {
			digitalWrite(motor_in3, LOW);
			digitalWrite(motor_in4, HIGH);
		}
	}
}

void set_speed(const int vA, const int vB){
   	analogWrite(motor_enA, vA);
	analogWrite(motor_enB, vB);
}

void init_motors(const MotorDir initA, const MotorDir initB){
	// Set all the motor control pins to outputs
	pinMode(motor_enA, OUTPUT);
	pinMode(motor_enB, OUTPUT);
	pinMode(motor_in1, OUTPUT);
	pinMode(motor_in2, OUTPUT);
	pinMode(motor_in3, OUTPUT);
	pinMode(motor_in4, OUTPUT);
	
	// Turn off motors - Initial state
	// digitalWrite(motor_in1, LOW);
	// digitalWrite(motor_in2, LOW);
	// digitalWrite(motor_in3, LOW);
	// digitalWrite(motor_in4, LOW);

	set_motor(left, initA);
	set_motor(right, initB);
}

// void test_motor(const int speed) {
// 	Serial.println("set speed");
// 	analogWrite(motor_enA, speed);
// 	analogWrite(motor_enB, speed);

// 	// Turn on motor A & B
// 	digitalWrite(motor_in1, HIGH);
// 	digitalWrite(motor_in2, LOW);
// 	digitalWrite(motor_in3, HIGH);
// 	digitalWrite(motor_in4, LOW);

// 	long oldPosition  = -999;

// 	for(int i=0; i < 20; ++i){
// 		long newPosition = encA.read();
// 		if (newPosition != oldPosition) {
// 			oldPosition = newPosition;
// 			Serial.println(newPosition);
// 		}
// 		delay(100);
// 	}
	
// 	Serial.println("reversing");
// 	// Now change motor directions
// 	digitalWrite(motor_in1, LOW);
// 	digitalWrite(motor_in2, HIGH);
// 	digitalWrite(motor_in3, LOW);
// 	digitalWrite(motor_in4, HIGH);
// 	delay(2000);

// 	 oldPosition  = -999;

// 	for(int i=0; i < 5; ++i){
// 		long newPosition = encA.read();
// 		if (newPosition != oldPosition) {
// 			oldPosition = newPosition;
// 			Serial.println(newPosition);
// 		}
// 		delay(100);
// 	}

// 	// Turn off motors
// 	digitalWrite(motor_in1, LOW);
// 	digitalWrite(motor_in2, LOW);
// 	digitalWrite(motor_in3, LOW);
// 	digitalWrite(motor_in4, LOW);
// }
