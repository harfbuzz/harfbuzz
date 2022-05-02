#include "benchmark/benchmark.h"

#include "hb.h"
#include "hb-ot.h"

static void
_hb_move_to (hb_draw_funcs_t *, void *, hb_draw_state_t *, float, float, void *) {}

static void
_hb_line_to (hb_draw_funcs_t *, void *, hb_draw_state_t *, float, float, void *) {}

static void
_hb_quadratic_to (hb_draw_funcs_t *, void *, hb_draw_state_t *, float, float, float, float, void *) {}

static void
_hb_cubic_to (hb_draw_funcs_t *, void *, hb_draw_state_t *, float, float, float, float, float, float, void *) {}

static void
_hb_close_path (hb_draw_funcs_t *, void *, hb_draw_state_t *, void *) {}

static void draw (benchmark::State &state, const char *font_path, bool is_var, backend_t backend)
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
  hb_draw_funcs_t *draw_funcs = hb_draw_funcs_create ();
  hb_draw_funcs_set_move_to_func (draw_funcs, _hb_move_to, nullptr, nullptr);
  hb_draw_funcs_set_line_to_func (draw_funcs, _hb_line_to, nullptr, nullptr);
  hb_draw_funcs_set_quadratic_to_func (draw_funcs, _hb_quadratic_to, nullptr, nullptr);
  hb_draw_funcs_set_cubic_to_func (draw_funcs, _hb_cubic_to, nullptr, nullptr);
  hb_draw_funcs_set_close_path_func (draw_funcs, _hb_close_path, nullptr, nullptr);

  if (backend == FREETYPE)
  {
    hb_ft_font_set_funcs (font);
    hb_ft_font_set_load_flags (font, FT_LOAD_NO_HINTING | FT_LOAD_NO_SCALE);
  }

  for (auto _ : state)
    for (unsigned gid = 0; gid < num_glyphs; ++gid)
      hb_font_get_glyph_shape (font, gid, draw_funcs, nullptr);

  hb_draw_funcs_destroy (draw_funcs);

  hb_font_destroy (font);
}

#define FONT_BASE_PATH "test/subset/data/fonts/"

BENCHMARK_CAPTURE (draw, cff - ot - SourceSansPro, FONT_BASE_PATH "SourceSansPro-Regular.otf", false, HARFBUZZ);
BENCHMARK_CAPTURE (draw, cff - ft - SourceSansPro, FONT_BASE_PATH "SourceSansPro-Regular.otf", false, FREETYPE);

BENCHMARK_CAPTURE (draw, cff2 - ot - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", false, HARFBUZZ);
BENCHMARK_CAPTURE (draw, cff2 - ft - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", false, FREETYPE);

BENCHMARK_CAPTURE (draw, cff2/vf - ot - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", true, HARFBUZZ);
BENCHMARK_CAPTURE (draw, cff2/vf - ft - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", true, FREETYPE);

BENCHMARK_CAPTURE (draw, glyf - ot - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", false, HARFBUZZ);
BENCHMARK_CAPTURE (draw, glyf - ft - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", false, FREETYPE);

BENCHMARK_CAPTURE (draw, glyf/vf - ot - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", true, HARFBUZZ);
BENCHMARK_CAPTURE (draw, glyf/vf - ft - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", true, FREETYPE);

BENCHMARK_CAPTURE (draw, glyf - ot - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", false, HARFBUZZ);
BENCHMARK_CAPTURE (draw, glyf - ft - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", false, FREETYPE);

BENCHMARK_CAPTURE (draw, glyf/vf - ot - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", true, HARFBUZZ);
BENCHMARK_CAPTURE (draw, glyf/vf - ft - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", true, FREETYPE);

BENCHMARK_CAPTURE (draw, glyf - ot - Roboto, FONT_BASE_PATH "Roboto-Regular.ttf", false, HARFBUZZ);
BENCHMARK_CAPTURE (draw, glyf - ft - Roboto, FONT_BASE_PATH "Roboto-Regular.ttf", false, FREETYPE);
