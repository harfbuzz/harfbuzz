run_test = find_program('run-tests.py')

tests = [
  'basic',
  'varc',
]

foreach t : tests
  fname = '@0@.tests'.format(t)

  test(t, run_test,
    args: [
      hb_draw,
      meson.current_source_dir() / 'data' / 'tests' / fname,
    ],
    workdir: meson.current_build_dir() / '..' / '..',
    protocol: 'tap',
    suite: 'draw',
  )
endforeach
