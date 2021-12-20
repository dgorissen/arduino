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
rm -rf $LOGS/*

echo
echo "* Current directory: " $(pwd)
echo "* Local IP detected as ${LOCAL_IP}"
echo "* Logs directory set to ${LOGS}"
echo

export ROS_LOG_DIR=${LOGS}/ros
# Start ROS master

echo "* Starting roscore"

# export ROS_HOSTNAME=localhost
export ROS_MASTER_URI=http://${LOCAL_IP}:11311
roscore -v --master-logger-level=info 2>&1 | tee $LOGS/roscore.txt &
echo $! > $LOGS/roscore.pid

# ID of the current run, generated when roscore is started
while [ -z `rosparam get /run_id` ]
do
    echo "Waiting for roscore to be detected..."
    sleep 1
done

RUN_ID=`rosparam get /run_id`
echo "Roscore running, runid is " ${RUN_ID}

# Log directory
ROSOUT_FILE=$(roslaunch-logs)/rosout.log

# Start web backend
echo "* Starting web backend"
set +x
source ~/openvino/bin/setupvars.sh
set -x
/home/dgorissen/.pyenv/shims/python3 $CLARE/server/app.py 2>&1 | tee $LOGS/backend.txt &
echo $! > $LOGS/backend.pid
sleep 2

# Start web frontend
echo "* Starting web frontend"
npm run serve --prefix $CLARE/client > $LOGS/frontend.txt 2>&1 &
echo $! > $LOGS/frontend.pid
sleep 2

# Start ROS nodes
echo "* Starting ROS nodes"
set +x
source ~/openvino/bin/setupvars.sh
source ~/catkin_ws/install/setup.bash
source ${CLARE}/../head/devel/setup.bash
set -x
roslaunch --wait ${CLARE}/../head/launch/clare.launch

# Execute any other command
exec "$@"

# Keep container running
echo "* Tailing rosout file ${ROSOUT_FILE}"
tail -f ${ROSOUT_FILE}
