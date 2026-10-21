#!/usr/bin/env bash
# Cross-build the SpiralQuant Windows VST3 + Standalone with MSVC under Wine.
# Runs inside the image from Dockerfile; the repo is mounted read-only at /src,
# this directory under /work/tc.
set -euo pipefail

SRC=${SRC:-/work/spiralquant}
TC=${TC:-/tc}
BUILD=${BUILD:-/work/build-msvc}
OUT=${OUT:-/work/out/windows}
JOBS=${JOBS:-$(nproc)}

export PATH=/opt/msvc/bin/x64:$PATH
export WINEDEBUG=-all

# vendor/JUCE is gitignored, so the checkout has to exist in the mounted repo.
if [ ! -f /src/CMakeLists.txt ] || [ ! -d /src/vendor/JUCE/modules ]; then
    echo "Mount the repo at /src with JUCE cloned into vendor/JUCE (see docs/BUILDING.md)." >&2
    exit 1
fi

# /src is read-only and holds build artifacts; take a clean writable copy.
mkdir -p "$(dirname "$SRC")"
rm -rf "$SRC"
mkdir -p "$SRC"
tar -C /src --exclude=./build --exclude='./build*' -cf - . | tar -C "$SRC" -xf -

# Keep a persistent wine server alive; starting one per compiler invocation is slow.
pgrep wineserver > /dev/null || wineserver -p
wine64 wineboot 2> /dev/null || true

# cl with /Zi, and the PE binaries run through wine, need the DIA matcher plus the
# release and debug CRTs in the prefix - JUCE's VST3 helper builds in Debug config.
S32=$HOME/.wine/drive_c/windows/system32
if [ ! -e "$S32/msdia140.dll" ]; then
    cp "/opt/msvc/DIA SDK/bin/amd64/msdia140.dll" "$S32/"
    wine64 regsvr32 "C:\\windows\\system32\\msdia140.dll" || true
fi
for dir in \
    "$(find /opt/msvc/VC/Redist/MSVC -type d -path '*/x64/Microsoft.VC14*.CRT' | head -1)" \
    "$(find /opt/msvc/VC/Redist/MSVC -type d -path '*/debug_nonredist/x64/*DebugCRT' | head -1)" \
    "$(find /opt/msvc -type d -path '*/bin/*/x64/ucrt' | head -1)"
do
    [ -n "$dir" ] && cp "$dir"/*.dll "$S32/"
done

# Cross-building needs JUCE's nested VST3 helper configure to see the toolchain.
# Only this throwaway copy is touched; vendor/JUCE in the repo stays pristine.
python3 "$TC/patch_juce_vst3_helper.py" "$SRC/vendor/JUCE/extras/Build/CMake/JUCEUtils.cmake"

rm -rf "$BUILD"
cmake -S "$SRC" -B "$BUILD" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$TC/toolchain-msvc.cmake" \
    -DCMAKE_BUILD_TYPE=Release

# In JUCE 8 the "SpiralQuant" target is just the shared code; the formats are separate.
# The first pass can fail inside the nested helper build (its deep object dirs only
# get created partway through), so retry once before giving up.
build_target() {
    cmake --build "$BUILD" --target "$1" -j"$JOBS" || cmake --build "$BUILD" --target "$1" -j"$JOBS"
}

for t in SpiralQuant_VST3 SpiralQuant_Standalone SpiralQuantTests; do
    build_target "$t"
done

rm -rf "$OUT"
mkdir -p "$OUT"
cp -a "$(find "$BUILD" -maxdepth 5 -name 'SpiralQuant.vst3' -type d)" "$OUT/"
cp -f "$(find "$BUILD" -maxdepth 4 -name 'SpiralQuant.exe' -type f | head -1)" "$OUT/"
cp -f "$(find "$BUILD" -maxdepth 4 -name 'SpiralQuantTests.exe' -type f | head -1)" "$OUT/"

# The offline render suite is also the smoke test that the PE binaries run at all.
# It is best effort here: the console test binary can trip the Wine/Rosetta
# translation layer in this container even when the plug-in itself is fine, and
# the native Linux build runs the very same suite as the authoritative gate.
# A failure is reported, but it must not throw the artefacts away.
if ! wine64 "$OUT/SpiralQuantTests.exe"; then
    echo "WARNING: the Windows render suite did not pass under Wine (see above)." >&2
    echo "         The VST3/Standalone were built and copied out; the native" >&2
    echo "         Linux build runs the same suite as the real gate." >&2
fi

echo "Windows artifacts in $OUT"
