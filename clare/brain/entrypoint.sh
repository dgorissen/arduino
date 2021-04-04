#!/bin/bash
# service ssh start

CLARE=/home/dgorissen/clare/brain
LOGS=$CLARE/logs
TRACKSUSB=/dev/ttyUSB0

mkdir -p $LOGS
rm $LOGS/*

# Start ROS master
roscore -v --master-logger-level=info 2>&1 | tee $LOGS/roscore.txt &
echo $! > $LOGS/roscore.pid

# Connect to tracks
rosrun rosserial_python serial_node.py -baud 115200 -port $TRACKSUSB  2>&1 | tee $LOGS/tracks.txt &
echo $! > $LOGS/tracks.pid

# Start web frontend
npm run serve --prefix $CLARE/client > $LOGS/frontend.txt 2>&1 &
echo $! > $LOGS/frontend.pid

# Start web backend
# /home/dgorissen/.pyenv/shims/python3 $CLARE/server/app.py > $LOGS/backend.txt 2>&1 
/home/dgorissen/.pyenv/shims/python3 $CLARE/server/app.py 2>&1 | tee $LOGS/backend.txt
echo $! > $LOGS/backend.pid

# Keep container running
# tail -f /dev/null

# exec "$@"