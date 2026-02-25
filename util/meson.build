hb_view_sources = [
  'hb-view.cc',
]

hb_shape_sources = [
  'hb-shape.cc',
]

hb_draw_sources = [
  'hb-vector.cc',
]

hb_raster_sources = [
  'hb-raster.cc',
]

hb_ft_raster_sources = [
  'hb-ft-raster.cc',
]

hb_info_sources = [
  'hb-info.cc',
]

hb_subset_cli_sources = [
  'hb-subset.cc',
]

# If found, invoke it on our four binaries to build man pages
# If not found, just skip this step
if not get_option('docs').disabled()
  help2man = find_program('help2man', required: false)
else
  help2man = disabler()
endif
if help2man.found()
  help2man_command = ['sh', '-c', help2man.full_path() + ' --help-option=--help-all --no-info -n "$(@INPUT@ --help | head -n 4 | tail -n 1)" --output=$0 @INPUT@', '@OUTPUT@']
  help2man_env = environment()
  help2man_env.set('HB_UTIL_HELP_VERBOSE', '1')
endif

util_deps = [freetype_dep, cairo_dep, cairo_ft_dep, glib_dep]

if conf.get('HAVE_GLIB', 0) == 1
  if conf.get('HAVE_CAIRO', 0) == 1
    hb_view = executable('hb-view', hb_view_sources,
      cpp_args: cpp_args,
      include_directories: [incconfig, incsrc],
      dependencies: [util_deps, chafa_dep],
      link_with: [libharfbuzz, libharfbuzz_cairo],
      install: true,
    )
    meson.override_find_program('hb-view', hb_view)
    if help2man.found()
      target = hb_view.name() + '.1'
      hb_view_manpage = custom_target(target,
        input: hb_view,
        output: target,
        command: help2man_command,
        depends: [hb_view],
        env: help2man_env,
        install: true,
        install_dir: get_option('mandir') / 'man1',
      )
    endif
  else
    hb_view = disabler()
  endif

  hb_shape = executable('hb-shape', hb_shape_sources,
    cpp_args: cpp_args,
    include_directories: [incconfig, incsrc],
    dependencies: [util_deps],
    link_with: [libharfbuzz],
    install: true,
  )
  meson.override_find_program('hb-shape', hb_shape)

  hb_vector = executable('hb-vector', hb_draw_sources,
    cpp_args: cpp_args,
    include_directories: [incconfig, incsrc],
    dependencies: [util_deps],
    link_with: [libharfbuzz, libharfbuzz_vector],
    install: true,
  )
  meson.override_find_program('hb-vector', hb_vector)

  hb_raster_util = executable('hb-raster', hb_raster_sources,
    cpp_args: cpp_args,
    include_directories: [incconfig, incsrc],
    dependencies: [util_deps],
    link_with: [libharfbuzz, libharfbuzz_raster],
    install: false,
  )

  if conf.get('HAVE_FREETYPE', 0) == 1
    hb_ft_raster_util = executable('hb-ft-raster', hb_ft_raster_sources,
      cpp_args: cpp_args,
      include_directories: [incconfig, incsrc],
      dependencies: [util_deps],
      link_with: [libharfbuzz],
      install: false,
    )

    hb_ft_raster_all = executable('hb-ft-raster-all', ['hb-ft-raster-all.cc'],
      cpp_args: cpp_args,
      include_directories: [incconfig, incsrc],
      dependencies: [util_deps],
      link_with: [libharfbuzz],
      install: false,
    )
  endif

  hb_draw_all = executable('hb-draw-all', ['hb-draw-all.c'],
    cpp_args: cpp_args,
    include_directories: [incconfig, incsrc],
    link_with: [libharfbuzz],
    install: false,
  )

  hb_vector_svg_all = executable('hb-vector-svg-all', ['hb-vector-svg-all.c'],
    cpp_args: cpp_args,
    include_directories: [incconfig, incsrc],
    link_with: [libharfbuzz, libharfbuzz_vector],
    install: false,
  )

  hb_paint_all = executable('hb-paint-all', ['hb-paint-all.c'],
    cpp_args: cpp_args,
    include_directories: [incconfig, incsrc],
    link_with: [libharfbuzz],
    install: false,
  )

  hb_raster_all = executable('hb-raster-all', ['hb-raster-all.cc'],
    cpp_args: cpp_args,
    include_directories: [incconfig, incsrc],
    link_with: [libharfbuzz, libharfbuzz_raster],
    install: false,
  )

  hb_info = executable('hb-info', [hb_info_sources, gobject_enums_h],
    cpp_args: cpp_args,
    include_directories: [incconfig, incsrc],
    dependencies: [util_deps, libharfbuzz_gobject_dep, chafa_dep],
    link_with: [libharfbuzz],
    install: true,
  )
  meson.override_find_program('hb-info', hb_info)

  hb_subset = executable('hb-subset', hb_subset_cli_sources,
    cpp_args: cpp_args,
    include_directories: [incconfig, incsrc],
    dependencies: util_deps,
    link_with: [libharfbuzz, libharfbuzz_subset],
    install: true,
  )
  meson.override_find_program('hb-subset', hb_subset)

  if help2man.found()
    foreach binary: [hb_shape, hb_vector, hb_info, hb_subset]
      target = binary.name() + '.1'
      manpage = custom_target(target,
        input: binary,
        output: target,
        command: help2man_command,
        depends: [binary],
        env: help2man_env,
        install: true,
        install_dir: get_option('mandir') / 'man1',
      )
    endforeach
  endif

  if get_option('experimental_api')
    test('test-hb-subset-parsing',
      executable('test-hb-subset-parsing',
        ['test-hb-subset-parsing.c'],
        cpp_args: cpp_args,
        c_args: ['-DHB_EXPERIMENTAL_API'],
        include_directories: [incconfig, incsrc],
        dependencies: util_deps,
        link_with: [libharfbuzz, libharfbuzz_subset],
        install: false,
    ),
    workdir : meson.current_source_dir(),
    suite: ['util'])
  endif
else
  # Disable tests that use this
  hb_view = disabler()
  hb_shape = disabler()
  hb_vector = disabler()
  hb_info = disabler()
  hb_subset = disabler()
endif
