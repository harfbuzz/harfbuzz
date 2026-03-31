#!/bin/sh

# Build the hb-gpu web demo with Emscripten + WebGPU.
# Prerequisites: source emsdk/emsdk_env.sh

set -e

SRCDIR="$(cd "$(dirname "$0")/../../.." && pwd)"
OUTDIR="$(dirname "$0")/out"

mkdir -p "$OUTDIR"

em++ \
  -std=c++17 \
  -Oz -flto \
  -I"$SRCDIR/src" \
  -I"$SRCDIR/util/gpu/web" \
  -DHAVE_CONFIG_H \
  -DHB_GPU_NO_GLFW \
  -include gl-stub.h \
  "$SRCDIR/src/harfbuzz-world.cc" \
  "$SRCDIR/util/gpu/demo-atlas.cc" \
  "$SRCDIR/util/gpu/demo-buffer.cc" \
  "$SRCDIR/util/gpu/demo-font.cc" \
  "$SRCDIR/util/gpu/demo-view.cc" \
  "$SRCDIR/util/gpu/web/hb-gpu-web-webgpu.cc" \
  --use-port=emdawnwebgpu \
  -sALLOW_MEMORY_GROWTH=1 \
  -sEXPORTED_FUNCTIONS='["_main","_web_load_font","_web_set_text","_web_get_text","_web_request_redraw","_web_cancel_gesture","_malloc","_free"]' \
  -sEXPORTED_RUNTIME_METHODS='["UTF8ToString","lengthBytesUTF8","stringToUTF8"]' \
  --shell-file "$SRCDIR/util/gpu/web/shell.html" \
  -o "$OUTDIR/webgpu.html"

echo "Built: $OUTDIR/webgpu.html"
echo "Serve: python3 -m http.server -d $OUTDIR"
