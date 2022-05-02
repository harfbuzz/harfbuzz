#include "benchmark/benchmark.h"
#include <cstring>

#include "hb.h"


struct test_input_t
{
  const char *text_path;
  const char *font_path;
} tests[] =
{
  {"perf/texts/fa-thelittleprince.txt",
   "perf/fonts/Amiri-Regular.ttf"},

  {"perf/texts/fa-thelittleprince.txt",
   "perf/fonts/NotoNastaliqUrdu-Regular.ttf"},

  {"perf/texts/fa-monologue.txt",
   "perf/fonts/Amiri-Regular.ttf"},

  {"perf/texts/fa-monologue.txt",
   "perf/fonts/NotoNastaliqUrdu-Regular.ttf"},

  {"perf/texts/en-thelittleprince.txt",
   "perf/fonts/Roboto-Regular.ttf"},

  {"perf/texts/en-words.txt",
   "perf/fonts/Roboto-Regular.ttf"},
};


static void BM_Shape (benchmark::State &state, const test_input_t &input)
{
  hb_font_t *font;
  {
    hb_blob_t *blob = hb_blob_create_from_file_or_fail (input.font_path);
    assert (blob);
    hb_face_t *face = hb_face_create (blob, 0);
    hb_blob_destroy (blob);
    font = hb_font_create (face);
    hb_face_destroy (face);
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
      hb_shape (font, buf, nullptr, 0);

      unsigned skip = end - text + 1;
      text_length -= skip;
      text += skip;
    }
  }
  hb_buffer_destroy (buf);

  hb_blob_destroy (text_blob);
  hb_font_destroy (font);
}

int main(int argc, char** argv)
{
  for (auto& test_input : tests)
  {
    char name[1024] = "BM_Shape";
    strcat (name, strrchr (test_input.text_path, '/'));
    strcat (name, strrchr (test_input.font_path, '/'));

    benchmark::RegisterBenchmark (name, BM_Shape, test_input)
     ->Unit(benchmark::kMillisecond);
  }

  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
}
