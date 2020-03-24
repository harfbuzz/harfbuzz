tests = [
  ['hb-shape-fuzzer.cc'],
  ['hb-subset-fuzzer.cc'],
  ['hb-set-fuzzer.cc'],
  ['hb-draw-fuzzer.cc'],
]

foreach test_data : tests
  fname = test_data[0]
  test_name = fname.split('.')[0].underscorify()

  exe = executable(test_name, [fname, 'main.cc'],
    cpp_args: cpp_args,
    include_directories: [incconfig, incsrc],
    dependencies: deps,
    link_with: [libharfbuzz, libharfbuzz_subset],
  )
  set_variable('@0@_exe'.format(test_name.underscorify()), exe)
endforeach

env = environment()
env.set('srcdir', meson.current_source_dir())

test('shape_fuzzer', find_program('run-shape-fuzzer-tests.py'),
  args: [
    hb_shape_fuzzer_exe,
  ],
  depends: [hb_shape_fuzzer_exe, libharfbuzz, libharfbuzz_subset],
  workdir: join_paths(meson.current_build_dir(), '..', '..'),
  env: env)

test('subset_fuzzer', find_program('run-subset-fuzzer-tests.py'),
  args: [
    hb_subset_fuzzer_exe,
  ],
  # as the tests are ran concurrently let's raise acceptable time here
  # ideally better to break and let meson handles them in parallel
  timeout: 120,
  workdir: join_paths(meson.current_build_dir(), '..', '..'),
  env: env)
