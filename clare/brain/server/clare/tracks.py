import rospy
from std_msgs.msg import String

class Tracks:
    def __init__(self):
        self._node_name = "brain_backend"
        self._cmd_topic = "track_cmds"
        self._status_topic = "track_status"
        self._connected = False
        self._pub = None

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
            msg = "Lon"
        else:
            msg = "Loff"
        
        rospy.loginfo(msg)
        self._pub.publish(msg)

    def status_callback(self, data):
        rospy.loginfo(rospy.get_caller_id() + "I heard %s", data.data)

    def is_connected(self):
        return self._connected