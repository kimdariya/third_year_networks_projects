FROM gcc:13 AS builder

RUN apt-get update && \
    apt-get install -y --no-install-recommends wget ca-certificates make && \
    wget -q https://github.com/Kitware/CMake/releases/download/v3.29.6/cmake-3.29.6-linux-aarch64.tar.gz && \
    tar -xzf cmake-3.29.6-linux-aarch64.tar.gz -C /opt && \
    ln -s /opt/cmake-3.29.6-linux-aarch64/bin/cmake /usr/local/bin/cmake && \
    rm cmake-3.29.6-linux-aarch64.tar.gz && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY include/       ./include/
COPY src/           ./src/
COPY CMakeLists.txt ./

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build -j

FROM gcc:13

WORKDIR /app
COPY --from=builder /app/build/lab_1 /app/lab_1

ENTRYPOINT ["/app/lab_1"]