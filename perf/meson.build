google_benchmark = subproject('google-benchmark')
google_benchmark_dep = google_benchmark.get_variable('google_benchmark_dep')

ttf_parser_dep = null_dep
if get_option('experimental_api') and add_languages('rust', required: false, native: true)
  ttf_parser_dep = subproject('ttf-parser').get_variable('ttf_parser_dep')
endif

if ttf_parser_dep.found()
  benchmark_cpp_args = ['-DHAVE_TTFPARSER']
else
  benchmark_cpp_args = []
endif

benchmark('perf', executable('perf', 'perf.cc',
  dependencies: [
    google_benchmark_dep, freetype_dep,

    # the last two, thread and dl, aren't nice as ttf-parser isn't no_std yet
    # https://github.com/RazrFalcon/ttf-parser/issues/29
    ttf_parser_dep, thread_dep, cpp.find_library('dl'),
  ],
  cpp_args: benchmark_cpp_args,
  include_directories: [incconfig, incsrc],
  link_with: [libharfbuzz],
  install: false,
), workdir: join_paths(meson.current_source_dir(), '..'), timeout: 100)
