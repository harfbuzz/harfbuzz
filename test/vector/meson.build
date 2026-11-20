if get_option('vector').disabled()
  subdir_done()
endif

run_test = find_program('run-tests.py')

test_vector_budget = executable('test-vector-budget', 'test-vector-budget.c',
  dependencies: [libharfbuzz_vector_dep],
)

test('budget', test_vector_budget,
  args: [meson.project_source_root() / 'test' / 'api' / 'fonts' / 'glyphs.ttf'],
  suite: 'vector',
)

test_vector_reset = executable('test-vector-reset', 'test-vector-reset.c',
  dependencies: [libharfbuzz_vector_dep],
)

test('reset', test_vector_reset,
  suite: 'vector',
)

test_vector_radial_tip = executable('test-vector-radial-tip', 'test-vector-radial-tip.c',
  dependencies: [libharfbuzz_vector_dep],
)

test('radial-tip', test_vector_radial_tip,
  suite: 'vector',
)

test_vector_sweep = executable('test-vector-sweep', 'test-vector-sweep.c',
  dependencies: [libharfbuzz_vector_dep],
)

test('sweep', test_vector_sweep,
  args: [meson.project_source_root() / 'test' / 'api' / 'fonts' / 'test_glyphs-glyf_colr_1.ttf'],
  suite: 'vector',
)

if zlib_dep.found()
  test_vector_png = executable('test-vector-png', 'test-vector-png.c',
    dependencies: [libharfbuzz_vector_dep, zlib_dep],
  )
  test('png', test_vector_png, suite: 'vector')
endif

tests = [
  'basic',
  'varc',
]

foreach t : tests
  fname = '@0@.tests'.format(t)

  test(t, run_test,
    args: [
      hb_vector,
      meson.current_source_dir() / 'data' / 'tests' / fname,
    ],
    workdir: meson.current_build_dir() / '..' / '..',
    protocol: 'tap',
    suite: 'vector',
  )
endforeach
