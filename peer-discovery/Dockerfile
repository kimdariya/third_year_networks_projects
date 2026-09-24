FROM gcc:13 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
        cmake g++ make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY include/       ./include/
COPY src/           ./src/
COPY CMakeLists.txt ./

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build -j

FROM gcc:13

WORKDIR /app
COPY --from=builder /app/build/peer-discovery /usr/local/bin/peer-discovery

ENTRYPOINT ["peer-discovery"]