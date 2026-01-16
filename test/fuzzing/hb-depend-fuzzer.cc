#include "hb-fuzzer.hh"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "hb-subset.h"

#ifdef HB_DEPEND_API
#include <hb.h>

/* Globals for configuration (set by main() in standalone mode) */
const char *_hb_depend_fuzzer_current_font_path = NULL;
unsigned _hb_depend_fuzzer_seed = 0;
uint64_t _hb_depend_fuzzer_single_test = 0;  /* Specific test to run, or 0 for all */
unsigned _hb_depend_fuzzer_num_tests = 1024; /* Number of tests to run */
bool _hb_depend_fuzzer_verbose = false;
bool _hb_depend_fuzzer_report_over_approximation = false;
hb_codepoint_t _hb_depend_fuzzer_debug_gid = HB_CODEPOINT_INVALID;  /* Specific glyph to debug */

static bool config_initialized = false;

/* Initialize configuration - uses globals set by main */
static void
init_fuzzer_config ()
{
  if (config_initialized)
    return;

  config_initialized = true;

  /* Initialize RNG with seed (already set by main or defaulted to random) */
  srand (_hb_depend_fuzzer_seed);

  /* Check for debug glyph ID */
  const char *debug_gid_str = getenv ("HB_DEPEND_FUZZER_DEBUG_GID");
  if (debug_gid_str)
    _hb_depend_fuzzer_debug_gid = (hb_codepoint_t) strtoul (debug_gid_str, NULL, 0);
}

/* Simple RNG helpers */
static unsigned
rng_range (unsigned min, unsigned max)
{
  if (max <= min)
    return min;
  return min + (rand () % (max - min + 1));
}

/* Select n random distinct values from range [0, max) */
static void
rng_select_distinct (hb_set_t *out, unsigned n, unsigned max)
{
  if (n >= max)
  {
    /* Select all */
    for (unsigned i = 0; i < max; i++)
      hb_set_add (out, i);
    return;
  }

  /* Simple approach: keep trying random values until we have enough distinct ones */
  while (hb_set_get_population (out) < n)
  {
    unsigned val = rng_range (0, max - 1);
    hb_set_add (out, val);
  }
}

/* Select n random distinct values from an existing set */
static void
rng_select_from_set (hb_set_t *out, unsigned n, const hb_set_t *source)
{
  unsigned source_size = hb_set_get_population (source);
  if (n == 0 || source_size == 0)
    return;

  if (n >= source_size)
  {
    /* Select all */
    hb_set_union (out, source);
    return;
  }

  /* Convert source to array for indexed access */
  hb_codepoint_t *array = (hb_codepoint_t *) malloc (source_size * sizeof (hb_codepoint_t));
  unsigned idx = 0;
  hb_codepoint_t val = HB_SET_VALUE_INVALID;
  while (hb_set_next (source, &val))
    array[idx++] = val;

  /* Select n random indices */
  while (hb_set_get_population (out) < n)
  {
    unsigned random_idx = rng_range (0, source_size - 1);
    hb_set_add (out, array[random_idx]);
  }

  free (array);
}

/* Test parameters generated from a 64-bit seed */
struct test_params
{
  bool is_gsub;           /* GSUB test (codepoint-based) vs non-GSUB (glyph-based) */
  unsigned target_size;   /* Target set size (may be clamped by font size) */
  hb_set_t *glyphs;       /* Starting glyphs (NULL if GSUB) */
  hb_set_t *codepoints;   /* Starting codepoints (NULL if non-GSUB) */
  hb_set_t *features;     /* Active features (NULL for default/all, only used for GSUB) */
};

/* Get all GSUB features from a face */
static hb_set_t *
get_gsub_features (hb_face_t *face)
{
  hb_set_t *features = hb_set_create ();

  /* Check if GSUB table exists */
  if (!hb_ot_layout_has_substitution (face))
    return features;

  /* Get all feature tags from GSUB table */
  unsigned int feature_count = hb_ot_layout_table_get_feature_tags (face,
                                                                     HB_OT_TAG_GSUB,
                                                                     0, nullptr, nullptr);
  hb_tag_t *feature_tags = (hb_tag_t *) malloc (feature_count * sizeof (hb_tag_t));
  if (feature_tags)
  {
    hb_ot_layout_table_get_feature_tags (face,
                                         HB_OT_TAG_GSUB,
                                         0, &feature_count, feature_tags);
    for (unsigned int i = 0; i < feature_count; i++)
      hb_set_add (features, feature_tags[i]);
    free (feature_tags);
  }

  return features;
}

/* Generate a test from a 64-bit seed
 *
 * The seed determines:
 * - Test type (GSUB vs non-GSUB, size category) via seed % 100
 * - Target size within range via middle bits
 * - Specific element selection via high bits
 * - For GSUB tests: which features to activate (from available GSUB features)
 *
 * Test type distribution (percentages of total tests):
 * - Non-GSUB single (1):      21%
 * - Non-GSUB small (2-5):     17%
 * - Non-GSUB medium (10-20):  10%
 * - Non-GSUB large (50-100):   6%
 * - Non-GSUB very large (N/10-N/4): 4%
 * - GSUB single (1):          21%
 * - GSUB small (2-5):         10%
 * - GSUB medium (10-20):       6%
 * - GSUB large (50-100):       4%
 * Total: 100% (unused: 1%)
 *
 * Feature selection distribution (GSUB tests only):
 * - Single feature:           30%
 * - Small (2-5 features):     35%
 * - Medium (6-15 features):   20%
 * - Large (16-50 features):   10%
 * - All features:              5%
 */
static test_params
generate_test_from_seed (uint64_t seed, hb_face_t *face, hb_font_t *font)
{
  test_params params;
  params.glyphs = NULL;
  params.codepoints = NULL;
  params.features = NULL;

  unsigned num_glyphs = hb_face_get_glyph_count (face);

  /* Get all codepoints for GSUB tests */
  hb_set_t *all_codepoints = hb_set_create ();
  hb_face_collect_unicodes (face, all_codepoints);
  unsigned num_codepoints = hb_set_get_population (all_codepoints);

  /* Step 1: Determine test type from seed % 100
   * Assumes seed is already pseudo-randomly distributed */
  unsigned type_selector = seed % 100;

  unsigned min_size, max_size;

  if (type_selector < 21) {
    /* Non-GSUB single (21%) */
    params.is_gsub = false;
    min_size = max_size = 1;
  } else if (type_selector < 38) {
    /* Non-GSUB small (17%) */
    params.is_gsub = false;
    min_size = 2;
    max_size = 5;
  } else if (type_selector < 48) {
    /* Non-GSUB medium (10%) */
    params.is_gsub = false;
    min_size = 10;
    max_size = 20;
  } else if (type_selector < 54) {
    /* Non-GSUB large (6%) */
    params.is_gsub = false;
    min_size = 50;
    max_size = 100;
  } else if (type_selector < 58) {
    /* Non-GSUB very large (4%) */
    params.is_gsub = false;
    min_size = num_glyphs / 10;
    max_size = num_glyphs / 4;
  } else if (type_selector < 79) {
    /* GSUB single (21%) */
    params.is_gsub = true;
    min_size = max_size = 1;
  } else if (type_selector < 89) {
    /* GSUB small (10%) */
    params.is_gsub = true;
    min_size = 2;
    max_size = 5;
  } else if (type_selector < 95) {
    /* GSUB medium (6%) */
    params.is_gsub = true;
    min_size = 10;
    max_size = 20;
  } else {
    /* GSUB large (4%) */
    params.is_gsub = true;
    min_size = 50;
    max_size = 100;
  }

  /* Step 2: Determine target size within range using middle bits */
  unsigned size_range = max_size - min_size + 1;
  params.target_size = min_size + ((seed >> 8) % size_range);

  /* Clamp to available glyphs/codepoints */
  if (params.is_gsub)
    params.target_size = params.target_size < num_codepoints ? params.target_size : num_codepoints;
  else
    params.target_size = params.target_size < num_glyphs ? params.target_size : num_glyphs;

  /* Skip tests on fonts too small for the test type */
  if (params.target_size < min_size)
  {
    hb_set_destroy (all_codepoints);
    params.target_size = 0;
    return params;
  }

  /* Step 3: Select specific elements using full seed with mixing */
  unsigned selection_seed = (unsigned)((seed >> 32) ^ seed);
  srand (selection_seed);

  if (params.is_gsub)
  {
    /* GSUB test: select codepoints and map to glyphs */
    params.codepoints = hb_set_create ();
    params.glyphs = hb_set_create ();

    /* Select random codepoints by index */
    hb_set_t *cp_indices = hb_set_create ();
    rng_select_distinct (cp_indices, params.target_size, num_codepoints);

    /* Convert indices to actual codepoints */
    hb_codepoint_t idx = HB_SET_VALUE_INVALID;
    while (hb_set_next (cp_indices, &idx))
    {
      hb_codepoint_t cp = HB_SET_VALUE_INVALID;
      for (unsigned j = 0; j <= idx && hb_set_next (all_codepoints, &cp); j++)
        ;
      hb_set_add (params.codepoints, cp);

      /* Map codepoint to glyph */
      hb_codepoint_t gid;
      if (hb_font_get_nominal_glyph (font, cp, &gid))
        hb_set_add (params.glyphs, gid);
    }
    hb_set_destroy (cp_indices);

    /* For GSUB tests, select features to activate
     * Distribution:
     * - Single feature (30%)
     * - Small (2-5 features) (35%)
     * - Medium (6-15 features) (20%)
     * - Large (16-50 features) (10%)
     * - All features (5%)
     */
    hb_set_t *available_features = get_gsub_features (face);
    unsigned num_features = hb_set_get_population (available_features);

    if (num_features > 0)
    {
      unsigned feature_selector = rand () % 100;
      unsigned num_to_select;

      if (feature_selector < 30)
        num_to_select = 1;  /* Single (30%) */
      else if (feature_selector < 65)
        num_to_select = rng_range (2, 5);  /* Small (35%) */
      else if (feature_selector < 85)
        num_to_select = rng_range (6, 15);  /* Medium (20%) */
      else if (feature_selector < 95)
        num_to_select = rng_range (16, 50);  /* Large (10%) */
      else
        num_to_select = num_features;  /* All (5%) */

      /* Cap at available */
      if (num_to_select > num_features)
        num_to_select = num_features;

      /* Select specific features */
      params.features = hb_set_create ();
      rng_select_from_set (params.features, num_to_select, available_features);
    }

    hb_set_destroy (available_features);
  }
  else
  {
    /* Non-GSUB test: select glyphs directly */
    params.glyphs = hb_set_create ();
    rng_select_distinct (params.glyphs, params.target_size, num_glyphs);
  }

  hb_set_destroy (all_codepoints);
  return params;
}

/*
 * Depend Fuzzer - Validates depend API correctness by comparing closures
 *
 * For each font, this fuzzer:
 * 1. Extracts the dependency graph using hb_depend_from_face()
 * 2. For various starting glyphs, computes closure via:
 *    a) Following the depend API dependency graph
 *    b) Using HarfBuzz's subset plan closure calculation
 * 3. Verifies that both methods produce identical results
 *
 * This validates that the depend API correctly extracts dependencies from
 * all OpenType tables (glyf, COLR, MATH, etc.) by comparing against the
 * battle-tested subset closure implementation.
 */

/* Compute closure using depend API by following the dependency graph
 * skip_gsub: if true, skip GSUB dependencies to match subset behavior
 * (subset doesn't follow GSUB when starting with glyph IDs)
 * active_features: if non-NULL, only follow GSUB dependencies from these features */
static void
compute_depend_closure (hb_depend_t *depend, hb_set_t *glyphs, bool skip_gsub,
                        hb_set_t *active_features)
{
  if (_hb_depend_fuzzer_debug_gid != HB_CODEPOINT_INVALID)
  {
    fprintf (stderr, "[DEBUG] Starting closure, glyph %u %s in initial set\n",
             _hb_depend_fuzzer_debug_gid,
             hb_set_has (glyphs, _hb_depend_fuzzer_debug_gid) ? "IS" : "is NOT");
  }

  hb_set_t *to_process = hb_set_create ();
  hb_set_union (to_process, glyphs);

  /* Track ligature glyph IDs separately for deferred processing */
  hb_set_t *deferred_ligatures = hb_set_create ();

  /* Phase 1: Process non-ligature dependencies normally */
  while (!hb_set_is_empty (to_process))
  {
    hb_codepoint_t gid = hb_set_get_min (to_process);
    hb_set_del (to_process, gid);

    if (gid == _hb_depend_fuzzer_debug_gid)
      fprintf (stderr, "[DEBUG] Processing glyph %u\n", gid);

    hb_codepoint_t index = 0;
    hb_tag_t table_tag;
    hb_codepoint_t dependent;
    hb_tag_t layout_tag;
    hb_codepoint_t ligature_set;

    while (hb_depend_get_glyph_entry (depend, gid, index++,
                                      &table_tag, &dependent,
                                      &layout_tag, &ligature_set))
    {
      if (dependent == _hb_depend_fuzzer_debug_gid || gid == _hb_depend_fuzzer_debug_gid)
        fprintf (stderr, "[DEBUG] Edge: %u -> %u (table=%c%c%c%c, ligset=%u)\n",
                 gid, dependent, HB_UNTAG(table_tag), ligature_set);
      /* Skip GSUB dependencies if requested (to match subset behavior) */
      if (skip_gsub && table_tag == HB_OT_TAG_GSUB)
        continue;

      /* If active_features is specified, filter GSUB deps by feature */
      if (active_features && table_tag == HB_OT_TAG_GSUB)
      {
        if (!hb_set_has (active_features, layout_tag))
          continue; /* Skip dependencies from inactive features */
      }

      /* Check if this is a ligature dependency (has a ligature set) */
      bool is_ligature = (ligature_set != HB_CODEPOINT_INVALID);

      if (is_ligature)
      {
        /* Defer ligature dependencies for later processing */
        hb_set_add (deferred_ligatures, dependent);
      }
      else
      {
        /* Non-ligature dependency: add immediately */
        if (!hb_set_has (glyphs, dependent))
        {
          if (dependent == _hb_depend_fuzzer_debug_gid)
            fprintf (stderr, "[DEBUG] Adding glyph %u to closure (from %u)\n", dependent, gid);
          hb_set_add (glyphs, dependent);
          hb_set_add (to_process, dependent);
        }
      }
    }
  }

  /* Phase 2: Process deferred ligatures iteratively
   * Only add a ligature when ALL glyphs in its ligature set are in closure */
  hb_set_t *ligature_set_glyphs = hb_set_create ();
  hb_set_t *ligatures_to_remove = hb_set_create ();
  bool made_progress = true;

  while (made_progress && !hb_set_is_empty (deferred_ligatures))
  {
    made_progress = false;
    hb_set_clear (ligatures_to_remove);

    hb_codepoint_t lig_glyph = HB_SET_VALUE_INVALID;
    while (hb_set_next (deferred_ligatures, &lig_glyph))
    {
      /* A ligature may appear in multiple ligature sets (different component combinations).
       * We need to check ALL ligature sets and add the ligature if ANY set is satisfied.
       * Example: uni1FFC can be formed by Omega+uni1FBE OR Omega+uni0345 */
      bool found_any_ligature_set = false;
      bool any_set_satisfied = false;

      /* Scan through all glyphs to find ALL ligature sets that produce this ligature */
      hb_codepoint_t src_glyph = HB_SET_VALUE_INVALID;
      while (hb_set_next (glyphs, &src_glyph))
      {
        hb_codepoint_t idx = 0;
        hb_tag_t ttag;
        hb_codepoint_t dep;
        hb_tag_t ltag;
        hb_codepoint_t ligset;

        while (hb_depend_get_glyph_entry (depend, src_glyph, idx++,
                                          &ttag, &dep, &ltag, &ligset))
        {
          if (dep == lig_glyph && ligset != HB_CODEPOINT_INVALID)
          {
            found_any_ligature_set = true;

            /* Check if this particular ligature set is satisfied */
            hb_set_clear (ligature_set_glyphs);
            hb_depend_get_set_from_index (depend, ligset, ligature_set_glyphs);

            if (hb_set_is_subset (ligature_set_glyphs, glyphs))
            {
              /* This ligature set is satisfied - we can add the ligature */
              any_set_satisfied = true;
              break;
            }
          }
        }
        if (any_set_satisfied)
          break;
      }

      if (!found_any_ligature_set)
      {
        /* No ligature set found at all, remove from deferred */
        hb_set_add (ligatures_to_remove, lig_glyph);
        continue;
      }

      /* If any ligature set was satisfied, add the ligature to closure */
      if (any_set_satisfied)
      {
        /* All components present - add the ligature to closure */
        if (!hb_set_has (glyphs, lig_glyph))
        {
          hb_set_add (glyphs, lig_glyph);

          /* Process the ligature's non-ligature dependencies */
          hb_codepoint_t index = 0;
          hb_tag_t table_tag;
          hb_codepoint_t dependent;
          hb_tag_t layout_tag;
          hb_codepoint_t ligature_set_id;

          while (hb_depend_get_glyph_entry (depend, lig_glyph, index++,
                                            &table_tag, &dependent,
                                            &layout_tag, &ligature_set_id))
          {
            /* Only process non-ligature dependencies */
            if (ligature_set_id == HB_CODEPOINT_INVALID)
            {
              if (!hb_set_has (glyphs, dependent))
              {
                hb_set_add (glyphs, dependent);
                hb_set_add (to_process, dependent);
              }
            }
          }

          /* Process newly added non-ligature dependencies */
          while (!hb_set_is_empty (to_process))
          {
            hb_codepoint_t gid = hb_set_get_min (to_process);
            hb_set_del (to_process, gid);

            hb_codepoint_t idx = 0;
            hb_tag_t ttag;
            hb_codepoint_t dep;
            hb_tag_t ltag;
            hb_codepoint_t ligset;

            while (hb_depend_get_glyph_entry (depend, gid, idx++,
                                              &ttag, &dep, &ltag, &ligset))
            {
              /* Skip GSUB if requested */
              if (skip_gsub && ttag == HB_OT_TAG_GSUB)
                continue;

              /* Filter by active features */
              if (active_features && ttag == HB_OT_TAG_GSUB)
              {
                if (!hb_set_has (active_features, ltag))
                  continue;
              }

              /* Only follow non-ligature dependencies */
              if (ligset == HB_CODEPOINT_INVALID)
              {
                if (!hb_set_has (glyphs, dep))
                {
                  hb_set_add (glyphs, dep);
                  hb_set_add (to_process, dep);
                }
              }
            }
          }

          made_progress = true;
        }

        /* Mark this ligature for removal from deferred list */
        hb_set_add (ligatures_to_remove, lig_glyph);
      }
    }

    /* Remove processed ligatures from deferred set */
    hb_set_subtract (deferred_ligatures, ligatures_to_remove);
  }

  hb_set_destroy (ligatures_to_remove);
  hb_set_destroy (ligature_set_glyphs);
  hb_set_destroy (deferred_ligatures);
  hb_set_destroy (to_process);
}

/* Compute closure using HarfBuzz subset mechanism
 * skip_gsub: if true, skip GSUB closure
 * codepoints: if non-NULL, start from codepoints instead of glyphs
 * active_features: if non-NULL, will be populated with the active features used */
static void
compute_subset_closure (hb_face_t *face, hb_set_t *glyphs, bool skip_gsub,
                        hb_set_t *codepoints, hb_set_t *active_features)
{
  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  if (!input) return;

  /* Set flags */
  unsigned flags = HB_SUBSET_FLAGS_NO_BIDI_CLOSURE;
  if (skip_gsub)
    flags |= HB_SUBSET_FLAGS_NO_LAYOUT_CLOSURE;
  hb_subset_input_set_flags (input, (hb_subset_flags_t) flags);

  /* Populate input - either from codepoints or glyph IDs */
  if (codepoints && !hb_set_is_empty (codepoints))
  {
    hb_set_t *input_codepoints = hb_subset_input_set (input, HB_SUBSET_SETS_UNICODE);
    hb_set_union (input_codepoints, codepoints);
  }
  else
  {
    hb_set_t *input_glyphs = hb_subset_input_glyph_set (input);
    hb_set_union (input_glyphs, glyphs);
  }

  /* Handle features:
   * - If active_features is NULL: use defaults
   * - If active_features is non-NULL and empty: capture defaults into it
   * - If active_features is non-NULL and non-empty: clear defaults and use specified features
   */
  if (active_features && !skip_gsub)
  {
    hb_set_t *input_features = hb_subset_input_set (input, HB_SUBSET_SETS_LAYOUT_FEATURE_TAG);
    if (hb_set_is_empty (active_features))
    {
      /* Capture defaults */
      hb_set_union (active_features, input_features);
    }
    else
    {
      /* Clear defaults and use specified features */
      hb_set_clear (input_features);
      hb_set_union (input_features, active_features);
    }
  }

  /* Create a subset plan which will compute the closure */
  hb_subset_plan_t *plan = hb_subset_plan_create_or_fail (face, input);
  if (!plan)
  {
    hb_subset_input_destroy (input);
    return;
  }

  /* Get the final glyph mapping which includes closure */
  hb_map_t *glyph_map = hb_subset_plan_old_to_new_glyph_mapping (plan);

  /* Extract all glyphs that got mapped (i.e., retained after closure) */
  hb_set_clear (glyphs);
  int idx = -1;
  hb_codepoint_t key, value;
  while (hb_map_next (glyph_map, &idx, &key, &value))
    hb_set_add (glyphs, key);

  if (_hb_depend_fuzzer_debug_gid != HB_CODEPOINT_INVALID)
  {
    fprintf (stderr, "[DEBUG] Subset closure %s glyph %u\n",
             hb_set_has (glyphs, _hb_depend_fuzzer_debug_gid) ? "INCLUDES" : "does not include",
             _hb_depend_fuzzer_debug_gid);
  }

  hb_subset_plan_destroy (plan);
  hb_subset_input_destroy (input);
}

/* Print test parameters when running a specific test */
static void
print_test_params (uint64_t seed, bool is_gsub,
                    hb_set_t *start_glyphs, hb_set_t *codepoints, hb_set_t *features)
{
  fprintf (stderr, "Test: seed=0x%016llx\n", (unsigned long long) seed);
  fprintf (stderr, "Test type: %s\n", is_gsub ? "GSUB" : "non-GSUB");

  if (start_glyphs && !hb_set_is_empty (start_glyphs))
  {
    fprintf (stderr, "Starting glyphs (%u): ", hb_set_get_population (start_glyphs));
    hb_codepoint_t gid = HB_SET_VALUE_INVALID;
    unsigned count = 0;
    while (hb_set_next (start_glyphs, &gid) && count < 20)
    {
      fprintf (stderr, "%u ", gid);
      count++;
    }
    if (count == 20 && hb_set_get_population (start_glyphs) > 20)
      fprintf (stderr, "...");
    fprintf (stderr, "\n");
  }

  if (codepoints && !hb_set_is_empty (codepoints))
  {
    fprintf (stderr, "Starting codepoints (%u): ", hb_set_get_population (codepoints));
    hb_codepoint_t cp = HB_SET_VALUE_INVALID;
    unsigned count = 0;
    while (hb_set_next (codepoints, &cp) && count < 20)
    {
      fprintf (stderr, "U+%04X ", cp);
      count++;
    }
    if (count == 20 && hb_set_get_population (codepoints) > 20)
      fprintf (stderr, "...");
    fprintf (stderr, "\n");
  }

  if (features && !hb_set_is_empty (features))
  {
    fprintf (stderr, "Active features (%u): ", hb_set_get_population (features));
    hb_codepoint_t feature = HB_SET_VALUE_INVALID;
    unsigned count = 0;
    while (hb_set_next (features, &feature) && count < 10)
    {
      fprintf (stderr, "%c%c%c%c ", HB_UNTAG ((hb_tag_t) feature));
      count++;
    }
    if (count == 10 && hb_set_get_population (features) > 10)
      fprintf (stderr, "...");
    fprintf (stderr, "\n");
  }
}

/* Compare depend vs subset closure
 * Returns true if they match, false otherwise
 * If they don't match and verbose logging is enabled, logs detailed mismatch info
 * seed: test identifier (64-bit seed used to generate test)
 * is_gsub: if false, skip GSUB dependencies
 * start_glyphs: starting set of glyphs for closure
 * codepoints: if non-NULL, start from codepoints (for GSUB testing) */
static bool
compare_closures (hb_face_t *face, hb_depend_t *depend,
                  uint64_t seed, const char *font_path,
                  bool is_gsub, hb_set_t *start_glyphs, hb_set_t *codepoints,
                  hb_set_t *test_features)
{
  hb_set_t *subset_closure = hb_set_create ();
  hb_set_union (subset_closure, start_glyphs);

  /* Set up active features:
   * - If test_features provided, use those
   * - Otherwise, get defaults from subset */
  hb_set_t *active_features = hb_set_create ();
  if (test_features)
    hb_set_union (active_features, test_features);

  /* skip_gsub = !is_gsub (skip GSUB for non-GSUB tests) */
  bool skip_gsub = !is_gsub;

  /* First run subset to get closure and active features.
   * This will replace subset_closure with the actual closure. */
  compute_subset_closure (face, subset_closure, skip_gsub, codepoints, active_features);

  /* Extract starting glyphs from subset's closure for fair comparison.
   * For GSUB tests starting from codepoints, subset internally maps them to glyphs
   * which may differ from our fuzzer's mapping. We need to use subset's starting
   * glyphs to ensure both methods start from the same point. */
  hb_set_t *actual_start_glyphs = hb_set_create ();

  /* Find glyphs that are in subset_closure.
   * For GSUB tests, this includes glyphs mapped from input codepoints.
   * We'll temporarily compute without GSUB to get just the starting glyphs. */
  if (is_gsub && codepoints && !hb_set_is_empty (codepoints))
  {
    hb_set_t *temp_closure = hb_set_create ();
    compute_subset_closure (face, temp_closure, true /* skip GSUB */, codepoints, NULL);
    hb_set_union (actual_start_glyphs, temp_closure);
    hb_set_destroy (temp_closure);
  }
  else
  {
    hb_set_union (actual_start_glyphs, start_glyphs);
  }

  /* Now compute depend closure with the same starting glyphs subset used */
  hb_set_t *depend_closure = hb_set_create ();
  hb_set_union (depend_closure, actual_start_glyphs);

  /* Subset always adds glyph 0 (.notdef) to the input before closure.
   * Mirror this behavior in depend closure to ensure fair comparison.
   * See hb-subset-plan.cc:451 */
  hb_set_add (depend_closure, 0);

  /* Run depend with same active features as subset */
  compute_depend_closure (depend, depend_closure, skip_gsub,
                          skip_gsub ? NULL : active_features);

  hb_set_destroy (actual_start_glyphs);

  /* Compare the two sets */
  bool match = hb_set_is_equal (depend_closure, subset_closure);

  /* Always compute differences to check for critical errors */
  hb_set_t *in_depend_not_subset = hb_set_create ();
  hb_set_t *in_subset_not_depend = hb_set_create ();

  hb_set_set (in_depend_not_subset, depend_closure);
  hb_set_subtract (in_depend_not_subset, subset_closure);

  hb_set_set (in_subset_not_depend, subset_closure);
  hb_set_subtract (in_subset_not_depend, depend_closure);

  /* CRITICAL ERROR: Subset found glyphs that depend API missed.
   * This means the depend API is incomplete - it failed to extract
   * dependencies that actually exist in the font.
   * Abort to signal fuzzer that a bug was found. */
  if (!hb_set_is_empty (in_subset_not_depend))
  {
    fprintf (stderr, "\n=== DEPEND API BUG DETECTED ===\n");
    fprintf (stderr, "Font: %s\n", font_path ? font_path : "(unknown)");
    fprintf (stderr, "Test seed: 0x%016llx\n", (unsigned long long) seed);

    /* DEBUG: Check if dependencies exist in the graph for missing glyphs */
    fprintf (stderr, "\n[DEBUG] Checking if dependencies exist in graph for missing glyphs:\n");
    hb_codepoint_t missing_gid = HB_SET_VALUE_INVALID;
    unsigned missing_count = 0;
    while (hb_set_next (in_subset_not_depend, &missing_gid) && missing_count < 3)
    {
      fprintf (stderr, "[DEBUG] Glyph %u: Looking for incoming edges...\n", missing_gid);

      // Check if any glyph in the depend closure has this as a dependency
      bool found_edge = false;
      hb_codepoint_t src_gid = HB_SET_VALUE_INVALID;
      while (hb_set_next (depend_closure, &src_gid))
      {
        unsigned int index = 0;
        hb_tag_t table_tag;
        hb_codepoint_t dependent;
        hb_tag_t layout_tag;
        hb_codepoint_t ligature_set;

        while (hb_depend_get_glyph_entry (depend, src_gid, index++,
                                          &table_tag, &dependent,
                                          &layout_tag, &ligature_set))
        {
          if (dependent == missing_gid)
          {
            fprintf (stderr, "[DEBUG]   Edge found: %u -> %u (table=%c%c%c%c, ligset=%u)\n",
                     src_gid, missing_gid, HB_UNTAG(table_tag), ligature_set);
            found_edge = true;
            break;
          }
        }
        if (found_edge) break;
      }

      if (!found_edge)
      {
        fprintf (stderr, "[DEBUG]   NO EDGE FOUND from any glyph in depend_closure!\n");
        fprintf (stderr, "[DEBUG]   This is a GRAPH EXTRACTION BUG - dependency not recorded.\n");
      }
      else
      {
        fprintf (stderr, "[DEBUG]   Edge exists but wasn't followed - CLOSURE BUG!\n");
      }

      missing_count++;
    }
    fprintf (stderr, "\n");

    fprintf (stderr, "Starting glyphs (%u): ", hb_set_get_population (start_glyphs));
    hb_codepoint_t gid = HB_SET_VALUE_INVALID;
    unsigned gid_count = 0;
    while (hb_set_next (start_glyphs, &gid) && gid_count < 20)
    {
      fprintf (stderr, "%u ", gid);
      gid_count++;
    }
    if (gid_count == 20 && hb_set_get_population (start_glyphs) > 20)
      fprintf (stderr, "...");
    fprintf (stderr, "\n");

    if (codepoints && !hb_set_is_empty (codepoints))
    {
      fprintf (stderr, "Starting codepoints (%u): ", hb_set_get_population (codepoints));
      hb_codepoint_t cp = HB_SET_VALUE_INVALID;
      unsigned cp_count = 0;
      /* Print all codepoints to enable exact reproduction */
      while (hb_set_next (codepoints, &cp))
      {
        fprintf (stderr, "U+%04X ", cp);
        cp_count++;
        if (cp_count % 20 == 0)
          fprintf (stderr, "\n  ");
      }
      fprintf (stderr, "\n");
    }

    if (!skip_gsub && !hb_set_is_empty (active_features))
    {
      fprintf (stderr, "Active features (%u): ", hb_set_get_population (active_features));
      hb_codepoint_t feature = HB_SET_VALUE_INVALID;
      unsigned count = 0;
      while (hb_set_next (active_features, &feature) && count < 10)
      {
        fprintf (stderr, "%c%c%c%c ",
                 HB_UNTAG ((hb_tag_t) feature));
        count++;
      }
      if (count == 10 && !hb_set_is_empty (active_features))
        fprintf (stderr, "...");
      fprintf (stderr, "\n");
    }

    /* DEBUG: Print both closures */
    fprintf (stderr, "\nDEBUG - Subset closure glyphs: ");
    gid = HB_SET_VALUE_INVALID;
    while (hb_set_next (subset_closure, &gid))
      fprintf (stderr, "%u ", gid);
    fprintf (stderr, "\n");

    fprintf (stderr, "DEBUG - Depend closure glyphs: ");
    gid = HB_SET_VALUE_INVALID;
    while (hb_set_next (depend_closure, &gid))
      fprintf (stderr, "%u ", gid);
    fprintf (stderr, "\n\n");

    fprintf (stderr, "Depend closure: %u glyphs\n", hb_set_get_population (depend_closure));
    fprintf (stderr, "Subset closure: %u glyphs\n", hb_set_get_population (subset_closure));
    fprintf (stderr, "\nMissing glyphs in depend (%u):",
             hb_set_get_population (in_subset_not_depend));
    gid = HB_SET_VALUE_INVALID;
    unsigned count = 0;
    while (hb_set_next (in_subset_not_depend, &gid) && count < 50)
    {
      fprintf (stderr, " %u", gid);
      count++;
    }
    if (count == 50 && !hb_set_is_empty (in_subset_not_depend))
      fprintf (stderr, " ...");
    fprintf (stderr, "\n");
    fprintf (stderr, "===============================\n\n");

    hb_set_destroy (in_subset_not_depend);
    hb_set_destroy (in_depend_not_subset);
    hb_set_destroy (active_features);
    hb_set_destroy (subset_closure);
    hb_set_destroy (depend_closure);

    /* Abort to signal fuzzer about the bug */
    assert (0 && "Depend API missed glyphs that subset found");
  }

  /* Report cases where depend has extra glyphs (over-approximation)
   * if verbose mode or over-approximation reporting is enabled
   * (not a critical error, but potentially inefficient) */
  if (!match && (_hb_depend_fuzzer_verbose || _hb_depend_fuzzer_report_over_approximation)
      && !hb_set_is_empty (in_depend_not_subset))
  {
    /* Use compact single-line format for -r flag, multi-line for -v */
    if (_hb_depend_fuzzer_report_over_approximation && !_hb_depend_fuzzer_verbose)
    {
      /* Compact format: Test 0xHEX: Depend found N extra glyphs: gid1 gid2 ... */
      fprintf (stderr, "Test 0x%016llx: Depend found %u extra glyphs not in subset closure:",
               (unsigned long long) seed, hb_set_get_population (in_depend_not_subset));

      hb_codepoint_t gid = HB_SET_VALUE_INVALID;
      while (hb_set_next (in_depend_not_subset, &gid))
        fprintf (stderr, " %u", gid);
      fprintf (stderr, "\n");
    }
    else
    {
      /* Verbose multi-line format */
      fprintf (stderr, "\n=== DEPEND API OVER-APPROXIMATION ===\n");
      fprintf (stderr, "Font: %s\n", font_path ? font_path : "(unknown)");
      fprintf (stderr, "Test seed: 0x%016llx\n", (unsigned long long) seed);
      fprintf (stderr, "Depend found %u extra glyphs not in subset closure:\n",
               hb_set_get_population (in_depend_not_subset));

      hb_codepoint_t gid = HB_SET_VALUE_INVALID;
      unsigned count = 0;
      while (hb_set_next (in_depend_not_subset, &gid) && count < 50)
      {
        fprintf (stderr, " %u", gid);
        count++;
      }
      if (count == 50)
        fprintf (stderr, " ...");
      fprintf (stderr, "\n");
      fprintf (stderr, "Note: This is not a bug, but may indicate over-conservative dependency extraction.\n");
      fprintf (stderr, "======================================\n\n");
    }
  }

  hb_set_destroy (in_subset_not_depend);
  hb_set_destroy (in_depend_not_subset);

  hb_set_destroy (active_features);
  hb_set_destroy (subset_closure);
  hb_set_destroy (depend_closure);

  return match;
}

#endif /* HB_DEPEND_API */

extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
#ifdef HB_DEPEND_API
  alloc_state = _fuzzing_alloc_state (data, size);

  /* Initialize configuration on first run */
  init_fuzzer_config ();

  hb_blob_t *blob = hb_blob_create ((const char *) data, size,
                                    HB_MEMORY_MODE_READONLY, nullptr, nullptr);
  hb_face_t *face = hb_face_create (blob, 0);

  /* Extract dependency graph */
  hb_depend_t *depend = hb_depend_from_face (face);
  if (!depend)
  {
    hb_face_destroy (face);
    hb_blob_destroy (blob);
    return 0;
  }

  /* Create font for codepoint-to-glyph mapping */
  hb_font_t *font = hb_font_create (face);

  /* Check if running single test or full test suite */
  if (_hb_depend_fuzzer_single_test != 0)
  {
    /* Run single specific test */
    uint64_t test_seed = _hb_depend_fuzzer_single_test;

    /* Generate test parameters from seed */
    test_params params = generate_test_from_seed (test_seed, face, font);

    /* Skip if font is too small for this test type */
    if (params.target_size == 0 || params.glyphs == NULL)
    {
      fprintf (stderr, "Warning: Font too small for this test type\n");
    }
    else
    {
      /* Always print parameters for single test */
      print_test_params (test_seed, params.is_gsub, params.glyphs, params.codepoints, params.features);

      /* Run closure comparison */
      compare_closures (face, depend, test_seed, _hb_depend_fuzzer_current_font_path,
                        params.is_gsub, params.glyphs, params.codepoints, params.features);
    }

    /* Clean up test parameters */
    if (params.glyphs)
      hb_set_destroy (params.glyphs);
    if (params.codepoints)
      hb_set_destroy (params.codepoints);
    if (params.features)
      hb_set_destroy (params.features);
  }
  else
  {
    /* Generate T pseudo-random 64-bit test seeds from the fuzzer seed
     * Each seed deterministically generates a test with specific parameters
     * Distribution follows research-backed proportions (see generate_test_from_seed) */

    unsigned T = _hb_depend_fuzzer_num_tests;

    /* Initialize RNG with fuzzer seed to generate test seeds */
    srand (_hb_depend_fuzzer_seed);

    for (unsigned i = 0; i < T; i++)
    {
      /* Generate pseudo-random 64-bit test seed */
      uint64_t test_seed = ((uint64_t)rand() << 32) | (uint64_t)rand();

      /* Generate test parameters from seed */
      test_params params = generate_test_from_seed (test_seed, face, font);

      /* Skip if font is too small for this test type */
      if (params.target_size == 0 || params.glyphs == NULL)
        continue;

      /* Print parameters if verbose */
      if (_hb_depend_fuzzer_verbose)
        print_test_params (test_seed, params.is_gsub, params.glyphs, params.codepoints, params.features);

      /* Run closure comparison */
      compare_closures (face, depend, test_seed, _hb_depend_fuzzer_current_font_path,
                        params.is_gsub, params.glyphs, params.codepoints, params.features);

      /* Clean up test parameters */
      if (params.glyphs)
        hb_set_destroy (params.glyphs);
      if (params.codepoints)
        hb_set_destroy (params.codepoints);
      if (params.features)
        hb_set_destroy (params.features);
    }
  }

  hb_font_destroy (font);

  hb_depend_destroy (depend);
  hb_face_destroy (face);
  hb_blob_destroy (blob);
#endif /* HB_DEPEND_API */

  return 0;
}
