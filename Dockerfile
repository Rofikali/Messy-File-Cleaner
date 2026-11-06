FROM ubuntu:24.04 AS build
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends build-essential cmake ninja-build && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY CMakeLists.txt ./
COPY include/ ./include/
COPY src/ ./src/
COPY configs/ ./configs/
RUN cmake -S . -B /build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=/usr/local && cmake --build /build --parallel && cmake --install /build

FROM ubuntu:24.04 AS runtime
ENV DEBIAN_FRONTEND=noninteractive
RUN groupadd --gid 10001 cleaner && useradd --uid 10001 --gid cleaner --create-home --shell /usr/sbin/nologin cleaner && install -d -o cleaner -g cleaner /output /output/journal
COPY --from=build /usr/local/bin/file-cleaner /usr/local/bin/file-cleaner
COPY --chown=cleaner:cleaner configs/ /app/configs/
ENV HOME=/home/cleaner
WORKDIR /output
USER cleaner:cleaner
ENTRYPOINT ["/usr/local/bin/file-cleaner"]
