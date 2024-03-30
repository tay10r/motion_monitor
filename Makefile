-include config.mk

DOCKER ?= docker
BASE ?= debian:11
ARCH ?= arm64
VERSION := 0.1
PLATFORM := linux/$(ARCH)
TAG := sentinel_builder_$(ARCH):$(VERSION)

build_opts = --rm --name sentinel_builder
build_opts += --mount type=bind,source=$(PWD)/artifacts,destination=/usr/local/src/artifacts
build_opts += --mount type=bind,source=$(PWD)/components,destination=/usr/local/src/components,readonly

.PHONY: all
all: build

.PHONY: build
build:
	$(DOCKER) run $(build_opts) -it $(TAG)

.PHONY: builder
builder:
	$(DOCKER) buildx build . --platform $(PLATFORM) --tag $(TAG) --build-arg=BASE=${BASE}

.PHONY: save
save:
	$(DOCKER) save $(TAG) >sentinel_builder_$(ARCH).tar
