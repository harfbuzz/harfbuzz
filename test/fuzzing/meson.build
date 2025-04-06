tests = [
  'hb-shape-fuzzer.cc',
  'hb-subset-fuzzer.cc',
  'hb-set-fuzzer.cc',
  'hb-draw-fuzzer.cc',
  'hb-repacker-fuzzer.cc',
]

run_fuzzer_tests = find_program('run-fuzzer-tests.py', required: true)

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

test('shape-fuzzer', run_fuzzer_tests,
  args: [
    hb_shape_fuzzer_exe,
    meson.current_source_dir() / 'fonts',
  ],
  depends: [hb_shape_fuzzer_exe, libharfbuzz, libharfbuzz_subset],
  workdir: meson.current_build_dir() / '..' / '..',
  priority: 1,
  suite: ['fuzzing'],
)

test('repacker-fuzzer', run_fuzzer_tests,
  args: [
    hb_repacker_fuzzer_exe,
    meson.current_source_dir() / 'graphs',
  ],
  workdir: meson.current_build_dir() / '..' / '..',
  priority: 1,
  suite: ['fuzzing'],
)

test('draw-fuzzer', run_fuzzer_tests,
  args: [
    hb_draw_fuzzer_exe,
    meson.current_source_dir() / 'fonts',
  ],
  workdir: meson.current_build_dir() / '..' / '..',
  suite: ['fuzzing'],
)

# Subset fuzzer: feed the fuzzer directly, without run-fuzzer-tests.py
fs = import('fs')
glob = run_command('find', meson.current_source_dir() / 'fonts', '-type', 'f', check:true).stdout().strip().split('\n')
glob += run_command('find', meson.current_source_dir() / '..' / 'subset' / 'data' / 'fonts' , '-type', 'f', check:true).stdout().strip().split('\n')
# Chunk glob and call runner for each chunk
chunk_size = 64
chunks = []
num_chunks = (glob.length() + chunk_size - 1) / chunk_size
foreach i : range(0, num_chunks)
  start = i * chunk_size
  end = (i + 1) * chunk_size
  if end > glob.length()
    end = glob.length()
  endif
  chunk = []
  foreach j : range(start, end)
    chunk += [glob[j]]
  endforeach
  chunks += [chunk]
endforeach

foreach i : range(0, chunks.length() - 1)
  chunk = chunks[i]
  test('subset-fuzzer-chunk-@0@'.format(i),
    hb_subset_fuzzer_exe,
    args: chunk,
    depends: [hb_shape_fuzzer_exe, libharfbuzz, libharfbuzz_subset],
    workdir: meson.current_build_dir() / '..' / '..',
    priority: 1,
    suite: ['fuzzing'],
  )
endforeach
