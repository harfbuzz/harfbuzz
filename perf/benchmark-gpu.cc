/*
 * Copyright (C) 2026  Behdad Esfahbod
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Author(s): Behdad Esfahbod
 */

#include "hb-benchmark.hh"

#include <hb-gpu.h>

#include <glib.h>

#define SUBSET_FONT_BASE_PATH "test/subset/data/fonts/"

static struct test_input_t
{
  const char *font_path;
} default_tests[] =
{
  {SUBSET_FONT_BASE_PATH "Roboto-Regular.ttf"},
  {SUBSET_FONT_BASE_PATH "SourceSansPro-Regular.otf"},
  {SUBSET_FONT_BASE_PATH "AdobeVFPrototype.otf"},
  {SUBSET_FONT_BASE_PATH "Comfortaa-Regular-new.ttf"},
  {SUBSET_FONT_BASE_PATH "NotoNastaliqUrdu-Regular.ttf"},
  {SUBSET_FONT_BASE_PATH "NotoSerifMyanmar-Regular.otf"},
};

static test_input_t *tests = default_tests;
static unsigned num_tests = sizeof (default_tests) / sizeof (default_tests[0]);

static void BM_GpuEncode (benchmark::State &state,
			   const test_input_t &test_input)
{
  hb_face_t *face = hb_benchmark_face_create_from_file_or_fail (test_input.font_path, 0);
  if (!face)
  {
    state.SkipWithError ("Failed to open font file");
    return;
  }

  hb_font_t *font = hb_font_create (face);
  unsigned num_glyphs = hb_face_get_glyph_count (face);
  hb_gpu_draw_t *draw = hb_gpu_draw_create_or_fail ();
  assert (draw);

  for (auto _ : state)
    for (unsigned gid = 0; gid < num_glyphs; ++gid)
    {
      hb_gpu_draw_clear (draw);
      hb_gpu_draw_glyph (draw, font, gid);
      hb_blob_t *blob = hb_gpu_draw_encode (draw, nullptr);
      hb_gpu_draw_recycle_blob (draw, blob);
    }

  hb_gpu_draw_destroy (draw);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static const char *font_file = nullptr;

static GOptionEntry entries[] =
{
  {"font-file", 0, 0, G_OPTION_ARG_STRING, &font_file, "Font file-path to benchmark", "FONTFILE"},
  {nullptr}
};

int main (int argc, char **argv)
{
  benchmark::Initialize (&argc, argv);

  GOptionContext *context = g_option_context_new ("");
  g_option_context_set_summary (context, "Benchmark GPU glyph encoding");
  g_option_context_add_main_entries (context, entries, nullptr);
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

  if (font_file)
  {
    num_tests = 1;
    tests = (test_input_t *) calloc (num_tests, sizeof (test_input_t));
    tests[0].font_path = font_file;
  }

  for (unsigned i = 0; i < num_tests; i++)
  {
    auto &test_input = tests[i];
    char name[1024] = "BM_GpuEncode/";
    const char *p = strrchr (test_input.font_path, '/');
    strcat (name, p ? p + 1 : test_input.font_path);

    benchmark::RegisterBenchmark (name, BM_GpuEncode, test_input)
      ->Unit (benchmark::kMillisecond);
  }

  benchmark::RunSpecifiedBenchmarks ();
  benchmark::Shutdown ();

  if (tests != default_tests)
    free (tests);
}
