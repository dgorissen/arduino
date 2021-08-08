from clare.middle import ClareMiddle
from flask import Flask, Response, render_template, session, request, jsonify
from random import randint
import subprocess
import json
import time
from flask_cors import CORS
from clare.tracks import Tracks
import rospy
from std_msgs.msg import String
from functools import wraps

class ClareState:
    def __init__(self):
        self.tracks = None
        self.middle = None

def is_connected(f):
    @wraps(f)
    def decorated_function(*args, **kwargs):
        if connected():
            return f(*args, **kwargs)
        else:
            return "Not connected", 403
    return decorated_function

# configuration
DEBUG = True
# TODO global vars
STATE = None

# instantiate the app and enable CORS
app = Flask(__name__)
app.config.from_object(__name__)
CORS(app, resources={r'/*': {'origins': '*'}})

# Create ros node
rospy.init_node("brain_backend", anonymous=False, disable_signals=True)

def init_state():
    global STATE

    if STATE is not None:
        raise Exception("init_state() called on already initialised STATE")

    track_pub = rospy.Publisher("track_cmds", String, queue_size=10)
    t = Tracks(track_pub)
    rospy.Subscriber("track_status", String, t.status_callback)

    m = ClareMiddle()
    rospy.Subscriber("clare_middle", String, m.status_callback)

    STATE = ClareState()
    STATE.tracks = t
    STATE.middle = m

    rospy.loginfo("State initialised")

def connected():
    return STATE is not None

@app.route("/")
def index():
    return jsonify({
        "cbrain": "CLARE API v0.1",
        "connected": connected()
    })

@app.route("/connect")
def connect():
    init_state()
    return ('Connected',200)

@app.route("/tracks/headlights")
@is_connected
def headlights():
    return jsonify({
        "value": STATE.tracks.get_headlights()
    })

@app.route("/tracks/headlights/<status>")
@is_connected
def set_headlights(status):
    if status == "on":
        STATE.tracks.set_headlights(True)
    elif status == "off":
        STATE.tracks.set_headlights(False)
    else:
        return "Invalid headlight status", 500

    return f'Headlights set to {status}', 200

@app.route("/tracks/state")
@is_connected
def tracks_state():
    return jsonify(STATE.tracks.get_state())

@app.route("/middle/state")
@is_connected
def middle_state():
    return jsonify(STATE.middle.get_state())

@app.route("/system/audio_devices")
def audio_devices():
    return shell_cmd(["aplay", "-l"])

@app.route("/system/usb_devices")
def usb_devices():
    return shell_cmd(["lsusb"])

@app.route("/tracks/stream")
def stream():
    return Response(event_stream(), mimetype="text/event-stream")

def shell_cmd(cmd):
    result = subprocess.run(cmd, stdout=subprocess.PIPE)
    return result.stdout

if __name__ == "__main__":
    # Important not threaded given our global state
    app.run(host='0.0.0.0', threaded=True)
