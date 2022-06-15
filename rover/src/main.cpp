#include <ArduinoLog.h>
#include "Freenove_WS2812_Lib_for_ESP32.h"
#include "Freenove_4WD_Car_For_ESP32.h"
#include "sbus.h"

#define LEDS_COUNT  12    //Define the count of WS2812
#define LEDS_PIN  	32    //Define the pin number for ESP32
#define CHANNEL	    0     //Define the channels that control WS2812

SbusRx x8r(&Serial1);
uint16_t rc_input[16];
bool rc_failSafe;
bool rc_lostFrame;

//Speed control
const int motor_maxpwm = 2000;
const int motor_minpwm = -2000;
const int motor_deadband = 80;

//RC receiver
const int rc_minpwm = 172;
const int rc_maxpwm = 1811;

Freenove_ESP32_WS2812 strip = Freenove_ESP32_WS2812(LEDS_COUNT, LEDS_PIN, CHANNEL, TYPE_GRB);

void setup() {
  strip.begin();		  //Initialize WS2812
  strip.setBrightness(10);//Set the brightness of WS2812
  pinMode(12, INPUT);
  pinMode(15, OUTPUT);
  Log.begin(LOG_LEVEL_NOTICE, &Serial);
}

// Simple manual differential steering where two channels control
// the two tracks separately
// returns a +/- pwm value
void dual_joystick(const int chL, const int chR){
	// Map to a symmetric pwm range centered over 0
	const int vL = map(chL, rc_minpwm, rc_maxpwm, motor_minpwm, motor_maxpwm);
	const int vR = map(chR, rc_minpwm, rc_maxpwm, motor_minpwm, motor_maxpwm);

	Log.verbose("Mapped RC intput to %d -> %d, %d -> %d\n", chL, vL, chR, vR);

  Motor_Move(vL, vL, vR, vR);
  delay(10);
}

void set_wheel_speeds(const int a, const int b, const int c, const int d){
  Motor_Move(a,b,c,d);
}

void rc_mode(){
  // look for a good SBUS packet from the receiver
  if (x8r.Read()) {
    if(x8r.failsafe()) {
  		Log.warning("FAILSAFE TRIGGERED\n");
  	} else {
      // Read the values for each channel
      const int ch1 = x8r.rx_channels()[0];
      const int ch2 = x8r.rx_channels()[1];
      const int ch3 = x8r.rx_channels()[2];
      const int ch4 = x8r.rx_channels()[3];
      const int ch5 = x8r.rx_channels()[4];
      Log.notice("Received RC input: %d, %d, %d, %d, %d\n", ch1, ch2, ch3, ch4, ch5);

      //mode_switch_val = ch5;

      dual_joystick(ch4, ch2);
    }
  } else {
    Log.trace("Failed to read a good RC packet");
    delay(50);
  }
}

void loop() {
  rc_mode();
  // for (int j = 0; j < 255; j += 2) {
  //   for (int i = 0; i < LEDS_COUNT; i++) {
	// 	strip.setLedColorData(i, strip.Wheel((i * 256 / LEDS_COUNT + j) & 255));//Set the color of the WS2812
  //   }
  //   strip.show();	      //Call WS2812 to display colors
  //   delay(5);
  // }  
}