#!/usr/bin/env python
 
import argparse
import time
import picamera
import rospy
import io
from picamera.array import PiRGBArray
import numpy
from openvino.inference_engine import IECore
import cv2 as cv
from clare_head_camera.msg import SemanticFrame
from vision_msgs.msg import Detection2D, Detection2DArray
from cv_bridge import CvBridge


# Input Image Preprocessing
def preprocess_image(image, dims_nchw):
    """
    Image Preprocessing steps, to match image 
    with Input Neural nets

    Image,
    (N, Channel, Height, Width)
    """
    n, c, h, w = dims_nchw
    scaled_image = cv.resize(image, (w, h))  # Resize width & height
    blob = scaled_image.transpose((2, 0, 1))  # Change data layout from HWC to CHW
    blob = blob.reshape((n, c, h, w))
    return scaled_image, blob

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


def run_faces(model_dir, threshold, face_publisher):
    #  Device Used for inference
    device = "MYRIAD"
    plugin = IECore()

    # Load the model
    exec_net, intput_dim, input_keys, output_keys = load_face_model(plugin, device, model_dir)

    # Message adaptor
    bridge = CvBridge()

    # Pull from the camera
    for image in picam_source():
        # image timestamp
        image_ts = time.time()

        # Perform inference
        scaled_image, blob = preprocess_image(image, intput_dim)
        req_handle = exec_net.start_async(request_id=0, inputs={input_keys: blob})

        # Get the result
        status = req_handle.wait()
        res = req_handle.output_blobs[output_keys].buffer

        bbox_ts = time.time()

        detection_arr = Detection2DArray()
        detection_arr.header.stamp = bbox_ts

        # Get Bounding Box Result
        for i, det in enumerate(res[0][0]):
            # Face detection Confidence
            confidence = float(det[2])  

            if confidence < threshold:
                continue

            # Obtain Bounding box coordinate, +-10 just for padding
            xmin = int(det[3] * image.shape[1] - 10)
            ymin = int(det[4] * image.shape[0] - 10)
            xmax = int(det[5] * image.shape[1] + 10)
            ymax = int(det[6] * image.shape[0] + 10)

            bbox = (xmin, ymin, xmax, ymax)
            # Center of the bounding box
            bbox_cx = (xmax - xmin) / 2
            bbox_cy = (ymax - ymin) / 2

            detection = Detection2D()
            detection.bbox.center.x = bbox_cx
            detection.bbox.center.y = bbox_cy
            detection.bbox.size_x = xmax - bbox_cx
            detection.bbox.size_y = ymax - bbox_cy

            if i == 0:
                # Store the image on the first detection only
                detection.source_img = bridge.cv2_to_imgmsg(scaled_image, encoding="passthrough")
                detection.source_img.header.stamp = image_ts

            detection_arr.detections.append(detection)
            face_publisher.publish(detection_arr)

            print("Face: ", bbox)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="OpenVINO Face Detection")
    parser.add_argument("-m", "--model_dir", required=True,
                        type=str, help="Directory containing downloaded models")
    parser.add_argument("-t", "--threshold", default=0.9,
                        type=int, help="Minimum detection threshold")

    args = parser.parse_args()

    # Create ros node
    rospy.init_node("face_detector", anonymous=False, disable_signals=False)
    face_pub = rospy.Publisher("image_faces", Detection2DArray, queue_size=10)

    run_faces(args.model_dir, args.threshold, face_pub)
