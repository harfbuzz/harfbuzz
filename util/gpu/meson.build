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
  'demo-renderer.h',
  'demo-renderer-gl.cc',
  'demo-shader.h',
  'demo-shader.cc',
  'demo-view.h',
  'demo-view.cc',
  'demo-fragment-glsl.hh',
  'demo-vertex-glsl.hh',
  'default-text-en.hh',
  'default-text-fa.hh',
  'default-font.hh',
  'matrix4x4.hh',
  'trackball.hh',
)

if host_machine.system() == 'darwin'
  gpu_demo_sources += files(
    'demo-metal.mm',
  )
endif

glew_dep = dependency('glew', required: false)
glfw_dep = dependency('glfw3', required: false)
if host_machine.system() == 'darwin'
  gl_dep = dependency('appleframeworks', modules: ['OpenGL'], required: false)
  metal_dep = dependency('appleframeworks', modules: ['Metal', 'QuartzCore', 'Foundation', 'AppKit'], required: false)
else
  gl_dep = dependency('gl', required: false)
  metal_dep = disabler()
endif
have_gpu_demo_deps = glew_dep.found() and glfw_dep.found() and gl_dep.found()
