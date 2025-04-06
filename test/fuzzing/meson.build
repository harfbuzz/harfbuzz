tests = [
  'hb-shape-fuzzer.cc',
  'hb-subset-fuzzer.cc',
  'hb-set-fuzzer.cc',
  'hb-draw-fuzzer.cc',
  'hb-repacker-fuzzer.cc',
]

# Build the binaries
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

glob_cmd = find_program('glob.py', required: true)

fonts_glob = run_command(glob_cmd, meson.current_source_dir() / 'fonts', check:true).stdout().strip().split('\n')
subset_fonts_glob = run_command(glob_cmd, meson.current_source_dir() / '..' / 'subset' / 'data' / 'fonts', check:true).stdout().strip().split('\n')
graphs_glob = run_command(glob_cmd, meson.current_source_dir() / 'graphs', check:true).stdout().strip().split('\n')
sets_glob = run_command(glob_cmd, meson.current_source_dir() / 'sets', check:true).stdout().strip().split('\n')

# Chunk the glob lists to avoid command line length limits, and for parallelization
chunk_size = 64
foreach glob_name : ['fonts_glob', 'subset_fonts_glob', 'graphs_glob', 'sets_glob']
  glob = get_variable(glob_name)
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
  set_variable('@0@_chunks'.format(glob_name), chunks)
endforeach

# Run fuzzers

i = 0
foreach chunk : fonts_glob_chunks
  test('shape-fuzzer-chunk-@0@'.format(i),
    hb_shape_fuzzer_exe,
    args: chunk,
    workdir: meson.current_build_dir() / '..' / '..',
    protocol: 'tap',
    suite: ['fuzzing'],
  )
  i += 1
endforeach

i = 0
foreach chunk : fonts_glob_chunks
  test('draw-fuzzer-chunk-@0@'.format(i),
    hb_draw_fuzzer_exe,
    args: chunk,
    workdir: meson.current_build_dir() / '..' / '..',
    protocol: 'tap',
    suite: ['fuzzing'],
  )
  i += 1
endforeach

i = 0
foreach chunk : fonts_glob_chunks + subset_fonts_glob_chunks
  test('subset-fuzzer-chunk-@0@'.format(i),
    hb_subset_fuzzer_exe,
    args: chunk,
    workdir: meson.current_build_dir() / '..' / '..',
    protocol: 'tap',
    suite: ['fuzzing'],
  )
  i += 1
endforeach

i = 0
foreach chunk : graphs_glob_chunks
  test('repacker-fuzzer-chunk-@0@'.format(i),
    hb_repacker_fuzzer_exe,
    args: chunk,
    workdir: meson.current_build_dir() / '..' / '..',
    protocol: 'tap',
    suite: ['fuzzing'],
  )
  i += 1
endforeach

i = 0
foreach chunk : sets_glob_chunks
  test('set-fuzzer-chunk-@0@'.format(i),
    hb_set_fuzzer_exe,
    args: chunk,
    workdir: meson.current_build_dir() / '..' / '..',
    protocol: 'tap',
    suite: ['fuzzing'],
  )
  i += 1
endforeach
