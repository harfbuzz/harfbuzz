#include "hb-benchmark.hh"

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
  unsigned orig_text_length;
  const char *orig_text = hb_blob_get_data (text_blob, &orig_text_length);

  hb_buffer_t *buf = hb_buffer_create ();
  for (auto _ : state)
  {
    unsigned text_length = orig_text_length;
    const char *text = orig_text;

    const char *end;
    while ((end = (const char *) memchr (text, '\n', text_length)))
    {
      hb_buffer_clear_contents (buf);
      hb_buffer_add_utf8 (buf, text, text_length, 0, end - text);
      hb_buffer_guess_segment_properties (buf);
      const char *shaper_list[] = {shaper, nullptr};
      hb_shape_full (font, buf, nullptr, 0, shaper_list);

      unsigned skip = end - text + 1;
      text_length -= skip;
      text += skip;
    }
  }
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

int main(int argc, char** argv)
{
  benchmark::Initialize(&argc, argv);

  test_input_t static_test = {};
  if (argc > 2)
  {
    static_test.font_path = argv[1];
    static_test.text_path = argv[2];
    tests = &static_test;
    num_tests = 1;
  }
  if (argc > 3)
    variation = argv[3];

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
