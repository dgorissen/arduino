#!/bin/bash
if [ "$EUID" -ne 0 ]
  then echo "Please run as root"
  exit
fi

export PYENV_ROOT="/home/dgorissen/.pyenv"
export PATH="$PYENV_ROOT/bin:$PATH"
eval "$(pyenv init --path)"
source /opt/ros/noetic/setup.bash
source /home/dgorissen/clare/head/devel/setup.sh
rosrun clare_lightring lightring_node.py

