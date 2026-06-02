FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    libmysqlclient-dev \
    libhiredis-dev \
    libzip-dev \
    libssl-dev \
    pkg-config \
    ca-certificates \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY CMakeLists.txt .
COPY src/ ./src/
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j$(nproc)

FROM ubuntu:22.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    libmysqlclient21 \
    libhiredis0.14 \
    libzip4 \
    libssl3 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /build/build/backend /usr/local/bin/backend
COPY frontend/ /app/frontend/

WORKDIR /app

EXPOSE 8080

CMD ["backend"]
