FROM debian:trixie-slim AS build
RUN apt-get update && apt-get install -y --no-install-recommends g++ make pkg-config libpq-dev libcurl4-openssl-dev libssl-dev libsqlite3-dev
WORKDIR /app
COPY Makefile ./
COPY cpp ./cpp
COPY vendor ./vendor
RUN make -j2

FROM debian:trixie-slim
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates libpq5 libcurl4t64 libssl3t64 libsqlite3-0 curl && rm -r /var/lib/apt/lists/*
WORKDIR /app
COPY --from=build /app/build/carlog-server /app/build/carlog-import /usr/local/bin/
COPY cpp/schema.sql /app/cpp/schema.sql
USER 65534:65534
CMD ["sh", "-c", "carlog-server --init && exec carlog-server"]
