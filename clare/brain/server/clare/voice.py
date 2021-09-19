import json
import time

class ClareVoice:
    def __init__(self):
        self._state = {}

    def get_state(self):
        return self._state

    def status_callback(self, data):
        cur = {}
        cur["ts"] = time.time()
        # Take the first one only
        cur["transcript"] = data.transcript[0]
        self._state = cur
