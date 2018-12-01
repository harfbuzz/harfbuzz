tests = ['basics', 'full-font', 'japanese']

run_test = find_program('run-tests.py')

foreach t : tests
  fname = '@0@.tests'.format(t)

  test(t, run_test,
    args: [
      hb_subset,
      join_paths(meson.current_source_dir(), 'data', 'tests', fname),
    ],
    workdir: join_paths(meson.current_build_dir(), '..', '..'),
  )
endforeach

