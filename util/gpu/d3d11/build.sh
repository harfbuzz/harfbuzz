#!/bin/sh

# Cross-compile the hb-gpu D3D11 demo with MinGW.
# Run with: wine ./util/gpu/d3d11/hb-gpu-demo-d3d11.exe

set -e

SRCDIR="$(cd "$(dirname "$0")/../../.." && pwd)"
OUTDIR="$(dirname "$0")"

x86_64-w64-mingw32-g++ \
  -std=c++17 \
  -O2 \
  -I"$SRCDIR/src" \
  -I"$SRCDIR/util/gpu/web" \
  -DHB_NO_MT \
  -DHAVE_ROUND \
  -DHB_HAS_GPU \
  -DHB_GPU_NO_GLFW \
  -D_USE_MATH_DEFINES \
  -include gl-stub.h \
  "$SRCDIR/src/harfbuzz-world.cc" \
  "$SRCDIR/util/gpu/demo-atlas.cc" \
  "$SRCDIR/util/gpu/demo-buffer.cc" \
  "$SRCDIR/util/gpu/demo-font.cc" \
  "$SRCDIR/util/gpu/demo-view.cc" \
  "$SRCDIR/util/gpu/d3d11/hb-gpu-d3d11.cc" \
  -ld3d11 -ld3dcompiler_47 -ldxgi \
  -luser32 -lgdi32 \
  -mwindows \
  -static-libgcc -static-libstdc++ \
  -o "$OUTDIR/hb-gpu-demo-d3d11.exe"

echo "Built: $OUTDIR/hb-gpu-demo-d3d11.exe"
echo "Run:   wine $OUTDIR/hb-gpu-demo-d3d11.exe"
