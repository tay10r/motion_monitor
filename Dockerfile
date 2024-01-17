FROM debian:11 as build
RUN apt-get update
RUN apt-get install -y gcc g++ cmake make libopencv-dev libspdlog-dev libuv1-dev
RUN useradd --create-home --shell /bin/bash user
USER user
WORKDIR /home/user
ADD --chown=user:user proto proto
ADD --chown=user:user server server
RUN mkdir motion_monitor_build ~/.local
RUN cmake -S server -B build -DCMAKE_INSTALL_PREFIX=/home/user/.local -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build --target install
RUN echo 'export PATH=$PATH:/home/user/.local/bin' >>~/.bashrc
FROM debian:11 as deploy
RUN useradd --create-home --shell /bin/bash user && apt-get update && apt-get install -y libopencv-dev libspdlog1 libuv1
RUN usermod -aG video user
#USER user
WORKDIR /home/user
COPY --from=build /home/user/.bashrc /home/user/.bashrc
COPY --from=build /home/user/.local /home/user/.local
ENTRYPOINT [ "/home/user/.local/bin/motion_monitor" ]
CMD [ ]
