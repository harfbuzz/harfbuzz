#include "benchmark/benchmark.h"
#include <cstring>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "hb.h"
#include "hb-ot.h"
#ifdef HAVE_FREETYPE
#include "hb-ft.h"
#endif


#define SUBSET_FONT_BASE_PATH "test/subset/data/fonts/"

struct test_input_t
{
  const char *font_path;
} tests[] =
{
  {SUBSET_FONT_BASE_PATH "Roboto-Regular.ttf"},
  {SUBSET_FONT_BASE_PATH "SourceSansPro-Regular.otf"},
  {SUBSET_FONT_BASE_PATH "AdobeVFPrototype.otf"},
  {SUBSET_FONT_BASE_PATH "SourceSerifVariable-Roman.ttf"},
  {SUBSET_FONT_BASE_PATH "Comfortaa-Regular-new.ttf"},
  {SUBSET_FONT_BASE_PATH "NotoNastaliqUrdu-Regular.ttf"},
  {SUBSET_FONT_BASE_PATH "NotoSerifMyanmar-Regular.otf"},
};


enum backend_t { HARFBUZZ, FREETYPE };

enum operation_t
{
  nominal_glyphs,
  glyph_h_advances,
  glyph_extents,
  glyph_shape,
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

static void BM_Font (benchmark::State &state,
		     bool is_var, backend_t backend, operation_t operation,
		     const test_input_t &test_input)
{
  hb_font_t *font;
  unsigned num_glyphs;
  {
    hb_blob_t *blob = hb_blob_create_from_file_or_fail (test_input.font_path);
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
#ifdef HAVE_FREETYPE
      hb_ft_font_set_funcs (font);
#endif
      break;
  }

  switch (operation)
  {
    case nominal_glyphs:
    {
      hb_set_t *set = hb_set_create ();
      hb_face_collect_unicodes (hb_font_get_face (font), set);
      unsigned pop = hb_set_get_population (set);
      hb_codepoint_t *unicodes = (hb_codepoint_t *) calloc (pop, sizeof (hb_codepoint_t));
      hb_codepoint_t *glyphs = (hb_codepoint_t *) calloc (pop, sizeof (hb_codepoint_t));

      hb_codepoint_t *p = unicodes;
      for (hb_codepoint_t u = HB_SET_VALUE_INVALID;
	   hb_set_next (set, &u);)
        *p++ = u;
      assert (p == unicodes + pop);

      for (auto _ : state)
	hb_font_get_nominal_glyphs (font,
				    pop,
				    unicodes, sizeof (*unicodes),
				    glyphs, sizeof (*glyphs));

      free (glyphs);
      free (unicodes);
      hb_set_destroy (set);
      break;
    }
    case glyph_h_advances:
    {
      hb_codepoint_t *glyphs = (hb_codepoint_t *) calloc (num_glyphs, sizeof (hb_codepoint_t));
      hb_position_t *advances = (hb_position_t *) calloc (num_glyphs, sizeof (hb_codepoint_t));

      for (unsigned g = 0; g < num_glyphs; g++)
        glyphs[g] = g;

      for (auto _ : state)
	hb_font_get_glyph_h_advances (font,
				      num_glyphs,
				      glyphs, sizeof (*glyphs),
				      advances, sizeof (*advances));

      free (advances);
      free (glyphs);
      break;
    }
    case glyph_extents:
    {
      hb_glyph_extents_t extents;
      for (auto _ : state)
	for (unsigned gid = 0; gid < num_glyphs; ++gid)
	  hb_font_get_glyph_extents (font, gid, &extents);
      break;
    }
    case glyph_shape:
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

static void test_backend (backend_t backend,
			  const char *backend_name,
			  bool variable,
			  operation_t op,
			  const char *op_name,
			  benchmark::TimeUnit time_unit,
			  const test_input_t &test_input)
{
  char name[1024] = "BM_Font/";
  strcat (name, op_name);
  strcat (name, strrchr (test_input.font_path, '/'));
  strcat (name, variable ? "/var" : "");
  strcat (name, "/");
  strcat (name, backend_name);

  benchmark::RegisterBenchmark (name, BM_Font, variable, backend, op, test_input)
   ->Unit(time_unit);
}

static void test_operation (operation_t op,
			    const char *op_name,
			    benchmark::TimeUnit time_unit)
{
  for (auto& test_input : tests)
  {
    for (int variable = 0; variable < 2; variable++)
    {
      bool is_var = (bool) variable;

      test_backend (HARFBUZZ, "hb", is_var, op, op_name, time_unit, test_input);
#ifdef HAVE_FREETYPE
      test_backend (FREETYPE, "ft", is_var, op, op_name, time_unit, test_input);
#endif
    }
  }
}

int main(int argc, char** argv)
{
#define TEST_OPERATION(op, time_unit) test_operation (op, #op, time_unit)

  TEST_OPERATION (nominal_glyphs, benchmark::kMicrosecond);
  TEST_OPERATION (glyph_h_advances, benchmark::kMicrosecond);
  TEST_OPERATION (glyph_extents, benchmark::kMicrosecond);
  TEST_OPERATION (glyph_shape, benchmark::kMicrosecond);

#undef TEST_OPERATION

  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
}
