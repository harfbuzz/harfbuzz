/*
 * Benchmarks for hb_set_t operations.
 */
#include "benchmark/benchmark.h"

#include <cstdlib>
#include "hb.h"

void RandomSet(unsigned size, unsigned max_value, hb_set_t* out) {
  hb_set_clear(out);

  srand(size * max_value);
  for (unsigned i = 0; i < size; i++) {
    while (true) {
      unsigned next = rand() % max_value;
      if (hb_set_has (out, next)) continue;

      hb_set_add(out, next);
      break;
    }
  }
}

// TODO(garretrieger): benchmark union/subtract/intersection etc.

/* Insert a single value into set of varying sizes. */
static void BM_SetInsert(benchmark::State& state) {
  unsigned set_size = state.range(0);
  unsigned max_value = state.range(0) * state.range(1);

  hb_set_t* original = hb_set_create ();
  RandomSet(set_size, max_value, original);
  assert(hb_set_get_population(original) == set_size);

  for (auto _ : state) {
    state.PauseTiming();
    hb_set_t* data = hb_set_copy(original);
    state.ResumeTiming();

    hb_set_add(data, rand() % max_value);

    state.PauseTiming();
    hb_set_destroy(data);
    state.ResumeTiming();
  }

  hb_set_destroy(original);
}
BENCHMARK(BM_SetInsert)
    ->ArgsProduct({
        benchmark::CreateRange(1 << 10, 1 << 16, 8), // Set size
        benchmark::CreateDenseRange(2, 8, /*step=*/2) // Density
      });

/* Single value lookup on sets of various sizes. */
static void BM_SetLookup(benchmark::State& state) {
  unsigned set_size = state.range(0);
  unsigned max_value = state.range(0) * state.range(1);

  hb_set_t* original = hb_set_create ();
  RandomSet(set_size, max_value, original);
  assert(hb_set_get_population(original) == set_size);

  for (auto _ : state) {
    benchmark::DoNotOptimize(
        hb_set_has (original, rand() % max_value));
  }

  hb_set_destroy(original);
}
BENCHMARK(BM_SetLookup)
    ->ArgsProduct({
        benchmark::CreateRange(1 << 10, 1 << 16, 8), // Set size
        benchmark::CreateDenseRange(2, 8, /*step=*/2) // Density
      });

/* Full iteration of sets of varying sizes. */
static void BM_SetIteration(benchmark::State& state) {
  unsigned set_size = state.range(0);
  unsigned max_value = state.range(0) * state.range(1);

  hb_set_t* original = hb_set_create ();
  RandomSet(set_size, max_value, original);
  assert(hb_set_get_population(original) == set_size);

  for (auto _ : state) {
    hb_codepoint_t cp = HB_SET_VALUE_INVALID;
    while (hb_set_next (original, &cp)) {
      continue;
    }
  }

  hb_set_destroy(original);
}
BENCHMARK(BM_SetIteration)
    ->Unit(benchmark::kMicrosecond)
    ->ArgsProduct({
        benchmark::CreateRange(1 << 10, 1 << 16, 8), // Set size
        benchmark::CreateDenseRange(2, 8, /*step=*/2) // Density
      });

BENCHMARK_MAIN();
