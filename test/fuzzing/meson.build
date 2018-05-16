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
endforeach

test('shape_fuzzer', python3,
  args: [
    files('run-shape-fuzzer-tests.py')[0],
    join_paths(meson.current_build_dir(), 'hb_shape_fuzzer')
  ],
  workdir: meson.current_source_dir())

test('subset_fuzzer', python3,
  args: [
    files('run-subset-fuzzer-tests.py')[0],
    join_paths(meson.current_build_dir(), 'hb_subset_fuzzer'),
    join_paths(meson.current_build_dir(), 'hb_subset_get_codepoints_fuzzer'),
  ],
  workdir: meson.current_source_dir())
