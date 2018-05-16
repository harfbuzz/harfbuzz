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

hb_base_headers = [
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
]

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

hb_ot_headers = [
  'hb-ot.h',
  'hb-ot-font.h',
  'hb-ot-layout.h',
  'hb-ot-math.h',
  'hb-ot-shape.h',
  'hb-ot-tag.h',
  'hb-ot-var.h',
]

hb_ot_ragel_sources = [
  'hb-ot-shape-complex-indic-machine.rl',
  'hb-ot-shape-complex-khmer-machine.rl',
  'hb-ot-shape-complex-myanmar-machine.rl',
  'hb-ot-shape-complex-use-machine.rl',
]

hb_ft_sources = [
  'hb-ft.cc',
]

hb_ft_headers = [
  'hb-ft.h',
]

hb_glib_sources = [
  'hb-glib.cc',
]

hb_glib_headers = [
  'hb-glib.h',
]

hb_graphite2_sources = [
  'hb-graphite2.cc',
]

hb_graphite2_headers = [
  'hb-graphite2.h',
]

hb_coretext_sources = [
  'hb-coretext.cc',
]

hb_directwrite_sources = [
  'hb-directwrite.cc',
]

hb_uniscribe_sources = [
  'hb-uniscribe.cc',
]

hb_ucdn_sources = [
  'hb-ucdn.cc',
  'hb-ucdn/ucdn.c',
]

hb_icu_sources = [
  'hb-icu.cc',
]

hb_icu_headers = [
  'hb-icu.h',
]

hb_subset_sources = [
  'hb-subset.cc',
  'hb-subset-glyf.cc',
  'hb-subset-input.cc',
  'hb-subset-plan.cc',
]

hb_subset_headers = [
  'hb-subset.h',
  'hb-subset-glyf.hh',
  'hb-subset-plan.hh',
  'hb-subset-private.hh',
]

hb_gobject_sources = [
  'hb-gobject-structs.cc'
]

hb_gobject_headers = [
  'hb-gobject.h',
  'hb-gobject-structs.h',
]

conf.set('HAVE_UCDN', 1)

incucdn = include_directories('hb-ucdn')
incsrc = include_directories('.')

hb_sources = hb_base_sources + hb_fallback_sources + hb_ot_sources + hb_ucdn_sources
hb_headers = hb_base_headers + hb_ot_headers + hb_ucdn_sources

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

libharfbuzz = library('harfbuzz', hb_sources,
  include_directories: [incbase, incucdn],
  dependencies: deps,
  cpp_args: cpp_args,
)

libharfbuzz_subset = library('harfbuzz-subset', hb_subset_sources,
  include_directories: incbase,
  link_with: [libharfbuzz],
  cpp_args: cpp_args)

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
    command: [python3, files('fix_get_types.py')[0], '@INPUT@', '@OUTPUT@']
  )

  hb_gobject_sources += [enum_c]
  hb_gobject_headers += [enum_h]

  libharfbuzz_gobject = library('harfbuzz-gobject', [hb_gobject_sources, enum_c, enum_h],
    include_directories: incbase,
    dependencies: deps,
    link_with: [libharfbuzz],
    cpp_args: cpp_args)
endif
