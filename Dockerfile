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
RUN useradd --create-home --shell /bin/bash user
USER user
WORKDIR /home/user

# Setup vcpkg
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
ENV VCPKG_ROOT=/home/user/vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git
RUN cd vcpkg && ./bootstrap-vcpkg.sh -disableMetrics
USER root
USER user
RUN cd vcpkg && ./vcpkg install spdlog opencv4 libuv glm yaml-cpp nlohmann-json alsa

# Setup emsdk
RUN git clone https://github.com/emscripten-core/emsdk
RUN cd emsdk && ./emsdk install latest && ./emsdk activate latest

# Import the project
ADD --chown=user:user proto proto
ADD --chown=user:user server server
ADD --chown=user:user dashboard dashboard
ADD --chown=user:user deps deps
# Build the dashboard
RUN cd emsdk && . ./emsdk_env.sh && cd .. && mkdir dashboard_build && cd dashboard_build && emcmake cmake -DCMAKE_BUILD_TYPE=Release -B . -S ../dashboard && cmake --build .
# Build the server
RUN mkdir server_build ~/.local
RUN cmake -S server -B server_build -DCMAKE_BUILD_TYPE=Release -DBUNDLE_PATH=/home/user/dashboard_build --preset vcpkg
RUN cmake --build server_build
USER root
ENTRYPOINT cd server_build && cpack -G DEB && mv *.deb /home/user/dist
