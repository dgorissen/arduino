import time
from cv_bridge import CvBridge, CvBridgeError
import cv2
import base64
import json
import RPi.GPIO as GPIO
from adafruit_extended_bus import ExtendedI2C as I2C
from adafruit_servokit import ServoKit
import adafruit_bme680
import asyncio
from evdev import InputDevice, categorize, ecodes
from threading import Thread
import board
import neopixel

"""
Simple set of classes to hold a state object for a single time step
"""
class BaseState:
    def __init__(self):
        self._state = {}

    def get_state(self):
        return self._state

    def get_state_ts(self):
        return self._state["ts"]

    def status_callback(self, data):
        raise NotImplementedError;

class Tracks(BaseState):
    def __init__(self, pub):
        super(Tracks, self).__init__()
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

    def send_move_command(self, x, y):
        msg = f"CX:{int(x)},CY:{int(y)},M:A"

        self._pub.publish(msg)

    def status_callback(self, data):
        cur = { "ts" : time.time() }
        for item in data.data.split(","):
            cmd, val = [x.strip() for x in item.split(":")]
            cur[cmd] = val

        self._state = cur


class HeadCamera(BaseState):
    def __init__(self):
        super(HeadCamera, self).__init__()
        self._bridge = CvBridge()

    def status_callback(self, data):

        cv2_img = self._bridge.imgmsg_to_cv2(data, "bgr8")
        cv2_img = cv2.resize(cv2_img, (320, 240))

        (flag, jpg_img) = cv2.imencode(".jpg", cv2_img)

        img = jpg_img

        cur = {
            "ts" : data.header.stamp.secs,
            "image": img
         }

        self._state = cur


class ClareMiddle(BaseState):
    def __init__(self):
        super(ClareMiddle, self).__init__()

    def status_callback(self, data):
        cur = json.loads(data.data)
        cur["ts"] = time.time()
        self._state = cur


class ClareTop(BaseState):
    def __init__(self):
        super(ClareTop, self).__init__()
        
        if GPIO.getmode() is None:            
            GPIO.setwarnings(True)
        
            # Use physical pin numbering
            GPIO.setmode(GPIO.BOARD)

        # 3x latching push buttons
        self._button_pins = [15, 16, 18]
        # 3 position rotary switch
        self._rotary_pins = [37, 38]
        # DC motor
        self._fan_pins = [31, 32]
        # IR receiver
        self._ir_pin = 36

        # Setup button handlers
        self._setup_buttons()

        # Setup servo connection
        # Device is /dev/i2c-3
        i2c = I2C(3)
        self._servokit = ServoKit(channels=16, i2c=i2c)

        # Setup environmental sensor
        self._env_sensor = adafruit_bme680.Adafruit_BME680_I2C(i2c)
        self._env_sensor.sea_level_pressure = 1013.25

        # Setup fan pins
        self._setup_fan()

        # Setup IR receiver
        self._ir_thread = Thread(target=self._setup_ir, daemon=True)
        self._ir_thread.start()

        # Setup LED ring
        self._setup_lights()
        self._led_thread = Thread(target=self._run_lights, daemon=True)
        self._led_thread.start()

    def __button_callback(self, idx, pin):
        val = GPIO.input(pin)
        if pin in self._button_pins:
            self._state[f"button{idx}"] = val
        else:
            # The rotary switch has 3 states. One pin high, the other low or both pins low

            # whats the state of the other pin
            other_pin = self._rotary_pins[(idx + 1) % 2] # we only have 2 pins
            other_val = GPIO.input(other_pin)

            if val and not other_val:
                self._state["rotary"] = 0;
            elif not val and other_val:
                self._state["rotary"] = 1;
            elif not val and not other_val:
                self._state["rotary"] = 2;
            else:
                assert(f"Invalid rotary switch state {pin}:{val} and {other_pin}:{other_val}")

    def _setup_buttons(self):        
        return
        for i, pin in enumerate(self._button_pins + self._rotary_pins):
            print(f"Setting up button handler for pin {pin}")
            # Set pin to be an input pin and set initial value to be pulled low (off)
            GPIO.setup(pin, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
            GPIO.add_event_detect(pin, GPIO.BOTH, callback=lambda _: self.button_callback(i, pin))

    def set_arm_pos(self, pos_left, pos_right):
        assert(0 <= pos_left <= 180)
        assert(0 <= pos_right <= 180)

        self._servokit[0] = pos_left
        self._servokit[1] = pos_right

        self._state["pos_left_arm"] = pos_left
        self._state["pos_right_arm"] = pos_right

    def _setup_fan(self):
        for pin in self._fan_pins:
            GPIO.setup(pin, GPIO.OUT)
            GPIO.output(pin, GPIO.LOW)

    def set_fan(self, state):
        val = GPIO.HIGH if bool else GPIO.LOW
        # Only change the first pin
        pin = self._fan_pins[0]
        GPIO.output(pin, val)

        self._state["fan"] = state
    
    def read_environmentals(self):
        self._state["temp"] = self._env_sensor.temperature
        self._state["gas"] = self._env_sensor.gas
        self._state["humidity"] = self._env_sensor.relative_humidity
        self._state["pressure"] = self._env_sensor.pressure
        self._state["altitude"] = self._env_sensor.altitude

    async def _ir_callback(self):
        async for ev in self._ir_dev.async_read_loop():
            print(repr(ev))
            print(categorize(ev))
            self._state["ir_cmd"] = repr(ev)

    def _setup_ir(self):
        self._ir_dev = InputDevice('/dev/input/event0')
        print(self._ir_dev)
        loop = asyncio.get_running_loop()
        loop.run_until_complete(self._ir_callback())

    def _setup_lights(self):
        self._pixels = neopixel.NeoPixel(board.D18, 24)

    def set_lights(self, val):
        self._state["led_ring"] = val
    
    def _run_lights(self):

        def wheel(pos):
            ORDER = neopixel.GRB
            
            # Input a value 0 to 255 to get a color value.
            # The colours are a transition r - g - b - back to r.
            if pos < 0 or pos > 255:
                r = g = b = 0
            elif pos < 85:
                r = int(pos * 3)
                g = int(255 - pos * 3)
                b = 0
            elif pos < 170:
                pos -= 85
                r = int(255 - pos * 3)
                g = 0
                b = int(pos * 3)
            else:
                pos -= 170
                r = 0
                g = int(pos * 3)
                b = int(255 - pos * 3)
            return (r, g, b) if ORDER in (neopixel.RGB, neopixel.GRB) else (r, g, b, 0)

        def rainbow_cycle(wait, num_pixels):
            for j in range(255):
                for i in range(num_pixels):
                    pixel_index = (i * 256 // num_pixels) + j
                    self._pixels[i] = wheel(pixel_index & 255)
                self._pixels.show()
                time.sleep(wait)

        while True:
            val = self._state.get("led_ring", None)

            if val == "red":
                self._pixels.fill((255, 0, 0))
                self._pixels.show()
            if val == "green":
                self._pixels.fill((0, 255, 0))
                self._pixels.show()
            if val == "blue":
                self._pixels.fill((0, 0, 255))
                self._pixels.show()
            elif val == "rainbow":
                rainbow_cycle(0.001, 24)
            elif not val:
                self._pixels.fill((0, 0, 0))
            else:
                pass

            time.sleep(0.01)


    def cleanup():
        GPIO.cleanup()


class ClareVoice(BaseState):
    def __init__(self):
        super(ClareVoice, self).__init__()

    def status_callback(self, data):
        cur = {}
        cur["ts"] = time.time()
        # Take the first one only
        cur["transcript"] = data.transcript[0]
        self._state = cur


class RealsenseDepth(BaseState):
    def __init__(self):
        super(RealsenseDepth, self).__init__()
        self._bridge = CvBridge()

    def status_callback(self, data):
        # Assumes a compressed depth image
        cv2_img = self._bridge.compressed_imgmsg_to_cv2(data) #, "mono16")
        cv2_img = cv2.resize(cv2_img, (320, 240))

        (flag, jpg_img) = cv2.imencode(".jpg", cv2_img)

        img = jpg_img

        cur = {
            "ts" : data.header.stamp.secs,
            "depth": img
         }

        self._state = cur

        
