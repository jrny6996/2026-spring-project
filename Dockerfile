# syntax=docker/dockerfile:1
FROM golang:1.22-bookworm AS builder

RUN apt-get update && apt-get install -y --no-install-recommends gcc libc6-dev \
	&& rm -rf /var/lib/apt/lists/*

WORKDIR /src/server
COPY server/go.mod server/go.sum ./
RUN go mod download
COPY server/ ./
RUN mkdir -p wasmdist

ENV CGO_ENABLED=1
RUN go build -trimpath -ldflags="-s -w" -o /server .

FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates libsqlite3-0 \
	&& rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /server /usr/local/bin/server
COPY --from=builder /src/server/*.html /app/
COPY --from=builder /src/server/wasmdist /app/wasmdist

ENV HTTP_ADDR=:6789
ENV DATABASE_PATH=/data/app.db

EXPOSE 6789
ENTRYPOINT ["/usr/local/bin/server"]
