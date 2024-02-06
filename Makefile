DOCKER ?= docker
BASE ?= debian:11
ARCH ?= arm64
VERSION := 0.1
TAG := motion_monitor_$(ARCH):$(VERSION)
PLATFORM := linux/$(ARCH)

.PHONY: all
all: build

.PHONY: build
build:
	$(DOCKER) run --name motion_monitor_builder --mount type=bind,source=$(PWD)/dist,destination=/home/user/dist -it $(TAG)
	$(DOCKER) rm motion_monitor_builder

.PHONY: builder
builder:
	$(DOCKER) buildx build . --platform $(PLATFORM) --tag $(TAG) --build-arg=BASE=${BASE}

.PHONY: save
save:
	$(DOCKER) save $(TAG) >motion_monitor_$(ARCH).tar
