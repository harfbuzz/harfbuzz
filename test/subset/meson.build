tests = [
  ['basics.tests'],
  ['full-font.tests'],
  ['japanese.tests'],
]

# The test tries to run which unconditionally, windows doesn't agree
if find_program('which', required: false).found()
  foreach test_data : tests
    fname = test_data[0]

    test_name = fname.split('.')[0].underscorify()

    test(test_name, python3,
      args: [
        files('run-tests.py')[0],
        hb_subset,
        join_paths(meson.current_source_dir(), 'data/tests', fname),
      ],
      workdir: join_paths(meson.current_build_dir(), '..', '..'),
    )
  endforeach
endif