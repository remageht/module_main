# Gateway production image (Debian bookworm, non-root, healthcheck).
# Build: docker build -t module-main-gateway .
# Run:   docker run --env-file .env -p 1111:1111 module-main-gateway
FROM debian:bookworm-slim AS build
RUN apt-get update \
    && apt-get install -y --no-install-recommends cmake g++ make libssl-dev \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY CMakeLists.txt ./
COPY src ./src
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j"$(nproc)"

FROM debian:bookworm-slim
RUN apt-get update \
    && apt-get install -y --no-install-recommends libssl3 curl \
    && rm -rf /var/lib/apt/lists/* \
    && useradd -r -s /usr/sbin/nologin app
COPY --from=build /src/build/main_module /usr/local/bin/main_module
USER app
EXPOSE 1111
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD curl -fsS "http://127.0.0.1:${PORT:-1111}/health" || exit 1
ENTRYPOINT ["/usr/local/bin/main_module"]
