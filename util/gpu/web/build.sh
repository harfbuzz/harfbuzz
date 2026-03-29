#!/bin/sh

# Build the hb-gpu web demo with Emscripten.
# Prerequisites: source emsdk/emsdk_env.sh

set -e

SRCDIR="$(cd "$(dirname "$0")/../../.." && pwd)"
OUTDIR="$(dirname "$0")/out"

mkdir -p "$OUTDIR"

em++ \
  -std=c++17 \
  -Oz -flto \
  -I"$SRCDIR/src" \
  -DHB_NO_MT \
  -DHAVE_ROUND \
  -DHB_GPU_ATLAS_2D \
  -DHB_HAS_GPU \
  "$SRCDIR/src/harfbuzz-world.cc" \
  "$SRCDIR/util/gpu/demo-atlas.cc" \
  "$SRCDIR/util/gpu/demo-buffer.cc" \
  "$SRCDIR/util/gpu/demo-font.cc" \
  "$SRCDIR/util/gpu/demo-glstate.cc" \
  "$SRCDIR/util/gpu/demo-renderer-gl.cc" \
  "$SRCDIR/util/gpu/demo-shader.cc" \
  "$SRCDIR/util/gpu/demo-view.cc" \
  "$SRCDIR/util/gpu/web/hb-gpu-web.cc" \
  -sUSE_GLFW=3 \
  -sUSE_WEBGL2=1 \
  -sFULL_ES3=1 \
  -sALLOW_MEMORY_GROWTH=1 \
  -sEXPORTED_FUNCTIONS='["_main","_web_load_font","_web_set_text","_web_get_text","_malloc","_free"]' \
  -sEXPORTED_RUNTIME_METHODS='["UTF8ToString","lengthBytesUTF8","stringToUTF8"]' \
  --shell-file "$SRCDIR/util/gpu/web/shell.html" \
  -o "$OUTDIR/index.html"

echo "Built: $OUTDIR/index.html"
echo "Serve: python3 -m http.server -d $OUTDIR"
