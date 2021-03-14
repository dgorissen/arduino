build:
	docker build -t dgorissen/clare .

run:
	docker run --name cbrain dgorissen/clare

runshell:
	docker run --name cbrain -it dgorissen/clare

create-buildx:
	docker buildx create --name clare-builder
	docker buildx inspect --bootstrap

buildx:
	docker buildx use clare-builder
	docker buildx build -t dgorissen/clare --platform linux/arm64 --push .

push:
	docker push dgorissen/clare
