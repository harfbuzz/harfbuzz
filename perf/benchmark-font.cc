#include "hb-benchmark.hh"

#include <glib.h>

#define SUBSET_FONT_BASE_PATH "test/subset/data/fonts/"

static hb_variation_t default_variations[] = {
    hb_variation_t {HB_TAG ('w','g','h','t'), 500},
    hb_variation_t {HB_TAG_NONE, 0}
};

static unsigned max_num_glyphs = 0;
static const char *text = " ";

static struct test_input_t
{
  hb_variation_t *variations;
  const char *font_path;
} default_tests[] =
{
  {nullptr,            SUBSET_FONT_BASE_PATH "Roboto-Regular.ttf"},
  {default_variations, SUBSET_FONT_BASE_PATH "RobotoFlex-Variable.ttf"},
  {default_variations, SUBSET_FONT_BASE_PATH "SourceSansPro-Regular.otf"},
  {default_variations, SUBSET_FONT_BASE_PATH "AdobeVFPrototype.otf"},
  {default_variations, SUBSET_FONT_BASE_PATH "SourceSerifVariable-Roman.ttf"},
  {nullptr,            SUBSET_FONT_BASE_PATH "Comfortaa-Regular-new.ttf"},
  {nullptr,            SUBSET_FONT_BASE_PATH "NotoNastaliqUrdu-Regular.ttf"},
  {nullptr,            SUBSET_FONT_BASE_PATH "NotoSerifMyanmar-Regular.otf"},
};

static test_input_t *tests = default_tests;
static unsigned num_tests = sizeof (default_tests) / sizeof (default_tests[0]);

enum operation_t
{
  nominal_glyphs,
  glyph_h_advances,
  glyph_v_advances,
  glyph_v_origins,
  glyph_extents,
  draw_glyph,
  paint_glyph,
  load_face_and_shape,
};

static void
_hb_move_to (hb_draw_funcs_t *, void *draw_data, hb_draw_state_t *, float x, float y, void *)
{
  float &i = * (float *) draw_data;
  i += x + y;
}

static void
_hb_line_to (hb_draw_funcs_t *, void *draw_data, hb_draw_state_t *, float x, float y, void *)
{
  float &i = * (float *) draw_data;
  i += x + y;
}

static void
_hb_quadratic_to (hb_draw_funcs_t *, void *draw_data, hb_draw_state_t *, float cx, float cy, float x, float y, void *)
{
  float &i = * (float *) draw_data;
  i += cx + cy + x + y;
}

static void
_hb_cubic_to (hb_draw_funcs_t *, void *draw_data, hb_draw_state_t *, float cx1, float cy1, float cx2, float cy2, float x, float y, void *)
{
  float &i = * (float *) draw_data;
  i += cx1 + cy1 + cx2 + cy2 + x + y;
}

static void
_hb_close_path (hb_draw_funcs_t *, void *draw_data, hb_draw_state_t *, void *)
{
  float &i = * (float *) draw_data;
  i += 1.0;
}

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
		     const hb_variation_t *variations, const char * backend,
		     operation_t operation,
		     const test_input_t &test_input)
{
  hb_font_t *font;
  unsigned num_glyphs;
  {
    hb_face_t *face = hb_benchmark_face_create_from_file_or_fail (test_input.font_path, 0);
    assert (face);
    num_glyphs = hb_face_get_glyph_count (face);
    if (max_num_glyphs && num_glyphs > max_num_glyphs)
      num_glyphs = max_num_glyphs;
    font = hb_font_create (face);
    hb_face_destroy (face);
  }

  if (variations)
  {
    unsigned count = 0;
    for (const hb_variation_t *v = variations; v->tag != HB_TAG_NONE; v++)
      count++;
    hb_font_set_variations (font, variations, count);
  }

  bool ret = hb_font_set_funcs_using (font, backend);
  if (!ret)
  {
    state.SkipWithError("Backend failed to initialize for font.");
    hb_font_destroy (font);
    return;
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
    case glyph_v_advances:
    {
      hb_codepoint_t *glyphs = (hb_codepoint_t *) calloc (num_glyphs, sizeof (hb_codepoint_t));
      hb_position_t *advances = (hb_position_t *) calloc (num_glyphs, sizeof (hb_codepoint_t));

      for (unsigned g = 0; g < num_glyphs; g++)
        glyphs[g] = g;

      for (auto _ : state)
	hb_font_get_glyph_v_advances (font,
				      num_glyphs,
				      glyphs, sizeof (*glyphs),
				      advances, sizeof (*advances));

      free (advances);
      free (glyphs);
      break;
    }
    case glyph_v_origins:
    {
      hb_codepoint_t *glyphs = (hb_codepoint_t *) calloc (num_glyphs, sizeof (hb_codepoint_t));
      hb_position_t *origins_x = (hb_position_t *) calloc (num_glyphs, sizeof (hb_codepoint_t));
      hb_position_t *origins_y = (hb_position_t *) calloc (num_glyphs, sizeof (hb_codepoint_t));

      for (unsigned g = 0; g < num_glyphs; g++)
        glyphs[g] = g;

      for (auto _ : state)
        hb_font_get_glyph_v_origins (font,
				      num_glyphs,
				      glyphs, sizeof (*glyphs),
				      origins_x, sizeof (*origins_x),
				      origins_y, sizeof (*origins_y));

      free (origins_y);
      free (origins_x);
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
    case draw_glyph:
    {
      hb_draw_funcs_t *draw_funcs = _draw_funcs_create ();
      for (auto _ : state)
      {
	float i = 0;
	for (unsigned gid = 0; gid < num_glyphs; ++gid)
	  hb_font_draw_glyph (font, gid, draw_funcs, &i);
      }
      hb_draw_funcs_destroy (draw_funcs);
      break;
    }
    case paint_glyph:
    {
      hb_paint_funcs_t *paint_funcs = hb_paint_funcs_create ();
      for (auto _ : state)
      {
	for (unsigned gid = 0; gid < num_glyphs; ++gid)
	  hb_font_paint_glyph (font, gid, paint_funcs, nullptr, 0, 0);
      }
      hb_paint_funcs_destroy (paint_funcs);
      break;
    }
    case load_face_and_shape:
    {
      for (auto _ : state)
      {
	hb_face_t *face = hb_benchmark_face_create_from_file_or_fail (test_input.font_path, 0);
	assert (face);
	hb_font_t *font = hb_font_create (face);
	hb_face_destroy (face);

	bool ret = hb_font_set_funcs_using (font, backend);
	if (!ret)
	{
	  state.SkipWithError("Backend failed to initialize for font.");
	  hb_font_destroy (font);
	  return;
	}

	hb_buffer_t *buffer = hb_buffer_create ();
	hb_buffer_add_utf8 (buffer, text, -1, 0, -1);
	hb_buffer_guess_segment_properties (buffer);

	hb_shape (font, buffer, nullptr, 0);

	hb_buffer_destroy (buffer);
	hb_font_destroy (font);
      }
      break;
    }
  }


  hb_font_destroy (font);
}

static void test_backend (const char *backend,
			  const hb_variation_t *variations,
			  operation_t op,
			  const char *op_name,
			  benchmark::TimeUnit time_unit,
			  const test_input_t &test_input)
{
  char name[1024] = "BM_Font/";
  strcat (name, op_name);
  strcat (name, "/");
  const char *p = strrchr (test_input.font_path, '/');
  strcat (name, p ? p + 1 : test_input.font_path);
  strcat (name, variations ? "/var" : "");
  strcat (name, "/");
  strcat (name, backend);

  benchmark::RegisterBenchmark (name, BM_Font, variations, backend, op, test_input)
   ->Unit(time_unit);
}

static void test_operation (operation_t op,
			    const char *op_name,
			    benchmark::TimeUnit time_unit)
{
  const char **supported_backends = hb_font_list_funcs ();
  for (unsigned i = 0; i < num_tests; i++)
  {
    auto& test_input = tests[i];
    for (int variable = 0; variable < int (test_input.variations != nullptr) + 1; variable++)
    {
      const hb_variation_t *variations = variable ? test_input.variations : nullptr;
      for (const char **backend = supported_backends; *backend; backend++)
	test_backend (*backend, variations, op, op_name, time_unit, test_input);
    }
  }
}

static const char *font_file = nullptr;
static const char *variations_str = nullptr;

static GOptionEntry entries[] =
{
  {"font-file", 0, 0, G_OPTION_ARG_STRING, &font_file, "Font file-path to benchmark", "FONTFILE"},
  {"variations", 0, 0, G_OPTION_ARG_STRING, &variations_str, "Variations to apply", "VAR"},
  {"max-glyphs", 0, 0, G_OPTION_ARG_INT, &max_num_glyphs, "Maximum number of glyphs to process", "NUM"},
  {"text", 0, 0, G_OPTION_ARG_STRING, &text, "Text to use for load_face_and_shape test", "TEXT"},
  {nullptr}
};

static void print_usage (const char *prgname)
{
  g_print ("Usage: %s [OPTIONS] [FONTFILE]\n", prgname);
}

int main(int argc, char** argv)
{
  const char *prgname = g_path_get_basename (argv[0]);

  GOptionContext *context = g_option_context_new ("");
  g_option_context_set_summary (context, "Benchmark font operations");
  g_option_context_set_description (context, "Benchmark Options:");
  g_option_context_add_main_entries (context, entries, nullptr);

  bool show_help = false;
  for (int i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
    {
      argv[i] = (char *) "--help"; // Ensure it is recognized by both google-benchmark
      show_help = true;
      break;
    }
  }
  if (show_help)
  {
    print_usage (prgname);

    gchar *help_text = g_option_context_get_help (context, false, nullptr);
    help_text = strstr (help_text, "\n\n");
    g_print ("%s", help_text);

    benchmark::Initialize(&argc, argv); // This shows the help for google-benchmark
  }

  benchmark::Initialize(&argc, argv);
  g_option_context_parse (context, &argc, &argv, nullptr);
  g_option_context_free (context);

  argc--;
  argv++;
  if (!font_file && argc)
  {
    font_file = *argv;
    argc--;
    argv++;
  }

  if (argc)
  {
    g_printerr ("Unexpected arguments: ");
    for (int i = 0; i < argc; i++)
      g_printerr ("%s ", argv[i]);
    g_printerr ("\n\n");
    print_usage (prgname);
    return 1;
  }

  if (font_file)
  {
    unsigned num_variations = 0;
    if (variations_str)
    {
      const char *p = variations_str;
      while (p && *p)
      {
	num_variations++;
	p = strchr (p, ',');
	if (p) p++;
      }
    }
    hb_variation_t *variations = num_variations ? (hb_variation_t *) calloc (num_variations, sizeof (hb_variation_t)) : nullptr;
    if (variations_str)
    {
      const char *p = variations_str;
      for (unsigned i = 0; i < num_variations; i++)
      {
	const char *end = strchr (p, ',');
	if (end)
	{
	  hb_variation_from_string (p, end - p, &variations[i]);
	  p = end + 1;
	}
	else
	{
	  hb_variation_from_string (p, -1, &variations[i]);
	  p = nullptr;
	}
      }
    }

    num_tests = 1;
    tests = (test_input_t *) calloc (num_tests, sizeof (test_input_t));
    tests[0].variations = num_variations ? variations : default_variations;
    tests[0].font_path = font_file;
  }

#define TEST_OPERATION(op, time_unit) test_operation (op, #op, time_unit)

  TEST_OPERATION (nominal_glyphs, benchmark::kMicrosecond);
  TEST_OPERATION (glyph_h_advances, benchmark::kMicrosecond);
  TEST_OPERATION (glyph_v_advances, benchmark::kMicrosecond);
  TEST_OPERATION (glyph_v_origins, benchmark::kMicrosecond);
  TEST_OPERATION (glyph_extents, benchmark::kMicrosecond);
  TEST_OPERATION (draw_glyph, benchmark::kMillisecond);
  TEST_OPERATION (paint_glyph, benchmark::kMillisecond);
  TEST_OPERATION (load_face_and_shape, benchmark::kMicrosecond);

#undef TEST_OPERATION

  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();

  if (tests != default_tests)
  {
    if (tests[0].variations != default_variations)
      free (tests[0].variations);
    free (tests);
  }
}

