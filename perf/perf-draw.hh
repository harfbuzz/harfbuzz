#include "benchmark/benchmark.h"

#include "hb.h"
#include "hb-ot.h"
#include "hb-ft.h"

enum backend_t { HARFBUZZ, FREETYPE };

enum operation_t
{
  GLYPH_EXTENTS,
  GLYPH_SHAPE,
};

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

static hb_draw_funcs_t *
_draw_funcs_create (void)
{
  hb_draw_funcs_t *draw_funcs = hb_draw_funcs_create ();
  hb_draw_funcs_set_move_to_func (draw_funcs, _hb_move_to, nullptr, nullptr);
  hb_draw_funcs_set_line_to_func (draw_funcs, _hb_line_to, nullptr, nullptr);
  hb_draw_funcs_set_quadratic_to_func (draw_funcs, _hb_quadratic_to, nullptr, nullptr);
  hb_draw_funcs_set_cubic_to_func (draw_funcs, _hb_cubic_to, nullptr, nullptr);
  hb_draw_funcs_set_close_path_func (draw_funcs, _hb_close_path, nullptr, nullptr);
  return draw_funcs;
}

static void BM_test (benchmark::State &state, const char *font_path, bool is_var, backend_t backend, operation_t operation)
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

  switch (backend)
  {
    case HARFBUZZ:
      hb_ot_font_set_funcs (font);
      break;

    case FREETYPE:
      hb_ft_font_set_funcs (font);
      break;
  }

  switch (operation)
  {
    case GLYPH_EXTENTS:
    {
      hb_glyph_extents_t extents;
      for (auto _ : state)
	for (unsigned gid = 0; gid < num_glyphs; ++gid)
	  hb_font_get_glyph_extents (font, gid, &extents);
      break;
    }
    case GLYPH_SHAPE:
    {
      hb_draw_funcs_t *draw_funcs = _draw_funcs_create ();
      for (auto _ : state)
	for (unsigned gid = 0; gid < num_glyphs; ++gid)
	  hb_font_get_glyph_shape (font, gid, draw_funcs, nullptr);
      break;
      hb_draw_funcs_destroy (draw_funcs);
    }
  }


  hb_font_destroy (font);
}

#define FONT_BASE_PATH "test/subset/data/fonts/"


BENCHMARK_CAPTURE (BM_test, GLYPH_EXTENTS/cff - ot - SourceSansPro, FONT_BASE_PATH "SourceSansPro-Regular.otf", false, HARFBUZZ, GLYPH_EXTENTS);
BENCHMARK_CAPTURE (BM_test, GLYPH_EXTENTS/cff - ft - SourceSansPro, FONT_BASE_PATH "SourceSansPro-Regular.otf", false, FREETYPE, GLYPH_EXTENTS);

BENCHMARK_CAPTURE (BM_test, GLYPH_EXTENTS/cff2 - ot - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", false, HARFBUZZ, GLYPH_EXTENTS);
BENCHMARK_CAPTURE (BM_test, GLYPH_EXTENTS/cff2 - ft - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", false, FREETYPE, GLYPH_EXTENTS);

BENCHMARK_CAPTURE (BM_test, GLYPH_EXTENTS/cff2/vf - ot - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", true, HARFBUZZ, GLYPH_EXTENTS);
BENCHMARK_CAPTURE (BM_test, GLYPH_EXTENTS/cff2/vf - ft - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", true, FREETYPE, GLYPH_EXTENTS);

BENCHMARK_CAPTURE (BM_test, GLYPH_EXTENTS/glyf - ot - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", false, HARFBUZZ, GLYPH_EXTENTS);
BENCHMARK_CAPTURE (BM_test, GLYPH_EXTENTS/glyf - ft - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", false, FREETYPE, GLYPH_EXTENTS);

BENCHMARK_CAPTURE (BM_test, GLYPH_EXTENTS/glyf/vf - ot - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", true, HARFBUZZ, GLYPH_EXTENTS);
BENCHMARK_CAPTURE (BM_test, GLYPH_EXTENTS/glyf/vf - ft - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", true, FREETYPE, GLYPH_EXTENTS);

BENCHMARK_CAPTURE (BM_test, GLYPH_EXTENTS/glyf - ot - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", false, HARFBUZZ, GLYPH_EXTENTS);
BENCHMARK_CAPTURE (BM_test, GLYPH_EXTENTS/glyf - ft - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", false, FREETYPE, GLYPH_EXTENTS);

BENCHMARK_CAPTURE (BM_test, GLYPH_EXTENTS/glyf/vf - ot - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", true, HARFBUZZ, GLYPH_EXTENTS);
BENCHMARK_CAPTURE (BM_test, GLYPH_EXTENTS/glyf/vf - ft - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", true, FREETYPE, GLYPH_EXTENTS);

BENCHMARK_CAPTURE (BM_test, GLYPH_EXTENTS/glyf - ot - Roboto, FONT_BASE_PATH "Roboto-Regular.ttf", false, HARFBUZZ, GLYPH_EXTENTS);
BENCHMARK_CAPTURE (BM_test, GLYPH_EXTENTS/glyf - ft - Roboto, FONT_BASE_PATH "Roboto-Regular.ttf", false, FREETYPE, GLYPH_EXTENTS);


BENCHMARK_CAPTURE (BM_test, GLYPH_SHAPE/cff - ot - SourceSansPro, FONT_BASE_PATH "SourceSansPro-Regular.otf", false, HARFBUZZ, GLYPH_SHAPE);
BENCHMARK_CAPTURE (BM_test, GLYPH_SHAPE/cff - ft - SourceSansPro, FONT_BASE_PATH "SourceSansPro-Regular.otf", false, FREETYPE, GLYPH_SHAPE);

BENCHMARK_CAPTURE (BM_test, GLYPH_SHAPE/cff2 - ot - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", false, HARFBUZZ, GLYPH_SHAPE);
BENCHMARK_CAPTURE (BM_test, GLYPH_SHAPE/cff2 - ft - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", false, FREETYPE, GLYPH_SHAPE);

BENCHMARK_CAPTURE (BM_test, GLYPH_SHAPE/cff2/vf - ot - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", true, HARFBUZZ, GLYPH_SHAPE);
BENCHMARK_CAPTURE (BM_test, GLYPH_SHAPE/cff2/vf - ft - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", true, FREETYPE, GLYPH_SHAPE);

BENCHMARK_CAPTURE (BM_test, GLYPH_SHAPE/glyf - ot - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", false, HARFBUZZ, GLYPH_SHAPE);
BENCHMARK_CAPTURE (BM_test, GLYPH_SHAPE/glyf - ft - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", false, FREETYPE, GLYPH_SHAPE);

BENCHMARK_CAPTURE (BM_test, GLYPH_SHAPE/glyf/vf - ot - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", true, HARFBUZZ, GLYPH_SHAPE);
BENCHMARK_CAPTURE (BM_test, GLYPH_SHAPE/glyf/vf - ft - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", true, FREETYPE, GLYPH_SHAPE);

BENCHMARK_CAPTURE (BM_test, GLYPH_SHAPE/glyf - ot - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", false, HARFBUZZ, GLYPH_SHAPE);
BENCHMARK_CAPTURE (BM_test, GLYPH_SHAPE/glyf - ft - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", false, FREETYPE, GLYPH_SHAPE);

BENCHMARK_CAPTURE (BM_test, GLYPH_SHAPE/glyf/vf - ot - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", true, HARFBUZZ, GLYPH_SHAPE);
BENCHMARK_CAPTURE (BM_test, GLYPH_SHAPE/glyf/vf - ft - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", true, FREETYPE, GLYPH_SHAPE);

BENCHMARK_CAPTURE (BM_test, GLYPH_SHAPE/glyf - ot - Roboto, FONT_BASE_PATH "Roboto-Regular.ttf", false, HARFBUZZ, GLYPH_SHAPE);
BENCHMARK_CAPTURE (BM_test, GLYPH_SHAPE/glyf - ft - Roboto, FONT_BASE_PATH "Roboto-Regular.ttf", false, FREETYPE, GLYPH_SHAPE);

BENCHMARK_MAIN ();
