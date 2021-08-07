
import rospy
import serial
import time
import json
from std_msgs.msg import String
import argparse


def run_node(port, baud):
    pub = rospy.Publisher('clare_middle', String, queue_size=10)
    rospy.init_node('clare_middle_node', anonymous=True)
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
                print(e)
                d = {"error": str(e), "line": line}
    
            # Easier than custom messages
            js = json.dumps(d)
            pub.publish(js)
            rate.sleep()
        ser.close()


if __name__ == '__main__':

    parser = argparse.ArgumentParser(description='Clare Middle.')
    parser.add_argument('--port', type=str, help='Serial port', default="/dev/ttyUSB1")
    parser.add_argument('--baud', type=int, help='Baud rate', default=115200)

    args = parser.parse_args()

    try:
        run_node(args.port, args.baud)
    except rospy.ROSInterruptException as e:
        print(e)
