SHELL := /bin/bash

# Get clare directory
mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir := $(dir $(mkfile_path))
rootdir := $(shell dirname $(mkfile_dir))

build_arch ?= $(shell arch)
real_arch = $(shell arch)

simplemode ?= $(shell simplemode)

ifeq ($(build_arch), armv7l)
	vinodist=raspbian
else
	vinodist=ubuntu20
endif

build:
	echo "Build arch is: ${build_arch} real arch is ${real_arch}"
ifeq ($(build_arch), $(real_arch))
	echo "=== Building for HOST"
	sleep 2
	docker build -t dgorissen/clare \
	--build-arg vinodist=${vinodist} \
	.
else
	echo "=== Building for ARM, vinodist is ${vinodist}"
	sleep 2
	docker buildx build -t dgorissen/clare \
	--build-arg vinodist=${vinodist} \
	--platform linux/arm/v7 \
	--progress=plain \
	--push .
endif

runpi:
	docker run \
	--rm \
	-v ${rootdir}:/home/dgorissen/clare \
	-v ${HOME}/.Xauthority:/home/dgorissen/.Xauthority \
	-h cbrain \
	-p5000:5000 -p11311:11311 -p8080:8080 \
	--device /dev/gpiomem \
	--privileged \
	--network=host \
	-e DISPLAY=${DISPLAY} \
	-v /dev:/dev \
	-v /opt/vc:/opt/vc \
	-v /sys/class/gpio:/sys/class/gpio \
	-v /run/udev/data:/run/udev/data \
	--env LD_LIBRARY_PATH=/opt/vc/lib \
	--name cbrain \
	dgorissen/clare

	# Unfortunately privileged, /dev, and host network is all needed
	# so that the neural compute stick  will work inside docker

	# /opt/vc stuff is needed for the pi camera
	# https://www.losant.com/blog/how-to-access-the-raspberry-pi-camera-in-docker

	# /run/udev/data is from
	# https://stackoverflow.com/questions/50789172/libinput-in-a-docker-container

execshell:
	docker exec -e DISPLAY=${DISPLAY} -it cbrain bash

push:
	docker push dgorissen/clare

pull:
	docker pull dgorissen/clare

clean:
	docker builder prune -a -f
	docker system prune -f

ros_build:
	# IMPORTANT to first source the respeaker workspace before building the clare one
	# See http://wiki.ros.org/catkin/Tutorials/workspace_overlaying
	# https://stackoverflow.com/questions/7507810/how-to-source-a-script-in-a-makefile
	# the setuptools option because of https://github.com/ros/catkin/issues/863
	. ~/catkin_ws/install/setup.bash && catkin_make -C ${rootdir}/head -DSETUPTOOLS_DEB_LAYOUT=OFF

ros_install:
	make -C ${rootdir}/head/build install

tracks_build:
	make -C ../tracks build

tracks_upload:
	make -C ../tracks upload

