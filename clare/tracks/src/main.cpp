#include <Arduino.h>
#include <Encoder.h>
#include "SBUS.h"
#include "config.h"
#include <ArduinoLog.h>
#include "motor_utils.h"
#include "utils.h"
#include <math.h>

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
	// Only useful if you want to catch setup output
	//while(!Serial && !Serial.available()){}

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

// Simple manual differential steering where two channels control
// the two tracks separately
// returns a +/- pwm value
void dual_joystick(const int chL, const int chR, int &vL, int &vR){
	// Map to a symmetric pwm range centered over 0
	vL = map(chL, rc_minpwm, rc_maxpwm, -motor_maxpwm, motor_maxpwm);
	vR = map(chR, rc_minpwm, rc_maxpwm, -motor_maxpwm, motor_maxpwm);

	Log.verbose("Mapped RC intput to %d -> %d, %d -> %d\n", chL, vL, chR, vR);
}

// Differential steering with one joystick
// returns a +/- pwm value
void single_joystick(const int chx, const int chy, int &vL, int &vR){
	// Map raw channel input to -1, 1
	const double joy_x = mapf(chx, rc_minpwm, rc_maxpwm, -1.0, 1.0);
	const double joy_y = mapf(chy, rc_minpwm, rc_maxpwm, -1.0, 1.0);
	const double r_max = 1.0;

	// The model is a single joystick that can move in the x (left-right) and y (up-down)
	// plane. We use a polar coordinate system where, for a particular point (r, theta)
	// r should represent the speed the robot needs to travel at and theta should represent
	// the bearing of the robot.

	// References:
	//  https://robotics.stackexchange.com/questions/7829/how-to-program-a-three-wheel-omni
	//  https://robotics.stackexchange.com/questions/20347/how-do-i-convert-centre-returning-joystick-values-to-dual-hobby-motor-direction
	//  https://math.stackexchange.com/questions/553478/how-covert-joysitck-x-y-coordinates-to-robot-motor-speed

	// Convert to polar coordinates
	const double theta = atan2(joy_y, joy_x);
	const double r = sqrt(joy_x * joy_x + joy_y * joy_y);

	// Take our polar space the one where r is maximal. So we have a cirlce with diameter r, where r is our max
	// speed. That means all possible speeds should fall within this circle.
	// That all sounds great but there is an issue if we stop here. Imagine moving the joystick to the top right corner.
	// Doing the polar conversion would result in r being equal to sqrt(x^2+y^2) which is larger than the max
	// we set originally for r. E.g., if we take the max robot speed to be 5 and use that as the radius for our cirlce.
	// then position the stick at the top right corner, do the polar conversion, you would get sqrt(50) ~= 7.
	// So the robot would go faster with the stick in top right, than with the stick directly forward.
	// That is not intuitive. We can solve this as follows:

	// Let point J = (joy_x, joy_y) be the original position of the stick then let OJ be the line segment connecting the origin
	// with point J. Denote the length of OJ as b. Let C = (x_c, y_c) be the point where the OJ intersects our circle with radius
	// r. OC thus has length r. if J is on the X-axis or Y-axis then b == r. For all other locations of J outside
	// of the cirlc we have to scale J to lie on the circle.

	// Not 100% sure about the calculation below but it kinda makes sense and seems to work on the robot

	// Figure out by how much we should scale OJ
	double s = -1;
	if(r > r_max){
		//OJ is outside the circle, scale
		s = (r_max / r);
	} else {
		//OJ is inside the circle, no scale
		s = 1;
	}

	// So this is x_c and y_c
	// Note the triangles J_xOJ_y and c_xOc_y are similar
	const double joy_x_scaled = joy_x * s;
	const double joy_y_scaled = joy_y * s;

	// Turn into wheel velocities
	// Ensure between [-1,1], adding scaled components together can still exceed 1
	// TODO: not 100% happy I understand why clipping is still needed
	double rawL = clip(joy_y_scaled + joy_x_scaled, -1, 1);
	double rawR = clip(joy_y_scaled - joy_x_scaled, -1, 1);	
	
	// Handle backwards turning more intuitively
	// As is, pulling the stick to bottom right has the right
	// track turning backwards, leading to a reverse left turn
	// Swap track command around.
	if(rawL < 0 && rawR < 0){
		double tmp = rawL;
		rawL = rawR;
		rawR = tmp;
	}

	Log.verbose("chx=%d, chy=%d, joy_x=%D, joy_y=%D, r=%D, theta=%D, joy_x_scaled=%D, joy_y_scaled=%D, rawL=%D, rawR=%D\n", chx, chy, joy_x, joy_y, r, theta, joy_x_scaled, joy_y_scaled, rawL, rawR);

	// Convert to PMN range with negative equalling backwards
	vL = (int) (rawL * 255.0);
	vR = (int) (rawR * 255.0);
}

void rc_mode(){
  // look for a good SBUS packet from the receiver
  if(x8r.read(&rc_input[0], &rc_failSafe, &rc_lostFrame)){
	// Read the values for each channel
	const int ch1 = rc_input[0];
	const int ch2 = rc_input[1];
	const int ch3 = rc_input[2];
	const int ch4 = rc_input[3];
	//Log.verbose("receiving: %d, %d, %d, %d\n", ch1, ch2 ,ch3,ch4);

	int vL = -999;
	int vR = -999;
	//dual_joystick(ch1, ch3, vL, vR);
	single_joystick(ch4, ch1, vL, vR);
	
	// Handle deadband and reversing
	const int deadband = motor_deadband;
	
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
	
	Log.verbose("Motor speeds calculated as %d, %d\n", vL, vR);

	// Command the motors
	set_speed(vL, vR);

  } else {
	  if(rc_failSafe){
			Log.warning("FAILSAFE TRIGGERED\n");
			// Stop motors
			set_speed(0, 0);
	  } else if (rc_lostFrame){
		  	Log.warning("FRAME LOST\n");
	  }else{
		  	//Log.warning("No good packet received");
			delay(100);
	  }
  }
}

void loop() {
	rc_mode();
}
