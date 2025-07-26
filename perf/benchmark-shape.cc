#include "hb-benchmark.hh"

#include <glib.h>

#define SUBSET_FONT_BASE_PATH "test/subset/data/fonts/"

struct test_input_t
{
  const char *font_path;
  const char *text_path;
} default_tests[] =
{

  {"perf/fonts/NotoNastaliqUrdu-Regular.ttf",
   "perf/texts/fa-thelittleprince.txt"},

  {"perf/fonts/NotoNastaliqUrdu-Regular.ttf",
   "perf/texts/fa-words.txt"},

  {"perf/fonts/Amiri-Regular.ttf",
   "perf/texts/fa-thelittleprince.txt"},

  {SUBSET_FONT_BASE_PATH "NotoSansDevanagari-Regular.ttf",
   "perf/texts/hi-words.txt"},

  {"perf/fonts/Roboto-Regular.ttf",
   "perf/texts/en-thelittleprince.txt"},

  {"perf/fonts/Roboto-Regular.ttf",
   "perf/texts/en-words.txt"},

  {SUBSET_FONT_BASE_PATH "SourceSerifVariable-Roman.ttf",
   "perf/texts/react-dom.txt"},
};

static test_input_t *tests = default_tests;
static unsigned num_tests = sizeof (default_tests) / sizeof (default_tests[0]);
const char *variation = nullptr;
const char *direction = nullptr;

static bool shape (hb_buffer_t *buf,
		   hb_font_t *font,
		   const char *text,
		   unsigned text_length,
		   const char *shaper)
{
  const char *end;
  while ((end = (const char *) memchr (text, '\n', text_length)))
  {
    hb_buffer_clear_contents (buf);
    hb_buffer_add_utf8 (buf, text, text_length, 0, end - text);
    hb_buffer_guess_segment_properties (buf);
    if (direction)
      hb_buffer_set_direction (buf, hb_direction_from_string (direction, -1));
    const char *shaper_list[] = {shaper, nullptr};
    if (!hb_shape_full (font, buf, nullptr, 0, shaper_list))
      return false;

    unsigned skip = end - text + 1;
    text_length -= skip;
    text += skip;
  }
  return true;
}

static void BM_Shape (benchmark::State &state,
		      const char *shaper,
		      const test_input_t &input)
{
  hb_font_t *font;
  {
    hb_face_t *face = hb_benchmark_face_create_from_file_or_fail (input.font_path, 0);
    assert (face);
    font = hb_font_create (face);
    hb_face_destroy (face);
  }

  if (variation)
  {
    hb_variation_t var;
    hb_variation_from_string (variation, -1, &var);
    hb_font_set_variations (font, &var, 1);
  }

  hb_blob_t *text_blob = hb_blob_create_from_file_or_fail (input.text_path);
  assert (text_blob);
  unsigned text_length;
  const char *text = hb_blob_get_data (text_blob, &text_length);

  hb_buffer_t *buf = hb_buffer_create ();

  // Shape once, to warm up the font and buffer.
  bool ret = shape (buf, font, text, text_length, shaper);
  if (!ret)
  {
    state.SkipWithMessage ("Shaping failed.");
    goto done;
  }

  for (auto _ : state)
  {
    bool ret = shape (buf, font, text, text_length, shaper);
    assert (ret);
  }

done:
  hb_buffer_destroy (buf);

  hb_blob_destroy (text_blob);
  hb_font_destroy (font);
}

static void test_shaper (const char *shaper,
			 const test_input_t &test_input)
{
  char name[1024] = "BM_Shape";
  const char *p;
  strcat (name, "/");
  p = strrchr (test_input.font_path, '/');
  strcat (name, p ? p + 1 : test_input.font_path);
  strcat (name, "/");
  p = strrchr (test_input.text_path, '/');
  strcat (name, p ? p + 1 : test_input.text_path);
  strcat (name, "/");
  strcat (name, shaper);

  benchmark::RegisterBenchmark (name, BM_Shape, shaper, test_input)
   ->Unit(benchmark::kMillisecond);
}

static const char *font_file = nullptr;
static const char *text_file = nullptr;

static GOptionEntry entries[] =
{
  {"font-file", 0, 0, G_OPTION_ARG_STRING, &font_file, "Font file-path to benchmark", "FONTFILE"},
  {"text-file", 0, 0, G_OPTION_ARG_STRING, &text_file, "Text file-path to benchmark", "TEXTFILE"},
  {"variations", 0, 0, G_OPTION_ARG_STRING, &variation, "Variations to apply during shaping", "VARIATIONS"},
  {"direction", 0, 0, G_OPTION_ARG_STRING, &direction, "Direction to apply during shaping", "DIRECTION"},
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
  g_option_context_set_summary (context, "Benchmark text shaping with different shapers");
  g_option_context_set_description (context, "Benchmark Options:");
  g_option_context_add_main_entries (context, entries, nullptr);

  // if --help is provided, we need to write help for both option parsers.
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
    argv++;
    argc--;
  }
  if (!text_file && argc)
  {
    text_file = *argv;
    argv++;
    argc--;
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

  test_input_t static_test = {};
  if (font_file && text_file)
  {
    if (font_file && text_file)
    {
      static_test.font_path = font_file;
      static_test.text_path = text_file;
      tests = &static_test;
      num_tests = 1;
    }
  }

  for (unsigned i = 0; i < num_tests; i++)
  {
    auto& test_input = tests[i];
    const char **shapers = hb_shape_list_shapers ();
    for (const char **shaper = shapers; *shaper; shaper++)
      test_shaper (*shaper, test_input);
  }

  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
}
