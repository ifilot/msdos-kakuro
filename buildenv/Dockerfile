FROM debian:trixie-slim

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    ca-certificates dosbox dosbox-x g++ make python3 \
    && rm -rf /var/lib/apt/lists/*

COPY BC/ /opt/dos-toolchain/BC/
ENV DOS_TOOLCHAIN=/opt/dos-toolchain \
    DOSBOX=dosbox-x \
    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
WORKDIR /workspace
CMD ["make", "smoke"]
