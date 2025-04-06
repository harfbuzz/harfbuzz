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

ls = find_program('ls')
if not ls.found()
  # On Windows, we need to use the 'dir' command to list files in a directory.
  # The 'ls' command is not available by default.
  find = [find_program('dir', required: true), '/b', '/a-d']
else
  # On other platforms, we can use the 'ls' command.
  find = [find_program('ls', required: true)]
endif
# Subset fuzzer: feed the fuzzer directly, without run-fuzzer-tests.py
font_dirs = [
  meson.current_source_dir() / 'fonts',
  meson.current_source_dir() / '..' / 'subset' / 'data' / 'fonts',
]
glob = []
foreach font_dir : font_dirs
  dir = run_command(find, font_dir, check:true).stdout().strip().split('\n')
  foreach item : dir
    glob += [font_dir / item]
  endforeach
endforeach

# Chunk glob and call runner for each chunk
chunk_size = 64
chunks = []
chunk = []
foreach item : glob
  if chunk.length() >= chunk_size
    chunks += [chunk]
    chunk = []
  endif
  chunk += [item]
endforeach
if chunk.length() > 0
  chunks += [chunk]
endif

i = 0
foreach chunk : chunks
  test('subset-fuzzer-chunk-@0@'.format(i),
    hb_subset_fuzzer_exe,
    args: chunk,
    depends: [hb_shape_fuzzer_exe, libharfbuzz, libharfbuzz_subset],
    workdir: meson.current_build_dir() / '..' / '..',
    protocol: 'tap',
    suite: ['fuzzing'],
  )
  i += 1
endforeach
