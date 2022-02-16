#include "benchmark/benchmark.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "perf-shaping.hh"
#ifdef HAVE_FREETYPE
enum backend_t { HARFBUZZ, FREETYPE, TTF_PARSER };
#include "perf-extents.hh"
#include "perf-draw.hh"
#endif

BENCHMARK_MAIN ();
