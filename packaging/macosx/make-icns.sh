#!/usr/bin/env bash
# Generate packaging/macosx/Subsurface.icns from a source SVG (default) or PNG.
# Usage:
#   ./packaging/macosx/make-icns.sh [source.svg|source.png]
# Defaults to icons/subsurface-icon.svg.
# Requires rsvg-convert for SVG sources (brew install librsvg).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

SRC="${1:-$REPO_ROOT/icons/subsurface-icon.svg}"
OUT="$REPO_ROOT/packaging/macosx/Subsurface.icns"
TMPDIR="$(mktemp -d)"
ICONSET="$TMPDIR/Subsurface.iconset"

if [ ! -f "$SRC" ]; then
    echo "error: source file not found: $SRC" >&2
    exit 1
fi

mkdir -p "$ICONSET"

ext="${SRC##*.}"
ext="$(echo "$ext" | tr '[:upper:]' '[:lower:]')"

render() {
    local sz="$1" out="$2"
    if [ "$ext" = "svg" ]; then
        rsvg-convert -w "$sz" -h "$sz" "$SRC" -o "$out"
    else
        sips -z "$sz" "$sz" "$SRC" --out "$out" >/dev/null
    fi
}

if [ "$ext" = "svg" ] && ! command -v rsvg-convert >/dev/null; then
    echo "error: rsvg-convert not found. Install with: brew install librsvg" >&2
    exit 1
fi

# Render every size the .icns format expects, directly from the source.
# Doing each size from the original source gives crisper results than
# downscaling a single 1024x1024 master.
for sz in 16 32 64 128 256 512 1024; do
    render "$sz" "$ICONSET/icon_${sz}x${sz}.png"
done

# Retina (@2x) variants — reuse the next larger base size
cp "$ICONSET/icon_32x32.png"     "$ICONSET/icon_16x16@2x.png"
cp "$ICONSET/icon_64x64.png"     "$ICONSET/icon_32x32@2x.png"
cp "$ICONSET/icon_256x256.png"   "$ICONSET/icon_128x128@2x.png"
cp "$ICONSET/icon_512x512.png"   "$ICONSET/icon_256x256@2x.png"
cp "$ICONSET/icon_1024x1024.png" "$ICONSET/icon_512x512@2x.png"

# These were only needed as @2x sources, not as standalone entries
rm "$ICONSET/icon_64x64.png" "$ICONSET/icon_1024x1024.png"

iconutil -c icns "$ICONSET" -o "$OUT"
rm -rf "$TMPDIR"

echo "wrote $OUT (from $SRC)"
echo "next steps:"
echo "  rm -rf build/Subsurface.app && (cd build && make)"
echo "  # if the new icon doesn't show up in Dock/Finder:"
echo "  killall Dock Finder"
