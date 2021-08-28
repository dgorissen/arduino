import json
import time

class ClareMiddle:
    def __init__(self):
        self._state = {}

    def get_state(self):
        return self._state

    def status_callback(self, data):
        cur = json.loads(data.data)
        cur["ts"] = time.time()

        self._state = cur
