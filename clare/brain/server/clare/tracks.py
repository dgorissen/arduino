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

    def parse_state(self, s):
        for item in s.split(","):
            cmd, val = [x.strip() for x in item.split(":")]
            self._state[cmd] = val
    
    def get_state(self):
        return self._state

    # TODO: push to ringbuffer
    def status_callback(self, data):
        # rospy.loginfo(rospy.get_caller_id() + "I heard %s", data.data)
        self.parse_state(data.data);
