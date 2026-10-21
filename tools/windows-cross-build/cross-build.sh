#!/usr/bin/env bash
# Build SpiralQuant for Windows (MSVC under Wine) and Linux from a macOS/Linux host,
# without a matching machine. Requires Docker (or OrbStack); on Apple Silicon the
# image is amd64 and runs under Rosetta, so the first build is slow.
#
#   ./tools/windows-cross-build/cross-build.sh            # both platforms
#   ./tools/windows-cross-build/cross-build.sh windows    # Windows only
#   ./tools/windows-cross-build/cross-build.sh linux      # Linux only
#
# The image and its /work volume are kept for repeat builds.
set -euo pipefail

REPO=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
TOOLS="$REPO/tools/windows-cross-build"
IMAGE=${IMAGE:-spiralquant-msvc:1}
CONTAINER=${CONTAINER:-spiralquant-msvc}
WORK_VOLUME=${WORK_VOLUME:-spiralquant-msvc-work}
TARGETS=${*:-windows linux}

if [ ! -d "$REPO/vendor/JUCE/modules" ] || [ ! -d "$REPO/vendor/clap-juce-extensions/clap-libs/clap" ]; then
    echo "Vendored dependencies are missing - run:" >&2
    echo "  git clone --depth 1 --branch 8.0.14 https://github.com/juce-framework/JUCE.git vendor/JUCE" >&2
    echo "  git clone https://github.com/dsp56300/clap-juce-extensions.git vendor/clap-juce-extensions" >&2
    echo "  git -C vendor/clap-juce-extensions checkout e1f67893cc409a40c1154fa2e78c97046da24ce0" >&2
    echo "  git -C vendor/clap-juce-extensions submodule update --init --recursive" >&2
    exit 1
fi

# The image layer that downloads MSVC is expensive; rebuild only on demand.
if [ -n "${REBUILD:-}" ] || [ -z "$(docker images -q "$IMAGE")" ]; then
    docker build --platform linux/amd64 -t "$IMAGE" -f "$TOOLS/Dockerfile" "$TOOLS"
fi

# Long-lived container: the wine prefix, /work and the MSVC download survive rebuilds.
if [ -z "$(docker ps -q -a -f name="^${CONTAINER}$")" ]; then
    docker volume create "$WORK_VOLUME" > /dev/null
    docker run -d --name "$CONTAINER" --platform linux/amd64 \
        -v "$WORK_VOLUME:/work" -v "$REPO:/src:ro" \
        --entrypoint sleep "$IMAGE" infinity > /dev/null
fi
docker start "$CONTAINER" > /dev/null

# Copied in rather than bind-mounted, so the container works regardless of which
# host directory it was originally started with.
docker exec "$CONTAINER" mkdir -p /work/tc
docker cp "$TOOLS/." "$CONTAINER:/work/tc/"

for target in $TARGETS; do
    echo "=== building $target ==="
    docker exec "$CONTAINER" rm -rf "/work/out/$target"
    docker exec -e TC=/work/tc "$CONTAINER" bash "/work/tc/build-$target.sh"
    rm -rf "${REPO:?}/build/$target"
    mkdir -p "$REPO/build/$target"
    docker cp "$CONTAINER:/work/out/$target/." "$REPO/build/$target/"
done

find "$REPO/build" -maxdepth 4 \( -name 'SpiralQuant.vst3' -o -name 'SpiralQuant.clap' \
    -o -name 'SpiralQuant*.exe' -o -name 'SpiralQuant_Standalone' -o -name 'SpiralQuantTests' \)
