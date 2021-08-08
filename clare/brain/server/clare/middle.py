import json

class ClareMiddle:
    def __init__(self):
        self._state = {}

    def parse_state(self, s):
        self._state = json.loads(s)
    
    def get_state(self):
        return self._state

    # TODO: push to ringbuffer
    def status_callback(self, data):
        self.parse_state(data.data);
