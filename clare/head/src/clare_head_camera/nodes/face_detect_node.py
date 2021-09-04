#!/usr/bin/env python
 
import argparse
import time
import picamera
import io
from picamera.array import PiRGBArray
import numpy
from openvino.inference_engine import IECore
import cv2 as cv
from clare_head_camera.msg import SemanticFrame


# Input Image Preprocessing
def image_preprocessing(image, dims_nchw):
    """
    Image Preprocessing steps, to match image 
    with Input Neural nets

    Image,
    (N, Channel, Height, Width)
    """
    n, c, h, w = dims_nchw
    blob = cv.resize(image, (w, h))  # Resize width & height
    blob = blob.transpose((2, 0, 1))  # Change data layout from HWC to CHW
    blob = blob.reshape((n, c, h, w))
    return blob

def load_model(plugin, model, weights, device):
    """
    Load OpenVino IR Models

    Input:
    Plugin = Hardware Accelerator IE Core
    Model = model_xml file 
    Weights = model_bin file

    Output:
    execution network (exec_net)
    """
    #  Read in Graph file (IR) to create network
    net = plugin.read_network(model, weights)
    # Load the Network using Plugin Device
    exec_net = plugin.load_network(network=net, device_name=device)
    return net, exec_net

def load_face_model(plugin, device, model_dir):
    model_xml = f"{model_dir}/face-detection-adas-0001/FP16/face-detection-adas-0001.xml"
    model_bin = model_xml.replace(".xml", ".bin")

    print(f"Loading model {model_xml}")

    # Create the execution network
    net_facedetect, exec_facedetect = load_model(plugin, model_xml, model_bin, device)

    # input/output tensors
    input_keys = 'data'
    output_keys = 'detection_out'

    #  Obtain image_count, channels, height and width
    n, c, h, w = net_facedetect.input_info[input_keys].input_data.shape

    return exec_facedetect, (n, c, h, w), input_keys, output_keys

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


def run_faces(model_dir, threshold):
    #  Device Used for inference
    device = "MYRIAD"
    plugin = IECore()

    # Load the model
    exec_net, intput_dim, input_keys, output_keys = load_face_model(plugin, device, model_dir)

    # Pull from the camera
    for image in picam_source():

        # Perform inference
        blob = image_preprocessing(image, intput_dim)
        req_handle = exec_net.start_async(request_id=0, inputs={input_keys: blob})

        # Get the result
        status = req_handle.wait()
        res = req_handle.output_blobs[output_keys].buffer

        # Get Bounding Box Result
        for detection in res[0][0]:
            # Face detection Confidence
            confidence = float(detection[2])  

            if confidence < threshold:
                continue

            # Obtain Bounding box coordinate, +-10 just for padding
            xmin = int(detection[3] * image.shape[1] - 10)
            ymin = int(detection[4] * image.shape[0] - 10)
            xmax = int(detection[5] * image.shape[1] + 10)
            ymax = int(detection[6] * image.shape[0] + 10)

            bbox = (xmin, ymin, xmax, ymax)

            print("Face: ", bbox)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="OpenVINO Face Detection")
    parser.add_argument("-m", "--model_dir", required=True,
                        type=str, help="Directory containing downloaded models")
    parser.add_argument("-t", "--threshold", default=0.9,
                        type=int, help="Minimum detection threshold")

    args = parser.parse_args()

    run_faces(args.model_dir, args.threshold)
