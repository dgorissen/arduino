#!/bin/bash
# service ssh start

set -x

LOCAL_IP=`hostname  -I | cut -f1 -d' '`

CLARE=/home/dgorissen/clare/brain
LOGS=$CLARE/logs

tracks_serial=/dev/ttyUSB0
tracks_usb=/dev/ttyACM0

middle_serial=/dev/ttyUSB1
middle_usb=/dev/ttyUSB4

mkdir -p $LOGS
rm $LOGS/*

echo
echo "* Current directory: " $(pwd)
echo "* Local IP detected as ${LOCAL_IP}"
echo "* Logs directory set to ${LOGS}"
echo

# Start ROS master

# export ROS_HOSTNAME=localhost
export ROS_MASTER_URI=http://${LOCAL_IP}:11311
roscore -v --master-logger-level=info 2>&1 | tee $LOGS/roscore.txt &
echo $! > $LOGS/roscore.pid

echo "* Giving roscore some time to start"
sleep 5

# ID of the current run, generated when roscore is started
RUN_ID=`rosparam get /run_id`
ROSOUT_FILE=~/.ros/log/${RUN_ID}/rosout.log

# Connect to tracks
echo "* Starting rosserial_python for tracks"
rosrun rosserial_python serial_node.py _baud:=115200 _port:="${tracks_serial}"  2>&1 | tee $LOGS/tracks.txt &
echo $! > $LOGS/tracks.pid
sleep 2

# Connect to middle
echo "* Starting middle node"
python3 ${CLARE}/../ultrasound/src/clare_middle_node.py --port ${middle_serial} 2>&1 | tee $LOGS/middle.txt &
echo $! > $LOGS/middle.pid
sleep 2

# Start web frontend
echo "* Starting web frontend"
npm run serve --prefix $CLARE/client > $LOGS/frontend.txt 2>&1 &
echo $! > $LOGS/frontend.pid

# Start the face detector
echo "* Starting head camera"
set +x
source ~/openvino/bin/setupvars.sh
source ${CLARE}/../head/install/setup.bash
set -x
rosrun clare_head_camera face_detect_node.py -d /home/dgorissen/openvino_models/intel/ -p 4 2>&1 | tee $LOGS/backend.txt &
echo $! > $LOGS/head_camera.pid
sleep 3

# Start speech recogniser
echo "* Starting respeaker node"
set +x
source ~/catkin_ws/devel/setup.bash
set -x
rosrun respeaker_ros respeaker_node.py 2>&1 | tee $LOGS/respeaker.txt &
echo $! > $LOGS/respeaker.pid
sleep 3

# Start speech to text
echo "* Starting speech to text"
rosrun respeaker_ros speech_to_text.py 2>&1 | tee $LOGS/speech_to_text.txt &
echo $! > $LOGS/speech_to_text.pid
sleep 3

# Start web backend
echo "* Starting web backend"
/home/dgorissen/.pyenv/shims/python3 $CLARE/server/app.py 2>&1 | tee $LOGS/backend.txt &
echo $! > $LOGS/backend.pid
sleep 2

# Execute any other command
exec "$@"

# Keep container running
echo "* Tailing rosout file ${ROSOUT_FILE}"
tail -f ${ROSOUT_FILE}

