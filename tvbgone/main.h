/*
Last Updated: 22 Oct. 2016
By Gabriel Staples, http://www.ElectricRCAircraftGuy.com 
-My contact info is available by clicking the "Contact Me" tab at the top of my website.
*/

#include <avr/pgmspace.h>

// The TV-B-Gone for Arduino can use either the EU (European Union) or the NA (North America) database of POWER CODES
// EU is for Europe, Middle East, Australia, New Zealand, and some countries in Africa and South America
// NA is for North America, Asia, and the rest of the world not covered by EU

// Two regions!
#define NA 1 //set by a HIGH on REGIONSWITCH pin
#define EU 0 //set by a LOW on REGIONSWITCH pin

// What pins do what
#define LED LED_BUILTIN //LED indicator pin (usually 13)
#define IRLED 11         //the IR sender LED 
#define TRIGGER 0       //the button pin; NB: this pin is "hard-coded" in the sleepNow() function in the primary .ino file by means of using external interrupt 0, which is hard-wired to pin 2
#define REGIONSWITCH 8  //HIGH (1) = NA, LOW (0) = EU; Pin 5 (REGIONSWITCH) is HIGH (via in input pullup resistor) for North America, or you (the user) must wire it to ground to set the codes for Europe.

// Lets us calculate the size of the NA/EU databases
#define NUM_ELEM(x) (sizeof (x) / sizeof (*(x)));

// set define to 0 to turn off debug output
#define DEBUG 1
#define DEBUGP(x) if (DEBUG == 1) { x ; }

// Shortcut to insert single, non-optimized-out nop
#define NOP __asm__ __volatile__ ("nop")

// Tweak this if neccessary to change timing
// -for 8MHz Arduinos, a good starting value is 11
// -for 16MHz Arduinos, a good starting value is 25
#define DELAY_CNT 25

// Makes the codes more readable. the OCRA is actually
// programmed in terms of 'periods' not 'freqs' - that
// is, the inverse!
#define freq_to_timerval(x) (F_CPU / 8 / x - 1)

// The structure of compressed code entries
struct IrCode {
  uint8_t timer_val;
  uint8_t numpairs;
  uint8_t bitcompression;
  uint16_t const *times;
  uint8_t const *codes;
};
