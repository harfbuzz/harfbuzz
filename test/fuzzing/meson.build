tests = [
  'hb-shape-fuzzer.cc',
  'hb-subset-fuzzer.cc',
  'hb-set-fuzzer.cc',
  'hb-draw-fuzzer.cc',
  'hb-repacker-fuzzer.cc',
]

foreach file_name : tests
  test_name = file_name.split('.')[0]

  sources = [file_name]
  fuzzer_ldflags = []
  extra_cpp_args = []

  if get_option('fuzzer_ldflags') == ''
    sources += 'main.cc'
  else
    fuzzer_ldflags += get_option('fuzzer_ldflags').split()
    extra_cpp_args += '-DHB_IS_IN_FUZZER'
  endif

  if get_option('experimental_api')
    extra_cpp_args += '-DHB_EXPERIMENTAL_API'
  endif

  exe = executable(test_name, sources,
    cpp_args: cpp_args + extra_cpp_args,
    include_directories: [incconfig, incsrc],
    link_args: fuzzer_ldflags,
    link_with: [libharfbuzz, libharfbuzz_subset],
    install: false,
  )
  set_variable('@0@_exe'.format(test_name.underscorify()), exe)
endforeach

test('shape-fuzzer', find_program('run-fuzzer-tests.py'),
  args: [
    hb_shape_fuzzer_exe,
    meson.current_source_dir() / 'fonts',
  ],
  depends: [hb_shape_fuzzer_exe, libharfbuzz, libharfbuzz_subset],
  workdir: meson.current_build_dir() / '..' / '..',
  priority: 1,
  suite: ['fuzzing'],
)

test('subset-fuzzer', find_program('run-fuzzer-tests.py'),
  args: [
    hb_subset_fuzzer_exe,
    meson.current_source_dir() / 'fonts',
    meson.current_source_dir() / '..' / 'subset' / 'data' / 'fonts',
  ],
  workdir: meson.current_build_dir() / '..' / '..',
  priority: 1,
  suite: ['fuzzing'],
)

test('repacker-fuzzer', find_program('run-fuzzer-tests.py'),
  args: [
    hb_repacker_fuzzer_exe,
    meson.current_source_dir() / 'graphs',
  ],
  workdir: meson.current_build_dir() / '..' / '..',
  priority: 1,
  suite: ['fuzzing'],
)

test('draw-fuzzer', find_program('run-fuzzer-tests.py'),
  args: [
    hb_draw_fuzzer_exe,
    meson.current_source_dir() / 'fonts',
  ],
  workdir: meson.current_build_dir() / '..' / '..',
  suite: ['fuzzing'],
)
