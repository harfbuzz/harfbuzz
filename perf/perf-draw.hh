#include "benchmark/benchmark.h"

#include "hb.h"
#include "hb-ot.h"
#include "hb-ft.h"
#include FT_OUTLINE_H

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


static void draw (benchmark::State &state, const char *font_path, bool is_var, bool is_ft)
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
  else
  {
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

  hb_font_destroy (font);
}

#define FONT_BASE_PATH "test/subset/data/fonts/"

BENCHMARK_CAPTURE (draw, cff - ot - SourceSansPro, FONT_BASE_PATH "SourceSansPro-Regular.otf", false, false);
BENCHMARK_CAPTURE (draw, cff - ft - SourceSansPro, FONT_BASE_PATH "SourceSansPro-Regular.otf", false, true);

BENCHMARK_CAPTURE (draw, cff2 - ot - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", false, false);
BENCHMARK_CAPTURE (draw, cff2 - ft - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", false, true);

BENCHMARK_CAPTURE (draw, cff2/vf - ot - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", true, false);
BENCHMARK_CAPTURE (draw, cff2/vf - ft - AdobeVFPrototype, FONT_BASE_PATH "AdobeVFPrototype.otf", true, true);

BENCHMARK_CAPTURE (draw, glyf - ot - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", false, false);
BENCHMARK_CAPTURE (draw, glyf - ft - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", false, true);

BENCHMARK_CAPTURE (draw, glyf/vf - ot - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", true, false);
BENCHMARK_CAPTURE (draw, glyf/vf - ft - SourceSerifVariable, FONT_BASE_PATH "SourceSerifVariable-Roman.ttf", true, true);

BENCHMARK_CAPTURE (draw, glyf - ot - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", false, false);
BENCHMARK_CAPTURE (draw, glyf - ft - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", false, true);

BENCHMARK_CAPTURE (draw, glyf/vf - ot - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", true, false);
BENCHMARK_CAPTURE (draw, glyf/vf - ft - Comfortaa, FONT_BASE_PATH "Comfortaa-Regular-new.ttf", true, true);

BENCHMARK_CAPTURE (draw, glyf - ot - Roboto, FONT_BASE_PATH "Roboto-Regular.ttf", false, false);
BENCHMARK_CAPTURE (draw, glyf - ft - Roboto, FONT_BASE_PATH "Roboto-Regular.ttf", false, true);
