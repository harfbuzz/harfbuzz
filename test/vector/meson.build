run_test = find_program('run-tests.py')

test_vector_budget = executable('test-vector-budget', 'test-vector-budget.c',
  dependencies: [libharfbuzz_vector_dep],
)

test('budget', test_vector_budget,
  args: [meson.project_source_root() / 'test' / 'api' / 'fonts' / 'glyphs.ttf'],
  suite: 'vector',
)

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
