import time
from cv_bridge import CvBridge, CvBridgeError
import cv2
import base64
import json

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

    def status_callback(self, data):
        raise NotImplementedError;

class Tracks(BaseState):
    def __init__(self, pub):
        super(Tracks, self).__init__()
        self._pub = pub

    def set_headlights(self, state):
        if state:
            msg = "H:1"
        else:
            msg = "H:0"
        
        self._pub.publish(msg)

    def get_headlights(self):
        s = self._state.get("H", "0")
        return True if s == '1' else False

    def send_move_command(self, x, y):
        msg = f"CX:{x},CY:{y},M:A"

        self._pub.publish(msg)

    def status_callback(self, data):
        cur = { "ts" : time.time() }
        for item in data.data.split(","):
            cmd, val = [x.strip() for x in item.split(":")]
            cur[cmd] = val

        self._state = cur


class HeadCamera(BaseState):
    def __init__(self):
        super(HeadCamera, self).__init__()
        self._bridge = CvBridge()

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

    def status_callback(self, data):
        cur = json.loads(data.data)
        cur["ts"] = time.time()
        self._state = cur


class ClareVoice(BaseState):
    def __init__(self):
        super(ClareVoice, self).__init__()

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

        
