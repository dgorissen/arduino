rootdir=/Users/dirk.gorissen/code/arduino/clare/
trackusb=/dev/ttype%
arch ?= "arm64"

ifeq ($(arch), "arm64")
	vinodist=raspbian
else
	vinodist=ubuntu20
endif

build:
ifeq ($(arch), "arm64")
	echo "=== Building for ARM"
	sleep 2
	docker buildx use clare-builder
	docker buildx build -t dgorissen/clare \
	--build-arg vinodist=${vinodist} \
	--platform linux/arm/v7 \
	--push .
else
	echo "=== Building for UBUNTU"
	sleep 2
	docker build -t dgorissen/clare \
	--build-arg vinodist=${vinodist} \
	.
endif

builder:
	docker buildx create --name clare-builder
	docker buildx inspect --bootstrap

run:
	docker run -v ${rootdir}:/home/dgorissen/clare -h cbrain \
	--rm \
	-p5000:5000 -p2222:22 -p11311:11311 -p8080:8080 \
	--name cbrain \
	dgorissen/clare

runshell:
	docker run -v ${rootdir}:/home/dgorissen/clare -h cbrain \
	--rm \
	-p5000:5000 -p2222:22 -p11311:11311 -p8080:8080 \
	--name cbrain \
	-it \
	dgorissen/clare
	# --device=${trackusb} \

execshell:
	docker exec -it cbrain bash

push:
	docker push dgorissen/clare
