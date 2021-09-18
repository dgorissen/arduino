from clare.middle import ClareMiddle
from flask import Flask, Response, render_template, session, request, jsonify, send_file
from random import randint
import subprocess
import json
import time
from flask_cors import CORS
from clare.tracks import Tracks
from clare.voice import ClareVoice
from clare.head import HeadCamera
import rospy
from std_msgs.msg import String
from functools import wraps
import datetime
from clare.utils import shell_cmd
from sensor_msgs.msg import Image
from vision_msgs.msg import Detection2DArray
from speech_recognition_msgs.msg import SpeechRecognitionCandidates
import re

class ClareState:
    def __init__(self):
        self.tracks = None
        self.middle = None
        self.head = None
        self.voice = None

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

    h = HeadCamera()
    rospy.Subscriber("images", Image, h.status_callback)

    v = ClareVoice()
    rospy.Subscriber("speech_to_text", SpeechRecognitionCandidates, v.status_callback)

    cs = ClareState()
    cs.tracks = t
    cs.middle = m
    cs.head = h
    cs.voice = v

    STATE = cs

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
    if not connected():
        init_state()

    return ('Connected',200)

@app.route("/tracks/headlights")
@is_connected
def headlights():
    val = STATE.tracks.get_headlights() if STATE.tracks else None
    return jsonify({
        "value": val
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

@app.route("/make_photo")
def make_photo():
    basename = "raspistill"
    suffix = datetime.datetime.now().strftime("%y%m%d_%H%M%S")
    fn = f"{basename}_{suffix}.jpg"

    log = shell_cmd(f"/opt/vc/bin/raspistill -q 50 -o /tmp/{fn}")

    return jsonify({"name": fn, "output": log})

@app.route("/photo/<fn>")
def get_photo(fn):
    return send_file(f"/tmp/{fn}", mimetype='image/jpeg')

@app.route("/voice/speak")
def speak():
    tts = request.args.get('tts', default="", type=str)
    # No naughty strings
    tts = re.sub(r'[^0-9a-zA-Z ]+', '', tts)

    if tts:
        return shell_cmd(f"festival -b '(voice_cmu_us_slt_arctic_hts)' '(SayText \"{tts}\")'")
    else:
        return "", 200

@app.route("/<source>/stream")
@is_connected
def get_stream(source):
    if source == "tracks":
        stream = make_stream("tracks", STATE.tracks.get_state)
    elif source == "middle":
        stream = make_stream("middle", STATE.middle.get_state)
    elif source == "voice":
        stream = make_stream("voice", STATE.voice.get_state)
    else:
        return f"Invalid stream source {source}", 500

    return Response(stream(), mimetype="text/event-stream")

def head_video_stream():
    last_ts = 0
    while True:
        # Get the current state
        tstate = STATE.head.get_state()
        img = tstate.get("image", None)
        if img is None:
            time.sleep(0.5)
        else:
            yield (b'--frame\r\n' b'Content-Type: image/jpeg\r\n\r\n' + bytearray(img) + b'\r\n')

@app.route("/head/facefeed")
@is_connected
def vidfeed():
	return Response(head_video_stream(),
		mimetype = "multipart/x-mixed-replace; boundary=frame")

def make_stream(msg_name, state_getter):
    def _stream():
        last_ts = 0
        while True:
            # Get the current state
            tstate = state_getter()
            ts = tstate.get("ts", -1)

            if ts > 0 and ts != last_ts:
                last_ts = ts
                yield f"event: {msg_name}" + "\ndata: " + json.dumps(tstate) + "\n\n"
            else:
                time.sleep(0.2)

    return _stream

if __name__ == "__main__":
    app.run(host='0.0.0.0', threaded=True)
