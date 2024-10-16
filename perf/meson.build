google_benchmark = subproject('google-benchmark')
google_benchmark_dep = google_benchmark.get_variable('google_benchmark_dep')

benchmarks = [
  'benchmark-font.cc',
  'benchmark-map.cc',
  'benchmark-ot.cc',
  'benchmark-set.cc',
  'benchmark-shape.cc',
  'benchmark-subset.cc',
]

foreach source : benchmarks
  benchmark_name = source.split('.')[0]
  benchmark(benchmark_name, executable(benchmark_name, source,
    dependencies: [
      google_benchmark_dep, freetype_dep, coretext_deps,
    ],
    cpp_args: [],
    include_directories: [incconfig, incsrc],
    link_with: [libharfbuzz, libharfbuzz_subset],
    install: false,
  ), workdir: meson.current_source_dir() / '..', timeout: 100)
endforeach
