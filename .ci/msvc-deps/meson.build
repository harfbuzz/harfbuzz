project('harfbuzz-msvc-deps', ['c', 'cpp'],
  meson_version: '>= 1.0.0',
  default_options: [
    'buildtype=release',
    'default_library=shared',
    'libdir=lib',
    'b_ndebug=if-release',
    'cpp_eh=none',
    'pkgconfig.relocatable=true',
    'glib:tests=false',
    'glib:installed_tests=false',
    'glib:documentation=false',
    'glib:man-pages=disabled',
    'glib:introspection=disabled',
    'glib:sysprof=disabled',
    'libffi:tests=false',
  ],
)

dependency('freetype2', required: true,
  default_options: ['harfbuzz=disabled'])
dependency('glib-2.0', required: true)
dependency('libpng', required: true)
dependency('zlib', required: true)
