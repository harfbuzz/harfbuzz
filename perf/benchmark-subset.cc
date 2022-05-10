#include "benchmark/benchmark.h"

#include "hb-subset.h"

// TODO(garretrieger): TrueType CJK font.
// TODO(garretrieger): Amiri + Devanagari

enum operation_t
{
  subset_codepoints,
  subset_glyphs
};

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
static void BM_subset (benchmark::State &state,
                       operation_t operation,
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
    switch (operation) {
      case subset_codepoints:
        hb_face_collect_unicodes (face, all_codepoints);
        AddCodepoints(all_codepoints, subset_size, input);
        break;
      case subset_glyphs:
        unsigned num_glyphs = hb_face_get_glyph_count (face);
        AddGlyphs(num_glyphs, subset_size, input);
        break;
    }
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

BENCHMARK_CAPTURE (BM_subset, subset_codepoints_roboto,
                   subset_codepoints,
                   "perf/fonts/Roboto-Regular.ttf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 4000);

BENCHMARK_CAPTURE (BM_subset, subset_glyphs_roboto,
                   subset_glyphs,
                   "perf/fonts/Roboto-Regular.ttf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 4000);

BENCHMARK_CAPTURE (BM_subset, subset_codepoints_amiri,
                   subset_codepoints,
                   "perf/fonts/Amiri-Regular.ttf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 4000);

BENCHMARK_CAPTURE (BM_subset, subset_glyphs_amiri,
                   subset_glyphs,
                   "perf/fonts/Amiri-Regular.ttf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 4000);

BENCHMARK_CAPTURE (BM_subset, subset_codepoints_noto_nastaliq_urdu,
                   subset_codepoints,
                   "perf/fonts/NotoNastaliqUrdu-Regular.ttf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 1000);

BENCHMARK_CAPTURE (BM_subset, subset_glyphs_noto_nastaliq_urdu,
                   subset_glyphs,
                   "perf/fonts/NotoNastaliqUrdu-Regular.ttf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 1000);

BENCHMARK_CAPTURE (BM_subset, subset_codepoints_noto_devangari,
                   subset_codepoints,
                   "perf/fonts/NotoSansDevanagari-Regular.ttf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 1000);

BENCHMARK_CAPTURE (BM_subset, subset_glyphs_noto_devangari,
                   subset_glyphs,
                   "perf/fonts/NotoSansDevanagari-Regular.ttf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 1000);

BENCHMARK_CAPTURE (BM_subset, subset_codepoints_mplus1p,
                   subset_codepoints,
                   "test/subset/data/fonts/Mplus1p-Regular.ttf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 10000);

BENCHMARK_CAPTURE (BM_subset, subset_glyphs_mplus1p,
                   subset_glyphs,
                   "test/subset/data/fonts/Mplus1p-Regular.ttf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 10000);



BENCHMARK_CAPTURE (BM_subset, subset_codepoints_sourcehansans,
                   subset_codepoints,
                   "test/subset/data/fonts/SourceHanSans-Regular_subset.otf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 10000);

BENCHMARK_CAPTURE (BM_subset, subset_glyphs_sourcehansans,
                   subset_glyphs,
                   "test/subset/data/fonts/SourceHanSans-Regular_subset.otf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 10000);

BENCHMARK_CAPTURE (BM_subset, subset_codepoints_sourcesanspro,
                   subset_codepoints,
                   "test/subset/data/fonts/SourceSansPro-Regular.otf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 2048);

BENCHMARK_CAPTURE (BM_subset, subset_glyphs_sourcesanspro,
                   subset_glyphs,
                   "test/subset/data/fonts/SourceSansPro-Regular.otf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 2048);



#if 0
BENCHMARK_CAPTURE (BM_subset, subset_codepoints_notocjk,
                   subset_codepoints,
                   "perf/fonts/NotoSansCJKsc-VF.ttf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 100000);

BENCHMARK_CAPTURE (BM_subset, subset_glyphs_notocjk,
                   subset_glyphs,
                   "perf/fonts/NotoSansCJKsc-VF.ttf")
    ->Unit(benchmark::kMillisecond)
    ->Range(10, 100000);
#endif



BENCHMARK_MAIN();
