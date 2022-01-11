roslib:
	rosrun rosserial_arduino make_libraries.py lib/

build:
	pio run -v

upload:
	echo "WARNING: when uploading the wires on pins 0 and 1 (TX/RX) need to be disconnected!"
	pio run -v --target upload

