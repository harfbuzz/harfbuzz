#include "benchmark/benchmark.h"

#include "hb.h"

static void shape (benchmark::State &state, const char *text_path,
		   hb_direction_t direction, hb_script_t script,
		   const char *font_path)
{
  hb_font_t *font;
  {
    hb_blob_t *blob = hb_blob_create_from_file_or_fail (font_path);
    assert (blob);
    hb_face_t *face = hb_face_create (blob, 0);
    hb_blob_destroy (blob);
    font = hb_font_create (face);
    hb_face_destroy (face);
  }

  hb_blob_t *text_blob = hb_blob_create_from_file_or_fail (text_path);
  assert (text_blob);
  unsigned text_length;
  const char *text = hb_blob_get_data (text_blob, &text_length);

  hb_buffer_t *buf = hb_buffer_create ();
  for (auto _ : state)
  {
    hb_buffer_add_utf8 (buf, text, text_length, 0, -1);
    hb_buffer_set_direction (buf, direction);
    hb_buffer_set_script (buf, script);
    hb_shape (font, buf, nullptr, 0);
    hb_buffer_clear_contents (buf);
  }
  hb_buffer_destroy (buf);

  hb_blob_destroy (text_blob);
  hb_font_destroy (font);
}

BENCHMARK_CAPTURE (shape, fa-thelittleprince.txt - Amiri,
		   "perf/texts/fa-thelittleprince.txt",
		   HB_DIRECTION_RTL, HB_SCRIPT_ARABIC,
		   "perf/fonts/Amiri-Regular.ttf");
BENCHMARK_CAPTURE (shape, fa-thelittleprince.txt - NotoNastaliqUrdu,
		   "perf/texts/fa-thelittleprince.txt",
		   HB_DIRECTION_RTL, HB_SCRIPT_ARABIC,
		   "perf/fonts/NotoNastaliqUrdu-Regular.ttf");

BENCHMARK_CAPTURE (shape, fa-monologue.txt - Amiri,
		   "perf/texts/fa-monologue.txt",
		   HB_DIRECTION_RTL, HB_SCRIPT_ARABIC,
		   "perf/fonts/Amiri-Regular.ttf");
BENCHMARK_CAPTURE (shape, fa-monologue.txt - NotoNastaliqUrdu,
		   "perf/texts/fa-monologue.txt",
		   HB_DIRECTION_RTL, HB_SCRIPT_ARABIC,
		   "perf/fonts/NotoNastaliqUrdu-Regular.ttf");

BENCHMARK_CAPTURE (shape, en-thelittleprince.txt - Roboto,
		   "perf/texts/en-thelittleprince.txt",
		   HB_DIRECTION_LTR, HB_SCRIPT_LATIN,
		   "perf/fonts/Roboto-Regular.ttf");

BENCHMARK_CAPTURE (shape, en-words.txt - Roboto,
		   "perf/texts/en-words.txt",
		   HB_DIRECTION_LTR, HB_SCRIPT_LATIN,
		   "perf/fonts/Roboto-Regular.ttf");
