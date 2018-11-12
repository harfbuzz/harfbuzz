project('harfbuzz', 'c', 'cpp',
  meson_version: '>= 0.47.0',
  version: '1.7.6')

pkgmod = import('pkgconfig')
cpp = meson.get_compiler('cpp')

python3 = import('python').find_installation('python3')

check_headers = [
  ['unistd.h'],
  ['sys/mman.h'],
  ['xlocale.h'],
  ['stdbool.h'],
]

check_funcs = [
  ['atexit'],
  ['mprotect'],
  ['sysconf'],
  ['getpagesize'],
  ['mmap'],
  ['isatty'],
  ['newlocale'],
  ['strtod_l'],
  ['round'],
]

freetype_dep = dependency('freetype2', required: get_option('freetype'),
                          fallback: ['freetype2', 'freetype_dep'])
glib_dep = dependency('glib-2.0', required: get_option('glib'),
                      fallback: ['glib', 'libglib_dep'])
gobject_dep = dependency('gobject-2.0', required: get_option('gobject'),
                         fallback: ['glib', 'libgobject_dep'])
cairo_dep = dependency('cairo', required: get_option('cairo'),
                       fallback: ['cairo', 'libcairo_dep'])
fontconfig_dep = dependency('fontconfig', required: get_option('fontconfig'),
                            fallback: ['fontconfig', 'fontconfig_dep'])
graphite2_dep = dependency('graphite2', required: get_option('graphite'))
icu_dep = dependency('icu-uc', required: get_option('icu'))
m_dep = cpp.find_library('m', required: false)

# Ensure that cairo-ft is fetched from the same library as cairo itself
if cairo_dep.found()
  if cairo_dep.type_name() == 'pkgconfig'
    cairo_ft_dep = dependency('cairo-ft', required: get_option('cairo'))
  else
    cairo_ft_dep = cairo_dep
  endif
else
  # Not-found dependency
  cairo_ft_dep = dependency('', required: false)
endif

deps = []

conf = configuration_data()
incbase = include_directories('.')
cpp_args = ['-DHAVE_CONFIG_H']

warn_cflags = [
  '-Wno-non-virtual-dtor',
]

cpp_args += cpp.get_supported_arguments(warn_cflags)

if m_dep.found()
  deps += [m_dep]
endif

if glib_dep.found()
  conf.set('HAVE_GLIB', 1)
  deps += [glib_dep]
endif

if gobject_dep.found()
  conf.set('HAVE_GOBJECT', 1)
  deps += [gobject_dep]
endif

if cairo_dep.found()
  conf.set('HAVE_CAIRO', 1)
  deps += [cairo_dep]
endif

if cairo_ft_dep.found()
  conf.set('HAVE_CAIRO_FT', 1)
  deps += [cairo_ft_dep]
endif

if graphite2_dep.found()
  conf.set('HAVE_GRAPHITE2', 1)
  deps += [graphite2_dep]
endif

if icu_dep.found()
  conf.set('HAVE_ICU', 1)
  conf.set('HAVE_ICU_BUILTIN', 1)
  deps += [icu_dep]
endif

if freetype_dep.found()
  conf.set('HAVE_FREETYPE', 1)
  deps += [freetype_dep]
  check_freetype_funcs = [
    ['FT_Get_Var_Blend_Coordinates', {'deps': freetype_dep}],
    ['FT_Set_Var_Blend_Coordinates', {'deps': freetype_dep}],
    ['FT_Done_MM_Var', {'deps': freetype_dep}],
  ]

  if freetype_dep.type_name() == 'internal'
    foreach func: check_freetype_funcs
      name = func[0]
      conf.set('HAVE_@0@'.format(name.to_upper()), 1)
    endforeach
  else
    check_funcs += check_freetype_funcs
  endif
endif

if fontconfig_dep.found()
  conf.set('HAVE_FONTCONFIG', 1)
  deps += [fontconfig_dep]
endif

# uniscribe (windows) - FIXME: untested
if host_machine.system() == 'windows' and not get_option('uniscribe').disabled()
  # TODO: make nicer once we have https://github.com/mesonbuild/meson/issues/3940
  if cpp.has_header('usp10.h') and cpp.has_header('windows.h')
    foreach usplib : ['usp10', 'gdi32', 'rpcrt4']
      deps += [cpp.find_library(usplib, required: true)]
    endforeach
    conf.set('HAVE_UNISCRIBE', 1)
  elif get_option('uniscribe').enabled()
    error('uniscribe was enabled explicitly, but some required headers are missing.')
  endif
endif

# DirectWrite (windows) - FIXME: untested
if host_machine.system() == 'windows' and not get_option('directwrite').disabled()
  if cpp.has_header('dwrite.h')
    deps += [cpp.find_library('dwrite', required: true)]
    conf.set('HAVE_DIRECTWRITE', 1)
  elif get_option('directwrite').enabled()
    error('DirectWrite was enabled explicitly, but required header is missing.')
  endif
endif

# threads
if host_machine.system() != 'windows'
  thread_dep = dependency('threads', required: false)

  if thread_dep.found()
    conf.set('HAVE_PTHREAD', 1)
    deps += [thread_dep]
  else
    check_headers += ['sched.h']
    check_funcs += ['sched_yield', {'link_with': 'rt'}]
  endif
endif

conf.set('HAVE_OT', 1)
conf.set('HAVE_FALLBACK', 1)
conf.set_quoted('PACKAGE_NAME', 'HarfBuzz')
conf.set_quoted('PACKAGE_VERSION', meson.project_version())

foreach check : check_headers
  name = check[0]

  if cpp.has_header(name)
    conf.set('HAVE_@0@'.format(name.to_upper().underscorify()), 1)
  endif
endforeach

foreach check : check_funcs
  name = check[0]
  opts = check.get(1, {})
  link_withs = opts.get('link_with', [])
  check_deps = opts.get('deps', [])
  extra_deps = []
  found = true

  # First try without linking

  found = cpp.has_function(name, dependencies: check_deps)

  if not found and link_withs.length() > 0
    found = true

    foreach link_with : link_withs
      dep = cpp.find_library(link_with, required: false)
      if dep.found()
	extra_deps += dep
      else
	found = false
      endif
    endforeach

    if found
      found = cpp.has_function(name, dependencies: check_deps + extra_deps)
    endif
  endif

  if found
    deps += extra_deps
    conf.set('HAVE_@0@'.format(name.to_upper()), 1)
  endif
endforeach

if cpp.links(files('meson-cc-tests/intel-atomic-primitives-test.c'), name: 'Intel atomics')
  conf.set('HAVE_INTEL_ATOMIC_PRIMITIVES', 1)
endif

if cpp.links(files('meson-cc-tests/solaris-atomic-operations.c'), name: 'Solaris atomic ops')
  conf.set('HAVE_SOLARIS_ATOMIC_OPS', 1)
endif

subdir('src')
subdir('util')
subdir('test')

configure_file(output: 'config.h', configuration: conf)
