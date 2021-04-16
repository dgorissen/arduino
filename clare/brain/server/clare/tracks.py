import rospy
from std_msgs.msg import String

class Tracks:
    def __init__(self):
        self._node_name = "brain_backend"
        self._cmd_topic = "track_cmds"
        self._status_topic = "track_status"
        self._connected = False
        self._pub = None
        self._state = {}

    def connect(self, disable_signals=False):
        rospy.init_node(self._node_name, anonymous=False, disable_signals=disable_signals)
        self._pub = rospy.Publisher(self._cmd_topic, String, queue_size=10)
        rospy.Subscriber(self._status_topic, String, self.status_callback)
        # TODO: better check
        self._connected = True
    
    def shutdown(self):
        rospy.signal_shutdown("Node shutdown function called")
        self._pub = None
        self._connected = False

    def set_headlights(self, state):
        if state:
            msg = "H:1"
        else:
            msg = "H:0"
        
        rospy.loginfo(msg)
        self._pub.publish(msg)

    def headlights(self):
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

    def is_connected(self):
        return self._connected