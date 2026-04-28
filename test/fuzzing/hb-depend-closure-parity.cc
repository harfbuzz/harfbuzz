#include "hb-fuzzer.hh"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "hb-subset.h"
#include "hb-ot-cmap-table.hh"

#include <random>
#include <vector>

#ifndef HB_NO_SUBSET_DEPEND
#include <hb.h>

/*
 * Depend Parity Checker — validates depend API correctness by comparing closures.
 *
 * For each font and starting glyph set, this checker:
 * 1. Builds a closure by walking the hb_subset_depend_t dependency graph.
 * 2. Builds the same closure using HarfBuzz's subset plan (the ground truth).
 * 3. Asserts that both closures are identical (or that any extra glyphs in the
 *    depend closure can be explained by a known over-approximation).
 *
 * The depend closure mirrors subset's table order: cmap UVS → GSUB → MATH → COLR → glyf/CFF.
 */


/* ============================================================
 * Configuration globals (set by main() in standalone mode)
 * ============================================================ */

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
static hb_codepoint_t debug_gid = HB_CODEPOINT_INVALID;
static bool trace_closure = false;

static bool config_initialized = false;

/* Debug messaging - follows HarfBuzz DEBUG_MSG pattern but simplified for parity checker */
#ifndef HB_DEBUG_DEPEND
#define HB_DEBUG_DEPEND 1
#endif

#if HB_DEBUG_DEPEND
#define DEBUG_MSG_DEPEND(msg, ...) fprintf (stderr, "%-10s" msg "\n", "DEPEND", ##__VA_ARGS__)
#else
#define DEBUG_MSG_DEPEND(msg, ...) do {} while (0)
#endif

/* Read env-var overrides for debug_gid and trace_closure. Called once on first run. */
static void
init_parity_config ()
{
  if (config_initialized)
    return;

  config_initialized = true;

  const char *debug_gid_str = getenv ("HB_DEPEND_PARITY_DEBUG_GID");
  if (debug_gid_str)
    debug_gid = (hb_codepoint_t) strtoul (debug_gid_str, NULL, 0);

  const char *trace_closure_str = getenv ("HB_DEPEND_PARITY_TRACE_CLOSURE");
  if (trace_closure_str && strcmp (trace_closure_str, "1") == 0)
    trace_closure = true;
}


/* ============================================================
 * Closure algorithm — depend API side
 * ============================================================ */

/**
 * deferred_ligature_t:
 *
 * Records a ligature edge whose dependent glyph cannot be added immediately
 * because not all component glyphs are yet in the closure.  Stored at deferral
 * time so we do not need to re-scan edges when revisiting it.
 */
struct deferred_ligature_t
{
  deferred_ligature_t () = default;
  deferred_ligature_t (hb_codepoint_t dependent_,
		       hb_codepoint_t ligature_set_index_,
		       hb_subset_depend_edge_flags_t flags_)
    : dependent (dependent_),
      ligature_set_index (ligature_set_index_),
      flags (flags_) {}

  hb_codepoint_t dependent;
  hb_codepoint_t ligature_set_index;
  hb_subset_depend_edge_flags_t flags;
};

/* Returns true with probability |prob| using |rng| (always false if rng is null or prob <= 0) */
static bool
rng_prob_hit (std::mt19937 *rng, float prob)
{
  if (!rng || prob <= 0.0f)
    return false;
  return std::uniform_real_distribution<float> (0.0f, 1.0f) (*rng) < prob;
}

/* Context shared across the table-stage calls inside compute_depend_closure */
struct depend_closure_ctx_t
{
  hb_subset_depend_t *depend;
  hb_set_t           *glyphs;          /* Accumulating closure set (modified in place) */
  hb_set_t           *active_features; /* NULL = no feature filter */
  bool                skip_gsub;
  bool               *hit_flagged_edge;
  std::mt19937       *injection_rng;
  std::vector<deferred_ligature_t> deferred_ligatures;
};

/* Check whether all context requirements encoded in |context_set_idx| are met.
 *
 * Elements in a context set can be:
 * - Direct GID (value < 0x80000000): that specific glyph must be in the closure.
 * - Indirect set reference (value >= 0x80000000): at least one glyph from the
 *   referenced set must be in the closure.
 *
 * Returns true if ALL requirements are satisfied (or if context_set_idx is invalid). */
static bool
check_context_satisfied (hb_subset_depend_t *depend,
                         hb_codepoint_t context_set_idx,
                         hb_set_t *current_closure)
{
  if (context_set_idx == HB_CODEPOINT_INVALID)
    return true;

  hb_set_t *context_elements = hb_set_create ();
  if (!hb_subset_depend_lookup_set (depend, context_set_idx, context_elements))
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
      /* Direct GID reference */
      if (!hb_set_has (current_closure, elem))
      {
        satisfied = false;
        break;
      }
    }
    else
    {
      /* Indirect set reference — at least one glyph from the set must be present */
      hb_codepoint_t set_idx = elem & 0x7FFFFFFF;
      hb_set_t *required_glyphs = hb_set_create ();

      if (!hb_subset_depend_lookup_set (depend, set_idx, required_glyphs))
      {
        hb_set_destroy (required_glyphs);
        continue;  /* Invalid set reference, skip this requirement */
      }

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

/* Process edges for glyphs in |to_process| that belong to tables in |table_filter|.
 * Appends newly reachable glyphs to ctx.glyphs and back to |to_process| for further expansion.
 * Returns true if any new glyphs were added. */
static bool
process_edges_for_tables (depend_closure_ctx_t &ctx,
                          hb_set_t *to_process,
                          hb_set_t *table_filter)
{
  bool added_any = false;

  while (!hb_set_is_empty (to_process))
  {
    hb_codepoint_t gid = hb_set_get_min (to_process);
    hb_set_del (to_process, gid);

    if (gid == debug_gid)
      DEBUG_MSG_DEPEND ("Processing glyph %u", gid);

    unsigned int total = hb_subset_depend_lookup_glyph (ctx.depend, gid, 0, nullptr, nullptr);
    for (unsigned int idx = 0; idx < total; idx++)
    {
      unsigned int count = 1;
      hb_subset_depend_entry_t entry;
      hb_subset_depend_lookup_glyph (ctx.depend, gid, idx, &count, &entry);

      hb_tag_t      table_tag   = entry.table_tag;
      hb_codepoint_t dependent  = entry.dependent;
      hb_tag_t      layout_tag  = entry.layout_tag;
      hb_codepoint_t ligature_set = entry.ligature_set_index;
      hb_codepoint_t context_set  = entry.context_set_index;
      hb_subset_depend_edge_flags_t flags = entry.flags;

      /* Debug: trace all edges touching the watched glyph */
      if (debug_gid != HB_CODEPOINT_INVALID &&
          (gid == debug_gid || dependent == debug_gid))
        DEBUG_MSG_DEPEND ("Considering edge: %u -> %u (feature=%c%c%c%c, ctx=%u)",
                    gid, dependent, HB_UNTAG (layout_tag), context_set);

      /* Filter by table if requested */
      if (table_filter && !hb_set_has (table_filter, table_tag))
        continue;

      if (dependent == debug_gid || gid == debug_gid)
        DEBUG_MSG_DEPEND ("Edge: %u -> %u (table=%c%c%c%c, ligset=%u)",
                    gid, dependent, HB_UNTAG (table_tag), ligature_set);

      /* UVS closure is handled via the cmap accelerator, not depend edges */
      if (table_tag == HB_TAG ('c','m','a','p'))
        continue;

      /* Skip GSUB edges when requested */
      if (ctx.skip_gsub && table_tag == HB_OT_TAG_GSUB)
        continue;

      /* Filter GSUB edges by active features; over-approx injection can bypass the filter */
      if (ctx.active_features && table_tag == HB_OT_TAG_GSUB &&
          !hb_set_has (ctx.active_features, layout_tag))
      {
        if (!rng_prob_hit (ctx.injection_rng, _hb_depend_fuzzer_inject_over_approx))
        {
          if (dependent == debug_gid)
            DEBUG_MSG_DEPEND ("Skipping edge %u -> %u: feature %c%c%c%c not active",
                        gid, dependent, HB_UNTAG (layout_tag));
          continue;
        }
      }

      /* Skip edges whose context requirements are unsatisfied; over-approx injection can bypass */
      if (context_set != HB_CODEPOINT_INVALID &&
          !check_context_satisfied (ctx.depend, context_set, ctx.glyphs))
      {
        if (!rng_prob_hit (ctx.injection_rng, _hb_depend_fuzzer_inject_over_approx))
        {
          if (dependent == debug_gid)
            DEBUG_MSG_DEPEND ("Skipping edge %u -> %u: context not satisfied (ctx_set=%u)",
                        gid, dependent, context_set);
          continue;
        }
      }

      if (ligature_set != HB_CODEPOINT_INVALID)
      {
        /* Defer: revisit once we know whether all components are in the closure.
         * Feature filtering has already been applied above. */
        ctx.deferred_ligatures.push_back (deferred_ligature_t (dependent, ligature_set, flags));
      }
      else
      {
        /* Under-approx injection: randomly drop this edge */
        if (rng_prob_hit (ctx.injection_rng, _hb_depend_fuzzer_inject_under_approx))
          continue;

        if (!hb_set_has (ctx.glyphs, dependent))
        {
          if (ctx.hit_flagged_edge && (flags & 0x03))
            *ctx.hit_flagged_edge = true;

          if (dependent == debug_gid)
            DEBUG_MSG_DEPEND ("Adding glyph %u to closure (from %u, feature=%c%c%c%c, ctx=%u)",
                        dependent, gid, HB_UNTAG (layout_tag), context_set);
          if (trace_closure)
            DEBUG_MSG_DEPEND ("Depend: %u -> %u (table=%c%c%c%c, feature=%c%c%c%c)",
                        gid, dependent, HB_UNTAG (table_tag), HB_UNTAG (layout_tag));
          hb_set_add (ctx.glyphs, dependent);
          hb_set_add (to_process, dependent);
          added_any = true;
        }
      }
    }
  }

  return added_any;
}

/* Compute closure using the depend API by following the dependency graph.
 *
 * On entry ctx.glyphs is the initial glyph set; on exit it holds the full closure.
 * input_unicodes (may be NULL) is used for UVS (cmap format 14) handling only.
 * Closure is built in table order: cmap UVS → GSUB → MATH → COLR → glyf/CFF.
 * There is no loop back to GSUB after glyf/CFF: component glyphs are rendering
 * details, not shaping inputs, and subset doesn't loop back either. */
static void
compute_depend_closure (hb_face_t *face, depend_closure_ctx_t &ctx,
                        const hb_set_t *input_unicodes)
{
  if (debug_gid != HB_CODEPOINT_INVALID)
    DEBUG_MSG_DEPEND ("Starting closure, glyph %u %s in initial set",
                debug_gid,
                hb_set_has (ctx.glyphs, debug_gid) ? "IS" : "is NOT");

  if (trace_closure)
  {
    hb_codepoint_t gid = HB_SET_VALUE_INVALID;
    fprintf (stderr, "DEPEND: Initial set:");
    while (hb_set_next (ctx.glyphs, &gid))
      fprintf (stderr, " %u", gid);
    fprintf (stderr, "\n");
  }

  hb_set_t *to_process = hb_set_create ();

  /* Stage 1: UVS closure (cmap format 14), processed before GSUB to match subset's order.
   * For each (base unicode, variation selector) pair present in input_unicodes,
   * look up the variant glyph and add it to the closure. */
  if (input_unicodes)
  {
    hb_set_t *all_vs = hb_set_create ();
    hb_face_collect_variation_selectors (face, all_vs);
    hb_set_intersect (all_vs, input_unicodes);

    if (!hb_set_is_empty (all_vs))
    {
      hb_font_t *font = hb_font_create (face);
      hb_codepoint_t vs = HB_SET_VALUE_INVALID;
      while (hb_set_next (all_vs, &vs))
      {
        hb_codepoint_t base = HB_SET_VALUE_INVALID;
        while (hb_set_next (input_unicodes, &base))
        {
          if (hb_set_has (all_vs, base))
            continue;  /* base is itself a VS, skip */
          hb_codepoint_t variant_glyph;
          if (hb_font_get_variation_glyph (font, base, vs, &variant_glyph))
          {
            hb_set_add (ctx.glyphs, variant_glyph);
            if (trace_closure && variant_glyph == debug_gid)
              DEBUG_MSG_DEPEND ("UVS: Added glyph %u via U+%04X+U+%04X",
                          variant_glyph, base, vs);
          }
        }
      }
      hb_font_destroy (font);
    }

    hb_set_destroy (all_vs);
    if (trace_closure)
      DEBUG_MSG_DEPEND ("After UVS (cmap format 14): closure has %u glyphs",
                  hb_set_get_population (ctx.glyphs));
  }

  /* Stage 2: GSUB closure — iterate until stable, matching subset's internal behavior.
   * Ligatures are resolved here (not after glyf/CFF): add a ligature only once all of
   * its required components are in the closure. */
  if (!ctx.skip_gsub)
  {
    hb_set_t *gsub_filter = hb_set_create ();
    hb_set_add (gsub_filter, HB_OT_TAG_GSUB);
    hb_set_t *ligature_set_glyphs = hb_set_create ();

    unsigned prev_gsub_count;
    unsigned iteration = 0;
    do {
      prev_gsub_count = hb_set_get_population (ctx.glyphs);

      hb_set_union (to_process, ctx.glyphs);
      process_edges_for_tables (ctx, to_process, gsub_filter);

      /* Drain deferred ligatures: add each ligature whose component set is now fully satisfied */
      bool lig_made_progress = true;
      while (lig_made_progress && !ctx.deferred_ligatures.empty ())
      {
        lig_made_progress = false;

        for (size_t i = 0; i < ctx.deferred_ligatures.size (); )
        {
          const deferred_ligature_t &def = ctx.deferred_ligatures[i];

          hb_set_clear (ligature_set_glyphs);
          hb_subset_depend_lookup_set (ctx.depend, def.ligature_set_index, ligature_set_glyphs);
          bool set_satisfied = hb_set_is_subset (ligature_set_glyphs, ctx.glyphs);

          /* Over-approx injection: pretend the set is satisfied even when it isn't */
          if (!set_satisfied)
            set_satisfied = rng_prob_hit (ctx.injection_rng, _hb_depend_fuzzer_inject_over_approx);

          if (set_satisfied)
          {
            /* Under-approx injection: skip adding this ligature */
            if (!rng_prob_hit (ctx.injection_rng, _hb_depend_fuzzer_inject_under_approx) &&
                !hb_set_has (ctx.glyphs, def.dependent))
            {
              if (ctx.hit_flagged_edge && (def.flags & 0x03))
                *ctx.hit_flagged_edge = true;
              if (trace_closure)
                DEBUG_MSG_DEPEND ("Adding GSUB ligature %u (components satisfied, flags=0x%02x)",
                            def.dependent, def.flags);
              hb_set_add (ctx.glyphs, def.dependent);
              lig_made_progress = true;
            }
            /* Erase by swapping with the last element */
            ctx.deferred_ligatures[i] = ctx.deferred_ligatures.back ();
            ctx.deferred_ligatures.pop_back ();
          }
          else
            i++;
        }
      }

      iteration++;
    } while (hb_set_get_population (ctx.glyphs) > prev_gsub_count && iteration < 64);

    hb_set_destroy (ligature_set_glyphs);
    hb_set_destroy (gsub_filter);
    if (trace_closure)
      DEBUG_MSG_DEPEND ("After GSUB (%u iterations): closure has %u glyphs",
                  iteration, hb_set_get_population (ctx.glyphs));
  }

  /* Stage 3: MATH closure */
  {
    hb_set_t *math_filter = hb_set_create ();
    hb_set_add (math_filter, HB_OT_TAG_MATH);
    hb_set_union (to_process, ctx.glyphs);
    process_edges_for_tables (ctx, to_process, math_filter);
    hb_set_destroy (math_filter);
    if (trace_closure)
      DEBUG_MSG_DEPEND ("After MATH: closure has %u glyphs", hb_set_get_population (ctx.glyphs));
  }

  /* Stage 4: COLR closure */
  {
    hb_set_t *colr_filter = hb_set_create ();
    hb_set_add (colr_filter, HB_TAG ('C','O','L','R'));
    hb_set_union (to_process, ctx.glyphs);
    process_edges_for_tables (ctx, to_process, colr_filter);
    hb_set_destroy (colr_filter);
    if (trace_closure)
      DEBUG_MSG_DEPEND ("After COLR: closure has %u glyphs", hb_set_get_population (ctx.glyphs));
  }

  /* Stage 5: glyf/CFF closure */
  {
    hb_set_t *glyf_filter = hb_set_create ();
    hb_set_add (glyf_filter, HB_TAG ('g','l','y','f'));
    hb_set_add (glyf_filter, HB_TAG ('C','F','F',' '));
    hb_set_union (to_process, ctx.glyphs);
    process_edges_for_tables (ctx, to_process, glyf_filter);
    hb_set_destroy (glyf_filter);
    if (trace_closure)
      DEBUG_MSG_DEPEND ("After glyf/CFF: closure has %u glyphs", hb_set_get_population (ctx.glyphs));
  }

  hb_set_destroy (to_process);
}


/* ============================================================
 * Closure algorithm — subset side (ground truth)
 * ============================================================ */

/* Compute closure using the HarfBuzz subset mechanism.
 * If active_features is non-NULL and empty on entry, it is populated with the
 * subset's default features so the depend side can use the same set. */
static void
compute_subset_closure (hb_face_t *face, hb_set_t *glyphs, bool skip_gsub,
                        hb_set_t *codepoints, hb_set_t *active_features)
{
  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  if (!input) return;

  unsigned flags = HB_SUBSET_FLAGS_NO_BIDI_CLOSURE;
  if (skip_gsub)
    flags |= HB_SUBSET_FLAGS_NO_LAYOUT_CLOSURE;
  hb_subset_input_set_flags (input, (hb_subset_flags_t) flags);

  /* Populate input — either from codepoints or glyph IDs */
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
   * - active_features NULL:      use subset defaults, don't capture them
   * - active_features non-empty: clear defaults and use the specified set
   * - active_features empty:     capture defaults into it (output parameter) */
  if (active_features && !skip_gsub)
  {
    hb_set_t *input_features = hb_subset_input_set (input, HB_SUBSET_SETS_LAYOUT_FEATURE_TAG);
    if (hb_set_is_empty (active_features))
      hb_set_union (active_features, input_features);
    else
    {
      hb_set_clear (input_features);
      hb_set_union (input_features, active_features);
    }
  }

  hb_subset_plan_t *plan = hb_subset_plan_create_or_fail (face, input);
  if (!plan)
  {
    hb_subset_input_destroy (input);
    return;
  }

  /* Extract the retained glyph set from the plan's glyph mapping */
  hb_map_t *glyph_map = hb_subset_plan_old_to_new_glyph_mapping (plan);
  hb_set_clear (glyphs);
  int idx = -1;
  hb_codepoint_t key, value;
  while (hb_map_next (glyph_map, &idx, &key, &value))
    hb_set_add (glyphs, key);

  if (debug_gid != HB_CODEPOINT_INVALID)
    DEBUG_MSG_DEPEND ("Subset closure %s glyph %u",
               hb_set_has (glyphs, debug_gid) ? "INCLUDES" : "does not include",
               debug_gid);

  hb_subset_plan_destroy (plan);
  hb_subset_input_destroy (input);
}


/* ============================================================
 * Comparison and reporting
 * ============================================================ */

/* Print the parameters of a single test to stderr */
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

/* Compare the depend-API closure against the subset-plan closure for one test case.
 *
 * Under-approximation (depend missed glyphs subset found) is always a bug: assert.
 * Over-approximation (depend found extra glyphs) is expected when a flagged edge
 * (FROM_CONTEXT_POSITION or FROM_NESTED_CONTEXT) was traversed, since depend cannot
 * know whether the intermediate glyph consumer will actually be present; otherwise
 * it is also a bug.
 *
 * Returns true if the two closures are identical. */
static bool
compare_closures (hb_face_t *face, hb_subset_depend_t *depend,
                  uint64_t seed, const char *font_path,
                  bool is_gsub, hb_set_t *start_glyphs, hb_set_t *codepoints,
                  hb_set_t *test_features)
{
  bool skip_gsub = !is_gsub;

  /* --- Subset closure (ground truth) --- */
  hb_set_t *subset_closure = hb_set_create ();
  hb_set_union (subset_closure, start_glyphs);

  /* active_features: pre-populate if caller supplied features, else let subset fill defaults */
  hb_set_t *active_features = hb_set_create ();
  if (test_features)
    hb_set_union (active_features, test_features);

  compute_subset_closure (face, subset_closure, skip_gsub, codepoints, active_features);

  if (trace_closure)
  {
    DEBUG_MSG_DEPEND ("Subset: Final closure has %u glyphs, checking for 82 and 124...",
               hb_set_get_population (subset_closure));
    DEBUG_MSG_DEPEND ("Subset: glyph 82 ('o') %s in closure",
               hb_set_has (subset_closure, 82) ? "IS" : "is NOT");
    DEBUG_MSG_DEPEND ("Subset: glyph 124 (ordmasculine) %s in closure",
               hb_set_has (subset_closure, 124) ? "IS" : "is NOT");
  }

  /* --- Depend closure --- */

  /* For GSUB tests starting from codepoints, map codepoints → glyphs via cmap only
   * (not glyf/COLR/MATH), matching subset's own ordering. */
  hb_set_t *actual_start_glyphs = hb_set_create ();
  if (is_gsub && codepoints && !hb_set_is_empty (codepoints))
  {
    hb_set_add (actual_start_glyphs, 0);  /* .notdef */
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
    hb_set_union (actual_start_glyphs, start_glyphs);

  hb_set_t *depend_closure = hb_set_create ();
  hb_set_union (depend_closure, actual_start_glyphs);
  hb_set_add (depend_closure, 0);  /* subset always adds .notdef; see hb-subset-plan.cc:451 */

  if (debug_gid != HB_CODEPOINT_INVALID && active_features)
  {
    hb_codepoint_t feat = HB_SET_VALUE_INVALID;
    fprintf (stderr, "DEPEND: Active features (%u):", hb_set_get_population (active_features));
    while (hb_set_next (active_features, &feat))
      fprintf (stderr, " %c%c%c%c", HB_UNTAG (feat));
    fprintf (stderr, "\n");
  }

  /* Seed RNG for reproducible error injection: same seed → same edges skipped/added */
  std::mt19937 injection_rng_storage;
  std::mt19937 *injection_rng = nullptr;
  if (_hb_depend_fuzzer_inject_over_approx > 0.0f ||
      _hb_depend_fuzzer_inject_under_approx > 0.0f)
  {
    injection_rng_storage.seed ((unsigned) seed);
    injection_rng = &injection_rng_storage;
  }

  /* Build the unicode set for UVS (cmap format 14) closure.
   * GSUB tests: use input codepoints directly.
   * Non-GSUB tests: reverse-map starting glyphs to their unicodes
   * (matches subset's behavior at hb-subset-plan.cc:310-319). */
  hb_set_t *uvs_unicodes = hb_set_create ();
  if (is_gsub && codepoints && !hb_set_is_empty (codepoints))
  {
    hb_set_union (uvs_unicodes, codepoints);
  }
  else
  {
    hb_map_t *unicode_to_glyph = hb_map_create ();
    hb_face_collect_nominal_glyph_mapping (face, unicode_to_glyph, NULL);
    int idx = -1;
    hb_codepoint_t unicode, glyph;
    while (hb_map_next (unicode_to_glyph, &idx, &unicode, &glyph))
      if (hb_set_has (actual_start_glyphs, glyph))
        hb_set_add (uvs_unicodes, unicode);
    hb_map_destroy (unicode_to_glyph);
  }

  bool hit_flagged_edge = false;
  depend_closure_ctx_t ctx;
  ctx.depend           = depend;
  ctx.glyphs           = depend_closure;
  ctx.active_features  = skip_gsub ? nullptr : active_features;
  ctx.skip_gsub        = skip_gsub;
  ctx.hit_flagged_edge = &hit_flagged_edge;
  ctx.injection_rng    = injection_rng;

  compute_depend_closure (face, ctx, uvs_unicodes);
  hb_set_destroy (uvs_unicodes);
  hb_set_destroy (actual_start_glyphs);

  /* --- Compare --- */
  bool match = hb_set_is_equal (depend_closure, subset_closure);

  hb_set_t *in_depend_not_subset = hb_set_create ();
  hb_set_t *in_subset_not_depend = hb_set_create ();
  hb_set_set (in_depend_not_subset, depend_closure);
  hb_set_subtract (in_depend_not_subset, subset_closure);
  hb_set_set (in_subset_not_depend, subset_closure);
  hb_set_subtract (in_subset_not_depend, depend_closure);

  /* Under-approximation: depend missed glyphs that subset found */
  if (!hb_set_is_empty (in_subset_not_depend))
  {
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
    if (!_hb_depend_fuzzer_report_under_approximation)
      assert (0 && "Depend API missed glyphs that subset found");
  }

  /* Over-approximation: depend found extra glyphs not in subset closure.
   * Expected when a flagged (contextual-position) edge was traversed; a bug otherwise. */
  if (!hb_set_is_empty (in_depend_not_subset))
  {
    if (hit_flagged_edge)
    {
      if (_hb_depend_fuzzer_verbose || _hb_depend_fuzzer_report_over_approximation)
      {
        if (_hb_depend_fuzzer_report_over_approximation && !_hb_depend_fuzzer_verbose)
        {
          fprintf (stderr, "Test 0x%016llx: Depend found %u extra glyphs (expected, hit flagged edge):",
                   (unsigned long long) seed, hb_set_get_population (in_depend_not_subset));
          hb_codepoint_t gid = HB_SET_VALUE_INVALID;
          while (hb_set_next (in_depend_not_subset, &gid))
            fprintf (stderr, " %u", gid);
          fprintf (stderr, "\n");
        }
        else
        {
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
      /* Unexpected: no flagged edge, but depend still has extra glyphs */
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


/* ============================================================
 * Test generation infrastructure
 * ============================================================ */

/* Use std::mt19937 for deterministic, cross-platform random behaviour */
static unsigned
rng_range (std::mt19937 &rng, unsigned min, unsigned max)
{
  if (max <= min)
    return min;
  std::uniform_int_distribution<unsigned> dist (min, max);
  return dist (rng);
}

/* Select n random distinct values from range [0, max) */
static void
rng_select_distinct (std::mt19937 &rng, hb_set_t *out, unsigned n, unsigned max)
{
  if (n >= max)
  {
    for (unsigned i = 0; i < max; i++)
      hb_set_add (out, i);
    return;
  }
  while (hb_set_get_population (out) < n)
    hb_set_add (out, rng_range (rng, 0, max - 1));
}

/* Select n random distinct values from an existing set */
static void
rng_select_from_set (std::mt19937 &rng, hb_set_t *out, unsigned n, const hb_set_t *source)
{
  unsigned source_size = hb_set_get_population (source);
  if (n == 0 || source_size == 0)
    return;

  if (n >= source_size)
  {
    hb_set_union (out, source);
    return;
  }

  /* Convert source to array for indexed access */
  hb_codepoint_t *array = (hb_codepoint_t *) malloc (source_size * sizeof (hb_codepoint_t));
  unsigned idx = 0;
  hb_codepoint_t val = HB_SET_VALUE_INVALID;
  while (hb_set_next (source, &val))
    array[idx++] = val;

  while (hb_set_get_population (out) < n)
    hb_set_add (out, array[rng_range (rng, 0, source_size - 1)]);

  free (array);
}

/* Return a newly-allocated set containing all GSUB feature tags for |face| */
static hb_set_t *
get_gsub_features (hb_face_t *face)
{
  hb_set_t *features = hb_set_create ();

  if (!hb_ot_layout_has_substitution (face))
    return features;

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

/* Parse comma-separated list of unicodes (U+XXXX, decimal, or ranges) into a set */
static bool
parse_unicodes (const char *str, hb_set_t *unicodes)
{
  if (!str || !*str)
    return false;

  const char *p = str;
  while (*p)
  {
    while (*p == ' ' || *p == ',')
      p++;
    if (!*p)
      break;

    const char *start = p;
    hb_codepoint_t start_cp;
    if (*p == 'U' && *(p + 1) == '+')
      start_cp = (hb_codepoint_t) strtoul (p + 2, (char **) &p, 16);
    else
      start_cp = (hb_codepoint_t) strtoul (p, (char **) &p, 10);

    if (p == start || (p == start + 2 && start[0] == 'U' && start[1] == '+'))
    {
      fprintf (stderr, "Error: Invalid unicode value at: %.20s\n", start);
      return false;
    }

    if (*p == '-')
    {
      p++;
      const char *end_start = p;
      hb_codepoint_t end_cp;
      if (*p == 'U' && *(p + 1) == '+')
        end_cp = (hb_codepoint_t) strtoul (p + 2, (char **) &p, 16);
      else
        end_cp = (hb_codepoint_t) strtoul (p, (char **) &p, 10);

      if (p == end_start || (p == end_start + 2 && end_start[0] == 'U' && end_start[1] == '+'))
      {
        fprintf (stderr, "Error: Invalid unicode value in range at: %.20s\n", end_start);
        return false;
      }

      hb_set_add_range (unicodes, start_cp, end_cp);
    }
    else
      hb_set_add (unicodes, start_cp);
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
    while (*p == ' ' || *p == ',')
      p++;
    if (!*p)
      break;

    const char *start = p;
    hb_codepoint_t start_gid = (hb_codepoint_t) strtoul (p, (char **) &p, 10);

    if (p == start)
    {
      fprintf (stderr, "Error: Invalid glyph ID at: %.20s\n", start);
      return false;
    }

    if (*p == '-')
    {
      p++;
      const char *end_start = p;
      hb_codepoint_t end_gid = (hb_codepoint_t) strtoul (p, (char **) &p, 10);

      if (p == end_start)
      {
        fprintf (stderr, "Error: Invalid glyph ID in range at: %.20s\n", end_start);
        return false;
      }

      hb_set_add_range (gids, start_gid, end_gid);
    }
    else
      hb_set_add (gids, start_gid);
  }

  return !hb_set_is_empty (gids);
}

/* Parse comma-separated list of feature tags into a set.
 * The special value "*" means all features from the face. */
static bool
parse_features (const char *str, hb_set_t *features, hb_face_t *face)
{
  if (!str || !*str)
    return false;

  if (strcmp (str, "*") == 0)
  {
    hb_set_t *all_features = get_gsub_features (face);
    hb_set_union (features, all_features);
    hb_set_destroy (all_features);
    return true;
  }

  const char *p = str;
  while (*p)
  {
    while (*p == ' ' || *p == ',')
      p++;
    if (!*p)
      break;

    char tag_str[5] = {0};
    int i = 0;
    while (*p && *p != ',' && *p != ' ' && i < 4)
      tag_str[i++] = *p++;

    if (i > 0)
    {
      while (i < 4)
        tag_str[i++] = ' ';
      tag_str[4] = '\0';
      hb_set_add (features, HB_TAG (tag_str[0], tag_str[1], tag_str[2], tag_str[3]));
    }
  }

  return !hb_set_is_empty (features);
}

/* Test parameters generated from a 64-bit seed or explicit command-line args */
struct test_params
{
  bool is_gsub;           /* GSUB test (codepoint-based) vs non-GSUB (glyph-based) */
  unsigned target_size;   /* Target set size (may be clamped by font size) */
  hb_set_t *glyphs;       /* Starting glyphs (NULL on failure) */
  hb_set_t *codepoints;   /* Starting codepoints (NULL if non-GSUB) */
  hb_set_t *features;     /* Active features (NULL for default/all, only used for GSUB) */
};

/* Generate test parameters from explicit command-line arguments */
static test_params
generate_test_from_explicit (hb_face_t *face, hb_font_t *font)
{
  test_params params;
  params.glyphs = hb_set_create ();
  params.codepoints = NULL;
  params.features = NULL;
  params.target_size = 0;

  if (_hb_depend_fuzzer_explicit_unicodes)
  {
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

/* Generate a test from a 64-bit seed.
 *
 * The seed encodes test type, size category, element selection, and feature selection:
 *
 * Test type distribution (seed % 100):
 * - Non-GSUB single (1):            21%
 * - Non-GSUB small (2–5):           17%
 * - Non-GSUB medium (10–20):        10%
 * - Non-GSUB large (50–100):         6%
 * - Non-GSUB very large (N/10–N/4):  4%
 * - GSUB single (1):                21%
 * - GSUB small (2–5):               10%
 * - GSUB medium (10–20):             6%
 * - GSUB large (50–100):             4%
 *
 * Feature selection (GSUB tests, applied after type):
 * - Single feature:    30%
 * - Small (2–5):       35%
 * - Medium (6–15):     20%
 * - Large (16–50):     10%
 * - All features:       5%
 */
static test_params
generate_test_from_seed (uint64_t seed, hb_face_t *face, hb_font_t *font)
{
  test_params params;
  params.glyphs = NULL;
  params.codepoints = NULL;
  params.features = NULL;

  unsigned num_glyphs = hb_face_get_glyph_count (face);

  hb_set_t *all_codepoints = hb_set_create ();
  hb_face_collect_unicodes (face, all_codepoints);
  unsigned num_codepoints = hb_set_get_population (all_codepoints);

  /* Step 1: Determine test type and size range from seed % 100 */
  unsigned type_selector = seed % 100;
  unsigned min_size, max_size;

  if (type_selector < 21) {
    params.is_gsub = false; min_size = max_size = 1;           /* Non-GSUB single (21%) */
  } else if (type_selector < 38) {
    params.is_gsub = false; min_size = 2;   max_size = 5;      /* Non-GSUB small (17%) */
  } else if (type_selector < 48) {
    params.is_gsub = false; min_size = 10;  max_size = 20;     /* Non-GSUB medium (10%) */
  } else if (type_selector < 54) {
    params.is_gsub = false; min_size = 50;  max_size = 100;    /* Non-GSUB large (6%) */
  } else if (type_selector < 58) {
    params.is_gsub = false;                                     /* Non-GSUB very large (4%) */
    min_size = num_glyphs / 10; max_size = num_glyphs / 4;
  } else if (type_selector < 79) {
    params.is_gsub = true;  min_size = max_size = 1;           /* GSUB single (21%) */
  } else if (type_selector < 89) {
    params.is_gsub = true;  min_size = 2;   max_size = 5;      /* GSUB small (10%) */
  } else if (type_selector < 95) {
    params.is_gsub = true;  min_size = 10;  max_size = 20;     /* GSUB medium (6%) */
  } else {
    params.is_gsub = true;  min_size = 50;  max_size = 100;    /* GSUB large (4%) */
  }

  /* Step 2: Pick target size within range, then clamp to what the font has */
  params.target_size = min_size + ((seed >> 8) % (max_size - min_size + 1));
  if (params.is_gsub)
    params.target_size = params.target_size < num_codepoints ? params.target_size : num_codepoints;
  else
    params.target_size = params.target_size < num_glyphs ? params.target_size : num_glyphs;

  if (params.target_size < min_size)
  {
    hb_set_destroy (all_codepoints);
    params.target_size = 0;
    return params;
  }

  /* Step 3: Select specific elements using a separate RNG seeded from the high bits */
  unsigned selection_seed = (unsigned)((seed >> 32) ^ seed);
  std::mt19937 selection_rng (selection_seed);

  if (params.is_gsub)
  {
    params.codepoints = hb_set_create ();
    params.glyphs = hb_set_create ();

    /* Select random codepoints by index */
    hb_set_t *cp_indices = hb_set_create ();
    rng_select_distinct (selection_rng, cp_indices, params.target_size, num_codepoints);

    hb_codepoint_t idx = HB_SET_VALUE_INVALID;
    while (hb_set_next (cp_indices, &idx))
    {
      hb_codepoint_t cp = HB_SET_VALUE_INVALID;
      for (unsigned j = 0; j <= idx && hb_set_next (all_codepoints, &cp); j++)
        ;
      hb_set_add (params.codepoints, cp);

      hb_codepoint_t gid;
      if (hb_font_get_nominal_glyph (font, cp, &gid))
        hb_set_add (params.glyphs, gid);
    }
    hb_set_destroy (cp_indices);

    /* Select features to activate */
    hb_set_t *available_features = get_gsub_features (face);
    unsigned num_features = hb_set_get_population (available_features);

    if (num_features > 0)
    {
      unsigned feature_selector = std::uniform_int_distribution<unsigned> (0, 99) (selection_rng);
      unsigned num_to_select;

      if (feature_selector < 30)
        num_to_select = 1;                                          /* Single (30%) */
      else if (feature_selector < 65)
        num_to_select = rng_range (selection_rng, 2, 5);           /* Small (35%) */
      else if (feature_selector < 85)
        num_to_select = rng_range (selection_rng, 6, 15);          /* Medium (20%) */
      else if (feature_selector < 95)
        num_to_select = rng_range (selection_rng, 16, 50);         /* Large (10%) */
      else
        num_to_select = num_features;                               /* All (5%) */

      if (num_to_select > num_features)
        num_to_select = num_features;

      params.features = hb_set_create ();
      rng_select_from_set (selection_rng, params.features, num_to_select, available_features);
    }

    hb_set_destroy (available_features);
  }
  else
  {
    params.glyphs = hb_set_create ();
    rng_select_distinct (selection_rng, params.glyphs, params.target_size, num_glyphs);
  }

  hb_set_destroy (all_codepoints);
  return params;
}

#endif /* !HB_NO_SUBSET_DEPEND */

extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
#ifndef HB_NO_SUBSET_DEPEND
  alloc_state = _fuzzing_alloc_state (data, size);

  init_parity_config ();

  hb_blob_t *blob = hb_blob_create ((const char *) data, size,
                                    HB_MEMORY_MODE_READONLY, nullptr, nullptr);
  hb_face_t *face = hb_face_create (blob, 0);

  hb_subset_depend_t *depend = hb_subset_depend_from_face_or_fail (face);
  if (!depend)
  {
    hb_face_destroy (face);
    hb_blob_destroy (blob);
    return 0;
  }

  /* Scale injection probabilities to achieve a target per-test detection rate
   * regardless of font complexity.
   *
   * The user supplies a TARGET_RATE (e.g. 0.02 = 2%).  We estimate the average
   * number of edges traversed per test as E_total * 0.0025 (0.25% of all edges),
   * then set per_edge_prob = TARGET_RATE / estimated_edges_per_test.
   *
   * The 0.0025 multiplier was tuned empirically across 8 system fonts
   * (measured: 2.12% under, 3.20% over on 4000 tests at target rate 0.02). */
  if (_hb_depend_fuzzer_inject_over_approx > 0.0f ||
      _hb_depend_fuzzer_inject_under_approx > 0.0f)
  {
    unsigned total_edges = 0;
    unsigned num_glyphs = hb_face_get_glyph_count (face);
    for (unsigned gid = 0; gid < num_glyphs; gid++)
      total_edges += hb_subset_depend_lookup_glyph (depend, gid, 0, nullptr, nullptr);

    float estimated_edges_per_test = total_edges * 0.0025f;
    if (estimated_edges_per_test < 5.0f)
      estimated_edges_per_test = 5.0f;

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

  hb_font_t *font = hb_font_create (face);

  if (_hb_depend_fuzzer_explicit_test)
  {
    test_params params = generate_test_from_explicit (face, font);
    if (params.glyphs == NULL)
      fprintf (stderr, "Error: Failed to generate test from explicit parameters\n");
    else
    {
      print_test_params (0, params.is_gsub, params.glyphs, params.codepoints, params.features);
      compare_closures (face, depend, 0, _hb_depend_fuzzer_current_font_path,
                        params.is_gsub, params.glyphs, params.codepoints, params.features);
    }
    if (params.glyphs)     hb_set_destroy (params.glyphs);
    if (params.codepoints) hb_set_destroy (params.codepoints);
    if (params.features)   hb_set_destroy (params.features);
  }
  else if (_hb_depend_fuzzer_single_test != 0)
  {
    uint64_t test_seed = _hb_depend_fuzzer_single_test;
    test_params params = generate_test_from_seed (test_seed, face, font);
    if (params.target_size == 0 || params.glyphs == NULL)
      fprintf (stderr, "Warning: Font too small for this test type\n");
    else
    {
      print_test_params (test_seed, params.is_gsub, params.glyphs, params.codepoints, params.features);
      compare_closures (face, depend, test_seed, _hb_depend_fuzzer_current_font_path,
                        params.is_gsub, params.glyphs, params.codepoints, params.features);
    }
    if (params.glyphs)     hb_set_destroy (params.glyphs);
    if (params.codepoints) hb_set_destroy (params.codepoints);
    if (params.features)   hb_set_destroy (params.features);
  }
  else
  {
    /* Generate T pseudo-random 64-bit seeds, then run compare_closures for each */
    std::mt19937 test_rng (_hb_depend_fuzzer_seed);
    std::uniform_int_distribution<uint32_t> uint32_dist;

    for (unsigned i = 0; i < _hb_depend_fuzzer_num_tests; i++)
    {
      /* Keep high bit clear for gint64 compatibility with -t parameter */
      uint64_t test_seed = (((uint64_t)uint32_dist (test_rng) << 32) | (uint64_t)uint32_dist (test_rng)) & 0x7FFFFFFFFFFFFFFFULL;

      test_params params = generate_test_from_seed (test_seed, face, font);
      if (params.target_size == 0 || params.glyphs == NULL)
        continue;

      if (_hb_depend_fuzzer_verbose)
        print_test_params (test_seed, params.is_gsub, params.glyphs, params.codepoints, params.features);

      compare_closures (face, depend, test_seed, _hb_depend_fuzzer_current_font_path,
                        params.is_gsub, params.glyphs, params.codepoints, params.features);

      if (params.glyphs)     hb_set_destroy (params.glyphs);
      if (params.codepoints) hb_set_destroy (params.codepoints);
      if (params.features)   hb_set_destroy (params.features);
    }
  }

  hb_font_destroy (font);
  hb_subset_depend_destroy (depend);
  hb_face_destroy (face);
  hb_blob_destroy (blob);
#endif /* !HB_NO_SUBSET_DEPEND */

  return 0;
}
