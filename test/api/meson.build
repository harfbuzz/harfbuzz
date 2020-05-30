if conf.get('HAVE_GLIB', 0) == 0
  message('You need to have glib support enabled to run test/api tests')
  subdir_done()
endif

tests = [
  'test-aat-layout.c',
  'test-baseline.c',
  'test-blob.c',
  'test-buffer.c',
  'test-c.c',
  'test-collect-unicodes.c',
  'test-cplusplus.cc',
  'test-common.c',
  'test-draw.c',
  'test-font.c',
  'test-map.c',
  'test-object.c',
  'test-ot-color.c',
  'test-ot-face.c',
  'test-ot-glyphname.c',
  'test-ot-ligature-carets.c',
  'test-ot-name.c',
  'test-ot-meta.c',
  'test-ot-metrics.c',
  'test-ot-tag.c',
  'test-ot-extents-cff.c',
  'test-ot-metrics-tt-var.c',
  'test-set.c',
  'test-shape.c',
  'test-subset.c',
  'test-subset-cmap.c',
  'test-subset-drop-tables.c',
  'test-subset-glyf.c',
  'test-subset-hdmx.c',
  'test-subset-hmtx.c',
  'test-subset-nameids.c',
  'test-subset-os2.c',
  'test-subset-post.c',
  'test-subset-vmtx.c',
  'test-subset-cff1.c',
  'test-subset-cff2.c',
  'test-subset-gvar.c',
  'test-subset-hvar.c',
  'test-subset-vvar.c',
  'test-subset-sbix.c',
  'test-subset-gpos.c',
  'test-subset-colr.c',
  'test-subset-cbdt.c',
  'test-unicode.c',
  'test-var-coords.c',
  'test-version.c',
]

if conf.get('HAVE_FREETYPE', 0) == 1
  tests += 'test-ot-math.c'
endif

if conf.get('HAVE_FREETYPE', 0) == 1 and conf.get('HAVE_PTHREAD', 0) == 1
  tests += 'test-multithread.c'
endif

# Default test running environment
env = environment()
env.set('MALLOC_CHECK_', '2')
env.set('G_DEBUG', 'gc-friendly')
env.set('G_SLICE', 'always-malloc')
env.set('G_TEST_SRCDIR', meson.current_source_dir())
env.set('G_TEST_BUILDDIR', meson.current_build_dir())

foreach source : tests
  test_name = source.split('.')[0]

  link_withs = libharfbuzz
  if test_name.contains('-subset')
    link_withs = [libharfbuzz, libharfbuzz_subset]
  elif test_name == 'test-unicode' and have_icu and not have_icu_builtin
    link_withs = [libharfbuzz, libharfbuzz_icu]
  endif

  test(test_name, executable(test_name, source,
    include_directories: [incconfig, incsrc],
    dependencies: deps,
    link_with: link_withs,
    install: false,
  ), env: env, suite: ['api'] + (test_name.contains('-subset') ? ['subset'] : []))
endforeach
