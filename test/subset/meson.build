tests = [
  'basics',
  'full-font',
  'cff-full-font',
  'japanese',
  'cff-japanese',
  'layout',
  'layout.gpos',
  'layout.gpos2',
  'layout.gpos3',
  'layout.gpos4',
  'layout.gpos6',
  'layout.gpos8',
  'layout.gsub3',
  'layout.gsub6',
  'layout.gdef',
  'layout.context',
  'layout.gdef-varstore',
  'layout.gdef-attachlist',
  'cmap',
  'cmap14',
  'sbix',
  'colr',
  'cbdt',
]

run_test = find_program('run-tests.py')

foreach t : tests
  fname = '@0@.tests'.format(t)

  test(t, run_test,
    args: [
      hb_subset,
      join_paths(meson.current_source_dir(), 'data', 'tests', fname),
    ],
    # as the tests are ran concurrently let's raise acceptable time here
    # ideally better to break and let meson handles them in parallel
    timeout: 500,
    workdir: join_paths(meson.current_build_dir(), '..', '..'),
    suite: ['subset', 'slow'],
  )
endforeach
