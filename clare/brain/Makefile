rootdir=/Users/dirk.gorissen/code/arduino/clare/brain

build:
	docker build -t dgorissen/clare .

run:
	docker run -v ${rootdir}:/home/dgorissen/cbrain -h cbrain -p5000:5000 -p2222:22 --name cbrain dgorissen/clare

runshell:
	docker run -v ${rootdir}:/home/dgorissen/cbrain -h cbrain -p5000:5000 -p2222:22 --name cbrain -it dgorissen/clare

execshell:
	docker exec -it cbrain bash

create-buildx:
	docker buildx create --name clare-builder
	docker buildx inspect --bootstrap

buildx:
	docker buildx use clare-builder
	docker buildx build -t dgorissen/clare --platform linux/arm64 --push .

push:
	docker push dgorissen/clare
