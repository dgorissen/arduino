#include "utils.h"
#include <Arduino.h>

void delayMillis(int milliseconds) {
  if(milliseconds < 1) return;
  for (int i = 0; i < milliseconds; i++) delayMicroseconds(1000);
}

void delaySeconds(int seconds){
    delayMillis(seconds*1000);
}