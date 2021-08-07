#!/bin/bash
# service ssh start

LOCAL_IP=`hostname  -I | cut -f1 -d' '`

CLARE=/home/dgorissen/clare/brain
LOGS=$CLARE/logs
track_serial_usb=/dev/ttyUSB0
# track_serial_usb=/dev/ttyS0

mkdir -p $LOGS
rm $LOGS/*

echo
echo "* Local IP detected as ${LOCAL_IP}"
echo "* Logs directory set to ${LOGS}"
echo

# Start ROS master

# export ROS_HOSTNAME=localhost
export ROS_MASTER_URI=http://${LOCAL_IP}:11311
roscore -v --master-logger-level=info 2>&1 | tee $LOGS/roscore.txt &
echo $! > $LOGS/roscore.pid

echo "Givng roscore some time to start"
sleep 3

# Connect to tracks
rosrun rosserial_python serial_node.py _baud:=115200 _port:="${track_serial_usb}"  2>&1 | tee $LOGS/tracks.txt &
echo $! > $LOGS/tracks.pid

# Start web frontend
npm run serve --prefix $CLARE/client > $LOGS/frontend.txt 2>&1 &
echo $! > $LOGS/frontend.pid

# Start web backend
/home/dgorissen/.pyenv/shims/python3 $CLARE/server/app.py 2>&1 | tee $LOGS/backend.txt &
echo $! > $LOGS/backend.pid

# Execute any other command
exec "$@"

# Keep container running
tail -f /dev/null
