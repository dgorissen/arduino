from flask import Flask, Response, render_template, session, request, jsonify
from random import randint
import json
import time
from flask_cors import CORS

# configuration
DEBUG = True

# instantiate the app
app = Flask(__name__)
app.config.from_object(__name__)

# enable CORS
CORS(app, resources={r'/*': {'origins': '*'}})

# TODO global flag
CONNECTED=False

def connected():
    return CONNECTED

def event_stream():
    data_dic = {}
    while True:

        time.sleep(0.005)
        # Simulate random CAN msg
        id, dlc = randint(100, 120), randint(1, 8)
        bytes = " ".join([str(randint(20, 80)) for _ in range(dlc)])

        event = "update"
        msg = {"id": id, "dlc": dlc, "bytes": bytes}

        if not data_dic.get(id):
            event = "new_id"

        data_dic[id] = msg
        data = json.dumps(msg) if event == "update" else json.dumps(data_dic)
        yield f"event:{event}\ndata:{data}\n\n"


@app.route("/")
def index():
    return jsonify({
        "cbrain": "CLARE API v0.1",
        "connected": connected()
    })

@app.route("/connect")
def connect():
    # TODO setup serial connections
    global CONNECTED
    CONNECTED = True
    return ('', 204)

@app.route("/disconnect")
def disconnect():
    # TODO tear down serial connections
    global CONNECTED
    CONNECTED = False
    return ('', 204)

@app.route("/stream")
def stream():
    return Response(event_stream(), mimetype="text/event-stream")


if __name__ == "__main__":
    app.run()
