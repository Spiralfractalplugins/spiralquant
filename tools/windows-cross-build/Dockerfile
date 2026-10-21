# Image for cross-building the SpiralQuant Windows VST3 with MSVC under Wine, and
# for native Linux (Ubuntu 24.04 x86_64) builds. Built for linux/amd64 so the output
# matches the target, also on Apple Silicon (Rosetta).
FROM public.ecr.aws/docker/library/ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
      wine64 \
      winbind \
      python3 \
      msitools \
      ca-certificates \
      curl \
      git \
      cmake \
      ninja-build \
      build-essential \
      pkg-config \
      libasound2-dev \
      libjack-jackd2-dev \
      libcurl4-openssl-dev \
      libdbus-1-dev \
      libfreetype-dev \
      libfontconfig-dev \
      libx11-dev \
      libxcomposite-dev \
      libxcursor-dev \
      libxdamage-dev \
      libxext-dev \
      libxfixes-dev \
      libxi-dev \
      libxinerama-dev \
      libxrandr-dev \
      libxss-dev \
      mesa-common-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# On noble the wine64 package ships only the loader; expose it as `wine64`
# so msvc-wine's `command -v wine64` lookup succeeds.
RUN ln -s /usr/lib/wine/wine64 /usr/local/bin/wine64 && \
    ln -s /usr/lib/wine/wineserver /usr/local/bin/wineserver

# Initialize the wine prefix; wait for wineserver to exit so it isn't corrupted.
RUN wine64 wineboot --init && \
    while pgrep wineserver > /dev/null; do sleep 1; done

WORKDIR /opt/msvc-build

# Pinned so repeat builds use the same MSVC/Windows SDK drop.
ARG MSVC_WINE_REV=514f8ea34842cd6d831804d0e9658d3a32870ae1
RUN git clone https://github.com/mstorsjo/msvc-wine.git && \
    cd msvc-wine && git checkout "$MSVC_WINE_REV"

# Downloads and unpacks MSVC plus the Windows SDK; license accepted explicitly.
RUN cd /opt/msvc-build/msvc-wine && \
    PYTHONUNBUFFERED=1 ./vsdownload.py --accept-license --dest /opt/msvc && \
    ./install.sh /opt/msvc && \
    cp msvcenv-native.sh /opt/msvcenv-native.sh

WORKDIR /work
