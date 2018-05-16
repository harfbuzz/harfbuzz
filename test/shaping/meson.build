in_house_tests = [
  ['arabic-fallback-shaping.tests'],
  ['arabic-feature-order.tests'],
  ['arabic-like-joining.tests'],
  ['arabic-mark-order.tests'],
  ['arabic-stch.tests'],
  ['automatic-fractions.tests'],
  ['cluster.tests'],
  ['color-fonts.tests'],
  ['context-matching.tests'],
  ['cursive-positioning.tests'],
  ['default-ignorables.tests'],
  ['emoji-flag-tags.tests'],
  ['fallback-positioning.tests'],
  ['fuzzed.tests'],
  ['hangul-jamo.tests'],
  ['hyphens.tests'],
  ['indic-consonant-with-stacker.tests'],
  ['indic-decompose.tests'],
  ['indic-init.tests'],
  ['indic-joiner-candrabindu.tests'],
  ['indic-joiners.tests'],
  ['indic-old-spec.tests'],
  ['indic-pref-blocking.tests'],
  ['indic-script-extensions.tests'],
  ['indic-special-cases.tests'],
  ['indic-syllable.tests'],
  ['language-tags.tests'],
  ['ligature-id.tests'],
  ['mark-attachment.tests'],
  ['mark-filtering-sets.tests'],
  ['mongolian-variation-selector.tests'],
  ['myanmar-syllable.tests'],
  ['none-directional.tests'],
  ['spaces.tests'],
  ['simple.tests'],
  ['tibetan-contractions-1.tests'],
  ['tibetan-contractions-2.tests'],
  ['tibetan-vowels.tests'],
  ['use.tests'],
  ['use-marchen.tests'],
  ['use-syllable.tests'],
  ['variations-rvrn.tests'],
  ['vertical.tests'],
  ['zero-width-marks.tests'],
]

text_rendering_tests = [
  ['AVAR-1.tests'],
  ['CFF-1.tests'],
  ['CFF2-1.tests'],
  ['CFF-2.tests'],
  ['CMAP-1.tests'],
  ['CMAP-2.tests'],
  ['CVAR-1.tests'],
  ['CVAR-2.tests'],
  ['GLYF-1.tests'],
  ['GPOS-1.tests'],
  ['GPOS-2.tests'],
  ['GPOS-3.tests'],
  ['GPOS-4.tests'],
  ['GPOS-5.tests'],
  ['GSUB-1.tests'],
  ['GSUB-2.tests'],
  ['GVAR-1.tests'],
  ['GVAR-2.tests'],
  ['GVAR-3.tests'],
  ['GVAR-4.tests'],
  ['GVAR-5.tests'],
  ['GVAR-6.tests'],
  ['GVAR-7.tests'],
  ['GVAR-8.tests'],
  ['GVAR-9.tests'],
  ['HVAR-1.tests'],
  ['HVAR-2.tests'],
  ['KERN-1.tests'],
  ['KERN-2.tests'],
  ['SHBALI-3.tests'],
  ['SHKNDA-1.tests'],
]

foreach test_data : in_house_tests
  fname = test_data[0]

  test_name = fname.split('.')[0].underscorify()

  test(test_name, python3,
    args: [
      files('run-tests.py')[0],
      hb_shape.full_path(),
      join_paths(meson.current_source_dir(), 'data/in-house/tests', fname),
    ],
    workdir: meson.current_source_dir(),
  )
endforeach

foreach test_data : text_rendering_tests
  fname = test_data[0]

  test_name = fname.split('.')[0].underscorify()

  test(test_name, python3,
    args: [
      files('run-tests.py')[0],
      hb_shape.full_path(),
      join_paths(meson.current_source_dir(), 'data/text-rendering-tests/tests', fname),
    ],
    workdir: meson.current_source_dir(),
  )
endforeach
