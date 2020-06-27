#define PERF_BENCH
#include "perf-extents.hh"

#include <stdlib.h>

const char *font_path = "test/api/fonts/Estedad-VF.ttf";
bool skip_var = false;

static void extents_bench (benchmark::State &state, bool is_var, bool is_ft)
{
  if (is_var && skip_var) return;
  extents (state, font_path, is_var, is_ft);
}
BENCHMARK_CAPTURE (extents_bench, ot, false, false);
BENCHMARK_CAPTURE (extents_bench, ft, false, true);
BENCHMARK_CAPTURE (extents_bench, ot/var, true, false);
BENCHMARK_CAPTURE (extents_bench, ft/var, true, true);

int main(int argc, char** argv)
{
  if (argc > 1 && argv[1][0] != '-') { font_path = argv[1]; ++argv; --argc; }

  {
    hb_blob_t *blob = hb_blob_create_from_file (font_path);
    hb_face_t *face = hb_face_create (blob, 0);
    hb_blob_destroy (blob);
    if (!hb_ot_var_get_axis_count (face))
    {
      printf ("\n** Runs with variation will be skipped for this non-var font **\n\n");
      skip_var = true;
    }
    hb_face_destroy (face);
  }


  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
  ::benchmark::RunSpecifiedBenchmarks();
}
