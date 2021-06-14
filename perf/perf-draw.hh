#include "benchmark/benchmark.h"

#include "hb.h"
#include "hb-ot.h"
#include "hb-ft.h"
#include FT_OUTLINE_H

#ifdef HAVE_TTFPARSER
#include "ttfparser.h"
#endif

#define HB_UNUSED __attribute__((unused))

static void
_hb_move_to (hb_position_t to_x HB_UNUSED, hb_position_t to_y HB_UNUSED, void *user_data HB_UNUSED) {}

static void
_hb_line_to (hb_position_t to_x HB_UNUSED, hb_position_t to_y HB_UNUSED, void *user_data HB_UNUSED) {}

static void
_hb_quadratic_to (hb_position_t control_x HB_UNUSED, hb_position_t control_y HB_UNUSED,
		  hb_position_t to_x HB_UNUSED, hb_position_t to_y HB_UNUSED,
		  void *user_data HB_UNUSED) {}

static void
_hb_cubic_to (hb_position_t control1_x HB_UNUSED, hb_position_t control1_y HB_UNUSED,
	      hb_position_t control2_x HB_UNUSED, hb_position_t control2_y HB_UNUSED,
	      hb_position_t to_x HB_UNUSED, hb_position_t to_y HB_UNUSED,
	      void *user_data HB_UNUSED) {}

static void
_hb_close_path (void *user_data HB_UNUSED) {}

static void
_ft_move_to (const FT_Vector* to HB_UNUSED, void* user HB_UNUSED) {}

static void
_ft_line_to (const FT_Vector* to HB_UNUSED, void* user HB_UNUSED) {}

static void
_ft_conic_to (const FT_Vector* control HB_UNUSED, const FT_Vector* to HB_UNUSED,
	      void* user HB_UNUSED) {}

static void
_ft_cubic_to (const FT_Vector* control1 HB_UNUSED, const FT_Vector* control2 HB_UNUSED,
	      const FT_Vector* to HB_UNUSED, void* user HB_UNUSED) {}

#ifdef HAVE_TTFPARSER
static void _tp_move_to (float x HB_UNUSED, float y HB_UNUSED, void *data HB_UNUSED) {}
static void _tp_line_to (float x, float y, void *data) {}
static void _tp_quad_to (float x1, float y1, float x, float y, void *data) {}
static void _tp_curve_to (float x1, float y1, float x2, float y2, float x, float y, void *data) {}
static void _tp_close_path (void *data) {}
#endif

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

  if (backend == HARFBUZZ)
  {
    if (is_var)
    {
      hb_variation_t wght = {HB_TAG ('w','g','h','t'), 500};
      hb_font_set_variations (font, &wght, 1);
    }
    hb_draw_funcs_t *draw_funcs = hb_draw_funcs_create ();
    hb_draw_funcs_set_move_to_func (draw_funcs, _hb_move_to);
    hb_draw_funcs_set_line_to_func (draw_funcs, _hb_line_to);
    hb_draw_funcs_set_quadratic_to_func (draw_funcs, _hb_quadratic_to);
    hb_draw_funcs_set_cubic_to_func (draw_funcs, _hb_cubic_to);
    hb_draw_funcs_set_close_path_func (draw_funcs, _hb_close_path);

    for (auto _ : state)
      for (unsigned gid = 0; gid < num_glyphs; ++gid)
	hb_font_draw_glyph (font, gid, draw_funcs, nullptr);

    hb_draw_funcs_destroy (draw_funcs);
  }
  else if (backend == FREETYPE)
  {
    if (is_var)
    {
      hb_variation_t wght = {HB_TAG ('w','g','h','t'), 500};
      hb_font_set_variations (font, &wght, 1);
    }
    hb_ft_font_set_funcs (font);
    FT_Face ft_face = hb_ft_font_get_face (font);
    hb_ft_font_set_load_flags (font, FT_LOAD_NO_HINTING | FT_LOAD_NO_SCALE);

    FT_Outline_Funcs draw_funcs;
    draw_funcs.move_to = (FT_Outline_MoveToFunc) _ft_move_to;
    draw_funcs.line_to = (FT_Outline_LineToFunc) _ft_line_to;
    draw_funcs.conic_to = (FT_Outline_ConicToFunc) _ft_conic_to;
    draw_funcs.cubic_to = (FT_Outline_CubicToFunc) _ft_cubic_to;
    draw_funcs.shift = 0;
    draw_funcs.delta = 0;

    for (auto _ : state)
      for (unsigned gid = 0; gid < num_glyphs; ++gid)
      {
	FT_Load_Glyph (ft_face, gid, FT_LOAD_NO_HINTING | FT_LOAD_NO_SCALE);
	FT_Outline_Decompose (&ft_face->glyph->outline, &draw_funcs, nullptr);
      }
  }
  else if (backend == TTF_PARSER)
  {
#ifdef HAVE_TTFPARSER
    ttfp_face *tp_font = (ttfp_face *) malloc (ttfp_face_size_of ());
    hb_blob_t *blob = hb_face_reference_blob (hb_font_get_face (font));
    assert (ttfp_face_init (hb_blob_get_data (blob, nullptr), hb_blob_get_length (blob), 0, tp_font));
    if (is_var) ttfp_set_variation (tp_font, TTFP_TAG('w','g','h','t'), 500);

    ttfp_outline_builder builder;
    builder.move_to = _tp_move_to;
    builder.line_to = _tp_line_to;
    builder.quad_to = _tp_quad_to;
    builder.curve_to = _tp_curve_to;
    builder.close_path = _tp_close_path;

    ttfp_rect bbox;
    for (auto _ : state)
      for (unsigned gid = 0; gid < num_glyphs; ++gid)
	ttfp_outline_glyph (tp_font, builder, &builder, gid, &bbox);

    hb_blob_destroy (blob);
    free (tp_font);
#endif
  }
  else abort ();

  hb_font_destroy (font);
}

#define FONT_BASE_PATH "test/subset/data/fonts/"

BENCHMARK_CAPTURE (draw, cff - ot - SourceSansPro, FONT_BASE_PATH "SourceSansPro-Regular.otf", false, HARFBUZZ);
BENCHMARK_CAPTURE (draw, cff - ft - SourceSansPro, FONT_BASE_PATH "SourceSansPro-Regular.otf", false, FREETYPE);
BENCHMARK_CAPTURE (draw, cff - tp - SourceSansPro, FONT_BASE_PATH "SourceSansPro-Regular.otf", false, TTF_PARSER);

BENCHMARK_CAPTURE (draw, cff2 - ot - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", false, HARFBUZZ);
BENCHMARK_CAPTURE (draw, cff2 - ft - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", false, FREETYPE);
BENCHMARK_CAPTURE (draw, cff2 - tp - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", false, TTF_PARSER);

BENCHMARK_CAPTURE (draw, cff2/vf - ot - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", true, HARFBUZZ);
BENCHMARK_CAPTURE (draw, cff2/vf - ft - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", true, FREETYPE);
BENCHMARK_CAPTURE (draw, cff2/vf - tp - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", true, TTF_PARSER);

BENCHMARK_CAPTURE (draw, glyf - ot - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", false, HARFBUZZ);
BENCHMARK_CAPTURE (draw, glyf - ft - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", false, FREETYPE);
BENCHMARK_CAPTURE (draw, glyf - tp - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", false, TTF_PARSER);

BENCHMARK_CAPTURE (draw, glyf/vf - ot - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", true, HARFBUZZ);
BENCHMARK_CAPTURE (draw, glyf/vf - ft - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", true, FREETYPE);
BENCHMARK_CAPTURE (draw, glyf/vf - tp - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", true, TTF_PARSER);

BENCHMARK_CAPTURE (draw, glyf - ot - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", false, HARFBUZZ);
BENCHMARK_CAPTURE (draw, glyf - ft - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", false, FREETYPE);
BENCHMARK_CAPTURE (draw, glyf - tp - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", false, TTF_PARSER);

BENCHMARK_CAPTURE (draw, glyf/vf - ot - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", true, HARFBUZZ);
BENCHMARK_CAPTURE (draw, glyf/vf - ft - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", true, FREETYPE);
BENCHMARK_CAPTURE (draw, glyf/vf - tp - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", true, TTF_PARSER);

BENCHMARK_CAPTURE (draw, glyf - ot - Roboto, FONT_BASE_PATH "Roboto-Regular.ttf", false, HARFBUZZ);
BENCHMARK_CAPTURE (draw, glyf - ft - Roboto, FONT_BASE_PATH "Roboto-Regular.ttf", false, FREETYPE);
BENCHMARK_CAPTURE (draw, glyf - tp - Roboto, FONT_BASE_PATH "Roboto-Regular.ttf", false, TTF_PARSER);
