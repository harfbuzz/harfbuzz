tests = [
  ['basics.tests'],
  ['full-font.tests'],
  ['japanese.tests'],
]

foreach test_data : tests
  fname = test_data[0]

  test_name = fname.split('.')[0].underscorify()

  test(test_name, python3,
    args: [
      files('run-tests.py')[0],
      hb_subset.full_path(),
      join_paths(meson.current_source_dir(), 'data/tests', fname),
    ],
    workdir: meson.current_source_dir(),
  )
endforeach
