build:
	docker build -t clare-brain .

run:
	docker run --name cbrain clare-brain

runshell:
	docker run --name cbrain -it clare-brain

create-buildx:
	docker buildx create --name clare-builder
	docker buildx inspect --bootstrap

buildx:
	docker buildx use clare-builder
	docker buildx build -t clare-brain --platform linux/arm64 --load .
