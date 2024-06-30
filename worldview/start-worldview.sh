#!/bin/bash
cd ~/arduino/worldview
bash brightness.sh 50
python3 -u worldview.py > log.txt
