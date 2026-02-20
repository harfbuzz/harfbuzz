hb_raster_all = executable('hb-raster-all', ['hb-raster-all.cc'],
  cpp_args: cpp_args,
  include_directories: [incconfig, incsrc],
  link_with: [libharfbuzz, libharfbuzz_raster],
  install: false,
)
meson.override_find_program('hb-raster-all', hb_raster_all)
