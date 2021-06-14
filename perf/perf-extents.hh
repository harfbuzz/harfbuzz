#include "benchmark/benchmark.h"

#include "hb.h"
#include "hb-ft.h"
#include "hb-ot.h"

#ifdef HAVE_TTFPARSER
#include "ttfparser.h"
#endif

static void extents (benchmark::State &state, const char *font_path, bool is_var, backend_t backend)
{
  hb_font_t *font;
  unsigned num_glyphs;
  {
    hb_blob_t *blob = hb_blob_create_from_file_or_fail (font_path);
    assert (blob);
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

  if (backend == HARFBUZZ || backend == FREETYPE)
  {
    if (backend == FREETYPE)
    {
      hb_ft_font_set_funcs (font);
      hb_ft_font_set_load_flags (font, FT_LOAD_NO_HINTING | FT_LOAD_NO_SCALE);
    }

    hb_glyph_extents_t extents;
    for (auto _ : state)
      for (unsigned gid = 0; gid < num_glyphs; ++gid)
	hb_font_get_glyph_extents (font, gid, &extents);
  }
  else if (backend == TTF_PARSER)
  {
#ifdef HAVE_TTFPARSER
    ttfp_face *tp_font = (ttfp_face *) malloc (ttfp_face_size_of ());
    hb_blob_t *blob = hb_face_reference_blob (hb_font_get_face (font));
    assert (ttfp_face_init (hb_blob_get_data (blob, nullptr), hb_blob_get_length (blob), 0, tp_font));
    if (is_var) ttfp_set_variation (tp_font, TTFP_TAG('w','g','h','t'), 500);

    ttfp_rect bbox;
    for (auto _ : state)
      for (unsigned gid = 0; gid < num_glyphs; ++gid)
	ttfp_get_glyph_bbox(tp_font, gid, &bbox);

    hb_blob_destroy (blob);
    free (tp_font);
#endif
  }

  hb_font_destroy (font);
}

#define FONT_BASE_PATH "test/subset/data/fonts/"

BENCHMARK_CAPTURE (extents, cff - ot - SourceSansPro, FONT_BASE_PATH "SourceSansPro-Regular.otf", false, HARFBUZZ);
BENCHMARK_CAPTURE (extents, cff - ft - SourceSansPro, FONT_BASE_PATH "SourceSansPro-Regular.otf", false, FREETYPE);
BENCHMARK_CAPTURE (extents, cff - tp - SourceSansPro, FONT_BASE_PATH "SourceSansPro-Regular.otf", false, TTF_PARSER);

BENCHMARK_CAPTURE (extents, cff2 - ot - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", false, HARFBUZZ);
BENCHMARK_CAPTURE (extents, cff2 - ft - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", false, FREETYPE);
BENCHMARK_CAPTURE (extents, cff2 - tp - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", false, TTF_PARSER);

BENCHMARK_CAPTURE (extents, cff2/vf - ot - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", true, HARFBUZZ);
BENCHMARK_CAPTURE (extents, cff2/vf - ft - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", true, FREETYPE);
BENCHMARK_CAPTURE (extents, cff2/vf - tp - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", true, TTF_PARSER);

BENCHMARK_CAPTURE (extents, glyf - ot - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", false, HARFBUZZ);
BENCHMARK_CAPTURE (extents, glyf - ft - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", false, FREETYPE);
BENCHMARK_CAPTURE (extents, glyf - tp - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", false, TTF_PARSER);

BENCHMARK_CAPTURE (extents, glyf/vf - ot - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", true, HARFBUZZ);
BENCHMARK_CAPTURE (extents, glyf/vf - ft - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", true, FREETYPE);
BENCHMARK_CAPTURE (extents, glyf/vf - tp - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", true, TTF_PARSER);

BENCHMARK_CAPTURE (extents, glyf - ot - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", false, HARFBUZZ);
BENCHMARK_CAPTURE (extents, glyf - ft - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", false, FREETYPE);
BENCHMARK_CAPTURE (extents, glyf - tp - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", false, TTF_PARSER);

BENCHMARK_CAPTURE (extents, glyf/vf - ot - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", true, HARFBUZZ);
BENCHMARK_CAPTURE (extents, glyf/vf - ft - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", true, FREETYPE);
BENCHMARK_CAPTURE (extents, glyf/vf - tp - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", true, TTF_PARSER);

BENCHMARK_CAPTURE (extents, glyf - ot - Roboto, FONT_BASE_PATH "Roboto-Regular.ttf", false, HARFBUZZ);
BENCHMARK_CAPTURE (extents, glyf - ft - Roboto, FONT_BASE_PATH "Roboto-Regular.ttf", false, FREETYPE);
BENCHMARK_CAPTURE (extents, glyf - tp - Roboto, FONT_BASE_PATH "Roboto-Regular.ttf", false, TTF_PARSER);

