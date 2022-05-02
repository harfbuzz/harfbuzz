#include "benchmark/benchmark.h"

#include "hb.h"
#include "hb-ot.h"
#include "hb-ft.h"


#define SUBSET_FONT_BASE_PATH "test/subset/data/fonts/"

struct test_input_t
{
  const char *font_path;
} tests[] =
{
  {SUBSET_FONT_BASE_PATH "SourceSansPro-Regular.otf"},
  {SUBSET_FONT_BASE_PATH "AdobeVFPrototype.otf"},
  {SUBSET_FONT_BASE_PATH "AdobeVFPrototype.otf"},
  {SUBSET_FONT_BASE_PATH "SourceSerifVariable-Roman.ttf"},
  {SUBSET_FONT_BASE_PATH "SourceSerifVariable-Roman.ttf"},
  {SUBSET_FONT_BASE_PATH "Comfortaa-Regular-new.ttf"},
  {SUBSET_FONT_BASE_PATH "Comfortaa-Regular-new.ttf"},
  {SUBSET_FONT_BASE_PATH "Roboto-Regular.ttf"},
};


enum backend_t { HARFBUZZ, FREETYPE };

enum operation_t
{
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
      hb_ft_font_set_funcs (font);
      break;
  }

  switch (operation)
  {
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
			  operation_t op,
			  const char *op_name,
			  const test_input_t &test_input)
{
  char name[1024] = "BM_Font/";
  strcat (name, op_name);
  strcat (name, "/");
  strcat (name, backend_name);
  strcat (name, strrchr (test_input.font_path, '/'));

  benchmark::RegisterBenchmark (name, BM_Font, false, backend, op, test_input)
   ->Unit(benchmark::kMicrosecond);
}

static void test_operation (operation_t op,
			    const char *op_name)
{
  for (auto& test_input : tests)
  {
    test_backend (HARFBUZZ, "hb", op, op_name, test_input);
    test_backend (FREETYPE, "ft", op, op_name, test_input);
  }
}

int main(int argc, char** argv)
{
#define TEST_OPERATION(op) test_operation (op, #op)

  TEST_OPERATION (glyph_extents);
  TEST_OPERATION (glyph_shape);

#undef TEST_OPERATION

  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
}
