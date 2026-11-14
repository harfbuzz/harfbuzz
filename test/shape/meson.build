
subdir('data/in-house') # in_house_tests
subdir('data/aots') # aots_tests
subdir('data/text-rendering-tests') # text_rendering_tests

shape_run_tests_py = find_program('run-tests.py')

sets = ['in-house', 'aots', 'text-rendering-tests']
foreach set : sets
  # in-house -> in_house_tests, aots -> aots_tests, text-rendering-tests -> text_rendering_tests
  varname = '@0@_tests'.format(set.underscorify())
  tests = get_variable(varname)
  file_names = []
  foreach test : tests
    file_name = meson.current_source_dir() / 'data' / set / 'tests' / test
    file_names += [file_name]
  endforeach
  test(set, shape_run_tests_py,
    args: [
      hb_shape,
      file_names,
    ],
    protocol: 'tap',
    #verbose: true,
    workdir: meson.current_build_dir() / '..' / '..',
    suite: ['shape', set],
  )
endforeach
