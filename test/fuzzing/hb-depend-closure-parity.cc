#include "hb-fuzzer.hh"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "hb-subset.h"

#include <vector>

#ifdef HB_DEPEND_API
#include <hb.h>

/**
 * deferred_ligature_t:
 *
 * Stores information needed to process a deferred ligature dependency.
 * When we encounter a ligature edge, we defer processing until we know
 * whether all glyphs in the ligature set are present in the closure.
 * By storing this info at deferral time, we avoid re-scanning edges later.
 */
struct deferred_ligature_t
{
  deferred_ligature_t () = default;
  deferred_ligature_t (hb_codepoint_t dependent_,
		       hb_codepoint_t ligature_set_index_,
		       uint8_t flags_)
    : dependent (dependent_),
      ligature_set_index (ligature_set_index_),
      flags (flags_) {}

  hb_codepoint_t dependent;
  hb_codepoint_t ligature_set_index;
  uint8_t flags;
};

/* Debug messaging - follows HarfBuzz DEBUG_MSG pattern but simplified for parity checker */
#ifndef HB_DEBUG_DEPEND
#define HB_DEBUG_DEPEND 1
#endif

#if HB_DEBUG_DEPEND
#define DEBUG_MSG_DEPEND(msg, ...) fprintf (stderr, "%-10s" msg "\n", "DEPEND", ##__VA_ARGS__)
#else
#define DEBUG_MSG_DEPEND(msg, ...) do {} while (0)
#endif

/* Globals for configuration (set by main() in standalone mode) */
const char *_hb_depend_fuzzer_current_font_path = NULL;
unsigned _hb_depend_fuzzer_seed = 0;
uint64_t _hb_depend_fuzzer_single_test = 0;  /* Specific test to run, or 0 for all */
unsigned _hb_depend_fuzzer_num_tests = 1024; /* Number of tests to run */
bool _hb_depend_fuzzer_verbose = false;
bool _hb_depend_fuzzer_report_over_approximation = false;
bool _hb_depend_fuzzer_report_under_approximation = false;
float _hb_depend_fuzzer_inject_over_approx = 0.0f;
float _hb_depend_fuzzer_inject_under_approx = 0.0f;

/* Explicit test mode */
bool _hb_depend_fuzzer_explicit_test = false;
const char *_hb_depend_fuzzer_explicit_unicodes = NULL;
const char *_hb_depend_fuzzer_explicit_gids = NULL;
bool _hb_depend_fuzzer_explicit_no_layout_closure = false;
const char *_hb_depend_fuzzer_explicit_layout_features = NULL;

/* Debug control (only used when HB_DEBUG_DEPEND > 0) */
static hb_codepoint_t debug_gid = HB_CODEPOINT_INVALID;  /* Specific glyph to debug */
static bool trace_closure = false;  /* Trace closure computation in detail */

static bool config_initialized = false;

/* Initialize configuration - uses globals set by main */
static void
init_parity_config ()
{
  if (config_initialized)
    return;

  config_initialized = true;

  /* Initialize RNG with seed (already set by main or defaulted to random) */
  srand (_hb_depend_fuzzer_seed);

  /* Check for debug glyph ID (only used when HB_DEBUG_DEPEND > 0) */
  const char *debug_gid_str = getenv ("HB_DEPEND_PARITY_DEBUG_GID");
  if (debug_gid_str)
    debug_gid = (hb_codepoint_t) strtoul (debug_gid_str, NULL, 0);

  /* Check for closure tracing (only used when HB_DEBUG_DEPEND > 0) */
  const char *trace_closure_str = getenv ("HB_DEPEND_PARITY_TRACE_CLOSURE");
  if (trace_closure_str && strcmp (trace_closure_str, "1") == 0)
    trace_closure = true;
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

/* Forward declaration */
static hb_set_t * get_gsub_features (hb_face_t *face);

/* Parse comma-separated list of unicodes (U+XXXX, decimal, or ranges) into a set */
static bool
parse_unicodes (const char *str, hb_set_t *unicodes)
{
  if (!str || !*str)
    return false;

  const char *p = str;
  while (*p)
  {
    /* Skip whitespace and commas */
    while (*p == ' ' || *p == ',')
      p++;
    if (!*p)
      break;

    /* Parse unicode value (U+XXXX or decimal) */
    const char *start = p;
    hb_codepoint_t start_cp;
    if (*p == 'U' && *(p + 1) == '+')
    {
      /* U+XXXX format */
      start_cp = (hb_codepoint_t) strtoul (p + 2, (char **) &p, 16);
    }
    else
    {
      /* Decimal format */
      start_cp = (hb_codepoint_t) strtoul (p, (char **) &p, 10);
    }

    /* Check if parsing advanced (avoid infinite loop on invalid input) */
    if (p == start || (p == start + 2 && start[0] == 'U' && start[1] == '+'))
    {
      fprintf (stderr, "Error: Invalid unicode value at: %.20s\n", start);
      return false;
    }

    /* Check for range */
    if (*p == '-')
    {
      p++;
      const char *end_start = p;
      hb_codepoint_t end_cp;
      if (*p == 'U' && *(p + 1) == '+')
        end_cp = (hb_codepoint_t) strtoul (p + 2, (char **) &p, 16);
      else
        end_cp = (hb_codepoint_t) strtoul (p, (char **) &p, 10);

      /* Check if parsing advanced */
      if (p == end_start || (p == end_start + 2 && end_start[0] == 'U' && end_start[1] == '+'))
      {
        fprintf (stderr, "Error: Invalid unicode value in range at: %.20s\n", end_start);
        return false;
      }

      hb_set_add_range (unicodes, start_cp, end_cp);
    }
    else
    {
      hb_set_add (unicodes, start_cp);
    }
  }

  return !hb_set_is_empty (unicodes);
}

/* Parse comma-separated list of glyph IDs (decimal or ranges) into a set */
static bool
parse_gids (const char *str, hb_set_t *gids)
{
  if (!str || !*str)
    return false;

  const char *p = str;
  while (*p)
  {
    /* Skip whitespace and commas */
    while (*p == ' ' || *p == ',')
      p++;
    if (!*p)
      break;

    /* Parse glyph ID */
    const char *start = p;
    hb_codepoint_t start_gid = (hb_codepoint_t) strtoul (p, (char **) &p, 10);

    /* Check if parsing advanced (avoid infinite loop on invalid input) */
    if (p == start)
    {
      fprintf (stderr, "Error: Invalid glyph ID at: %.20s\n", start);
      return false;
    }

    /* Check for range */
    if (*p == '-')
    {
      p++;
      const char *end_start = p;
      hb_codepoint_t end_gid = (hb_codepoint_t) strtoul (p, (char **) &p, 10);

      /* Check if parsing advanced */
      if (p == end_start)
      {
        fprintf (stderr, "Error: Invalid glyph ID in range at: %.20s\n", end_start);
        return false;
      }

      hb_set_add_range (gids, start_gid, end_gid);
    }
    else
    {
      hb_set_add (gids, start_gid);
    }
  }

  return !hb_set_is_empty (gids);
}

/* Parse comma-separated list of feature tags into a set
 * Special value "*" means all features */
static bool
parse_features (const char *str, hb_set_t *features, hb_face_t *face)
{
  if (!str || !*str)
    return false;

  /* Check for special "*" value (all features) */
  if (strcmp (str, "*") == 0)
  {
    /* Get all GSUB features */
    hb_set_t *all_features = get_gsub_features (face);
    hb_set_union (features, all_features);
    hb_set_destroy (all_features);
    return true;
  }

  const char *p = str;
  while (*p)
  {
    /* Skip whitespace and commas */
    while (*p == ' ' || *p == ',')
      p++;
    if (!*p)
      break;

    /* Parse 4-character tag */
    char tag_str[5] = {0};
    int i = 0;
    while (*p && *p != ',' && *p != ' ' && i < 4)
      tag_str[i++] = *p++;

    if (i > 0)
    {
      /* Pad with spaces if needed */
      while (i < 4)
        tag_str[i++] = ' ';
      tag_str[4] = '\0';

      hb_tag_t tag = HB_TAG (tag_str[0], tag_str[1], tag_str[2], tag_str[3]);
      hb_set_add (features, tag);
    }
  }

  return !hb_set_is_empty (features);
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

/* Generate test parameters from explicit command-line arguments */
static test_params
generate_test_from_explicit (hb_face_t *face, hb_font_t *font)
{
  test_params params;
  params.glyphs = hb_set_create ();
  params.codepoints = NULL;
  params.features = NULL;
  params.target_size = 0;

  /* Parse unicodes or gids */
  if (_hb_depend_fuzzer_explicit_unicodes)
  {
    /* GSUB test mode with unicodes */
    params.is_gsub = true;
    params.codepoints = hb_set_create ();

    if (!parse_unicodes (_hb_depend_fuzzer_explicit_unicodes, params.codepoints))
    {
      fprintf (stderr, "Error: Failed to parse --unicodes argument\n");
      hb_set_destroy (params.glyphs);
      hb_set_destroy (params.codepoints);
      params.glyphs = NULL;
      params.codepoints = NULL;
      return params;
    }

    /* Map codepoints to glyphs */
    hb_codepoint_t cp = HB_SET_VALUE_INVALID;
    while (hb_set_next (params.codepoints, &cp))
    {
      hb_codepoint_t gid;
      if (hb_font_get_nominal_glyph (font, cp, &gid))
        hb_set_add (params.glyphs, gid);
    }

    params.target_size = hb_set_get_population (params.codepoints);
  }
  else if (_hb_depend_fuzzer_explicit_gids)
  {
    /* Non-GSUB test mode with glyph IDs */
    params.is_gsub = !_hb_depend_fuzzer_explicit_no_layout_closure;

    if (!parse_gids (_hb_depend_fuzzer_explicit_gids, params.glyphs))
    {
      fprintf (stderr, "Error: Failed to parse --gids argument\n");
      hb_set_destroy (params.glyphs);
      params.glyphs = NULL;
      return params;
    }

    params.target_size = hb_set_get_population (params.glyphs);
  }
  else
  {
    fprintf (stderr, "Error: Explicit mode requires --unicodes or --gids\n");
    hb_set_destroy (params.glyphs);
    params.glyphs = NULL;
    return params;
  }

  /* Parse layout features if specified */
  if (_hb_depend_fuzzer_explicit_layout_features)
  {
    params.features = hb_set_create ();
    if (!parse_features (_hb_depend_fuzzer_explicit_layout_features, params.features, face))
    {
      fprintf (stderr, "Error: Failed to parse --layout-features argument\n");
      hb_set_destroy (params.features);
      params.features = NULL;
    }
  }

  return params;
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
 * Depend Parity Checker - Validates depend API correctness by comparing closures
 *
 * For each font, this parity checker:
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

/* Check if context requirements are satisfied
 *
 * Context sets encode which backtrack and lookahead glyphs must be present
 * for a dependency edge to apply. Elements in the context set can be:
 * - Direct GID reference (value < 0x80000000): that specific glyph must be in closure
 * - Indirect set reference (value >= 0x80000000): at least one glyph from that set must be in closure
 *
 * Returns true if ALL context requirements are met, false otherwise.
 */
static bool
check_context_satisfied (hb_depend_t *depend,
                         hb_codepoint_t context_set_idx,
                         hb_set_t *current_closure)
{
  if (context_set_idx == HB_CODEPOINT_INVALID)
    return true;  /* No context requirements */

  hb_set_t *context_elements = hb_set_create ();
  if (!hb_depend_get_set_from_index (depend, context_set_idx, context_elements))
  {
    hb_set_destroy (context_elements);
    return true;  /* Invalid context set, don't filter */
  }

  bool satisfied = true;
  hb_codepoint_t elem = HB_SET_VALUE_INVALID;
  while (hb_set_next (context_elements, &elem))
  {
    if (elem < 0x80000000)
    {
      /* Direct GID reference - this specific glyph must be present */
      if (!hb_set_has (current_closure, elem))
      {
        satisfied = false;
        break;
      }
    }
    else
    {
      /* Indirect set reference - at least one glyph from this set must be present */
      hb_codepoint_t set_idx = elem & 0x7FFFFFFF;
      hb_set_t *required_glyphs = hb_set_create ();

      if (!hb_depend_get_set_from_index (depend, set_idx, required_glyphs))
      {
        hb_set_destroy (required_glyphs);
        continue;  /* Invalid set reference, skip this requirement */
      }

      /* Check if ANY glyph from required_glyphs is in current_closure */
      bool any_present = false;
      hb_codepoint_t gid = HB_SET_VALUE_INVALID;
      while (hb_set_next (required_glyphs, &gid))
      {
        if (hb_set_has (current_closure, gid))
        {
          any_present = true;
          break;
        }
      }

      hb_set_destroy (required_glyphs);

      if (!any_present)
      {
        satisfied = false;
        break;
      }
    }
  }

  hb_set_destroy (context_elements);
  return satisfied;
}

/* Helper: Process edges for specific table(s)
 * table_filter: if non-NULL, only process edges from these tables
 * hit_flagged_edge: if non-NULL, set to true if any edge with FROM_CONTEXT_POSITION flag is traversed
 * Returns: true if any new glyphs were added */
static bool
process_edges_for_tables (hb_depend_t *depend,
                          hb_set_t *glyphs,
                          hb_set_t *to_process,
                          std::vector<deferred_ligature_t> *deferred_ligatures,
                          hb_set_t *table_filter,
                          bool skip_gsub,
                          hb_set_t *active_features,
                          bool *hit_flagged_edge)
{
  bool added_any = false;

  while (!hb_set_is_empty (to_process))
  {
    hb_codepoint_t gid = hb_set_get_min (to_process);
    hb_set_del (to_process, gid);

    if (gid == debug_gid)
      DEBUG_MSG_DEPEND("Processing glyph %u", gid);

    hb_codepoint_t index = 0;
    hb_tag_t table_tag;
    hb_codepoint_t dependent;
    hb_tag_t layout_tag;
    hb_codepoint_t ligature_set;
    hb_codepoint_t context_set;
    uint8_t flags;

    while (hb_depend_get_glyph_entry (depend, gid, index++,
                                      &table_tag, &dependent,
                                      &layout_tag, &ligature_set, &context_set, &flags))
    {
      /* Debug: trace all edges involving our debug glyph */
      if (debug_gid != HB_CODEPOINT_INVALID &&
          (gid == debug_gid || dependent == debug_gid))
        DEBUG_MSG_DEPEND("Considering edge: %u -> %u (feature=%c%c%c%c, ctx=%u)",
                   gid, dependent, HB_UNTAG(layout_tag), context_set);

      /* Filter by table if requested */
      if (table_filter && !hb_set_has (table_filter, table_tag))
        continue;

      if (dependent == debug_gid || gid == debug_gid)
        DEBUG_MSG_DEPEND("Edge: %u -> %u (table=%c%c%c%c, ligset=%u)",
                   gid, dependent, HB_UNTAG(table_tag), ligature_set);

      /* Skip GSUB dependencies if requested (to match subset behavior) */
      if (skip_gsub && table_tag == HB_OT_TAG_GSUB)
        continue;

      /* If active_features is specified, filter GSUB deps by feature */
      if (active_features && table_tag == HB_OT_TAG_GSUB)
      {
        if (!hb_set_has (active_features, layout_tag))
        {
          /* Over-approximation injection: sometimes follow inactive feature edges */
          bool skip_feature_check = false;
          if (_hb_depend_fuzzer_inject_over_approx > 0.0f) {
            float r = (float)rand() / (float)RAND_MAX;
            if (r < _hb_depend_fuzzer_inject_over_approx) {
              skip_feature_check = true;
            }
          }

          if (!skip_feature_check)
          {
            if (dependent == debug_gid)
              DEBUG_MSG_DEPEND("Skipping edge %u -> %u: feature %c%c%c%c not active",
                         gid, dependent, HB_UNTAG(layout_tag));
            continue; /* Skip dependencies from inactive features */
          }
        }
      }

      /* Check context requirements - skip edge if context not satisfied */
      if (context_set != HB_CODEPOINT_INVALID)
      {
        if (!check_context_satisfied (depend, context_set, glyphs))
        {
          /* Over-approximation injection: sometimes follow edges with unsatisfied context */
          bool skip_context_check = false;
          if (_hb_depend_fuzzer_inject_over_approx > 0.0f) {
            float r = (float)rand() / (float)RAND_MAX;
            if (r < _hb_depend_fuzzer_inject_over_approx) {
              skip_context_check = true;
            }
          }

          if (!skip_context_check)
          {
            if (dependent == debug_gid)
              DEBUG_MSG_DEPEND("Skipping edge %u -> %u: context not satisfied (ctx_set=%u)",
                         gid, dependent, context_set);
            continue;  /* Context requirements not met, skip this edge */
          }
        }
      }

      /* Check if this is a ligature dependency (has a ligature set) */
      bool is_ligature = (ligature_set != HB_CODEPOINT_INVALID);

      if (is_ligature)
      {
        /* Defer ligature dependencies for later processing.
         * Store all info needed to process later: dependent glyph, ligature set index, and flags.
         * Feature filtering has already been done above, so no need to re-check later. */
        deferred_ligatures->push_back (deferred_ligature_t (dependent, ligature_set, flags));
      }
      else
      {
        /* Under-approximation injection: randomly skip following this edge */
        if (_hb_depend_fuzzer_inject_under_approx > 0.0f) {
          float r = (float)rand() / (float)RAND_MAX;
          if (r < _hb_depend_fuzzer_inject_under_approx) {
            /* Skip this edge to inject under-approximation */
            continue;
          }
        }

        /* Non-ligature dependency: add immediately */
        if (!hb_set_has (glyphs, dependent))
        {
          /* Track if we hit a flagged edge (from contextual position or nested context)
           * Flag values: 0x01 = FROM_CONTEXT_POSITION, 0x02 = FROM_NESTED_CONTEXT */
          if (hit_flagged_edge && (flags & 0x03))
            *hit_flagged_edge = true;

          if (dependent == debug_gid)
            DEBUG_MSG_DEPEND("Adding glyph %u to closure (from %u, feature=%c%c%c%c, ctx=%u)",
                       dependent, gid, HB_UNTAG(layout_tag), context_set);
          if (trace_closure)
            DEBUG_MSG_DEPEND("Depend: %u -> %u (table=%c%c%c%c, feature=%c%c%c%c)",
                       gid, dependent, HB_UNTAG(table_tag), HB_UNTAG(layout_tag));
          hb_set_add (glyphs, dependent);
          hb_set_add (to_process, dependent);
          added_any = true;
        }
      }
    }
  }

  return added_any;
}

/* Compute closure using depend API by following the dependency graph
 * skip_gsub: if true, skip GSUB dependencies to match subset behavior
 * (subset doesn't follow GSUB when starting with glyph IDs)
 * active_features: if non-NULL, only follow GSUB dependencies from these features
 * hit_flagged_edge: if non-NULL, set to true if any edge with FROM_CONTEXT_POSITION flag is traversed */
static void
compute_depend_closure (hb_depend_t *depend, hb_set_t *glyphs, bool skip_gsub,
                        hb_set_t *active_features, bool *hit_flagged_edge)
{
  if (debug_gid != HB_CODEPOINT_INVALID)
  {
    DEBUG_MSG_DEPEND("Starting closure, glyph %u %s in initial set",
               debug_gid,
               hb_set_has (glyphs, debug_gid) ? "IS" : "is NOT");
  }

  if (trace_closure)
  {
    hb_codepoint_t gid = HB_SET_VALUE_INVALID;
    fprintf (stderr, "DEPEND: Initial set:");
    while (hb_set_next (glyphs, &gid))
      fprintf (stderr, " %u", gid);
    fprintf (stderr, "\n");
  }

  hb_set_t *to_process = hb_set_create ();
  std::vector<deferred_ligature_t> deferred_ligatures;

  /* Process edges in stages matching subset's table order:
   * 1. GSUB (iterate internally until stable - handles context dependencies)
   * 2. MATH (on glyphs from GSUB)
   * 3. COLR (on glyphs from GSUB+MATH)
   * 4. glyf/CFF (on glyphs from GSUB+MATH+COLR)
   *
   * No loop back to GSUB after glyf/CFF - glyf components are rendering
   * implementation details, not shaping inputs. Subset doesn't loop back either. */

  /* Helper set for ligature processing (used in GSUB stage) */
  hb_set_t *ligature_set_glyphs = hb_set_create ();

  {
    /* Stage 1: GSUB closure - iterate until stable (like subset does internally)
     * Ligatures are processed HERE as part of GSUB, not after glyf/CFF.
     * This matches subset behavior where GSUB closure completes before glyf. */
    if (!skip_gsub)
  {
    hb_set_t *gsub_filter = hb_set_create ();
    hb_set_add (gsub_filter, HB_OT_TAG_GSUB);

    unsigned prev_gsub_count;
    unsigned iteration = 0;
    do {
      prev_gsub_count = hb_set_get_population (glyphs);

      /* Process non-ligature GSUB edges */
      hb_set_union (to_process, glyphs);
      process_edges_for_tables (depend, glyphs, to_process, &deferred_ligatures,
                                gsub_filter, skip_gsub, active_features, hit_flagged_edge);

      /* Process deferred ligatures (GSUB ligatures only)
       * Only add a ligature when ALL glyphs in its ligature set are in closure.
       * Since we stored {dependent, ligature_set_index, flags} at deferral time,
       * we just need to check if each ligature set is satisfied - no re-scanning needed. */
      bool lig_made_progress = true;
      while (lig_made_progress && !deferred_ligatures.empty ())
      {
        lig_made_progress = false;

        for (size_t i = 0; i < deferred_ligatures.size (); )
        {
          const deferred_ligature_t &def = deferred_ligatures[i];

          /* Check if ligature set is satisfied */
          hb_set_clear (ligature_set_glyphs);
          hb_depend_get_set_from_index (depend, def.ligature_set_index, ligature_set_glyphs);

          bool set_satisfied = hb_set_is_subset (ligature_set_glyphs, glyphs);

          /* Over-approximation injection: sometimes add ligature even when components NOT satisfied */
          if (!set_satisfied && _hb_depend_fuzzer_inject_over_approx > 0.0f) {
            float r = (float)rand() / (float)RAND_MAX;
            if (r < _hb_depend_fuzzer_inject_over_approx) {
              set_satisfied = true;  /* Pretend it's satisfied */
            }
          }

          if (set_satisfied)
          {
            /* Under-approximation injection: randomly skip adding ligature */
            bool skip_ligature = false;
            if (_hb_depend_fuzzer_inject_under_approx > 0.0f) {
              float r = (float)rand() / (float)RAND_MAX;
              if (r < _hb_depend_fuzzer_inject_under_approx) {
                skip_ligature = true;
              }
            }

            if (!skip_ligature && !hb_set_has (glyphs, def.dependent))
            {
              /* Check if this ligature edge has flags (from contextual position or nested context) */
              if (hit_flagged_edge && (def.flags & 0x03))
                *hit_flagged_edge = true;

              if (trace_closure)
                DEBUG_MSG_DEPEND("Adding GSUB ligature %u (components satisfied, flags=0x%02x)",
                           def.dependent, def.flags);
              hb_set_add (glyphs, def.dependent);
              lig_made_progress = true;
            }

            /* Remove from deferred list by swapping with last element */
            deferred_ligatures[i] = deferred_ligatures.back ();
            deferred_ligatures.pop_back ();
            /* Don't increment i - need to check the swapped element */
          }
          else
          {
            i++;
          }
        }
      }

      iteration++;
    } while (hb_set_get_population (glyphs) > prev_gsub_count && iteration < 64);

    hb_set_destroy (gsub_filter);

    if (trace_closure)
      DEBUG_MSG_DEPEND("After GSUB (%u iterations): closure has %u glyphs",
                 iteration, hb_set_get_population (glyphs));
  }

  /* Stage 2: MATH closure */
  {
    hb_set_t *math_filter = hb_set_create ();
    hb_set_add (math_filter, HB_OT_TAG_MATH);

    hb_set_union (to_process, glyphs);
    process_edges_for_tables (depend, glyphs, to_process, &deferred_ligatures,
                              math_filter, skip_gsub, active_features, hit_flagged_edge);
    hb_set_destroy (math_filter);

    if (trace_closure)
      DEBUG_MSG_DEPEND("After MATH: closure has %u glyphs",
                 hb_set_get_population (glyphs));
  }

  /* Stage 3: COLR closure */
  {
    hb_set_t *colr_filter = hb_set_create ();
    hb_set_add (colr_filter, HB_TAG('C','O','L','R'));

    hb_set_union (to_process, glyphs);
    process_edges_for_tables (depend, glyphs, to_process, &deferred_ligatures,
                              colr_filter, skip_gsub, active_features, hit_flagged_edge);
    hb_set_destroy (colr_filter);

    if (trace_closure)
      DEBUG_MSG_DEPEND("After COLR: closure has %u glyphs",
                 hb_set_get_population (glyphs));
  }

  /* Stage 4: glyf/CFF closure
   * Note: cmap edges (UVS) are NOT included here because:
   * 1. Subset doesn't follow UVS for glyph-based closure
   * 2. UVS is about Unicode→glyph mapping, not glyph→glyph rendering deps */
  {
    hb_set_t *glyf_filter = hb_set_create ();
    hb_set_add (glyf_filter, HB_TAG('g','l','y','f'));
    hb_set_add (glyf_filter, HB_TAG('C','F','F',' '));

    hb_set_union (to_process, glyphs);
    process_edges_for_tables (depend, glyphs, to_process, &deferred_ligatures,
                              glyf_filter, skip_gsub, active_features, NULL);
    hb_set_destroy (glyf_filter);

    if (trace_closure)
      DEBUG_MSG_DEPEND("After glyf/CFF: closure has %u glyphs",
                 hb_set_get_population (glyphs));
  }
  }  /* End table stages */

  /* Cleanup */
  hb_set_destroy (ligature_set_glyphs);
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

  if (debug_gid != HB_CODEPOINT_INVALID)
  {
    DEBUG_MSG_DEPEND("Subset closure %s glyph %u",
               hb_set_has (glyphs, debug_gid) ? "INCLUDES" : "does not include",
               debug_gid);
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

  if (trace_closure)
  {
    DEBUG_MSG_DEPEND("Subset: Final closure has %u glyphs, checking for 82 and 124...",
               hb_set_get_population (subset_closure));
    DEBUG_MSG_DEPEND("Subset: glyph 82 ('o') %s in closure",
               hb_set_has (subset_closure, 82) ? "IS" : "is NOT");
    DEBUG_MSG_DEPEND("Subset: glyph 124 (ordmasculine) %s in closure",
               hb_set_has (subset_closure, 124) ? "IS" : "is NOT");
  }

  /* Extract starting glyphs from subset's closure for fair comparison.
   * For GSUB tests starting from codepoints, we need to map codepoints to glyphs
   * using ONLY cmap (not glyf/COLR/MATH), matching subset's order. */
  hb_set_t *actual_start_glyphs = hb_set_create ();

  /* For GSUB tests, map codepoints to glyphs via cmap only */
  if (is_gsub && codepoints && !hb_set_is_empty (codepoints))
  {
    /* Add .notdef */
    hb_set_add (actual_start_glyphs, 0);

    /* Map each codepoint to its glyph via cmap */
    hb_font_t *font = hb_font_create (face);
    hb_codepoint_t cp = HB_SET_VALUE_INVALID;
    while (hb_set_next (codepoints, &cp))
    {
      hb_codepoint_t gid;
      if (hb_font_get_nominal_glyph (font, cp, &gid))
        hb_set_add (actual_start_glyphs, gid);
    }
    hb_font_destroy (font);
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
  if (debug_gid != HB_CODEPOINT_INVALID && active_features)
  {
    hb_codepoint_t feat = HB_SET_VALUE_INVALID;
    fprintf (stderr, "DEPEND: Active features (%u):", hb_set_get_population (active_features));
    while (hb_set_next (active_features, &feat))
      fprintf (stderr, " %c%c%c%c", HB_UNTAG(feat));
    fprintf (stderr, "\n");
  }

  /* Seed RNG for error injection based on test seed to ensure reproducibility.
   * For a given test seed and injection probability, the same edges will be
   * skipped/added every time. */
  if (_hb_depend_fuzzer_inject_over_approx > 0.0f ||
      _hb_depend_fuzzer_inject_under_approx > 0.0f)
  {
    srand ((unsigned) seed);
  }

  bool hit_flagged_edge = false;
  compute_depend_closure (depend, depend_closure, skip_gsub,
                          skip_gsub ? NULL : active_features, &hit_flagged_edge);

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
   * dependencies that actually exist in the font. */
  if (!hb_set_is_empty (in_subset_not_depend))
  {
    /* Report if verbose or reporting flag is set */
    if (_hb_depend_fuzzer_report_under_approximation ||
        _hb_depend_fuzzer_report_over_approximation ||
        _hb_depend_fuzzer_verbose)
    {
      fprintf (stderr, "Test 0x%016llx: UNDER-APPROX - Depend missed %u glyphs:",
               (unsigned long long) seed, hb_set_get_population (in_subset_not_depend));

      hb_codepoint_t gid = HB_SET_VALUE_INVALID;
      unsigned count = 0;
      while (hb_set_next (in_subset_not_depend, &gid) && count < 20)
      {
        fprintf (stderr, " %u", gid);
        count++;
      }
      if (count == 20)
        fprintf (stderr, " ...");
      fprintf (stderr, "\n");
    }

    /* Abort unless reporting flag is set */
    if (!_hb_depend_fuzzer_report_under_approximation)
      assert (0 && "Depend API missed glyphs that subset found");
  }

  /* Handle over-approximation cases where depend has extra glyphs.
   * Two scenarios:
   * 1. Expected: hit a flagged edge (from contextual position) → informational only
   * 2. Unexpected: no flagged edge hit → CRITICAL ERROR (like under-approximation) */
  if (!hb_set_is_empty (in_depend_not_subset))
  {
    if (hit_flagged_edge)
    {
      /* Expected over-approximation: hit an edge from contextual position.
       * Report if verbose or over-approximation reporting enabled. */
      if (_hb_depend_fuzzer_verbose || _hb_depend_fuzzer_report_over_approximation)
      {
        /* Use compact single-line format for -r flag, multi-line for -v */
        if (_hb_depend_fuzzer_report_over_approximation && !_hb_depend_fuzzer_verbose)
        {
          /* Compact format: Test 0xHEX: Depend found N extra glyphs: gid1 gid2 ... */
          fprintf (stderr, "Test 0x%016llx: Depend found %u extra glyphs (expected, hit flagged edge):",
                   (unsigned long long) seed, hb_set_get_population (in_depend_not_subset));

          hb_codepoint_t gid = HB_SET_VALUE_INVALID;
          while (hb_set_next (in_depend_not_subset, &gid))
            fprintf (stderr, " %u", gid);
          fprintf (stderr, "\n");
        }
        else
        {
          /* Verbose multi-line format */
          fprintf (stderr, "\n=== DEPEND API OVER-APPROXIMATION (EXPECTED) ===\n");
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
          fprintf (stderr, "Note: Hit flagged edge from contextual position (intermediate glyph consumer).\n");
          fprintf (stderr, "This is expected over-approximation, not a bug.\n");
          fprintf (stderr, "===============================================\n\n");
        }
      }
    }
    else
    {
      /* UNEXPECTED over-approximation: No flagged edge hit, but still have extra glyphs.
       * This is a CRITICAL ERROR - depend is including glyphs it shouldn't. */
      if (_hb_depend_fuzzer_report_under_approximation ||
          _hb_depend_fuzzer_report_over_approximation ||
          _hb_depend_fuzzer_verbose)
      {
        fprintf (stderr, "Test 0x%016llx: UNEXPECTED OVER-APPROX - Depend found %u extra glyphs WITHOUT hitting flagged edge:",
                 (unsigned long long) seed, hb_set_get_population (in_depend_not_subset));

        hb_codepoint_t gid = HB_SET_VALUE_INVALID;
        unsigned count = 0;
        while (hb_set_next (in_depend_not_subset, &gid) && count < 20)
        {
          fprintf (stderr, " %u", gid);
          count++;
        }
        if (count == 20)
          fprintf (stderr, " ...");
        fprintf (stderr, "\n");
      }

      /* Abort unless reporting flag is set */
      if (!_hb_depend_fuzzer_report_under_approximation)
        assert (0 && "Depend API has unexpected over-approximation without flagged edges");
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
  init_parity_config ();

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

  /* Calculate adaptive probabilities based on graph size if error injection is enabled.
   * Goal: achieve ~2% detection rate regardless of font complexity.
   *
   * Strategy:
   * - Count total edges E in the dependency graph
   * - Estimate average edges traversed per test: V ≈ E * 0.0025 (0.25%)
   * - Calculate per-edge probability: P = target_rate / V
   *
   * This makes user input (e.g., -u 0.02) specify the TARGET detection rate (2%),
   * which is then scaled based on font complexity. Complex fonts like NotoNastaliq
   * get lower per-edge probability, simple fonts get higher per-edge probability.
   *
   * The 0.0025 multiplier was tuned empirically across 8 system fonts to achieve
   * avg ~2% detection rate (measured: 2.12% under, 3.20% over on 4000 tests).
   */
  if (_hb_depend_fuzzer_inject_over_approx > 0.0f ||
      _hb_depend_fuzzer_inject_under_approx > 0.0f)
  {
    /* Count total edges in dependency graph */
    unsigned total_edges = 0;
    unsigned num_glyphs = hb_face_get_glyph_count (face);
    for (unsigned gid = 0; gid < num_glyphs; gid++)
    {
      hb_codepoint_t idx = 0;
      hb_tag_t table_tag;
      hb_codepoint_t dependent;
      hb_tag_t layout_tag;
      hb_codepoint_t ligature_set;
      hb_codepoint_t context_set;

      while (hb_depend_get_glyph_entry (depend, gid, idx++,
                                        &table_tag, &dependent,
                                        &layout_tag, &ligature_set, &context_set, NULL))
      {
        total_edges++;
      }
    }

    /* Estimate average edges traversed per test (roughly 0.25% of total edges) */
    float estimated_edges_per_test = total_edges * 0.0025f;
    if (estimated_edges_per_test < 5.0f)
      estimated_edges_per_test = 5.0f;  /* Minimum to avoid division issues */

    /* Calculate adaptive probabilities to achieve target detection rate
     * User input is now interpreted as TARGET_RATE not per-edge probability */
    if (_hb_depend_fuzzer_inject_over_approx > 0.0f)
    {
      float target_rate = _hb_depend_fuzzer_inject_over_approx;
      _hb_depend_fuzzer_inject_over_approx = target_rate / estimated_edges_per_test;
    }

    if (_hb_depend_fuzzer_inject_under_approx > 0.0f)
    {
      float target_rate = _hb_depend_fuzzer_inject_under_approx;
      _hb_depend_fuzzer_inject_under_approx = target_rate / estimated_edges_per_test;
    }

    if (_hb_depend_fuzzer_verbose)
    {
      fprintf (stderr, "# Adaptive error injection: %u total edges, est. %.0f edges/test\n",
               total_edges, estimated_edges_per_test);
      fprintf (stderr, "# Calculated per-edge probabilities: over=%.6f under=%.6f\n",
               _hb_depend_fuzzer_inject_over_approx,
               _hb_depend_fuzzer_inject_under_approx);
    }
  }

  /* Create font for codepoint-to-glyph mapping */
  hb_font_t *font = hb_font_create (face);

  /* Check test mode: explicit, single test, or full suite */
  if (_hb_depend_fuzzer_explicit_test)
  {
    /* Run explicit test with command-line parameters */
    test_params params = generate_test_from_explicit (face, font);

    /* Check if parsing succeeded */
    if (params.glyphs == NULL)
    {
      fprintf (stderr, "Error: Failed to generate test from explicit parameters\n");
    }
    else
    {
      /* Always print parameters for explicit test */
      print_test_params (0, params.is_gsub, params.glyphs, params.codepoints, params.features);

      /* Run closure comparison (use seed 0 for explicit tests) */
      compare_closures (face, depend, 0, _hb_depend_fuzzer_current_font_path,
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
  else if (_hb_depend_fuzzer_single_test != 0)
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
    /* Generate T pseudo-random 64-bit test seeds from the parity checker seed
     * Each seed deterministically generates a test with specific parameters
     * Distribution follows research-backed proportions (see generate_test_from_seed) */

    unsigned T = _hb_depend_fuzzer_num_tests;

    /* Initialize RNG with parity checker seed to generate test seeds */
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
