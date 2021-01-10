#ifndef CONFIG_H
#define CONFIG_H
 
// Motor A connections
const int motor_enA = 7;
const int motor_in1 = 8;
const int motor_in2 = 9;

// Motor B connections
const int motor_enB = 12;
const int motor_in3 = 10;
const int motor_in4 = 11;

// Encoder pins
const int motor_encA1 = 2;
const int motor_encA2 = 3;
const int motor_encB1 = 4;
const int motor_encB2 = 5;

//Speed control
const int motor_maxpwm = 255;
const int motor_minpwm = 0;
const int motor_deadband = 80;

//RC receiver
const int rc_minpwm = 172;
const int rc_maxpwm = 1811;

#endif
