hb_view_sources = [
  'hb-view.cc',
  'options.cc',
  'ansi-print.cc',
  'helper-cairo.cc',
  'helper-cairo-ansi.cc',
  'view-cairo.cc',
]

hb_shape_sources = [
  'hb-shape.cc',
  'options.cc',
]

hb_ot_shape_closure_sources = [
  'hb-ot-shape-closure.cc',
  'options.cc',
]

hb_subset_cli_sources = [
  'hb-subset.cc',
  'options.cc',
]

hb_view = executable('hb-view', hb_view_sources,
  cpp_args: cpp_args,
  include_directories: [incbase, incsrc],
  dependencies: deps,
  link_with: [libharfbuzz]
)

hb_shape = executable('hb-shape', hb_shape_sources,
  cpp_args: cpp_args,
  include_directories: [incbase, incsrc],
  dependencies: deps,
  link_with: [libharfbuzz]
)

hb_subset = executable('hb-subset', hb_subset_cli_sources,
  cpp_args: cpp_args,
  include_directories: [incbase, incsrc],
  dependencies: deps,
  link_with: [libharfbuzz, libharfbuzz_subset]
)

hb_ot_shape_closure = executable('hb-ot-shape-closure', hb_ot_shape_closure_sources,
  cpp_args: cpp_args,
  include_directories: [incbase, incsrc],
  dependencies: deps,
  link_with: [libharfbuzz]
)
