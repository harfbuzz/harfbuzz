hb_draw_all = executable('hb-draw-all', ['hb-draw-all.c'],
  cpp_args: cpp_args,
  include_directories: [incconfig, incsrc],
  link_with: [libharfbuzz],
)
meson.override_find_program('hb-draw-all', hb_draw_all)
