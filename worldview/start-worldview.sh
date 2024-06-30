#!/bin/bash
cd ~pi/code/worldview
bash brightness.sh 50
python3 -u worldview.py > /dev/null
