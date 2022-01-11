#!/usr/bin/env python
 
import rospy
import serial
import time
import json
from std_msgs.msg import String, Bool, Float32
from clare_middle.msg import GasMessage, UltrasoundMessage
import argparse


def run_node(port, baud):
    while not rospy.is_shutdown():
        with serial.Serial(port=port, baudrate=baud, timeout=2) as ser:
            time.sleep(2)
            print(f"Serial port {ser.name}:{ser.baudrate} opened")
            while not rospy.is_shutdown() and ser.isOpen():
                try:
                    line = ser.readline().decode('utf-8')
                    items = [x.split("=") for x in line.split()]

                    for k, v in items:
                        if k == "V":
                            volt_pub.publish(float(v))
                        elif k == "P":
                            pir_pub.publish(v == "1")
                        elif k == "G":
                            a, b = [float(x) for x in v.split(",")]
                            msg = GasMessage()
                            msg.methane = a;
                            msg.h2s = b
                            gas_pub.publish(msg)
                        elif k == "U":
                            a, b, c = [int(x) for x in v.split(",")]
                            msg = UltrasoundMessage()
                            msg.left = c;
                            msg.right = b;
                            msg.middle = a;
                            ultra_pub.publish(msg)
                        else:
                            rospy.logwarn(f"Ignoring unknown entry '{k}':'{v}'")
                except ValueError as e:
                    msg = f"Error parsing line '{line}'"
                    rospy.logwarn(msg)
                    d = {"error": msg}

if __name__ == '__main__':
    rospy.init_node('clare_middle', anonymous=False)

    port = rospy.get_param("~port")
    baud = rospy.get_param("~baud", 115200)

    pir_pub = rospy.Publisher('clare/pir', Bool, queue_size=10)
    gas_pub = rospy.Publisher('clare/gas', GasMessage, queue_size=10)
    volt_pub = rospy.Publisher('clare/voltage', Float32, queue_size=10)
    ultra_pub = rospy.Publisher('clare/ultrasound', UltrasoundMessage, queue_size=10)

    try:
        run_node(port, baud)
    except rospy.ROSInterruptException as e:
        print(e)
