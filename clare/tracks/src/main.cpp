#include <Arduino.h>
#include <Encoder.h>
#include "SBUS.h"
#include "config.h"
#include <ArduinoLog.h>
#include "motor_utils.h"

// Motor variables
Encoder encA(motor_encA1, motor_encA2);
Encoder encB(motor_encB1, motor_encB2);
MotorDir directionL;
MotorDir directionR;

// Sbus object on hardware serial port
SBUS x8r(Serial5);

// channel, fail safe, and lost frames data
uint16_t rc_input[16];
bool rc_failSafe;
bool rc_lostFrame;

void setup_motors(){
	// Turn on motors and set to forward
	MotorDir directionL = fwd;
	MotorDir directionR = fwd;
	init_motors(directionL, directionR);
}

void setup_rc(){
	x8r.begin();
}

void setup_logging(){
	Serial.begin(115200);
	while(!Serial && !Serial.available()){}
	// Available levels are:
    // LOG_LEVEL_SILENT, LOG_LEVEL_FATAL, LOG_LEVEL_ERROR, LOG_LEVEL_WARNING, LOG_LEVEL_NOTICE, LOG_LEVEL_TRACE, LOG_LEVEL_VERBOSE
    Log.begin(LOG_LEVEL_VERBOSE, &Serial);
    //Log.begin(LOG_LEVEL_NOTICE, &Serial);
}

void setup() {
	setup_logging();
	setup_motors();
	setup_rc();
	Log.notice("Setup done\n");
}

void rc_mode(){
  // look for a good SBUS packet from the receiver
  if(x8r.read(&rc_input[0], &rc_failSafe, &rc_lostFrame)){
	// Read the values for each channel
	const int ch1 = rc_input[0];
	const int ch2 = rc_input[1];
	const int ch3 = rc_input[2];
	const int ch4 = rc_input[3];
	Log.verbose("receiving: %d, %d, %d, %d\n", ch1, ch2 ,ch3,ch4);

	// Map to a symmetric range
	int vL = map(ch1, rc_minpwm, rc_maxpwm, -motor_maxpwm, motor_maxpwm);
	int vR = map(ch3, rc_minpwm, rc_maxpwm, -motor_maxpwm, motor_maxpwm);
	const int deadband = motor_deadband;

	Log.notice("Mapped RC intput to %d -> %d, %d -> %d\n", ch1, vL, ch2, vR);

	if((-deadband <= vL) && (vL <= deadband)){
		vL = 0;
	}else if(vL < -deadband) {
		vL = abs(vL);
		if(directionL == fwd){
			set_motor(left, reverse);
			directionL = reverse;
		}
	} else {
		if(directionL == reverse){
			set_motor(left, fwd);
			directionL = fwd;
		}
	}

	if((-deadband <= vR) && (vR <= deadband)){
		vR = 0;
	}else if(vR < -deadband) {
		vR = abs(vR);
		if(directionR == fwd){
			set_motor(right, reverse);
			directionR = reverse;
		}
	} else {
		if(directionR == reverse){
			set_motor(right, fwd);
			directionR = fwd;
		}
	}

	Log.notice("set speed to %d, %d\n", vL, vR);
	set_speed(vL, vR);

  } else {
	  if(rc_failSafe){
			Log.warning("FAILSAFE TRIGGERED\n");
			// Stop motors
			set_speed(0, 0);
	  } else if (rc_lostFrame){
		  	Log.warning("FRAME LOST\n");
	  }else{
		  	Log.warning("No good packet received");
	  }
  }
}

void loop() {
	rc_mode();
}
