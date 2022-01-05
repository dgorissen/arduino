#include <Arduino.h>
#include <Encoder.h>
#include "sbus.h"
#include "config.h"
#include <ArduinoLog.h>
#include "motor_utils.h"
#include "utils.h"
#include <math.h>
// So we can use a different serial port
// needs to happen before ros.h include
#define USE_TEENSY_HW_SERIAL
#include <ros.h>
#include <std_msgs/String.h>
#include <std_msgs/Empty.h>
#include <std_msgs/Bool.h>
#include <std_msgs/Header.h>
#include <clare_tracks/EncoderMessage.h>
#include <clare_tracks/JoystickInput.h>
#include <clare_tracks/MotorSpeeds.h>

// Fwd declarations
void set_headlights(bool b);
void command_callback(const clare_tracks::JoystickInput& input_msg);
void headlight_callback(const std_msgs::Bool& input_msg);

// ROS variables
// Wrapper class so we can use a custom serial port
class TeensyHardware5 : public ArduinoHardware {
  public:
  TeensyHardware5():ArduinoHardware(&Serial5, BAUD){};
};

ros::NodeHandle_<TeensyHardware5> nh;
ros::Subscriber<clare_tracks::JoystickInput> joystick_sub("clare/tracks/joystick_input", command_callback);
ros::Subscriber<std_msgs::Bool> headlight_sub("clare/tracks/headlights", headlight_callback);

clare_tracks::EncoderMessage encoder_msg;
clare_tracks::MotorSpeeds motor_msg;

ros::Publisher encoder_pub("clare/tracks/encoders", &encoder_msg);
ros::Publisher motor_pub("clare/tracks/motor_speeds", &motor_msg);

// Where to publish 
const String pubto = "ros";
//const String pubto = "serial"

// State
clare_tracks::JoystickInput cur_input_cmd;
clare_tracks::JoystickInput last_input_cmd;
long last_command_ts = -999;

// Motor variables
Encoder encA(motor_encA1, motor_encA2);
Encoder encB(motor_encB1, motor_encB2);
long encA_pos = -999;
long encB_pos = -999;
long motors_moved = false;
MotorDir directionL;
MotorDir directionR;

// Sbus object on hardware serial port
SbusRx x8r(&Serial1);

// channel, fail safe, and lost frames data
uint16_t rc_input[16];
bool rc_failSafe;
bool rc_lostFrame;

void command_callback(const clare_tracks::JoystickInput& input_msg){
	Log.verbose("Received command");
	cur_input_cmd = input_msg;
	last_command_ts = millis();
}

void headlight_callback(const std_msgs::Bool& input_msg){
	Log.verbose("Received headlight command: '%s'\n", input_msg);
	set_headlights(input_msg.data);
}

void publish_motor_state(const int vL, const int vR, const Mode m) {
	motor_msg.left = vL;
	motor_msg.right = vR;

	if(m == RC_CONTROL) {
		motor_msg.status = "RC_CONTROL";
	} else if(m == SW_CONTROL) {
		motor_msg.status = "SW_CONTROL";
	} else if(m == FAILSAFE) {
		motor_msg.status = "FAILSAFE";
	} else {
		motor_msg.status = "UNKNOWN";
	}

	motor_pub.publish(&motor_msg);
}

void setup_ros() {
  nh.initNode();
  nh.advertise(encoder_pub);
  nh.advertise(motor_pub);
  nh.subscribe(headlight_sub);
  nh.subscribe(joystick_sub);
  //while(!nh.connected()) nh.spinOnce();
  //nh.loginfo("Startup complete");
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
    Log.begin(LOG_LEVEL_NOTICE, &Serial);
    //Log.begin(LOG_LEVEL_VERBOSE, &Serial);
}

// Publish a message to the outside world
void publish(const String s){
	if(pubto == "ros"){
		nh.loginfo(s.c_str());
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
void single_joystick(const int chx, const int chy, int &vL, int &vR, const bool scale = true){
	double joy_x, joy_y;

	if(scale) {
		// Map raw channel input to -1, 1
		joy_x = mapf(chx, rc_minpwm, rc_maxpwm, -1.0, 1.0);
		joy_y = mapf(chy, rc_minpwm, rc_maxpwm, -1.0, 1.0);
	} else {
		if(chx < -1 || chy < -1 || chx > 1 || chy > 1) {
			Log.error("Input channel values do not lie in [-1, 1], chx: %d chy: %d", chx, chy);
			// Dont try to recover, return
			return;
		}
		joy_x = chx;
		joy_y = chy;
	}
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

	// Convert to PWM range with negative equalling backwards
	vL = (int) (rawL * 255.0);
	vR = (int) (rawR * 255.0);
}

void set_motor_speeds(const int chA, const int chB, int &vL, int &vR, const bool scale=true) {
	//dual_joystick(chA, chB, vL, vR);
	single_joystick(chA, chB, vL, vR, scale);
	
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

void trigger_failsafe(){
	// Stop motors
	set_speed(0, 0);
	// Update the state
	publish_motor_state(0, 0, FAILSAFE);
	// Give some time to stop
	delay(2000);
}

void software_mode(const bool new_input){
  Log.trace("Doing one software mode iteration");
  
  // Ok, we have received at least one valid command. Is it an old or a new one

  if(new_input) {
	// Yep, its new, do it

	// Read the values for each channel
	const int chx = cur_input_cmd.x;
	const int chy = cur_input_cmd.y;
	Log.notice("received CmdX, CmdY as: %d, %d\n", chx, chy);

	// Scale from [-100, 100] to [-1, 1]
	int vL = -999;
	int vR = -999;
	set_motor_speeds(chx / 100.0, chy / 100.0, vL, vR, false);

	// Update the state
	publish_motor_state(vL, vR, SW_CONTROL);
  } else {
	  // Nope, its a command from a previous iteration, how long ago?
	  const long delta = millis() - last_command_ts;
	  
	  if (delta < auto_timeout_sec * 1000) {
		// Ok, its pretty recent, leave things as is
	  } else {
		// Its been a long time since we heard anything, not good
		// Enter failsafe mode and come to a stop
		Log.warning("AUTO TIMEOUT EXCEEDED (%l ms > %d s), FAILSAFE TRIGGERED\n", delta, auto_timeout_sec);
		trigger_failsafe();
		// Reset timer
		last_command_ts = -999;
	  }	  
  }
}

void rc_mode(){
  // look for a good SBUS packet from the receiver
  if (x8r.Read()) {
    if(x8r.failsafe()) {
		Log.warning("FAILSAFE TRIGGERED\n");
		trigger_failsafe();
	} else {
		// Read the values for each channel
		const int ch1 = x8r.rx_channels()[0];
		const int ch2 = x8r.rx_channels()[1];
		const int ch3 = x8r.rx_channels()[2];
		const int ch4 = x8r.rx_channels()[3];
		const int ch5 = x8r.rx_channels()[4];
		Log.notice("Received RC input: %d, %d, %d, %d, %d\n", ch1, ch2, ch3, ch4, ch5);

		int vL = -999;
		int vR = -999;
		set_motor_speeds(ch4, ch1, vL, vR);
		// Publish state
		publish_motor_state(vL, vR, RC_CONTROL);
	}
  } else {
	  Log.trace("Failed to read a good RC packet");
	  delay(50);
  }
}

void read_encoders() {
  const long newA = encA.read();
  const long newB = encB.read();

  if(newA != encA_pos){
	  encA_pos = newA;
	  motors_moved = true;
  }

  if(newB != encB_pos){
	  encB_pos = newB;
	  motors_moved = true;
  }
}

void act_upon_commands() {
	
	// TODO: update to only listen to cmds / go to auto mode if TX or button allows it

	// Assume that if we once ever received a command we are in auto/sw controlled mode
	const bool sw_mode = last_command_ts > 0;

	// Did we receive any (new) instructions?
	const bool new_input = (last_input_cmd.header.stamp.toSec() < cur_input_cmd.header.stamp.toSec());

	if (new_input) {
		// Yes we did!
		Log.trace("Received an input command");
		
		// Update the previous command
		last_input_cmd = cur_input_cmd;
	}
	
	if (sw_mode) {	
		// sw controlled mode
		software_mode(new_input);
	} else {
		// Default to RC
		rc_mode();
	}

	// Give some time for the motors
	delay(50);
}

void loop() {
	// Act upon any input state we received or had
	act_upon_commands();	
	
	// Publish encoders only if they changed
	read_encoders();
	if(motors_moved) {
		encoder_msg.left = encA_pos;
		encoder_msg.right = encB_pos;
		encoder_pub.publish(&encoder_msg);
		motors_moved = false;
	}

	// Add a short delay
	delay(3);

	// Keep rosserial synced and process incomming msgs
	nh.spinOnce();
}
