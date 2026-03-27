gpu_demo_sources = [
  'demo-atlas.h',
  'demo-atlas.cc',
  'demo-buffer.h',
  'demo-buffer.cc',
  'demo-common.h',
  'demo-font.h',
  'demo-font.cc',
  'demo-glstate.h',
  'demo-glstate.cc',
  'demo-shader.h',
  'demo-shader.cc',
  'demo-view.h',
  'demo-view.cc',
  'hb-gpu-demo.cc',
  'matrix4x4.h',
  'matrix4x4.c',
  'trackball.h',
  'trackball.c',
]

gpu_demo_shaders = [
  [ 'demo-fragment.glsl', 'demo-fragment-glsl.h', 'demo_fragment_glsl' ],
  [ 'demo-vertex.glsl', 'demo-vertex-glsl.h', 'demo_vertex_glsl' ],
  [ 'default-text.txt', 'default-text.h', 'default_text' ],
]

gpu_stringize = find_program('../../src/stringize')

gpu_demo_shader_sources = []

foreach p: gpu_demo_shaders
  glsl = p.get(0)
  header = p.get(1)
  decl = p.get(2)
  gpu_demo_shader_sources += custom_target('@0@ shader'.format(glsl),
    input: glsl,
    output: header,
    capture: true,
    command: [ gpu_stringize, 'static const char *' + decl, '@INPUT@' ])
endforeach

glew_dep = dependency('glew', required: true)
glfw_dep = dependency('glfw3', required: true)
gl_dep = dependency('gl', required: true)

hexify = find_program('hexify')

gpu_demo_shader_sources += custom_target('default font',
  input: 'default-font.ttf',
  output: 'default-font.h',
  capture: true,
  command: [ hexify, 'static const char default_font[]', '@INPUT@' ])

hb_gpu_demo = executable('hb-gpu', gpu_demo_sources + gpu_demo_shader_sources,
  cpp_args: cpp_args,
  include_directories: [incconfig, incsrc],
  dependencies: [gl_dep, glew_dep, glfw_dep],
  link_with: [libharfbuzz, libharfbuzz_gpu],
  install: false,
)
