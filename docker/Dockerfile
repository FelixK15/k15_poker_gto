# The solver server for Linux.
#
#   docker build -f docker/Dockerfile -t k15-gto-server .
#   docker run -p 50051:50051 -e GTO_SERVER_TOKEN=<long random string> \
#       -v gto-data:/data k15-gto-server
#
# NOTE: this file has not been built yet; nobody has had Docker at hand. The
# steps mirror the Linux instructions in README.md.

FROM ubuntu:24.04 AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
      build-essential ca-certificates cmake curl git ninja-build pkg-config \
      tar unzip zip \
    && rm -rf /var/lib/apt/lists/*

ENV VCPKG_ROOT=/opt/vcpkg
RUN git clone https://github.com/microsoft/vcpkg "$VCPKG_ROOT" \
    && "$VCPKG_ROOT/bootstrap-vcpkg.sh" -disableMetrics

WORKDIR /src
# The dependencies change rarely; building them in their own layer keeps
# gRPC from being rebuilt for every source change.
COPY vcpkg.json ./
RUN "$VCPKG_ROOT/vcpkg" install --triplet x64-linux --x-install-root=/src/vcpkg_installed

COPY CMakeLists.txt CMakePresets.json ./
COPY proto ./proto
COPY engine ./engine
COPY server ./server
COPY cli ./cli
RUN cmake --preset linux -DGTO_BUILD_TESTS=OFF \
    && cmake --build --preset linux --target gto-server

FROM ubuntu:24.04

RUN useradd --system --create-home --home-dir /data gto
COPY --from=build /src/build/linux/server/gto-server /usr/local/bin/gto-server

USER gto
VOLUME /data
EXPOSE 50051

# Listening on all interfaces requires GTO_SERVER_TOKEN; the server refuses
# to start without it.
ENTRYPOINT ["gto-server", "--host", "0.0.0.0", "--port", "50051", "--data-dir", "/data"]
