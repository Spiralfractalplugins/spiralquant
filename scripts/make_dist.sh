#!/usr/bin/env bash
# Assemble a distributable folder (and zip) from a built tree.
# Usage: scripts/make_dist.sh [build-dir] [platform]
#   build-dir: default "build" (a native build; the artefacts folder is used)
#   platform:  overrides the host autodetection - "windows" or "linux" point at
#              the cross-build output of tools/windows-cross-build/cross-build.sh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${1:-$ROOT/build}"
PLATFORM="${2:-}"

case "$(basename "$BUILD")" in
    windows) PLATFORM=${PLATFORM:-windows} ;;
    linux)   PLATFORM=${PLATFORM:-linux} ;;
esac

VERSION="$(sed -n 's/^project(SpiralQuant VERSION \([0-9.]*\).*/\1/p' "$ROOT/CMakeLists.txt")"
[ -n "$VERSION" ] || { echo "could not read version from CMakeLists.txt" >&2; exit 1; }

if [ -z "$PLATFORM" ]; then
    case "$(uname -s)" in
        Darwin) PLATFORM=macos ;;
        Linux)  PLATFORM=linux ;;
        MINGW*|MSYS*|CYGWIN*) PLATFORM=windows ;;
        *) echo "unknown host platform - pass it explicitly" >&2; exit 1 ;;
    esac
fi

case "$PLATFORM" in
    macos)   ART="$BUILD/SpiralQuant_artefacts"; NAME="SpiralQuant-v$VERSION-macos-universal" ;;
    windows) ART="$BUILD";                       NAME="SpiralQuant-v$VERSION-windows-x64" ;;
    linux)   ART="$BUILD";                       NAME="SpiralQuant-v$VERSION-linux-x86_64" ;;
    *) echo "platform must be macos, windows or linux" >&2; exit 1 ;;
esac
OUT="$ROOT/dist/$NAME"

[ -d "$ART" ] || { echo "no build output in $ART - build first:" >&2
    echo "  cmake --build \"$BUILD\" --config Release --parallel" >&2; exit 1; }

find_artefact() {
    # Search both single-config (artefacts/VST3) and multi-config (artefacts/Release/VST3) layouts.
    local pattern="$1"
    find "$ART" -not -path '*/CMakeFiles/*' -maxdepth 4 -name "$pattern" 2>/dev/null | head -1 || true
}

VST3="$(find_artefact 'SpiralQuant.vst3')"
[ -n "$VST3" ] || { echo "SpiralQuant.vst3 not found under $ART" >&2; exit 1; }

rm -rf "$OUT"
mkdir -p "$OUT"
cp -R "$VST3" "$OUT/"

case "$PLATFORM" in
    macos)
        AU="$(find_artefact 'SpiralQuant.component')"
        SA="$(find_artefact 'SpiralQuant.app')"
        CLAP="$(find_artefact 'SpiralQuant.clap')"
        [ -n "$AU" ] && cp -R "$AU" "$OUT/"
        [ -n "$SA" ] && cp -R "$SA" "$OUT/"
        [ -n "$CLAP" ] && cp -R "$CLAP" "$OUT/"
        INSTALL_LINES="VST3:       copy SpiralQuant.vst3 to ~/Library/Audio/Plug-Ins/VST3
AU:         copy SpiralQuant.component to ~/Library/Audio/Plug-Ins/Components
CLAP:       copy SpiralQuant.clap to ~/Library/Audio/Plug-Ins/CLAP
Standalone: open SpiralQuant.app"
        ;;
    windows)
        SA="$(find_artefact 'SpiralQuant.exe')"
        CLAP="$(find_artefact 'SpiralQuant.clap')"
        [ -n "$SA" ] && cp "$SA" "$OUT/"
        [ -n "$CLAP" ] && cp -R "$CLAP" "$OUT/"
        INSTALL_LINES="VST3:       copy SpiralQuant.vst3 to C:\\Program Files\\Common Files\\VST3
CLAP:       copy SpiralQuant.clap to C:\\Program Files\\Common Files\\CLAP
Standalone: run SpiralQuant.exe"
        ;;
    linux)
        SA="$(find_artefact 'SpiralQuant_Standalone' || true)"
        [ -z "$SA" ] && SA="$(find_artefact 'SpiralQuant' | grep -i standalone || true)"
        CLAP="$(find_artefact 'SpiralQuant.clap')"
        [ -n "$SA" ] && cp "$SA" "$OUT/SpiralQuant_Standalone"
        [ -n "$CLAP" ] && cp -R "$CLAP" "$OUT/"
        INSTALL_LINES="VST3:       copy SpiralQuant.vst3 to ~/.vst3
CLAP:       copy SpiralQuant.clap to ~/.clap
Standalone: run ./SpiralQuant_Standalone"
        ;;
esac

cp "$ROOT/README.md" "$OUT/README.md"
[ -d "$ROOT/docs" ] && cp -R "$ROOT/docs" "$OUT/docs"

{
    echo "SpiralQuant v$VERSION ($PLATFORM)"
    echo
    printf '%s\n' "$INSTALL_LINES"
} > "$OUT/INSTALL.txt"

( cd "$ROOT/dist" && rm -f "$NAME.zip" && zip -qry "$NAME.zip" "$NAME" )

echo "dist ready:"
echo "  $OUT"
echo "  $ROOT/dist/$NAME.zip"
