tests = [
  'basics',
  'full-font',
  'cff-full-font',
  'japanese',
  'cff-japanese',
  'cff.notoserifmyanmar',
  'glyf_bug_3131',
  'layout',
  'layout.gpos',
  'layout.gpos2',
  'layout.gpos3',
  'layout.gpos4',
  'layout.gpos5',
  'layout.gpos6',
  'layout.gpos8',
  'layout.gpos8.amiri',
  'layout.gpos9',
  'layout.gsub3',
  'layout.gsub5',
  'layout.gsub6',
  'layout.gsub8',
  'layout.gdef',
  'layout.khmer',
  'layout.context',
  'layout.gdef-varstore',
  'layout.gdef-attachlist',
  'layout.notonastaliqurdu',
  'cmap',
  'cmap14',
  'sbix',
  'colr',
# TODO: re-enable once colrv1 subsetting is stabilized.
# 'colrv1.notoemoji',
# 'colrv1',
  'colr_with_components',
  'cbdt',
  'variable',
]

repack_tests = [
  'basic',
  'prioritization',
  'table_duplication',
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
    suite: 'subset',
  )
endforeach

run_repack_test = find_program('run-repack-tests.py')

foreach t : repack_tests
  fname = '@0@.tests'.format(t)

  test(t, run_repack_test,
    args: [
      hb_subset,
      join_paths(meson.current_source_dir(), 'data', 'repack_tests', fname),
    ],
    workdir: join_paths(meson.current_build_dir(), '..', '..'),
    suite: ['subset', 'repack'],
  )
endforeach
