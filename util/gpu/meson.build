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
  'default-text-en.hh',
  'default-text-fa.hh',
  'default-font.hh',
  'matrix4x4.hh',
  'trackball.hh',
)

demo_shader_sources = [
  [ 'demo-vertex.glsl',   'demo-vertex-glsl.hh' ],
  [ 'demo-fragment.glsl', 'demo-fragment-glsl.hh' ],
]

demo_shader_helper = find_program('../../src/gen-gpu-shader-artifacts.py')
foreach p: demo_shader_sources
  gpu_demo_sources += custom_target('@0@'.format(p.get(1)),
    build_by_default: true,
    input: p.get(0),
    output: p.get(1),
    command: [ demo_shader_helper, '@OUTPUT@',
	       meson.current_source_dir(), '@INPUT@' ])
endforeach

if host_machine.system() == 'darwin'
  gpu_demo_sources += files(
    'demo-metal.mm',
  )
endif

if host_machine.system() == 'windows'
  gpu_demo_sources += files(
    'demo-renderer-d3d11.cc',
  )
endif

gpu_demo_opt = get_option('gpu_demo')
have_gpu_demo_deps = false
if not gpu_demo_opt.disabled()
  glew_dep = dependency('glew', required: gpu_demo_opt)
  glfw_dep = dependency('glfw3', required: gpu_demo_opt)
  if host_machine.system() == 'darwin'
    gl_dep = dependency('appleframeworks', modules: ['OpenGL'], required: gpu_demo_opt)
    metal_dep = dependency('appleframeworks', modules: ['Metal', 'QuartzCore', 'Foundation', 'AppKit'], required: gpu_demo_opt)
  else
    gl_dep = dependency('gl', required: gpu_demo_opt)
    metal_dep = disabler()
  endif
  d3d11_dep = meson.get_compiler('cpp').find_library('d3d11', required: false)
  d3dcompiler_dep = meson.get_compiler('cpp').find_library('d3dcompiler_47', required: false)
  have_gpu_demo_deps = glew_dep.found() and glfw_dep.found() and gl_dep.found()
endif
