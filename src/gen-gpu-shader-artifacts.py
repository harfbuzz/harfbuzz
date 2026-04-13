#!/usr/bin/env python3

"This tool is intended to be used from meson"

import os, sys, shutil

if len(sys.argv) < 4:
    sys.exit(__doc__)

OUTPUT = sys.argv[1]
CURRENT_SOURCE_DIR = sys.argv[2]
INPUT = sys.argv[3]

hh = os.path.basename(OUTPUT)
decl_map = {
    'hb-gpu-fragment-glsl.hh': 'hb_gpu_fragment_glsl',
    'hb-gpu-vertex-glsl.hh': 'hb_gpu_vertex_glsl',
    'hb-gpu-fragment-msl.hh': 'hb_gpu_fragment_msl',
    'hb-gpu-vertex-msl.hh': 'hb_gpu_vertex_msl',
    'hb-gpu-fragment-wgsl.hh': 'hb_gpu_fragment_wgsl',
    'hb-gpu-vertex-wgsl.hh': 'hb_gpu_vertex_wgsl',
    'hb-gpu-fragment-hlsl.hh': 'hb_gpu_fragment_hlsl',
    'hb-gpu-vertex-hlsl.hh': 'hb_gpu_vertex_hlsl',
    'hb-gpu-draw-fragment-glsl.hh': 'hb_gpu_draw_fragment_glsl',
    'hb-gpu-draw-fragment-msl.hh': 'hb_gpu_draw_fragment_msl',
    'hb-gpu-draw-fragment-wgsl.hh': 'hb_gpu_draw_fragment_wgsl',
    'hb-gpu-draw-fragment-hlsl.hh': 'hb_gpu_draw_fragment_hlsl',
}

decl = decl_map.get(hh)
if not decl:
    sys.exit(f'Unknown shader output: {hh}')

# Stringize the GLSL source into a C string literal
with open(INPUT, 'r') as f:
    lines = f.read().splitlines()

parts = [f'static const char *{decl} =\n']
for line in lines:
    line = line.replace('\\', '\\\\').replace('"', '\\"')
    parts.append(f'"{line}\\n"\n')
parts.append(';\n')

content = ''.join(parts)

with open(OUTPUT, 'w') as f:
    f.write(content)

# Copy back to source tree; if read-only, verify committed copy matches.
src_copy = os.path.join(CURRENT_SOURCE_DIR, hh)
try:
    shutil.copyfile(OUTPUT, src_copy)
except OSError:
    import filecmp
    if not filecmp.cmp(OUTPUT, src_copy, shallow=False):
        sys.exit(f'{src_copy} is out of date; regenerate with a writable source tree')
