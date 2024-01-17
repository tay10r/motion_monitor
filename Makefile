DOCKER ?= docker
ARCH ?= arm64
VERSION := 0.1
TAG := motion_monitor_$(ARCH):$(VERSION)
PLATFORM := linux/$(ARCH)

.PHONY: all
all:
	$(DOCKER) buildx build . --platform $(PLATFORM) --tag $(TAG)

.PHONY: run
run:
	$(DOCKER) run --name motion_monitor --device /dev/video0:/dev/video0 --publish 5100:5100 -it $(TAG) --device 0 --ip 0.0.0.0

.PHONY: save
save:
	$(DOCKER) save $(TAG) >motion_monitor_$(ARCH).tar
