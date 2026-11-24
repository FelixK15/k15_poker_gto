# The solver server for Linux.
#
#   docker build -f docker/Dockerfile -t k15-gto-server .
#   docker run -d --name k15-gto-server --restart unless-stopped \
#       -p 127.0.0.1:50051:50051 --env-file /etc/k15-gto/server.env \
#       -v k15-gto-data:/data k15-gto-server
#
# The env file holds GTO_SERVER_TOKEN=<long random string>. Published on
# loopback as above, the server is reached through an SSH tunnel. To publish
# it to the network instead, mount a certificate and add
# "--tls-cert /certs/server.pem --tls-key /certs/server.key" after the image
# name; note that Docker's published ports bypass ufw.

FROM ubuntu:24.04 AS build

# The last line is what vcpkg and its ports (OpenSSL, gRPC) need to build.
RUN apt-get update && apt-get install -y --no-install-recommends \
      build-essential ca-certificates cmake git ninja-build \
      curl zip unzip tar pkg-config perl python3 linux-libc-dev autoconf automake libtool \
    && rm -rf /var/lib/apt/lists/*

# Parallel compile jobs. gRPC is a long build; on a small machine that also
# runs other things, 1 leaves a core free for them.
ARG JOBS=2
ENV VCPKG_MAX_CONCURRENCY=$JOBS

ENV VCPKG_ROOT=/opt/vcpkg
RUN git clone https://github.com/microsoft/vcpkg "$VCPKG_ROOT" \
    && "$VCPKG_ROOT/bootstrap-vcpkg.sh" -disableMetrics

WORKDIR /src
# The dependencies change rarely; building them in their own layer keeps
# gRPC from being rebuilt for every source change.
COPY vcpkg.json ./
RUN "$VCPKG_ROOT/vcpkg" install --triplet x64-linux --x-install-root=/src/vcpkg_installed \
    && rm -rf "$VCPKG_ROOT/buildtrees" "$VCPKG_ROOT/downloads" "$VCPKG_ROOT/packages"

COPY CMakeLists.txt CMakePresets.json ./
COPY proto ./proto
COPY engine ./engine
COPY server ./server
COPY cli ./cli
RUN cmake --preset linux -DGTO_BUILD_TESTS=OFF \
    && cmake --build --preset linux --target gto-server gto-cli -j "$JOBS"

FROM ubuntu:24.04

RUN useradd --system --create-home --home-dir /data gto
COPY --from=build /src/build/linux/server/gto-server /src/build/linux/cli/gto-cli /usr/local/bin/

USER gto
VOLUME /data
EXPOSE 50051

# Listening on all interfaces of the container requires GTO_SERVER_TOKEN; the
# server refuses to start without it.
ENTRYPOINT ["gto-server", "--host", "0.0.0.0", "--port", "50051", "--data-dir", "/data"]
