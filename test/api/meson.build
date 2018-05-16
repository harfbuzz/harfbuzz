tests = [
  ['test-blob.c'],
  ['test-buffer.c'],
  ['test-common.c'],
  ['test-font.c'],
  ['test-object.c'],
  ['test-set.c'],
  ['test-shape.c'],
  ['test-subset.c'],
  ['test-subset-codepoints.c'],
  ['test-subset-cmap.c'],
  ['test-subset-glyf.c'],
  ['test-subset-hdmx.c'],
  ['test-subset-hmtx.c'],
  ['test-subset-os2.c'],
  ['test-subset-post.c'],
  ['test-subset-vmtx.c'],
  ['test-unicode.c'],
  ['test-version.c'],
  ['test-ot-color.c'],
  ['test-ot-tag.c'],
]

if conf.get('HAVE_FREETYPE', 0) == 1
  tests += [['test-ot-math.c']]
endif

if conf.get('HAVE_GLIB', 0) == 1
  env = environment()
  env.set('G_TEST_SRCDIR', meson.current_source_dir())
  env.set('G_TEST_BUILDDIR', meson.current_build_dir())

  foreach test_data : tests
    fname = test_data[0]
    opts = test_data.length() > 1 ? test_data[1] : {}
    extra_c_args = opts.get('c_args', [])



    test_name = fname.split('.')[0].underscorify()
    exe = executable(test_name, fname,
      cpp_args: cpp_args + extra_c_args,
      include_directories: [incbase, incsrc],
      dependencies: deps,
      link_with: [libharfbuzz, libharfbuzz_subset],
    )

    test(test_name, exe,
      env: env)
  endforeach
endif
