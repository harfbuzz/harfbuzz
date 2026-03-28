gpu_demo_sources = files(
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
  'demo-fragment-glsl.hh',
  'demo-vertex-glsl.hh',
  'default-text.hh',
  'default-font.hh',
  'matrix4x4.hh',
  'trackball.hh',
)

glew_dep = dependency('glew', required: false)
glfw_dep = dependency('glfw3', required: false)
gl_dep = dependency('gl', required: false)
have_gpu_demo_deps = glew_dep.found() and glfw_dep.found() and gl_dep.found()
