#include "benchmark/benchmark.h"

#include "hb-subset.h"

// TODO(garretrieger): TrueType CJK font.
// TODO(garretrieger): Amiri + Devanagari

void AddCodepoints(const hb_set_t* codepoints_in_font,
                   unsigned subset_size,
                   hb_subset_input_t* input)
{
  hb_codepoint_t cp = HB_SET_VALUE_INVALID;
  for (unsigned i = 0; i < subset_size; i++) {
    // TODO(garretrieger): pick randomly.
    assert (hb_set_next (codepoints_in_font, &cp));
    hb_set_add (hb_subset_input_unicode_set (input), cp);
  }
}

/* benchmark for subsetting a font */
static void BM_subset_codepoints (benchmark::State &state,
                                  const char *font_path,
                                  unsigned subset_size)
{
  hb_face_t *face;
  {
    hb_blob_t *blob = hb_blob_create_from_file_or_fail (font_path);
    assert (blob);
    face = hb_face_create (blob, 0);
    hb_blob_destroy (blob);
  }

  hb_subset_input_t* input = hb_subset_input_create_or_fail ();
  assert (input);

  {
    hb_set_t* all_codepoints = hb_set_create ();
    hb_face_collect_unicodes (face, all_codepoints);

    AddCodepoints(all_codepoints, subset_size, input);

    hb_set_destroy (all_codepoints);
  }

  for (auto _ : state)
  {
    hb_face_t* subset = hb_subset_or_fail (face, input);
    assert (subset);
    hb_face_destroy (subset);
  }

  hb_subset_input_destroy (input);
  hb_face_destroy (face);
}

// TODO(garretrieger): Use range() for subset size.
BENCHMARK_CAPTURE (BM_subset_codepoints, subset_roboto_10,
                   "perf/fonts/Roboto-Regular.ttf", 10)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_CAPTURE (BM_subset_codepoints, subset_roboto_100,
                   "perf/fonts/Roboto-Regular.ttf", 100)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_CAPTURE (BM_subset_codepoints, subset_noto_nastaliq_urdu_10,
                   "perf/fonts/NotoNastaliqUrdu-Regular.ttf", 10)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_CAPTURE (BM_subset_codepoints, subset_noto_nastaliq_urdu_100,
                   "perf/fonts/NotoNastaliqUrdu-Regular.ttf", 100)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
