ARG BASE=debian:11
FROM ${BASE}
RUN apt-get update
RUN apt-get install -y \
  gcc g++ make ninja-build pkg-config autoconf automake libtool \
  curl zip unzip git \
  python3 python3-pip \
  libx11-dev libxft-dev libxext-dev libxi-dev libxtst-dev libxkbcommon-x11-dev libx11-xcb-dev libxrandr-dev libxcursor-dev libxdamage-dev libxinerama-dev \
  bison flex
RUN pip3 install cmake jinja2

# Expect in this directory:
#
#  /usr/local/src/components : Contains the 'components' directory with the source code.
#  /usr/local/src/artifacts  : Contains a directory in which artifacts can be pushed to.

WORKDIR /usr/local/src

# Setup vcpkg
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
ENV VCPKG_ROOT=/usr/local/src/vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git
RUN cd vcpkg && ./bootstrap-vcpkg.sh -disableMetrics
RUN cd vcpkg && ./vcpkg install spdlog opencv4[dnn] libuv glm yaml-cpp nlohmann-json alsa

# Setup emsdk
RUN git clone https://github.com/emscripten-core/emsdk
RUN cd emsdk && ./emsdk install latest && ./emsdk activate latest

ADD scripts/build.sh .
CMD cd emsdk && . ./emsdk_env.sh && cd .. && ./build.sh
