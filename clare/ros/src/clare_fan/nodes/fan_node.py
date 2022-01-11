#!/usr/bin/env python

import time
import rospy
import RPi.GPIO as GPIO
from clare_fan.msg import FanControl
from threading import Timer


class FanController(object):
    def __init__(self):
        super(FanController, self).__init__()

        self._fan_pins = [31, 32]

        # Initialise the pins to off/low
        for pin in self._fan_pins:
            GPIO.setup(pin, GPIO.OUT)
            GPIO.output(pin, GPIO.LOW)

        rospy.init_node("clare_fan", anonymous=False, disable_signals=False)
        self._fan_sub = rospy.Subscriber("clare/fan", FanControl, self._fan_callback)

    def set_fan(self, state):
        val = GPIO.HIGH if state else GPIO.LOW
        # Only change the first pin
        pin = self._fan_pins[0]
        GPIO.output(pin, val)

    def _fan_callback(self, msg):
        state = bool(msg.state)
        dur = int(msg.duration)

        self.set_fan(state)

        if dur > 0:
            t = Timer(dur, self.set_fan, args=(not state,))
            t.start()

    def run(self):
        rospy.logdebug('Fan node ready and listening')
        rospy.spin()
        GPIO.cleanup()

if __name__ == "__main__":
  
    GPIO.setwarnings(True)
    # Use physical pin numbering
    GPIO.setmode(GPIO.BOARD)

    n = FanController()
    try:
        n.run()
    except rospy.ROSInterruptException:
        pass
