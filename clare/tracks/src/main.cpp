#include <Arduino.h>
#include <Encoder.h>
#include "sbus.h"
#include "config.h"
#include <ArduinoLog.h>
#include "motor_utils.h"
#include "state.h"
#include "utils.h"
#include <math.h>
// So we can use a different serial port
// needs to happen before ros.h include
#define USE_TEENSY_HW_SERIAL
#include <ros.h>
#include <std_msgs/String.h>
#include <std_msgs/Empty.h>

// Fwd declarations
void set_headlights(bool b);

// ROS variables
// Wrapper class so we can use a custom serial port
class TeensyHardware5 : public ArduinoHardware {
  public:
  TeensyHardware5():ArduinoHardware(&Serial5, BAUD){};
};

ros::NodeHandle_<TeensyHardware5>  nh;
void command_callback(const std_msgs::String& input_msg);
ros::Subscriber<std_msgs::String> sub("track_cmds", command_callback);
std_msgs::String track_status_msg;
ros::Publisher track_status("track_status", &track_status_msg);

// Where to publish 
const String pubto = "ros";
//const String pubto = "serial"

// State
State state_out;
State state_in;

// Motor variables
Encoder encA(motor_encA1, motor_encA2);
Encoder encB(motor_encB1, motor_encB2);
long encA_pos = -999;
long encB_pos = -999;
MotorDir directionL;
MotorDir directionR;

// Sbus object on hardware serial port
SbusRx x8r(&Serial1);

// channel, fail safe, and lost frames data
uint16_t rc_input[16];
bool rc_failSafe;
bool rc_lostFrame;

void command_callback(const std_msgs::String& input_msg){
	Log.verbose("Received command from brain: '%s'\n", input_msg);
	bool res = State::parseState(input_msg.data, state_in);

	if(!res) {
		Log.error("Invalid command, failed to parse, ignored\n");
		state_in.reset();
	} else {
		Log.verbose("Successfully parsed command message\n");
	}
}

void setup_ros() {
  nh.initNode();
  nh.advertise(track_status);
  nh.subscribe(sub);
  //while(!nh.connected()) nh.spinOnce();
  //nh.loginfo("Startup complete");
}

void publish_ros(const String s) {
  track_status_msg.data = s.c_str();
  track_status.publish(&track_status_msg);
}

void setup_motors(){
	// Turn on motors and set to forward
	MotorDir directionL = fwd;
	MotorDir directionR = fwd;
	init_motors(directionL, directionR);
}

void setup_rc(){
	x8r.Begin();
}

void setup_logging(){
	Serial.begin(BAUD);
	// Only useful if you want to catch setup output
	//while(!Serial && !Serial.available()){}

	// Available levels are:
    // LOG_LEVEL_SILENT, LOG_LEVEL_FATAL, LOG_LEVEL_ERROR, LOG_LEVEL_WARNING,
	// LOG_LEVEL_NOTICE, LOG_LEVEL_TRACE, LOG_LEVEL_VERBOSE
    //Log.begin(LOG_LEVEL_VERBOSE, &Serial);
    Log.begin(LOG_LEVEL_NOTICE, &Serial);
}

// Publish a message to the outside world
void publish(const String s){
	if(pubto == "ros"){
		publish_ros(s);
	} else {
		Serial5.println(s);
	}
}

void setup_publish(){
	if(pubto == "ros"){
		setup_ros();
	} else {
		Serial5.begin(BAUD);
	}
}

bool read_headlights(){
	return digitalRead(headlights) == HIGH;
}

void set_headlights(bool b){
	if(b){
		digitalWrite(headlights, HIGH);
	}else{
		digitalWrite(headlights, LOW);
	}
}

void setup_headlights() {
	pinMode(headlights, OUTPUT);
	set_headlights(false);
}

void setup() {
	setup_logging();
	setup_publish();
	setup_motors();
	setup_rc();
	setup_headlights();
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
		s = 1.0;
	}

	// So this is x_c and y_c
	// Note the triangles J_xOJ_y and c_xOc_y are similar
	const double joy_x_scaled = joy_x * s;
	const double joy_y_scaled = joy_y * s;

	// Turn into wheel velocities
	// Ensure between [-1,1], adding scaled components together can still exceed 1
	// TODO: not 100% happy I understand why clipping is still needed
	double rawL = clip(joy_y_scaled + joy_x_scaled, -1.0, 1.0);
	double rawR = clip(joy_y_scaled - joy_x_scaled, -1.0, 1.0);	
	
	// Handle backwards turning more intuitively
	// As is, pulling the stick to bottom right has the right
	// track turning backwards, leading to a reverse left turn

	// Swap track command around.
	if(rawL < EPS && rawR < EPS){
		double tmp = rawL;
		rawL = rawR;
		rawR = tmp;
	}

	Log.verbose("chx=%d, chy=%d, joy_x=%D, joy_y=%D, r=%D, theta=%D, joy_x_scaled=%D, joy_y_scaled=%D, rawL=%D, rawR=%D\n", chx, chy, joy_x, joy_y, r, theta, joy_x_scaled, joy_y_scaled, rawL, rawR);

	// Convert to PMN range with negative equalling backwards
	vL = (int) (rawL * 255.0);
	vR = (int) (rawR * 255.0);
}

void set_motor_speeds(const int chA, const int chB, int &vL, int &vR) {
	//dual_joystick(chA, chB, vL, vR);
	single_joystick(chA, chB, vL, vR);
	
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
}

void rc_mode(){
  // look for a good SBUS packet from the receiver
  if (x8r.Read()) {
    if(x8r.failsafe()) {
		Log.warning("FAILSAFE TRIGGERED\n");
		// Stop motors
		set_speed(0, 0);
		// Update the state
		state_out.setMotors(0, 0);
		state_out.setMode(FAILSAFE);
		// Give some time
		delay(100);
	} else {
		// Read the values for each channel
		const int ch1 = x8r.rx_channels()[0];
		const int ch2 = x8r.rx_channels()[1];
		const int ch3 = x8r.rx_channels()[2];
		const int ch4 = x8r.rx_channels()[3];
		Log.verbose("receiving: %d, %d, %d, %d\n", ch1, ch2 ,ch3,ch4);

		int vL = -999;
		int vR = -999;
		set_motor_speeds(ch4, ch1, vL, vR);

		// Update the state
		state_out.setMode(MANUAL);
		state_out.setMotors(vL, vR);
	}
  } else {
	  Log.verbose("Failed to read a good RC packet");
	  delay(20);
  }
}

void read_encoders() {
  const long newA = encA.read();
  const long newB = encB.read();

  if(newA != encA_pos){
	  encA_pos = newA;
  }

  if(newB != encB_pos){
	  encB_pos = newB;
  }
}

void act_upon_commands() {
	// Set the control mode
	if(state_in.getMode() == AUTONOMOUS) {
		// TODO implement
		state_out.setMode(AUTONOMOUS);
	} else if(state_in.getMode() == MANUAL) {
		// Follow RC commands
		// This sets the mode and motor state_out fields internally
		rc_mode();
	} else {
		// Mode not specified, dont change anything
	}
	
	// set the headlights
	if(state_in.getHeadlights() == HL_ON) {
		set_headlights(true);
	} else if(state_in.getHeadlights() == HL_OFF) {
		set_headlights(false);
	} else {
		// not set, nothing to do
	}

	// All actions taken, clear all values
	state_in.reset();
}

void loop() {
	// Did we receive any instructions?
	if(state_in.isModified()) {
		// Act upon any commands
		act_upon_commands();
	} else {
		// No instructions, default to one manual iteration
		rc_mode();
	}
	
	// Set and publish the output state
	read_encoders();
	state_out.setEncoders(encA_pos, encB_pos);
	state_out.setHeadlights(read_headlights());

	// To avoid spamming, only publish if something changed
	if(state_out.isModified()) {
		const String s = state_out.serialise();
		publish(s);
		Log.notice("Published state: %s\n", s.c_str());
		state_out.clearStatus();
	} else {
		const String s = state_out.serialise();
		Log.verbose("State not changed/published: %s\n", s.c_str());
	}

	// Keep rosserial synced and process incomming msgs
	nh.spinOnce();
}
