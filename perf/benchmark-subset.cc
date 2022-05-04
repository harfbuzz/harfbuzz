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
    if (!hb_set_next (codepoints_in_font, &cp)) return;
    hb_set_add (hb_subset_input_unicode_set (input), cp);
  }
}

void AddGlyphs(unsigned num_glyphs_in_font,
               unsigned subset_size,
               hb_subset_input_t* input)
{
  for (unsigned i = 0; i < subset_size && i < num_glyphs_in_font; i++) {
    // TODO(garretrieger): pick randomly.
    hb_set_add (hb_subset_input_glyph_set (input), i);
  }
}

/* benchmark for subsetting a font */
static void BM_subset_codepoints (benchmark::State &state,
                                  const char *font_path)
{
  unsigned subset_size = state.range(0);

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

BENCHMARK_CAPTURE (BM_subset_codepoints, subset_roboto,
                   "perf/fonts/Roboto-Regular.ttf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 4000);

BENCHMARK_CAPTURE (BM_subset_codepoints, subset_amiri,
                   "perf/fonts/Amiri-Regular.ttf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 4000);

BENCHMARK_CAPTURE (BM_subset_codepoints, subset_noto_nastaliq_urdu,
                   "perf/fonts/NotoNastaliqUrdu-Regular.ttf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 1000);

BENCHMARK_CAPTURE (BM_subset_codepoints, subset_noto_devangari,
                   "perf/fonts/NotoSansDevanagari-Regular.ttf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 1000);

BENCHMARK_CAPTURE (BM_subset_codepoints, subset_mplus1p,
                   "test/subset/data/fonts/Mplus1p-Regular.ttf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 10000);

#if 0
BENCHMARK_CAPTURE (BM_subset_codepoints, subset_notocjk,
                   "perf/fonts/NotoSansCJKsc-VF.ttf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 100000);
#endif


static void BM_subset_glyphs (benchmark::State &state,
                              const char *font_path)
{
  unsigned subset_size = state.range(0);

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
    unsigned num_glyphs = hb_face_get_glyph_count (face);

    AddGlyphs(num_glyphs, subset_size, input);
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

BENCHMARK_CAPTURE (BM_subset_glyphs, subset_roboto,
                   "perf/fonts/Roboto-Regular.ttf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 4000);

BENCHMARK_CAPTURE (BM_subset_glyphs, subset_amiri,
                   "perf/fonts/Amiri-Regular.ttf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 4000);

BENCHMARK_CAPTURE (BM_subset_glyphs, subset_noto_nastaliq_urdu,
                   "perf/fonts/NotoNastaliqUrdu-Regular.ttf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 1000);

BENCHMARK_CAPTURE (BM_subset_glyphs, subset_noto_devangari,
                   "perf/fonts/NotoSansDevanagari-Regular.ttf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 1000);

BENCHMARK_CAPTURE (BM_subset_glyphs, subset_mplus1p,
                   "test/subset/data/fonts/Mplus1p-Regular.ttf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 10000);

#if 0
BENCHMARK_CAPTURE (BM_subset_glyphs, subset_notocjk,
                   "perf/fonts/NotoSansCJKsc-VF.ttf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 100000);
#endif



BENCHMARK_MAIN();
