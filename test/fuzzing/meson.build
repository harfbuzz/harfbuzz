tests = [
  ['hb-shape-fuzzer.cc'],
  ['hb-subset-fuzzer.cc'],
  ['hb-subset-get-codepoints-fuzzer.cc'],
]

foreach test_data : tests
  fname = test_data[0]
  test_name = fname.split('.')[0].underscorify()

  exe = executable(test_name, [fname, 'main.cc'],
    cpp_args: cpp_args,
    include_directories: [incbase, incsrc],
    dependencies: deps,
    link_with: [libharfbuzz, libharfbuzz_subset],
  )
  set_variable('@0@_exe'.format(test_name.underscorify()), exe)
endforeach

env = environment()
env.set('srcdir', meson.current_source_dir())

run_shape_fuzzer_tests = find_program('run-shape-fuzzer-tests.py')

test('shape_fuzzer', run_shape_fuzzer_tests,
  args: [
    hb_shape_fuzzer_exe,
  ],
  depends: [hb_shape_fuzzer_exe, libharfbuzz, libharfbuzz_subset],
  workdir: join_paths(meson.current_build_dir(), '..', '..'),
  env: env)

test('subset_fuzzer', python3,
  args: [
    files('run-subset-fuzzer-tests.py')[0],
    hb_subset_fuzzer_exe,
    hb_subset_get_codepoints_fuzzer_exe,
  ],
  workdir: join_paths(meson.current_build_dir(), '..', '..'),
  env: env)
