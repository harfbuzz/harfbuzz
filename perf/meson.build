if conf.get('HAVE_GLIB', 0) == 0
  message('You need to have glib support enabled to build perf benchmarks.')
  subdir_done()
endif

google_benchmark = subproject('google-benchmark')
google_benchmark_dep = google_benchmark.get_variable('google_benchmark_dep')

benchmarks = [
  'benchmark-font.cc',
  'benchmark-map.cc',
  'benchmark-ot.cc',
  'benchmark-set.cc',
  'benchmark-shape.cc',
]

foreach source : benchmarks
  benchmark_name = source.split('.')[0]
  benchmark(benchmark_name, executable(benchmark_name, source,
    dependencies: [
      google_benchmark_dep, libharfbuzz_dep
    ],
    cpp_args: [],
    include_directories: [incconfig, incsrc],
    install: false,
  ), workdir: meson.current_source_dir() / '..', timeout: 100)
endforeach

benchmarks_subset = [
  'benchmark-subset.cc',
]

foreach source : benchmarks_subset
  benchmark_name = source.split('.')[0]
  benchmark(benchmark_name, executable(benchmark_name, source,
    dependencies: [
      google_benchmark_dep, libharfbuzz_dep, libharfbuzz_subset_dep
    ],
    cpp_args: [],
    include_directories: [incconfig, incsrc],
    install: false,
  ), workdir: meson.current_source_dir() / '..', timeout: 100)
endforeach

hb_draw_all = executable('hb-draw-all', ['hb-draw-all.c'],
  cpp_args: cpp_args,
  include_directories: [incconfig, incsrc],
  link_with: [libharfbuzz],
)
meson.override_find_program('hb-draw-all', hb_draw_all)

hb_paint_all = executable('hb-paint-all', ['hb-paint-all.c'],
  cpp_args: cpp_args,
  include_directories: [incconfig, incsrc],
  link_with: [libharfbuzz],
)
meson.override_find_program('hb-paint-all', hb_paint_all)
