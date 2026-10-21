#!/usr/bin/env bash
# Native Linux (x86_64) build of the SpiralQuant VST3, CLAP, Standalone and test binary.
# Runs inside the image from Dockerfile, which is the same Ubuntu release the
# artifacts are targeted at.
set -euo pipefail

SRC=${SRC:-/work/spiralquant-linux}
BUILD=${BUILD:-/work/build-linux}
OUT=${OUT:-/work/out/linux}
JOBS=${JOBS:-$(nproc)}

if [ ! -f /src/CMakeLists.txt ] || [ ! -d /src/vendor/JUCE/modules ] || [ ! -d /src/vendor/clap-juce-extensions/clap-libs/clap ]; then
    echo "Mount the repo at /src with JUCE cloned into vendor/JUCE and clap-juce-extensions" >&2
    echo "into vendor/clap-juce-extensions (submodules included); see docs/BUILDING.md." >&2
    exit 1
fi

# Untouched copy: the Windows script patches JUCEUtils.cmake in its own tree, and
# neither build should inherit the other's CMake cache.
mkdir -p "$(dirname "$SRC")"
rm -rf "$SRC"
mkdir -p "$SRC"
tar -C /src --exclude=./build --exclude='./build*' -cf - . | tar -C "$SRC" -xf -

rm -rf "$BUILD"
cmake -S "$SRC" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD" -j"$JOBS" --target SpiralQuant_VST3 SpiralQuant_Standalone SpiralQuant_CLAP SpiralQuantTests

rm -rf "$OUT"
mkdir -p "$OUT"
cp -a "$(find "$BUILD" -maxdepth 5 -name 'SpiralQuant.vst3' -type d)" "$OUT/"
cp -a "$(find "$BUILD" -maxdepth 5 -name 'SpiralQuant.clap' -type f | head -1)" "$OUT/"
cp -f "$(find "$BUILD" -maxdepth 4 -path '*Standalone*' -name 'SpiralQuant' -type f | head -1)" "$OUT/SpiralQuant_Standalone"
cp -f "$(find "$BUILD" -maxdepth 4 -name 'SpiralQuantTests' -type f | head -1)" "$OUT/"

"$OUT/SpiralQuantTests"

echo "Linux artifacts in $OUT"
