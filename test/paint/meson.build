hb_paint_all = executable('hb-paint-all', ['hb-paint-all.c'],
  cpp_args: cpp_args,
  include_directories: [incconfig, incsrc],
  link_with: [libharfbuzz],
)
meson.override_find_program('hb-paint-all', hb_paint_all)
