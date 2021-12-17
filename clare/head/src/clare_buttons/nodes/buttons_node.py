#!/usr/bin/env python
 
import time
import rospy
from diagnostic_msgs.msg import KeyValue
from std_msgs.msg import String
import RPi.GPIO as GPIO
import board


class ButtonMonitor(object):
    def __init__(self):
        super(ButtonMonitor, self).__init__()

        # 3x latching push buttons
        self._button_pins = [15, 16, 18]

        # 3 position rotary switch
        self._rotary_pins = [37, 38]

        # Create ros node
        rospy.init_node("clare_top", anonymous=False, disable_signals=True)
        self._button_pub = rospy.Publisher("clare/buttons", KeyValue, queue_size=10)

        for i, pin in enumerate(self._button_pins + self._rotary_pins):
            print(f"Setting up button handler for pin {pin}")
            # Set pin to be an input pin and set initial value to be pulled low (off)
            GPIO.setup(pin, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
            GPIO.add_event_detect(pin, GPIO.BOTH, callback=lambda _: self._button_callback(i, pin))

    def button_callback(self, idx, pin):
        val = GPIO.input(pin)
        if pin in self._button_pins:
            kv = KeyValue(f"button{idx}", str(val))
        else:
            # The rotary switch has 3 states. One pin high, the other low or both pins low

            # whats the state of the other pin
            other_pin = self._rotary_pins[(idx + 1) % 2] # we only have 2 pins
            other_val = GPIO.input(other_pin)

            if val and not other_val:
                kv = KeyValue("rotary", "0")
            elif not val and other_val:
                kv = KeyValue("rotary", "1")
            elif not val and not other_val:
                kv = KeyValue("rotary", "2")
            else:
                assert(f"Invalid rotary switch state {pin}:{val} and {other_pin}:{other_val}")

        self._button_pub.publish(kv)
            
def run():
    rospy.logdebug('Button node ready and listening')
    rospy.spin()

if __name__ == "__main__":
    GPIO.setwarnings(True)
    
    # Use physical pin numbering
    GPIO.setmode(GPIO.BOARD)

    n = ButtonMonitor()
    n.run()
