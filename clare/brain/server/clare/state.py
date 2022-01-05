import time
from cv_bridge import CvBridge, CvBridgeError
import cv2
import base64
import json
import rospy
from threading import Thread
from std_msgs.msg import String
from diagnostic_msgs.msg import KeyValue
from clare_arms.msg import ArmMovement
from clare_fan.msg import FanControl
from clare_lightring.msg import LightRingMessage
from clare_env.msg import BME680Message
from clare_middle.msg import GasMessage, UltrasoundMessage
from clare_tracks.msg import EncoderMessage, JoystickInput, MotorSpeeds
from std_msgs.msg import String, Bool, Float32
from sensor_msgs.msg import Image
from vision_msgs.msg import Detection2DArray
from speech_recognition_msgs.msg import SpeechRecognitionCandidates
from sensor_msgs.msg import CompressedImage

"""
Simple set of classes to hold a state object for a single time step
"""
class BaseState:
    def __init__(self):
        self._state = {}

    def get_state(self):
        return self._state

    def get_state_ts(self):
        return self._state["ts"]

    def update_ts_to_now(self):
        self._state["ts"] = time.time()

    def status_callback(self, data):
        raise NotImplementedError;

class Tracks(BaseState):
    def __init__(self):
        super(Tracks, self).__init__()
        self._joy_pub = rospy.Publisher("/clare/tracks/joystick_input", JoystickInput, queue_size=3)
        self._headlight_pub = rospy.Publisher("/clare/tracks/headlights", Bool, queue_size=3)
        rospy.Subscriber("clare/tracks/motor_speeds", MotorSpeeds, self._motor_cb)
        rospy.Subscriber("clare/tracks/encoders", EncoderMessage, self._encoder_cb)

    def set_headlights(self, state):
        self._headlight_pub.publish(state)

        self._state["headlights"] = state
        self.update_ts_to_now()

    def get_headlights(self):
        return self._state.get("headlights", "?")

    def send_move_command(self, x, y):
        msg = JoystickInput()
        msg.header.stamp = rospy.Time.now()
        msg.x = int(x)
        msg.y = int(y)
        self._joy_pub.publish(msg)

        self._state["input_x"] = x
        self._state["input_y"] = y
        self.update_ts_to_now()

    def _motor_cb(self, msg):
        self._state["motor_left"] = msg.left
        self._state["motor_right"] = msg.right

        self._state["motor_left_dir"] = chr(msg.left_direction)
        self._state["motor_right_dir"] = chr(msg.right_direction)

        self._state["mode"] = msg.status
        self.update_ts_to_now()

    def _encoder_cb(self, msg):
        self._state['enc_left'] = msg.left
        self._state['enc_right'] = msg.right
        self.update_ts_to_now()


class HeadCamera(BaseState):
    def __init__(self):
        super(HeadCamera, self).__init__()
        self._bridge = CvBridge()
        rospy.Subscriber("/clare/head/images", Image, self.status_callback)

    def status_callback(self, data):

        cv2_img = self._bridge.imgmsg_to_cv2(data, "bgr8")
        cv2_img = cv2.resize(cv2_img, (320, 240))

        (flag, jpg_img) = cv2.imencode(".jpg", cv2_img)

        img = jpg_img

        cur = {
            "ts" : data.header.stamp.secs,
            "image": img
         }

        self._state = cur


class ClareMiddle(BaseState):
    def __init__(self):
        super(ClareMiddle, self).__init__()
        rospy.Subscriber("/clare/pir", Bool, self._pir_cb)
        rospy.Subscriber("/clare/gas", GasMessage, self._gas_cb)
        rospy.Subscriber("/clare/voltage", Float32, self._volt_cb)
        rospy.Subscriber("/clare/ultrasound", UltrasoundMessage, self._ultra_cb)

    def _pir_cb(self, msg):
        self._state["pir"] = msg.data
        self.update_ts_to_now()

    def _gas_cb(self, msg):
        self._state["methane"] = msg.methane
        self._state["h2s"] = msg.h2s
        self.update_ts_to_now()

    def _volt_cb(self, msg):
        self._state["voltage"] = msg.data
        self.update_ts_to_now()

    def _ultra_cb(self, msg):
        self._state["ultra_left"] = msg.left
        self._state["ultra_right"] = msg.right
        self._state["ultra_middle"] = msg.middle
        self.update_ts_to_now()

class ClareTop(BaseState):
    def __init__(self):
        super(ClareTop, self).__init__()

        self._arm_pub = rospy.Publisher("/clare/arms", ArmMovement, queue_size=10)
        self._fan_pub = rospy.Publisher("/clare/fan", FanControl, queue_size=10)
        self._lightring_pub = rospy.Publisher("/clare/lightring", LightRingMessage, queue_size=10)
        rospy.Subscriber("/clare/buttons", KeyValue, self.button_cb)
        rospy.Subscriber("/clare/env", BME680Message, self.env_cb)
        rospy.Subscriber("/clare/ir", String, self.ir_cb)

    def set_arms(self, left, right):
        m = ArmMovement()
        m.shoulder_left_angle = left
        m.shoulder_right_angle = right
        self._arm_pub.publish(m)

    def set_fan(self, state, dur):
        m = FanControl()
        m.state = state
        m.duration = dur
        self._fan_pub.publish(m)
    
    def set_lightring(self, pat):
        m = LightRingMessage()
        m.pattern = pat
        self._lightring_pub.publish(m)
    
    def env_cb(self, msg):
        keys = "gas,humidity,pressure,altitude,temp".split(",")
        for k in keys:
            self._state[k] = getattr(msg, k)
        self.update_ts_to_now()

    def ir_cb(self, msg):
        self._state["ir_cmd"] = msg.data
        self.update_ts_to_now()

    def button_cb(self, msg):
        self._state[msg.key] = msg.value
        self.update_ts_to_now()


class ClareVoice(BaseState):
    def __init__(self):
        super(ClareVoice, self).__init__()
        rospy.Subscriber("speech_to_text", SpeechRecognitionCandidates, self.status_callback)

    def status_callback(self, data):
        cur = {}
        cur["ts"] = time.time()
        # Take the first one only
        cur["transcript"] = data.transcript[0]
        self._state = cur


class RealsenseDepth(BaseState):
    def __init__(self):
        super(RealsenseDepth, self).__init__()
        self._bridge = CvBridge()
        rospy.Subscriber("/camera/depth/image_rect_raw/compressed", CompressedImage, self.status_callback)

    def status_callback(self, data):
        # Assumes a compressed depth image
        cv2_img = self._bridge.compressed_imgmsg_to_cv2(data) #, "mono16")
        cv2_img = cv2.resize(cv2_img, (320, 240))

        (flag, jpg_img) = cv2.imencode(".jpg", cv2_img)

        img = jpg_img

        cur = {
            "ts" : data.header.stamp.secs,
            "depth": img
         }

        self._state = cur

        
