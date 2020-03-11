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
  'layout.gsub3',
  'layout.gsub6',
  'layout.gdef',
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
    workdir: join_paths(meson.current_build_dir(), '..', '..'),
    # as the test are ran concurrently and we have numerous ones here let's raise acceptable time
    timeout: 120,
  )
endforeach

