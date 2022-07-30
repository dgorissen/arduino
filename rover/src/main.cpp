#include <ArduinoLog.h>
#include "Freenove_WS2812_Lib_for_ESP32.h"
#include "Freenove_4WD_Car_For_ESP32.h"
#include "sbus.h"
#include "driver/uart.h"

#define LEDS_COUNT 12 // Define the count of WS2812
#define LEDS_PIN 32   // Define the pin number for ESP32
#define CHANNEL 0     // Define the channels that control WS2812
#define RC_PIN 15

void setupUart()
{
  const int rc_rx_pin = 15;
  const int rc_tx_pin = 12;

  const uart_port_t uart_num = UART_NUM_1;

  uart_config_t uart_config = {
      .baud_rate = 100000,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_EVEN,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .rx_flow_ctrl_thresh = 122,
  };

  // Configure UART parameters
  uart_param_config(uart_num, &uart_config);
  int r = uart_set_pin(uart_num, rc_tx_pin, rc_rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  //uart_set_line_inverse(UART_NUM_1, UART_INVERSE_RXD);
  uart_driver_install(UART_NUM_1, 2048, 2048, 20, NULL, 0);
  
  if (r == ESP_OK)
  {
    Serial.println("success");
  }
  else
  {
    Serial.println("fail");
  }
}

uint16_t rc_input[16];
bool rc_failSafe;
bool rc_lostFrame;

// Speed control
const int motor_maxpwm = 2000;
const int motor_minpwm = -2000;
const int motor_deadband = 80;

// RC receiver
const int rc_minpwm = 172;
const int rc_maxpwm = 1811;

Freenove_ESP32_WS2812 strip = Freenove_ESP32_WS2812(LEDS_COUNT, LEDS_PIN, CHANNEL, TYPE_GRB);

void setup()
{
  Serial.begin(115200);
  setupUart();
  strip.begin();           // Initialize WS2812
  strip.setBrightness(10); // Set the brightness of WS2812
  Log.begin(LOG_LEVEL_NOTICE, &Serial);
}

// Simple manual differential steering where two channels control
// the two tracks separately
// returns a +/- pwm value
void dual_joystick(const int chL, const int chR)
{
  // Map to a symmetric pwm range centered over 0
  const int vL = map(chL, rc_minpwm, rc_maxpwm, motor_minpwm, motor_maxpwm);
  const int vR = map(chR, rc_minpwm, rc_maxpwm, motor_minpwm, motor_maxpwm);

  Log.verbose("Mapped RC intput to %d -> %d, %d -> %d\n", chL, vL, chR, vR);

  // Motor_Move(vL, vL, vR, vR);
  delay(10);
}

void set_wheel_speeds(const int a, const int b, const int c, const int d)
{
  // Motor_Move(a,b,c,d);
}

void rc_mode()
{
  SbusRx x8r(&Serial1);

  while (true)
  {

    // look for a good SBUS packet from the receiver
    if (x8r.Read())
    {
      if (x8r.failsafe())
      {
        Log.warning("FAILSAFE TRIGGERED\n");
      }
      else
      {
        // Read the values for each channel
        const int ch1 = x8r.rx_channels()[0];
        const int ch2 = x8r.rx_channels()[1];
        const int ch3 = x8r.rx_channels()[2];
        const int ch4 = x8r.rx_channels()[3];
        const int ch5 = x8r.rx_channels()[4];
        Log.notice("Received RC input: %d, %d, %d, %d, %d\n", ch1, ch2, ch3, ch4, ch5);
        Serial.println(ch1);
        Serial.println(ch2);

        // mode_switch_val = ch5;

        dual_joystick(ch4, ch2);
      }
    }
    else
    {
      Log.trace("Failed to read a good RC packet");
      Serial.println("no packet");
      delay(50);
    }
    Serial.println("Looping");
    delay(100);
  }
}

void loop()
{
  rc_mode();
  // for (int j = 0; j < 255; j += 2) {
  //   for (int i = 0; i < LEDS_COUNT; i++) {
  // 	strip.setLedColorData(i, strip.Wheel((i * 256 / LEDS_COUNT + j) & 255));//Set the color of the WS2812
  //   }
  //   strip.show();	      //Call WS2812 to display colors
  //   delay(5);
  // }
}