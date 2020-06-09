#include "benchmark/benchmark.h"

#include "hb.h"
#include "hb-ft.h"
#include "hb-ot.h"

static void extents (benchmark::State &state, const char *font_path, bool is_var, bool is_ft)
{
  hb_font_t *font;
  unsigned num_glyphs;
  {
    hb_blob_t *blob = hb_blob_create_from_file (font_path);
    assert (hb_blob_get_length (blob));
    hb_face_t *face = hb_face_create (blob, 0);
    hb_blob_destroy (blob);
    num_glyphs = hb_face_get_glyph_count (face);
    font = hb_font_create (face);
    hb_face_destroy (face);
  }

  if (is_var)
  {
    hb_variation_t wght = {HB_TAG ('w','g','h','t'), 500};
    hb_font_set_variations (font, &wght, 1);
  }

  if (is_ft)
  {
    hb_ft_font_set_funcs (font);
    hb_ft_font_set_load_flags (font, FT_LOAD_NO_HINTING | FT_LOAD_NO_SCALE);
  }

  hb_glyph_extents_t extents;
  for (auto _ : state)
    for (unsigned gid = 0; gid < num_glyphs; ++gid)
      hb_font_get_glyph_extents (font, gid, &extents);

  hb_font_destroy (font);
}

#define FONT_BASE_PATH "test/subset/data/fonts/"

BENCHMARK_CAPTURE (extents, cff - ot - SourceSansPro, FONT_BASE_PATH "SourceSansPro-Regular.otf", false, false);
BENCHMARK_CAPTURE (extents, cff - ft - SourceSansPro, FONT_BASE_PATH "SourceSansPro-Regular.otf", false, true);

BENCHMARK_CAPTURE (extents, cff2 - ot - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", false, false);
BENCHMARK_CAPTURE (extents, cff2 - ft - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", false, true);

BENCHMARK_CAPTURE (extents, cff2/vf - ot - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", true, false);
BENCHMARK_CAPTURE (extents, cff2/vf - ft - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", true, true);

BENCHMARK_CAPTURE (extents, glyf - ot - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", false, false);
BENCHMARK_CAPTURE (extents, glyf - ft - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", false, true);

BENCHMARK_CAPTURE (extents, glyf/vf - ot - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", true, false);
BENCHMARK_CAPTURE (extents, glyf/vf - ft - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", true, true);

BENCHMARK_CAPTURE (extents, glyf - ot - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", false, false);
BENCHMARK_CAPTURE (extents, glyf - ft - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", false, true);

BENCHMARK_CAPTURE (extents, glyf/vf - ot - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", true, false);
BENCHMARK_CAPTURE (extents, glyf/vf - ft - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", true, true);

BENCHMARK_CAPTURE (extents, glyf - ot - Roboto, FONT_BASE_PATH "Roboto-Regular.ttf", false, false);
BENCHMARK_CAPTURE (extents, glyf - ft - Roboto, FONT_BASE_PATH "Roboto-Regular.ttf", false, true);
