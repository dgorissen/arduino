#include <Servo.h>

Servo myservo;

void setup() {
	myservo.attach(9);
}

void loop() {
myservo.write(180);   //clockwise rotation for 1pm feeding
delay(1000);          //rotation duration in ms
myservo.detach();     //detach servo to prevent "creeping" effect
delay(2000);      //8 hours pause between 1pm and 9pm
myservo.attach(9);    //reattach servo to pin 9
myservo.write(180);   //clockwise rotation for 9pm feeding
delay(1000);          //rotation duration in ms
myservo.detach();     //detach servo to prevent "creeping" effect
delay(2000);      //16 hours pause until 1pm next day
myservo.attach(9);    //reattach servo to pin 9 before looping
}
