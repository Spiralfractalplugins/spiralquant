#!/usr/bin/env bash
# Assemble a distributable folder (and zip) from the latest Release build.
# Usage: scripts/make_dist.sh [build-dir]   (default: build)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${1:-$ROOT/build}"
ART="$BUILD/SpiralQuant_artefacts"

VERSION="$(sed -n 's/^project(SpiralQuant VERSION \([0-9.]*\).*/\1/p' "$ROOT/CMakeLists.txt")"
[ -n "$VERSION" ] || { echo "could not read version from CMakeLists.txt" >&2; exit 1; }

case "$(uname -s)" in
    Darwin) OS=macos; ARCH=universal ;;
    Linux)  OS=linux;  ARCH="$(uname -m)" ;;
    MINGW*|MSYS*|CYGWIN*) OS=windows; ARCH=x64 ;;
    *) OS=unknown; ARCH="$(uname -m)" ;;
esac

NAME="SpiralQuant-v$VERSION-$OS-$ARCH"
OUT="$ROOT/dist/$NAME"

find_artefact() {
    # Search both single-config (artefacts/VST3) and multi-config (artefacts/Release/VST3) layouts.
    local pattern="$1"
    find "$ART" -maxdepth 4 -name "$pattern" 2>/dev/null | head -1
}

VST3="$(find_artefact 'SpiralQuant.vst3' || true)"
AU="$(find_artefact 'SpiralQuant.component' || true)"
SA="$(find_artefact 'SpiralQuant.app' || true)"

if [ -z "$VST3" ] && [ -z "$AU" ] && [ -z "$SA" ]; then
    echo "no artefacts found under $ART — build first:" >&2
    echo "  cmake --build \"$BUILD\" --config Release --parallel" >&2
    exit 1
fi

rm -rf "$OUT"
mkdir -p "$OUT"
[ -n "$VST3" ] && cp -R "$VST3" "$OUT/"
[ -n "$AU" ]   && cp -R "$AU"   "$OUT/"
[ -n "$SA" ]   && cp -R "$SA"   "$OUT/"
cp "$ROOT/README.md" "$OUT/README.md"
[ -d "$ROOT/docs" ] && cp -R "$ROOT/docs" "$OUT/docs"

INSTALL_NOTE="$OUT/INSTALL.txt"
{
    echo "SpiralQuant v$VERSION ($OS-$ARCH)"
    echo
    echo "VST3:      copy SpiralQuant.vst3 to your VST3 folder"
    echo "           (macOS: ~/Library/Audio/Plug-Ins/VST3)"
    echo "AU:        copy SpiralQuant.component to ~/Library/Audio/Plug-Ins/Components"
    echo "Standalone: run SpiralQuant.app"
} > "$INSTALL_NOTE"

( cd "$ROOT/dist" && rm -f "$NAME.zip" && zip -qry "$NAME.zip" "$NAME" )

echo "dist ready:"
echo "  $OUT"
echo "  $ROOT/dist/$NAME.zip"
