import time
from cv_bridge import CvBridge, CvBridgeError
import cv2
import base64

class HeadCamera:
    def __init__(self):
        self._state = {}
        self._bridge = CvBridge()

    def get_state(self):
        return self._state

    def get_state_ts(self):
        return self._state["ts"]

    def status_callback(self, data):

        cv2_img = self._bridge.imgmsg_to_cv2(data, "bgr8")
        cv2_img = cv2.resize(cv2_img, (320, 240))

        (flag, jpg_img) = cv2.imencode(".jpg", cv2_img)

        # img = base64.b64encode(jpg_img).decode('utf-8')
        img = jpg_img

        cur = {
            "ts" : data.header.stamp.secs,
            "image": img
         }

        self._state = cur

