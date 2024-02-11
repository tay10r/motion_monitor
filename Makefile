DOCKER ?= docker
BASE ?= debian:11
ARCH ?= arm64
VERSION := 0.1
TAG := sentinel_builder_$(ARCH):$(VERSION)
PLATFORM := linux/$(ARCH)

.PHONY: all
all: build

.PHONY: build
build:
	$(DOCKER) run --name sentinel_builder --mount type=bind,source=$(PWD)/dist,destination=/home/user/dist -it $(TAG)
	$(DOCKER) rm sentinel_builder

.PHONY: builder
builder:
	$(DOCKER) buildx build . --platform $(PLATFORM) --tag $(TAG) --build-arg=BASE=${BASE}

.PHONY: save
save:
	$(DOCKER) save $(TAG) >sentinel_builder_$(ARCH).tar
