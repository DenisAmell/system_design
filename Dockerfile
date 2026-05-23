FROM --platform=linux/amd64 ghcr.io/userver-framework/ubuntu-22.04-userver:latest AS build

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      wget \
      curl \
      git \
      libsqlite3-dev \
      sqlite3 \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY CMakeLists.txt ./
COPY services/ ./services/

RUN --mount=type=cache,target=/root/.cache/ccache \
     cmake -S . -B build \
      -DCMAKE_BUILD_TYPE=Release \
      -DUSERVER_FEATURE_POSTGRESQL=OFF \
      -DUSERVER_FEATURE_REDIS=OFF \
      -DUSERVER_FEATURE_MONGODB=OFF \
      -DUSERVER_FEATURE_GRPC=OFF \
      -DUSERVER_FEATURE_SQLITE=ON \
 && cmake --build build -j"$(nproc)"

FROM --platform=linux/amd64 ghcr.io/userver-framework/ubuntu-22.04-userver:latest AS runtime

RUN apt-get update \
 && apt-get install -y --no-install-recommends libsqlite3-0 \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=build /src/build/user_service     ./user_service
COPY --from=build /src/build/driver_service   ./driver_service
COPY --from=build /src/build/ride_service     ./ride_service
COPY --from=build /src/services/user-service/conf   ./conf/user-service
COPY --from=build /src/services/driver-service/conf ./conf/driver-service
COPY --from=build /src/services/ride-service/conf   ./conf/ride-service

RUN mkdir -p /data
VOLUME ["/data"]

EXPOSE 8080
