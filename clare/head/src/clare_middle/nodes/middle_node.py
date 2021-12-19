#!/usr/bin/env python
 
import rospy
import serial
import time
import json
from std_msgs.msg import String
import argparse


def run_node(port, baud):
    rate = rospy.Rate(10)  # 10hz

    while not rospy.is_shutdown():
        ser = serial.Serial(port=port, baudrate=baud, timeout=2)
        time.sleep(2)
        print(f"Serial port {ser.name}:{ser.baudrate} opened")
        while ser.isOpen():
            try:
                line = ser.readline().decode('utf-8')
                items = line.split()
                d = dict(zip(items[0::2], [float(x) for x in items[1::2]]))
            except ValueError as e:
                msg = f"Error parsing line '{line}'"
                rospy.logwarn(msg)
                d = {"error": msg}
    
            # Easier than custom messages
            js = json.dumps(d)
            pub.publish(js)
            rate.sleep()
        ser.close()


if __name__ == '__main__':
    rospy.init_node('clare_middle_node', anonymous=True)

    port = rospy.get_param("~port", "/dev/ttyUSB1")
    baud = rospy.get_param("~baud", 115200)

    pub = rospy.Publisher('clare/middle', String, queue_size=10)

    try:
        run_node(port, baud)
    except rospy.ROSInterruptException as e:
        print(e)
