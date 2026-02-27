#!/bin/bash
cd ~/arduino/worldview
bash brightness.sh 50
if [ -x ".venv/bin/python" ]; then
  .venv/bin/python -u worldview.py >> log.txt 2>&1
else
  python3 -u worldview.py >> log.txt 2>&1
fi
