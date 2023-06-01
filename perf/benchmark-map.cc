/*
 * Benchmarks for hb_map_t operations.
 */
#include "benchmark/benchmark.h"

#include <cassert>
#include <cstdlib>
#include "hb.h"

void RandomMap(unsigned size, hb_map_t* out, hb_set_t* key_sample) {
  hb_map_clear(out);

  unsigned sample_denom = 1;
  if (size > 10000)
    sample_denom = size / 10000;

  srand(size);
  for (unsigned i = 0; i < size; i++) {
    while (true) {
      hb_codepoint_t next = rand();
      if (hb_map_has (out, next)) continue;

      hb_codepoint_t val = rand();
      if (key_sample && val % sample_denom == 0)
        hb_set_add (key_sample, next);
      hb_map_set (out, next, val);
      break;
    }
  }
}

/* Insert a single value into map of varying sizes. */
static void BM_MapInsert(benchmark::State& state) {
  unsigned map_size = state.range(0);

  hb_map_t* original = hb_map_create ();
  RandomMap(map_size, original, nullptr);
  assert(hb_map_get_population(original) == map_size);

  unsigned mask = 1;
  while (mask < map_size)
    mask <<= 1;
  mask--;

  auto needle = 0u;
  for (auto _ : state) {
    // TODO(garretrieger): create a copy of the original map.
    //                     Needs a hb_map_copy(..) in public api.

    hb_map_set (original, needle++ & mask, 1);
  }

  hb_map_destroy(original);
}
BENCHMARK(BM_MapInsert)
    ->Range(1 << 4, 1 << 20);

/* Single value lookup on map of various sizes where the key is not present. */
static void BM_MapLookupMiss(benchmark::State& state) {
  unsigned map_size = state.range(0);

  hb_map_t* original = hb_map_create ();
  RandomMap(map_size, original, nullptr);
  assert(hb_map_get_population(original) == map_size);

  auto needle = map_size / 2;

  for (auto _ : state) {
    benchmark::DoNotOptimize(
        hb_map_get (original, needle++));
  }

  hb_map_destroy(original);
}
BENCHMARK(BM_MapLookupMiss)
    ->Range(1 << 4, 1 << 20); // Map size

/* Single value lookup on map of various sizes. */
static void BM_MapLookupHit(benchmark::State& state) {
  unsigned map_size = state.range(0);

  hb_map_t* original = hb_map_create ();
  hb_set_t* key_set = hb_set_create ();
  RandomMap(map_size, original, key_set);
  assert(hb_map_get_population(original) == map_size);

  unsigned num_keys = hb_set_get_population (key_set);
  hb_codepoint_t* key_array =
    (hb_codepoint_t*) calloc (num_keys, sizeof(hb_codepoint_t));
  
  hb_codepoint_t cp = HB_SET_VALUE_INVALID;
  unsigned i = 0;
  while (hb_set_next (key_set, &cp))
    key_array[i++] = cp;

  i = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        hb_map_get (original, key_array[i++ % num_keys]));
  }

  hb_set_destroy (key_set);
  free (key_array);
  hb_map_destroy(original);
}
BENCHMARK(BM_MapLookupHit)
    ->Range(1 << 4, 1 << 20); // Map size


BENCHMARK_MAIN();
