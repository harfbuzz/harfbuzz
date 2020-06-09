google_benchmark = subproject('google-benchmark')
google_benchmark_dep = google_benchmark.get_variable('google_benchmark_dep')

benchmark('perf', executable('perf', 'perf.cc',
  dependencies : [google_benchmark_dep, freetype_dep],
  include_directories: [incconfig, incsrc],
  link_with: [libharfbuzz],
  install: false,
), workdir: meson.current_source_dir() / '..', timeout: 100)
