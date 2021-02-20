#ifndef CONFIG_H
#define CONFIG_H
 
// Motor A connections
const int motor_enA = 12;
const int motor_in1 = 11;
const int motor_in2 = 10;

// Motor B connections
const int motor_enB = 7;
const int motor_in3 = 9;
const int motor_in4 = 8;

// Encoder pins
const int motor_encA1 = 3;
const int motor_encA2 = 4;
const int motor_encB1 = 5;
const int motor_encB2 = 6;

//Speed control
const int motor_maxpwm = 255;
const int motor_minpwm = 0;
const int motor_deadband = 80;

//RC receiver
const int rc_minpwm = 172;
const int rc_maxpwm = 1811;

#endif
