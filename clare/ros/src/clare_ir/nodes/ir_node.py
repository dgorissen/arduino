#!/usr/bin/env python
import time
import rospy
from std_msgs.msg import String
from evdev import InputDevice, categorize, ecodes
from threading import Thread
import asyncio
import subprocess

# Helpful info https://www.sigmdel.ca/michel/ha/opi/ir_01_en.html

def shell_cmd(cmd):
    result = subprocess.run(cmd, shell=True, stdout=subprocess.PIPE)
    return result.stdout.decode("utf-8")

def get_ir_device():
    return shell_cmd(" libinput list-devices | grep -A1 gpio | tail -n 1 | cut -f2 -d':'").strip()
    
class IRMonitor(object):
    def __init__(self):
        super(IRMonitor, self).__init__()

        dev = get_ir_device()
        print("IR input device resolved to " + dev)
        self._ir_dev = InputDevice(dev)

        rospy.init_node("clare_ir", anonymous=False, disable_signals=False)
        self._ir_pub = rospy.Publisher("clare/ir", String, queue_size=10)

    def _create_device(self):
        out = shell_cmd(" libinput list-devices | grep -A1 gpio | tail -n 1 | cut -f2 -d':'").strip()

    def resolve_event(self, ev):
        # TODO fill out codebook
        codebook = {}
        
        # Type 0 is synchronization event
        if ev.type == 4:
            # print(f"Event type {ev.type} code {ev.code} value {ev.value}")
            val = codebook.get(ev.value, str(ev.value))
            return val
        
        return None

    def run(self):
        rospy.logdebug('IR node ready and listening')

        def _run():
            async def _ir_callback():
                async for ev in self._ir_dev.async_read_loop():
                    val = self.resolve_event(ev)
                    if val:
                        self._ir_pub.publish(str(ev.value))

            loop = asyncio.new_event_loop()
            asyncio.set_event_loop(loop)
            loop.run_until_complete(_ir_callback())

        Thread(target=_run, daemon=True).start()
 
        rospy.spin()

if __name__ == "__main__":
  
    n = IRMonitor()
    try:
        n.run()
    except rospy.ROSInterruptException:
        pass
