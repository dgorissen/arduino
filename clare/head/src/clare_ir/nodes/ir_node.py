#!/usr/bin/env python
import time
import rospy
from std_msgs.msg import String
from evdev import InputDevice, categorize, ecodes
from threading import Thread
import asyncio


class IRMonitor(object):
    def __init__(self):
        super(IRMonitor, self).__init__()

        self._ir_dev = InputDevice('/dev/input/event0')

        rospy.init_node("clare_ir", anonymous=False, disable_signals=False)
        self._button_pub = rospy.Publisher("clare/ir", String, queue_size=10)

    def run(self):
        rospy.logdebug('IR node ready and listening')

        async def _ir_callback():
            async for ev in self._ir_dev.async_read_loop():
                print(repr(ev))
                self._ir_pub.publish(repr(ev))

        loop = asyncio.get_running_loop()
        loop.run_until_complete(_ir_callback())

if __name__ == "__main__":
  
    n = IRMonitor()
    try:
        n.run()
    except rospy.ROSInterruptException:
        pass
