#include <Arduino.h>
#include "face.h"
#include "utils.h"

Face face = Face(0, 1, 2, 3);

void face_setup() {
  Serial.begin(9600);
  face.reset();
}

void face_loop() {
  while (true) {
    face.happy();
    delaySeconds(2);
    face.sad();
    delaySeconds(2);
    face.silly();
    delaySeconds(2);
    face.angry();
    delaySeconds(2);
    face.confused();
    delaySeconds(2);
    face.ugh();
    delaySeconds(2);
    face.surprised();
    delaySeconds(2);
    face.kiss();
    delaySeconds(2);
  }
}
