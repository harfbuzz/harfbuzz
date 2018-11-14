hb_base_sources = [
  'hb-blob.cc',
  'hb-buffer-serialize.cc',
  'hb-buffer.cc',
  'hb-common.cc',
  'hb-face.cc',
  'hb-font.cc',
  'hb-ot-tag.cc',
  'hb-set.cc',
  'hb-shape.cc',
  'hb-shape-plan.cc',
  'hb-shaper.cc',
  'hb-unicode.cc',
  'hb-warning.cc',
]

hb_base_headers = files([
  'hb.h',
  'hb-blob.h',
  'hb-buffer.h',
  'hb-common.h',
  'hb-deprecated.h',
  'hb-face.h',
  'hb-font.h',
  'hb-set.h',
  'hb-shape.h',
  'hb-shape-plan.h',
  'hb-unicode.h',
  'hb-version.h',
])

hb_base_ragel_sources = [
  'hb-buffer-deserialize-json.rl',
  'hb-buffer-deserialize-text.rl',
]

hb_fallback_sources = [
  'hb-fallback-shape.cc',
]

hb_ot_sources = [
  'hb-aat-layout.cc',
  'hb-ot-font.cc',
  'hb-ot-layout.cc',
  'hb-ot-color.cc',
  'hb-ot-map.cc',
  'hb-ot-math.cc',
  'hb-ot-shape.cc',
  'hb-ot-shape-complex-arabic.cc',
  'hb-ot-shape-complex-default.cc',
  'hb-ot-shape-complex-hangul.cc',
  'hb-ot-shape-complex-hebrew.cc',
  'hb-ot-shape-complex-indic.cc',
  'hb-ot-shape-complex-indic-table.cc',
  'hb-ot-shape-complex-khmer.cc',
  'hb-ot-shape-complex-myanmar.cc',
  'hb-ot-shape-complex-thai.cc',
  'hb-ot-shape-complex-tibetan.cc',
  'hb-ot-shape-complex-use.cc',
  'hb-ot-shape-complex-use-table.cc',
  'hb-ot-shape-normalize.cc',
  'hb-ot-shape-fallback.cc',
  'hb-ot-var.cc',
]

hb_ot_headers = files([
  'hb-ot.h',
  'hb-ot-font.h',
  'hb-ot-layout.h',
  'hb-ot-math.h',
  'hb-ot-shape.h',
  'hb-ot-tag.h',
  'hb-ot-var.h',
])

hb_ot_ragel_sources = [
  'hb-ot-shape-complex-indic-machine.rl',
  'hb-ot-shape-complex-khmer-machine.rl',
  'hb-ot-shape-complex-myanmar-machine.rl',
  'hb-ot-shape-complex-use-machine.rl',
]

hb_ft_sources = [
  'hb-ft.cc',
]

hb_ft_headers = files([
  'hb-ft.h',
])

hb_glib_sources = [
  'hb-glib.cc',
]

hb_glib_headers = files([
  'hb-glib.h',
])

hb_graphite2_sources = [
  'hb-graphite2.cc',
]

hb_graphite2_headers = files([
  'hb-graphite2.h',
])

hb_icu_sources = [
  'hb-icu.cc',
]

hb_icu_headers = files([
  'hb-icu.h',
])

hb_subset_sources = [
  'hb-subset.cc',
  'hb-subset-glyf.cc',
  'hb-subset-input.cc',
  'hb-subset-plan.cc',
]

hb_subset_headers = files([
  'hb-subset.h',
  'hb-subset-glyf.hh',
  'hb-subset-plan.hh',
  'hb-subset-private.hh',
])

hb_gobject_sources = [
  'hb-gobject-structs.cc'
]

hb_gobject_headers = files([
  'hb-gobject.h',
  'hb-gobject-structs.h',
])

incucdn = []
incsrc = include_directories('.')

hb_sources = hb_base_sources + hb_fallback_sources + hb_ot_sources
hb_headers = hb_base_headers + hb_ot_headers

if not get_option('ucdn').disabled()
  conf.set('HAVE_UCDN', 1)
  hb_sources += ['hb-ucdn.cc', 'hb-ucdn/ucdn.c']
  incucdn = include_directories('hb-ucdn')
endif

if conf.get('HAVE_FREETYPE', 0) == 1
  hb_sources += hb_ft_sources
  hb_headers += hb_ft_headers
endif

if conf.get('HAVE_GRAPHITE2', 0) == 1
  hb_sources += hb_graphite2_sources
  hb_headers += hb_graphite2_headers
endif

if conf.get('HAVE_GLIB', 0) == 1
  hb_sources += hb_glib_sources
  hb_headers += hb_glib_headers
endif

if conf.get('HAVE_ICU', 0) == 1
  hb_sources += hb_icu_sources
  hb_headers += hb_icu_headers
endif

if conf.get('HAVE_UNISCRIBE', 0) == 1
  hb_sources += ['hb-uniscribe.cc']
  hb_headers += ['hb-uniscribe.h']
endif

if conf.get('HAVE_DIRECTWRITE', 0) == 1
  hb_sources += ['hb-directwrite.cc']
  hb_headers += ['hb-directwrite.h']
endif

if conf.get('HAVE_CORETEXT', 0) == 1
  hb_sources += ['hb-coretext.cc']
  hb_headers += ['hb-coretext.h']
endif

# harfbuzz
gen_def = find_program('gen-def.py')
harfbuzz_def = custom_target('harfbuzz.def',
    command: [gen_def, '@OUTPUT@', '@INPUT@'],
    input: hb_headers,
    output: 'harfbuzz.def')

version = '0.' + '0'.join(meson.project_version().split('.')) + '.0'

libharfbuzz = library('harfbuzz', hb_sources,
  include_directories: [incbase, incucdn],
  dependencies: deps,
  cpp_args: cpp_args,
  vs_module_defs: harfbuzz_def,
  soversion: '0',
  version: version,
  install: true,
  name_prefix: 'lib')

libharfbuzz_dep = declare_dependency(
  link_with: libharfbuzz,
  include_directories: [incsrc, incucdn],
  dependencies: deps)

# harfbuzz-subset
harfbuzz_subset_def = custom_target('harfbuzz-subset.def',
    command: [gen_def, '@OUTPUT@', '@INPUT@'],
    input: hb_subset_headers,
    output: 'harfbuzz-subset.def')

libharfbuzz_subset = library('harfbuzz-subset', hb_subset_sources,
  include_directories: incbase,
  link_with: [libharfbuzz],
  cpp_args: cpp_args,
  vs_module_defs: harfbuzz_subset_def,
  soversion: '0',
  version: version,
  install: true,
  name_prefix: 'lib')

libharfbuzz_subset_dep = declare_dependency(
  link_with: libharfbuzz_subset,
  include_directories: incsrc,
  dependencies: deps)

pkgmod.generate(libharfbuzz,
  description: 'HarfBuzz text shaping library',
  subdirs: [meson.project_name()],
  version: meson.project_version(),
)

pkgmod.generate(libharfbuzz_subset,
  description: 'HarfBuzz font subsetter',
  subdirs: [meson.project_name()],
  version: meson.project_version(),
)

if conf.get('HAVE_GOBJECT', 0) == 1
  gnome = import('gnome')

  h_templ = configure_file(
    input: 'hb-gobject-enums.h.tmpl',
    output: 'hb-gobject-enums-tmp.h.tmpl',
    configuration: configuration_data(),
    format: 'cmake')

  cc_templ = configure_file(
    input: 'hb-gobject-enums.cc.tmpl',
    output: 'hb-gobject-enums-tmp.cc.tmpl',
    configuration: configuration_data(),
    format: 'cmake')

  enums = gnome.mkenums('hb-gobject',
    sources: hb_headers,
    h_template: h_templ,
    c_template: cc_templ,
    identifier_prefix: 'hb_',
    symbol_prefix: 'hb_gobject',
  )

  enum_c = custom_target('hb-gobject-enums.cc',
    input: enums[0],
    output: 'hb-gobject-enums.cc',
    command: [python3, files('fix_get_types.py')[0], '@INPUT@', '@OUTPUT@']
  )

  enum_h = custom_target('hb-gobject-enums.h',
    input: enums[1],
    output: 'hb-gobject-enums.h',
    command: [python3, files('fix_get_types.py')[0], '@INPUT@', '@OUTPUT@'],
    install: true,
    install_dir: join_paths(get_option('prefix'), get_option('includedir'), meson.project_name()),
  )

  hb_gobject_sources += [enum_c]

  harfbuzz_gobject_def = custom_target('harfbuzz-gobject.def',
    command: [gen_def, '@OUTPUT@', '@INPUT@'],
    input: [hb_gobject_headers, enum_h],
    output: 'harfbuzz-gobject.def')

  libharfbuzz_gobject = library('harfbuzz-gobject', [hb_gobject_sources, enum_c, enum_h],
    include_directories: incbase,
    dependencies: deps,
    link_with: [libharfbuzz],
    cpp_args: cpp_args,
    vs_module_defs: harfbuzz_gobject_def,
    soversion: '0',
    version: version,
    install: true,
    name_prefix: 'lib')

  gir = find_program('g-ir-scanner', required: get_option('introspection'))
  build_gir = gir.found() and not meson.is_cross_build()

  if build_gir
    hb_gen_files_gir = gnome.generate_gir(libharfbuzz, libharfbuzz_gobject,
      sources: [hb_headers, hb_sources, hb_gobject_headers, hb_gobject_sources, enum_h],
      namespace: 'HarfBuzz',
      nsversion: '0.0',
      identifier_prefix: 'hb_',
      symbol_prefix: 'hb_',
      includes: ['GObject-2.0'],
      install: true,
      extra_args:  ['--cflags-begin',
                    '-DHB_H',
                    '-DHB_H_IN',
                    '-DHB_OT_H',
                    '-DHB_OT_H_IN',
                    '-DHB_GOBJECT_H',
                    '-DHB_GOBJECT_H_IN',
                    '-DHB_EXTERN=',
                    '--cflags-end'])
  endif

  libharfbuzz_gobject_dep = declare_dependency(
    link_with: libharfbuzz_gobject,
    include_directories: incsrc,
    sources: hb_gen_files_gir,
    dependencies: deps)

  pkgmod.generate(libharfbuzz_gobject,
    description: 'HarfBuzz text shaping library GObject integration',
    subdirs: [meson.project_name()],
    version: meson.project_version(),
  )
else
  libharfbuzz_gobject_dep = dependency('', required: false)
endif

install_headers(hb_headers + hb_gobject_headers + hb_subset_headers, subdir: meson.project_name())
