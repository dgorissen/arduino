from flask import Flask, Response, render_template, session, request, jsonify
from random import randint
import json
import time
from flask_cors import CORS
from clare.tracks import Tracks

# configuration
DEBUG = True
# TODO global vars
TRACKS = None

# instantiate the app and enable CORS
app = Flask(__name__)
app.config.from_object(__name__)
CORS(app, resources={r'/*': {'origins': '*'}})

def connected():
    return TRACKS and TRACKS.is_connected()

def connect_to_tracks():
    global TRACKS
    if not connected():
        TRACKS = Tracks()
        TRACKS.connect(disable_signals=True)

@app.route("/")
def index():
    return jsonify({
        "cbrain": "CLARE API v0.1",
        "connected": connected()
    })

@app.route("/tracks/connect")
def connect():
    connect_to_tracks()
    return ('Connected',200)

@app.route("/tracks/headlights")
def headlights():
    if not connected():
        return "Not connected", 403

    return jsonify({
        "value": TRACKS.headlights()
    })

@app.route("/tracks/headlights/<status>")
def set_headlights(status):
    if not connected():
        return "Not connected", 403

    if status == "on":
        TRACKS.set_headlights(True)
    elif status == "off":
        TRACKS.set_headlights(False)
    else:
        return "Invalid headlight status", 500

    return f'Headlights set to {status}', 200

@app.route("/tracks/state")
def tracks_state():
    if not connected():
        return "Not connected", 403
    return jsonify(TRACKS.get_state())

@app.route("/tracks/stream")
def stream():
    return Response(event_stream(), mimetype="text/event-stream")


if __name__ == "__main__":
    # Important not threaded given our global state
    app.run(host='0.0.0.0', threaded=True)
