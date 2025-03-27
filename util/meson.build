hb_view_sources = [
  'hb-view.cc',
]

hb_shape_sources = [
  'hb-shape.cc',
]

hb_info_sources = [
  'hb-info.cc',
]

hb_subset_cli_sources = [
  'hb-subset.cc',
  'helper-subset.hh',
]

# If found, invoke it on our four binaries to build man pages
# If not found, just skip this step
if not get_option('docs').disabled()
  help2man = find_program('help2man', required: false)
else
  help2man = disabler()
endif
help2man_command = [help2man, '--help-option=--help-all', '--no-info', '--output=' + '@OUTPUT@', '@INPUT@']
help2man_env = environment()
help2man_env.set('HB_UTIL_HELP_VERBOSE', '1')

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
  endif

  hb_shape = executable('hb-shape', hb_shape_sources,
    cpp_args: cpp_args,
    include_directories: [incconfig, incsrc],
    dependencies: [util_deps],
    link_with: [libharfbuzz],
    install: true,
  )
  meson.override_find_program('hb-shape', hb_shape)

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
    foreach binary: [hb_shape, hb_info, hb_subset]
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
        ['test-hb-subset-parsing.c', 'helper-subset.hh'],
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
  hb_shape = disabler()
  hb_subset = disabler()
endif