import time
class Tracks:
    def __init__(self, pub):
        self._state = {}
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

    def get_state(self):
        return self._state

    def get_state_ts(self):
        return self._state["ts"]

    def status_callback(self, data):
        cur = { "ts" : time.time() }
        for item in data.data.split(","):
            cmd, val = [x.strip() for x in item.split(":")]
            cur[cmd] = val

        self._state = cur
