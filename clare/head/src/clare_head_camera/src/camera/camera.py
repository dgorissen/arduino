import time
import picamera
from picamera.array import PiRGBArray

def picam_source(resolution=(640,480), framerate=24):
    with picamera.PiCamera() as camera:
        camera.resolution = resolution
        camera.framerate = framerate
        rawCapture = PiRGBArray(camera, size=resolution)
        
        # allow the camera to warmup
        time.sleep(0.5)
    
        # capture frames from the camera
        for frame in camera.capture_continuous(rawCapture, format="bgr", use_video_port=True):
            # grab the raw NumPy array representing the image
            image = frame.array
            yield image

            # clear the stream in preparation for the next frame
            rawCapture.truncate(0)