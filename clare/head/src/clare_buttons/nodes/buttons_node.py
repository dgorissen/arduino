#!/usr/bin/env python
 
import RPi.GPIO as GPIO
import time
import rospy
from diagnostic_msgs.msg import KeyValue
from functools import partial


class ButtonMonitor(object):
    def __init__(self):
        super(ButtonMonitor, self).__init__()

        # 3x latching push buttons
        self._button_pins = [18, 15, 16]

        # 3 position rotary switch
        self._rotary_pins = [37, 38]

        # Button events are noisy, keep a cache
        self._state = {}
        self._state_dirty = False

        # Create ros node
        rospy.init_node("clare_top", anonymous=False, disable_signals=True)
        self._button_pub = rospy.Publisher("clare/buttons", KeyValue, queue_size=10)

        # Setup Handlers
        for i, pin in enumerate(self._button_pins + self._rotary_pins):
            print(f"Setting up button handler for pin {pin}")
            # Set pin to be an input pin and set initial value to be pulled low (off)
            GPIO.setup(pin, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
            cb = partial(self._button_callback, i)
            GPIO.add_event_detect(pin, GPIO.BOTH, callback=cb)

        # Read the opening state
        self.read_button_states()
        self._state_dirty = True
    
    def get_rotary_pos(self):
        # The rotary switch has 3 states. One pin high, the other low or both pins low

        # whats the state of the other pin
        v1 = GPIO.input(self._rotary_pins[0])
        v2 = GPIO.input(self._rotary_pins[1])

        if v1 and not v2:
            return 2
        elif not v1 and v2:
            return 1
        elif not v1 and not v2:
            return 0
        else:
            raise RuntimeError(f"Invalid rotary switch state {pin}:{val} and {other_pin}:{other_val}")

    def read_button_states(self):
        for i, pin in enumerate(self._button_pins):
            self._state[f"button{i}"] = GPIO.input(pin)

        self._state["rotary"] = self.get_rotary_pos()

    # The GPIO edge detection is noisy and sometimes you get multiple
    # events from different pins. So dont publish everything directly on callback trigger
    # Instead accumulate in a state object
    def _button_callback(self, idx, pin):
        val = GPIO.input(pin)
        # print(f"Callback for idx {idx} on pin {pin} with val {val}")

        if pin in self._button_pins:
            self._state[f"button{idx}"] = val
        else:
            self._state["rotary"] = self.get_rotary_pos()
        
        self._state_dirty = True
            
    def run(self):
        rospy.logdebug('Button node ready and listening')
        old_state = {}
        while not rospy.is_shutdown():
            if self._state_dirty:
                self.read_button_states()
                for k,v in self._state.items():
                    # Only publish buttons that have changed
                    if old_state.get(k, None) != v:
                        kv = KeyValue(k, str(v))
                        self._button_pub.publish(kv)
                        old_state[k] = v
                self._state_dirty = False
            
            time.sleep(0.5)


if __name__ == "__main__":
  
    GPIO.setwarnings(True)
    # Use physical pin numbering
    GPIO.setmode(GPIO.BOARD)

    n = ButtonMonitor()
    try:
        n.run()
    except rospy.ROSInterruptException:
        pass
