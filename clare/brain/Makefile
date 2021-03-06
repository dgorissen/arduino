# Get clare directory
mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir := $(dir $(mkfile_path))
rootdir := $(shell dirname $(mkfile_dir))

track_teensy_usb=/dev/ttyACM0
track_serial_usb=/dev/ttyUSB0
build_arch ?= $(shell arch)
real_arch = $(shell arch)

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
	docker buildx use clare-builder
	docker buildx build -t dgorissen/clare \
	--build-arg vinodist=${vinodist} \
	--platform linux/arm/v7 \
	--push .
endif

builder:
	docker buildx create --name clare-builder
	docker buildx inspect --bootstrap

run:
	docker run \
	--rm \
	-v ${rootdir}:/home/dgorissen/clare \
	-h cbrain \
	-p5000:5000 -p11311:11311 -p8080:8080 \
	--name cbrain \
	dgorissen/clare

runpi:
	docker run \
	--rm \
	-v ${rootdir}:/home/dgorissen/clare \
	-h cbrain \
	-p5000:5000 -p11311:11311 -p8080:8080 \
	--device=${track_teensy_usb} \
	--device=${track_serial_usb} \
	--device /dev/gpiomem \
	--privileged \
	--network=host \
	-e DISPLAY=${DISPLAY} \
	-v /dev:/dev \
	--name cbrain \
	dgorissen/clare

	# Unfortunately privileged, /dev, and host network is all needed
	# to thet the neural compute stick to work inside docker

runshell:
	docker run -v ${rootdir}:/home/dgorissen/clare -h cbrain \
	--rm \
	-p5000:5000 -p11311:11311 -p8080:8080 \
	--name cbrain \
	-it \
	dgorissen/clare

execshell:
	docker exec -e DISPLAY=${DISPLAY} -it cbrain bash

push:
	docker push dgorissen/clare

pull:
	docker pull dgorissen/clare

clean:
	docker builder prune -a -f
	docker system prune -f
