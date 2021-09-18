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
        cur["transcript"] = zip(data.transcript, data.confidence)
        self._state = cur
