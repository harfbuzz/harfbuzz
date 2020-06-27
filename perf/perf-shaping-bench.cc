#define PERF_BENCH
#include "perf-shaping.hh"

#include <stdlib.h>

const char *font_path = "perf/fonts/Amiri-Regular.ttf";
const char *text_path = "perf/texts/fa-thelittleprince.txt";

static void shape_bench (benchmark::State &state)
{
  shape (state, text_path, HB_DIRECTION_INVALID, HB_SCRIPT_INVALID, font_path);
}

BENCHMARK (shape_bench);

int main(int argc, char** argv)
{
  if (argc > 1 && argv[1][0] != '-') { font_path = argv[1]; ++argv; --argc; }
  if (argc > 1 && argv[1][0] != '-') { text_path = argv[1]; ++argv; --argc; }

  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
  ::benchmark::RunSpecifiedBenchmarks();
}
