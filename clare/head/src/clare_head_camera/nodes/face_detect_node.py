#!/usr/bin/env python
 
import argparse
import time
import rospy
import io
import numpy
from openvino.inference_engine import IECore
import cv2
from clare_head_camera.msg import SemanticFrame
from vision_msgs.msg import Detection2D, Detection2DArray
from sensor_msgs.msg import Image
from cv_bridge import CvBridge
from camera.model import IntelFaceDetection
from camera.pipeline import AsyncPipeline
from camera.utils import ColorPalette, InputTransform, OutputTransform
from camera.camera import picam_source
import logging
import sys


def print_raw_results(size, detections, labels, threshold):
    rospy.logdebug(' Class ID | Confidence | XMIN | YMIN | XMAX | YMAX ')
    for detection in detections:
        if detection.score > threshold:
            xmin = max(int(detection.xmin), 0)
            ymin = max(int(detection.ymin), 0)
            xmax = min(int(detection.xmax), size[1])
            ymax = min(int(detection.ymax), size[0])
            class_id = int(detection.id)
            det_label = labels[class_id] if labels and len(labels) >= class_id else '#{}'.format(class_id)
            rospy.logdebug('{:^9} | {:10f} | {:4} | {:4} | {:4} | {:4} '
                     .format(det_label, detection.score, xmin, ymin, xmax, ymax))

def draw_detections(frame, detections, palette, labels, threshold):
    size = frame.shape[:2]
    for detection in detections:
        if detection.score > threshold:
            class_id = int(detection.id)
            color = palette[class_id]
            det_label = labels[class_id] if labels and len(labels) >= class_id else '#{}'.format(class_id)
            xmin = max(int(detection.xmin), 0)
            ymin = max(int(detection.ymin), 0)
            xmax = min(int(detection.xmax), size[1])
            ymax = min(int(detection.ymax), size[0])
            cv2.rectangle(frame, (xmin, ymin), (xmax, ymax), color, 2)
            cv2.putText(frame, '{} {:.1%}'.format(det_label, detection.score),
                        (xmin, ymin - 7), cv2.FONT_HERSHEY_COMPLEX, 0.6, color, 1)
    return frame

def publish_to_ros(image_pub, face_pub, frame, objects, start_time):
    # Publish the image
    bridge = CvBridge()
    image_ts = rospy.Time.from_sec(start_time)
    immsg = bridge.cv2_to_imgmsg(frame, encoding="bgr8")
    immsg.header.stamp = image_ts
    image_pub.publish(immsg)

    # Publish the bounding boxes
    detection_arr = Detection2DArray()
    detection_arr.header.stamp = rospy.Time.now()

    for det in objects:
        xmin, ymin, xmax, ymax = det.xmin, det.ymin, det.xmax, det.ymax
        # Center of the bounding box
        bbox_cx = (xmax - xmin) / 2
        bbox_cy = (ymax - ymin) / 2

        detection = Detection2D()
        detection.bbox.center.x = bbox_cx
        detection.bbox.center.y = bbox_cy
        detection.bbox.size_x = xmax - bbox_cx
        detection.bbox.size_y = ymax - bbox_cy
        detection.source_img.header.stamp = image_ts
        detection_arr.detections.append(detection)

    face_pub.publish(detection_arr)

def run_face_detection(model_dir, model_name, threshold, num_infer_requests, image_pub, face_pub):
    #  Device Used for inference
    device = "MYRIAD"
    ie = IECore()

    # Debug outupt
    raw_output_message = False

    # Load the model
    model_xml = f"{model_dir}/{model_name}/FP16/{model_name}.xml"
    input_transform = InputTransform(False, None, None)
    model = IntelFaceDetection(ie, model_xml, input_transform, threshold=threshold)
    plugin_config = {}

    # For drawing bounding boxes
    palette = ColorPalette(len(model.labels) if model.labels else 100)

    detector_pipeline = AsyncPipeline(ie,
                            model,
                            plugin_config,
                            device=device,
                            max_num_requests=num_infer_requests)

    next_frame_id = 0
    next_frame_id_to_show = 0

    rospy.loginfo("Starting inference...")

    cap = picam_source()

    while not rospy.is_shutdown():
        if detector_pipeline.callback_exceptions:
            raise detector_pipeline.callback_exceptions[0]

        # Process all completed requests
        results = detector_pipeline.get_result(next_frame_id_to_show)
        if results:
            objects, frame_meta = results
            frame = frame_meta['frame']
            start_time = frame_meta['start_time']

            if raw_output_message and len(objects):
                print_raw_results(frame.shape[:2], objects, model.labels, threshold)
    
            frame = draw_detections(frame, objects, palette, model.labels, threshold)
            
            publish_to_ros(image_pub, face_pub, frame, objects, start_time)

            next_frame_id_to_show += 1
            continue

        if detector_pipeline.is_ready():
            # Get new image/frame
            frame = next(cap)
            start_time = time.time()

            # Submit for inference
            detector_pipeline.submit_data(frame, next_frame_id, {'frame': frame, 'start_time': start_time})
            next_frame_id += 1
        else:
            # Wait for empty request
            rospy.logdebug("Pipeline is full, waiting...")
            detector_pipeline.await_any()


if __name__ == "__main__":
    # Create ros node
    rospy.init_node("clare_head_camera", anonymous=False, disable_signals=False)

    # Read parameters
    model_dir = rospy.get_param("~model_dir")
    model_name = rospy.get_param("~model_name")
    threshold = rospy.get_param("~threshold")
    parallel_requests = rospy.get_param("~parallel_requests")

    # Publishers
    image_pub = rospy.Publisher("clare/head/images", Image, queue_size=10)
    face_pub = rospy.Publisher("clare/head/faces", Detection2DArray, queue_size=10)

    try:
        run_face_detection(model_dir, model_name, threshold, parallel_requests, image_pub, face_pub)
    except rospy.ROSInterruptException:
        pass
