/*
 * Copyright © 2025  Skef Iterum
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
 */

#include "hb-test.h"

#include <hb.h>
#include <hb-ot.h>

#ifdef HB_DEPEND_API

/*
 * ============================================================================
 * DEPEND TEST DEVELOPMENT NOTES (TEMPORARY - REMOVE WHEN COMPLETE)
 * ============================================================================
 *
 * TESTING PHILOSOPHY (Option B: "Double Duty")
 * ----------------------------------------------
 * We use a "double duty" approach where tests serve multiple purposes:
 * 1. Basic substitution tests verify the fundamental substitution types work
 * 2. Contextual format tests verify BOTH:
 *    - The contextual format parsing/traversal works
 *    - Recursion correctly invokes nested lookups
 *    - The invoked basic substitution type produces correct dependencies
 *
 * This reduces redundancy while maximizing coverage. Instead of separate tests
 * for "SingleSubst works" and "ContextSubst Format 2 works", we test
 * "ContextSubst Format 2 correctly invokes SingleSubst".
 *
 * COVERAGE REQUIREMENTS
 * ---------------------
 * Basic GSUB substitution types:
 *   ✓ SingleSubst - covered via contextual recursion tests
 *   ✓ MultipleSubst - covered via contextual recursion tests
 *   ✓ AlternateSubst - standalone test (has special AlternateSet behavior)
 *   ✓ LigatureSubst - standalone test (has special ligature_set tracking)
 *   ✓ ReverseChainSingleSubst - standalone test (special type 8)
 *
 * Contextual/Chaining substitution formats:
 *   ✓ ContextSubst Format 1 (glyph-based with SubRuleSets)
 *   ✓ ContextSubst Format 2 (class-based with SubClassSets)
 *   ✓ ContextSubst Format 3 (coverage-based inline)
 *   ✓ ChainContextSubst Format 1 (glyph-based with backtrack/lookahead)
 *   ✓ ChainContextSubst Format 2 (class-based with backtrack/lookahead)
 *   ✓ ChainContextSubst Format 3 (coverage-based with backtrack/lookahead)
 *
 * Special features:
 *   ✓ ExtensionSubst (Type 7) - implicitly tested (174 instances in NotoNastaliqUrdu)
 *   ✓ Feature tagging - verify layout_tag propagates through recursion
 *   ✓ Ligature set tracking - verify ligature_set is assigned for alternates
 *   ✓ Contextual filtering - verify glyphs unreachable from context are excluded
 *
 * IMPLICIT vs EXPLICIT TESTING
 * -----------------------------
 * Explicit: We have a test assertion that verifies a specific dependency
 *           from a known lookup format. If the format is broken, test fails.
 *
 * Implicit: The code path executes (font has those lookups), but we don't
 *           assert a specific dependency from that format. If format is broken,
 *           tests might still pass.
 *
 * We prefer EXPLICIT tests for all major formats. Implicit is acceptable only
 * for ExtensionSubst (which is transparent) and edge cases.
 *
 * HOW TO FIND TEST CASES
 * ----------------------
 * 1. Extract GSUB table from font:
 *    ttx -q -t GSUB test/api/fonts/FontName.ttf -o /tmp/font-gsub.ttx
 *
 * 2. Find contextual lookups of desired format:
 *    grep -n "ContextSubst Format=\"2\"" /tmp/font-gsub.ttx
 *
 * 3. Look at the lookup structure (around the line number from step 2):
 *    sed -n 'START,END p' /tmp/font-gsub.ttx
 *    - Note the Coverage (which glyphs this applies to)
 *    - Note the ClassDef (for Format 2) or inline coverages (for Format 3)
 *    - Find SubstLookupRecord entries showing which lookups are invoked
 *
 * 4. Check what the invoked lookup does:
 *    grep -A15 '<Lookup index="N">' /tmp/font-gsub.ttx
 *
 * 5. Find glyph IDs:
 *    ttx -q -t GlyphOrder test/api/fonts/FontName.ttf -o - 2>&1 | grep "GlyphName"
 *
 * 6. Verify the dependency exists (test in isolation first if uncertain)
 *
 * CRITERIA FOR GOOD TEST CASES
 * -----------------------------
 * 1. Simple: Prefer lookups with small coverage, simple contexts
 * 2. Clear: The glyph→glyph dependency should be unambiguous
 * 3. Direct: Avoid deeply nested recursion if possible
 * 4. Documented: Comments should explain:
 *    - Which lookup(s) are involved
 *    - What format is being tested
 *    - What the context is (if applicable)
 *    - What basic substitution type is invoked
 *
 * FIND_DEPENDENCY HELPER
 * ----------------------
 * The find_dependency() helper has special IN-OUT parameter behavior:
 *
 * layout_tag as filter (IN):
 *   If *out_layout_tag != HB_TAG_NONE, only matches that specific layout tag
 *   Example: layout_tag = HB_TAG('c','c','m','p');
 *            find_dependency(..., &layout_tag, NULL);
 *
 * layout_tag as output (OUT):
 *   If *out_layout_tag == HB_TAG_NONE, returns the layout_tag found
 *   Example: layout_tag = HB_TAG_NONE;
 *            find_dependency(..., &layout_tag, &ligature_set);
 *            // layout_tag now contains the tag of the dependency found
 *
 * This dual mode reduces code duplication for common patterns.
 *
 * REMAINING TESTS TO ADD
 * ----------------------
 * The depend API extracts dependencies from these tables:
 *   ✓ GSUB - glyph substitution (all formats and critical edge cases tested)
 *       - Multiple subtables per lookup (regression test for Bug #1)
 *       - Same lookup called multiple times in context (regression test for Bug #2)
 *   ✓ cmap - character mapping (COMPLETE)
 *       - Nominal glyph mappings: NOT stored as dependencies (Unicode → glyph
 *         is not a glyph→glyph relationship). Stored in separate nominal_glyphs map.
 *       - Unicode Variation Sequences (UVS): Only NonDefault UVS creates dependencies
 *         (nominal_glyph(U) → variant_glyph(U+S) with layout_tag=S)
 *       - Default UVS entries do NOT create dependencies
 *       - Tested with base.ttf: U+904D + 0xE01E5 → variant glyph
 *   ✓ glyf - composite glyphs (COMPLETE)
 *       - Component references in TrueType composite glyphs
 *       - Tested: Afghani (202) → three components in NotoNastaliqUrdu
 *       - Tagged with HB_TAG('g','l','y','f')
 *   ✓ COLR - color layering (COMPLETE)
 *       - COLRv0: Base glyph → layer glyphs (tested with COLRv0.extents.ttf)
 *       - COLRv1: Paint graph dependencies (tested with test_glyphs-glyf_colr_1.ttf)
 *       - Tagged with HB_TAG('C','O','L','R')
 *       - Fixed: Added guard to prevent v0-only fonts from accessing v1 data structures
 *   ✓ MATH - mathematical variants (COMPLETE)
 *       - Vertical/horizontal variants (tested with arrowup variants)
 *       - Size variants (uni2191_size2, etc.)
 *       - Glyph assembly parts (bottom, vertical)
 *       - Tested with MathTestFontFull.otf
 *       - Tagged with HB_TAG('M','A','T','H')
 *
 * SUMMARY OF TEST COVERAGE:
 * ---------------------------
 * ✓ GSUB: Complete - all 8 substitution types, 6 contextual formats, and edge cases:
 *     - Lookups with multiple subtables (first subtable may not contain target rule)
 *     - Contextual rules calling same lookup at multiple sequence positions
 * ✓ cmap: Complete - NonDefault UVS dependencies tested (base.ttf)
 * ✓ glyf: Complete - composite glyph dependencies tested
 * ✓ MATH: Complete - size variants and glyph assembly tested
 * ✓ COLR: Complete - both v0 layer dependencies and v1 Paint graph tested
 *
 * ADDING NEW TESTS
 * ----------------
 * When adding tests for new tables:
 * 1. Follow the same "double duty" philosophy where applicable
 * 2. Document format coverage in comments
 * 3. Test both basic types AND contextual/recursive mechanisms (if applicable)
 * 4. Include negative tests where filtering or exclusion is important
 * 5. Verify feature tagging where relevant (table_tag should be correct)
 * 6. Update this comment block with new insights
 *
 * TEST FONTS
 * ----------
 * - SourceSansPro-Regular.otf: Basic Latin font with common GSUB features
 *   Used for: AlternateSubst, LigatureSubst, ChainContextSubst Format 3
 *
 * - NotoSans-Bold.ttf: Fast font with comprehensive GSUB formats (0.01-0.02s init)
 *   Used for: glyf composite glyphs, ContextSubst Format 1
 *   Has: ContextSubst Format 1/2, ChainContextSubst Format 1/2/3
 *
 * - NotoNastaliqUrdu-Regular.ttf: Complex Arabic Nastaliq font (4+ seconds init)
 *   Used for: Contextual filtering negative test (issue #3397)
 *   TODO: Create minimal subset for remaining Format 2 tests
 *   Has: 174 ExtensionSubst lookups, 28 ChainContextSubst Format 2
 *
 * - Qahiri-Regular.ttf: Arabic font with various contextual formats
 *   Used for: ReverseChainSingleSubst, ContextSubst Format 3,
 *            ChainContextSubst Format 1
 *
 * KEY INSIGHTS FROM DEVELOPMENT
 * ------------------------------
 * 1. ExtensionSubst (Type 7) is transparent - it wraps other lookup types.
 *    No need for explicit tests; if wrapped types work, extension works.
 *
 * 2. ChainContextSubst Format 2 is the most complex:
 *    - Has BacktrackClassDef, InputClassDef, LookAheadClassDef
 *    - Rules reference classes, not glyphs directly
 *    - Need to map glyphs → classes → rules to find what happens
 *
 * 3. Contextual filtering (issue #3397) is crucial:
 *    - When context C invokes lookup L, only glyphs reachable from C's
 *      input pattern should get dependencies from L
 *    - Test both positive (dependency exists) and negative (doesn't exist)
 *
 * 4. Feature tagging propagation:
 *    - When feature F's lookup invokes another lookup via context,
 *      the resulting dependencies should be tagged with F
 *    - Important for subsetting - tells which glyphs are needed for which features
 *
 * ============================================================================
 */

/*
 * Utility function to search for a specific dependency.
 * Loops through all dependency entries for source_glyph and returns TRUE
 * if a dependency on target_glyph with the expected table_tag is found.
 *
 * If out_layout_tag is non-NULL and *out_layout_tag is not HB_TAG_NONE,
 * only matches entries with that specific layout_tag (in-out parameter).
 * Otherwise returns the layout_tag of the match found (out parameter).
 *
 * If found, optionally returns the ligature_set via out parameter.
 */
static hb_bool_t
find_dependency (hb_depend_t *depend,
                 hb_codepoint_t source_glyph,
                 hb_codepoint_t target_glyph,
                 hb_tag_t expected_table_tag,
                 hb_tag_t *out_layout_tag,          /* IN-OUT (optional) */
                 hb_codepoint_t *out_ligature_set)  /* OUT (optional) */
{
  hb_tag_t table_tag;
  hb_codepoint_t dependent;
  hb_tag_t layout_tag;
  hb_codepoint_t ligature_set;
  hb_codepoint_t context_set;
  hb_tag_t filter_layout_tag = (out_layout_tag && *out_layout_tag != HB_TAG_NONE)
                                ? *out_layout_tag : HB_TAG_NONE;

  for (hb_codepoint_t index = 0; ; index++)
  {
    hb_bool_t found = hb_depend_get_glyph_entry (depend, source_glyph, index,
                                                  &table_tag, &dependent,
                                                  &layout_tag, &ligature_set, &context_set,
                                                  NULL);
    if (!found)
      break;

    if (table_tag == expected_table_tag && dependent == target_glyph)
    {
      /* If filtering by layout_tag, check if it matches */
      if (filter_layout_tag != HB_TAG_NONE && layout_tag != filter_layout_tag)
        continue;

      if (out_layout_tag)
        *out_layout_tag = layout_tag;
      if (out_ligature_set)
        *out_ligature_set = ligature_set;
      return TRUE;
    }
  }

  return FALSE;
}

/* Test cmap (character mapping) dependency extraction */
static void
test_depend_cmap (void)
{
  /* ===== UNDERSTANDING cmap DEPENDENCIES ===== */
  /*
   * IMPORTANT: Nominal cmap mappings (Unicode → glyph) are NOT stored as
   * glyph→glyph dependencies. They're collected in a separate nominal_glyphs
   * map for lookup purposes.
   *
   * Only Unicode Variation Sequences (UVS) with NonDefault mappings create
   * glyph dependencies. The model is:
   *   - Unicode U nominally maps to glyph G
   *   - U + selector S → variant glyph V (NonDefault UVS)
   *   - This creates dependency: G depends on V with layout_tag=S
   *
   * Default UVS entries (where selector uses the nominal glyph) do NOT
   * create dependencies.
   */

  /* ===== UNICODE VARIATION SEQUENCES (UVS) ===== */

  /* base.ttf has NonDefault UVS entries:
   * U+904D nominally maps to cid00007 (GID 7)
   * U+904D + selector 0xE01E5 → cid00010 (GID 10) via NonDefault UVS
   * This creates dependency: GID 7 → GID 10 with layout_tag=0xE01E5 */

  hb_face_t *face = hb_test_open_font_file ("fonts/base.ttf");
  hb_depend_t *depend = hb_depend_from_face_or_fail (face);
  g_assert_nonnull (depend);

  g_test_message ("Testing cmap NonDefault UVS: cid00007 (7) → cid00010 (10) with selector 0xE01E5");
  hb_tag_t layout_tag = 0xE01E5;  /* UVS selector stored in layout_tag */
  g_assert_true (find_dependency (depend, 7, 10, HB_TAG('c','m','a','p'),
                                  &layout_tag, NULL));

  hb_depend_destroy (depend);
  hb_face_destroy (face);
}

/* Test glyf (composite glyph) dependency extraction */
static void
test_depend_glyf (void)
{
  /* NotoSans-Bold.ttf has composite glyphs */
  hb_face_t *face = hb_test_open_font_file ("fonts/NotoSans-Bold.ttf");
  hb_depend_t *depend = hb_depend_from_face_or_fail (face);
  g_assert_nonnull (depend);

  /* Composite glyph "Aacute" (131) is composed of two components:
   * - A (36)
   * - acute (118)
   * The composite glyph depends on all its components. */

  g_test_message ("Testing glyf composite: Aacute (131) → A (36)");
  g_assert_true (find_dependency (depend, 131, 36, HB_TAG('g','l','y','f'),
                                  NULL, NULL));

  g_test_message ("Testing glyf composite: Aacute (131) → acute (118)");
  g_assert_true (find_dependency (depend, 131, 118, HB_TAG('g','l','y','f'),
                                  NULL, NULL));

  hb_depend_destroy (depend);
  hb_face_destroy (face);
}

/* Test CFF SEAC (Standard Encoding Accented Character) dependency extraction */
static void
test_depend_cff (void)
{
  /* cff1_seac.C0.otf has a SEAC composite glyph */
  hb_face_t *face = hb_test_open_font_file ("fonts/cff1_seac.C0.otf");
  hb_depend_t *depend = hb_depend_from_face_or_fail (face);
  g_assert_nonnull (depend);

  /* Glyph 2 is a SEAC composite defined as base (glyph 1) + accent (glyph 3).
   * The composite glyph depends on both components. */

  g_test_message ("Testing CFF SEAC composite: glyph 2 → base (1)");
  g_assert_true (find_dependency (depend, 2, 1, HB_TAG('C','F','F',' '),
                                  NULL, NULL));

  g_test_message ("Testing CFF SEAC composite: glyph 2 → accent (3)");
  g_assert_true (find_dependency (depend, 2, 3, HB_TAG('C','F','F',' '),
                                  NULL, NULL));

  hb_depend_destroy (depend);
  hb_face_destroy (face);
}

/* Test COLR (color layering) dependency extraction */
static void
test_depend_colr (void)
{
  /* COLRv0: base glyph with layer glyphs */
  hb_face_t *face = hb_test_open_font_file ("fonts/COLRv0.extents.ttf");
  hb_depend_t *depend = hb_depend_from_face_or_fail (face);
  g_assert_nonnull (depend);

  g_test_message ("Testing COLRv0: glyph00013 (13) → glyph00010 (10)");
  g_assert_true (find_dependency (depend, 13, 10, HB_TAG('C','O','L','R'), NULL, NULL));

  hb_depend_destroy (depend);
  hb_face_destroy (face);

  /* COLRv1: Paint graph dependencies */
  face = hb_test_open_font_file ("fonts/test_glyphs-glyf_colr_1.ttf");
  depend = hb_depend_from_face_or_fail (face);
  g_assert_nonnull (depend);

  g_test_message ("Testing COLRv1 v0 layer: glyph 168 → glyph 176");
  g_assert_true (find_dependency (depend, 168, 176, HB_TAG('C','O','L','R'), NULL, NULL));

  g_test_message ("Testing COLRv1 PaintGlyph Format 10: glyph 12 → glyph 176");
  g_assert_true (find_dependency (depend, 12, 176, HB_TAG('C','O','L','R'), NULL, NULL));

  g_test_message ("Testing COLRv1 PaintGlyph self-reference filtered: glyph 8 should NOT depend on itself");
  g_assert_false (find_dependency (depend, 8, 8, HB_TAG('C','O','L','R'), NULL, NULL));

  hb_depend_destroy (depend);
  hb_face_destroy (face);
}

/* Test MATH (mathematical variants) dependency extraction */
static void
test_depend_math (void)
{
  /* MathTestFontFull.otf has full MATH table with variants and assemblies */
  hb_face_t *face = hb_test_open_font_file ("fonts/MathTestFontFull.otf");
  hb_depend_t *depend = hb_depend_from_face_or_fail (face);
  g_assert_nonnull (depend);

  /* Base glyph "arrowup" (31) has:
   * - Size variants: uni2191_size2 (68), uni2191_size3 (69), etc.
   * - Glyph assembly parts: bottom (62), vertical (64)
   */

  g_test_message ("Testing MATH size variant: arrowup (31) → uni2191_size2 (68)");
  g_assert_true (find_dependency (depend, 31, 68, HB_TAG('M','A','T','H'),
                                  NULL, NULL));

  g_test_message ("Testing MATH assembly part: arrowup (31) → bottom (62)");
  g_assert_true (find_dependency (depend, 31, 62, HB_TAG('M','A','T','H'),
                                  NULL, NULL));

  g_test_message ("Testing MATH assembly part: arrowup (31) → vertical (64)");
  g_assert_true (find_dependency (depend, 31, 64, HB_TAG('M','A','T','H'),
                                  NULL, NULL));

  hb_depend_destroy (depend);
  hb_face_destroy (face);
}

/* Test GSUB dependency extraction for all substitution formats */
static void
test_depend_gsub_formats (void)
{
  hb_tag_t layout_tag;
  hb_codepoint_t ligature_set;

  /* ===== BASIC SUBSTITUTION TYPES ===== */

  /* SourceSansPro-Regular.otf */
  hb_face_t *face_source = hb_test_open_font_file ("fonts/SourceSansPro-Regular.otf");
  hb_depend_t *depend_source = hb_depend_from_face_or_fail (face_source);
  g_assert_nonnull (depend_source);

  /* AlternateSubst: A → A.sc (tests AlternateSet with multiple alternates) */
  g_test_message ("Testing AlternateSubst: A (2) → A.sc (1217)");
  g_assert_true (find_dependency (depend_source, 2, 1217, HB_OT_TAG_GSUB,
                                  NULL, NULL));

  /* LigatureSubst: multiple glyphs to one (tests Ligature and LigatureSet) */
  g_test_message ("Testing LigatureSubst: uni0302 (1829) → uni03020300 (1910)");
  g_assert_true (find_dependency (depend_source, 1829, 1910, HB_OT_TAG_GSUB,
                                  &layout_tag, &ligature_set));
  g_test_message ("  ligature_set = %u", ligature_set);

  /* Verify ligature set is valid and contains the other component(s) */
  g_assert_cmpuint (HB_CODEPOINT_INVALID, !=, ligature_set);
  hb_set_t *lig_set_source = hb_set_create ();
  g_assert_true (hb_depend_get_set_from_index (depend_source, ligature_set, lig_set_source));
  g_assert_cmpuint (0, <, hb_set_get_population (lig_set_source));  /* Non-empty */
  g_test_message ("  ligature set contains %u component(s)", hb_set_get_population (lig_set_source));
  hb_set_destroy (lig_set_source);

  hb_depend_destroy (depend_source);
  hb_face_destroy (face_source);

  /* Qahiri-Regular.ttf */
  hb_face_t *face_qahiri = hb_test_open_font_file ("fonts/Qahiri-Regular.ttf");
  hb_depend_t *depend_qahiri = hb_depend_from_face_or_fail (face_qahiri);
  g_assert_nonnull (depend_qahiri);

  /* ReverseChainSingleSubst */
  g_test_message ("Testing ReverseChainSingleSubst: glyph00047 (47) → glyph00463 (463)");
  g_assert_true (find_dependency (depend_qahiri, 47, 463, HB_OT_TAG_GSUB,
                                  NULL, NULL));

  /* ===== CONTEXTUAL SUBSTITUTION FORMATS (with recursion) ===== */

  /* ContextSubst Format 3 → SingleSubst
   * Lookup 31: Format 3 context invokes Lookup 1 (SingleSubst) */
  g_test_message ("Testing ContextSubst Format 3 → SingleSubst: glyph00053 (53) → glyph00052 (52)");
  g_assert_true (find_dependency (depend_qahiri, 53, 52, HB_OT_TAG_GSUB,
                                  NULL, NULL));

  /* ChainContextSubst Format 1 → SingleSubst
   * Lookup 11: Format 1 chain context invokes Lookup 2 (SingleSubst) twice */
  g_test_message ("Testing ChainContextSubst Format 1 → SingleSubst: glyph00159 (159) → glyph00162 (162)");
  g_assert_true (find_dependency (depend_qahiri, 159, 162, HB_OT_TAG_GSUB,
                                  NULL, NULL));

  /* Verify second call to same lookup (Bug regression test)
   * Same Lookup 11 calls Lookup 2 AGAIN at SequenceIndex=1 for glyph 154
   * This catches Bug #2: lookups_seen blocking repeated lookup calls */
  g_test_message ("Testing same lookup called twice (Bug regression): glyph00154 (154) → glyph00155 (155)");
  g_assert_true (find_dependency (depend_qahiri, 154, 155, HB_OT_TAG_GSUB,
                                  NULL, NULL));

  /* Lookup 28: Multiple subtables with nested lookup calls (Bug regression tests)
   * This lookup has 3 subtables:
   *   - Subtable 0: Has 4 inputs, SubstCount=0 (no nested lookups)
   *   - Subtable 1: Has 2 inputs, calls Lookup 3 twice
   *   - Subtable 2: Additional rules
   * Tests both bugs:
   *   - Bug #1: Must process all subtables (not stop after first)
   *   - Bug #2: Must allow same lookup called multiple times */

  /* Lookup 28 Subtable 1 calls Lookup 3 at SequenceIndex=0 for glyph 47 */
  g_test_message ("Testing multiple subtables (Bug regression): glyph00047 (47) → glyph00051 (51) via Lookup 28→3");
  g_assert_true (find_dependency (depend_qahiri, 47, 51, HB_OT_TAG_GSUB,
                                  NULL, NULL));

  /* Lookup 28 Subtable 1 calls Lookup 3 AGAIN at SequenceIndex=1 for glyph 212 */
  g_test_message ("Testing same lookup twice in context (Bug regression): glyph00212 (212) → glyph00220 (220) via Lookup 28→3");
  g_assert_true (find_dependency (depend_qahiri, 212, 220, HB_OT_TAG_GSUB,
                                  NULL, NULL));

  hb_depend_destroy (depend_qahiri);
  hb_face_destroy (face_qahiri);

  /* NotoSans-Bold.ttf has ContextSubst Format 1, Format 2, and ChainContextSubst Format 2 */
  hb_face_t *face_notosans = hb_test_open_font_file ("fonts/NotoSans-Bold.ttf");
  hb_depend_t *depend_notosans = hb_depend_from_face_or_fail (face_notosans);
  g_assert_nonnull (depend_notosans);

  /* ContextSubst Format 1 → LigatureSubst
   * Lookup 88: Format 1 context invokes Lookup 106 (LigatureSubst)
   * When uni092C is followed by uni094D, creates ligature baprehalfdeva */
  g_test_message ("Testing ContextSubst Format 1 → LigatureSubst: uni092C (3893) → baprehalfdeva (4156)");
  layout_tag = HB_TAG('h','a','l','f');
  g_assert_true (find_dependency (depend_notosans, 3893, 4156, HB_OT_TAG_GSUB,
                                  &layout_tag, &ligature_set));

  /* REGRESSION TEST: Sequential accumulation for intermediate glyphs
   *
   * Test case: NotoSans-Bold Devanagari contextual substitution
   *   - Glyph 3948: uni093C (nukta combining mark)
   *   - Glyph 3969: uni0944 (vowel sign)
   *   - Glyph 4017: rrvocalicvowelsignnuktaleftdeva (ligature output)
   *
   * Contextual rule flow:
   *   1. Match pattern: [nukta-consonant] + [vowel-sign-uni0944]
   *   2. Apply Lookup A at position 0: Decompose nukta-consonant → base + uni093C
   *   3. Apply Lookup B at position 1: Ligate uni093C + uni0944 → glyph 4017
   *
   * The bug (before fix):
   *   - Lookup B only saw glyphs from input coverage
   *   - uni093C (3948) is an intermediate output, not in original coverage
   *   - Edge 3948 → 4017 was MISSING from dependency graph
   *
   * The fix (sequential accumulation):
   *   - Process lookups in order, accumulating outputs
   *   - Lookup B now sees accumulated glyphs including 3948
   *   - Edge 3948 → 4017 is correctly recorded
   *
   * This edge is critical: if a user subsets glyph 3948, they need 4017 too. */
  g_test_message ("Testing sequential accumulation: uni093C (3948) → rrvocalicvowelsignnuktaleftdeva (4017)");
  layout_tag = HB_TAG('b','l','w','s');
  g_assert_true (find_dependency (depend_notosans, 3948, 4017, HB_OT_TAG_GSUB,
                                  &layout_tag, &ligature_set));
  g_test_message ("  ligature_set = %u", ligature_set);

  /* Verify ligature set contains the other component glyph (3969) */
  g_assert_cmpuint (HB_CODEPOINT_INVALID, !=, ligature_set);
  hb_set_t *lig_set = hb_set_create ();
  g_assert_true (hb_depend_get_set_from_index (depend_notosans, ligature_set, lig_set));
  g_assert_true (hb_set_has (lig_set, 3969));  /* uni0944 is the other ligature component */
  hb_set_destroy (lig_set);

  hb_depend_destroy (depend_notosans);
  hb_face_destroy (face_notosans);

  /* ContextFormat2-Depend-Test.ttf - Test font for contextual substitution formats
   * Created from NotoNastaliqUrdu-Regular.ttf (486KB → 85KB, 83% reduction)
   * - GSUB table minimized to 6 lookups needed for tests
   * - All glyph outline data removed (only table structures kept)
   * Test lookups:
   *   - Lookup 9, 10: ContextSubst for contextual filtering test
   *   - Lookup 17, 20: ContextSubst Format 2 → MultipleSubst test
   *   - Lookup 25, 119: ChainContextSubst Format 2 → SingleSubst test
   * Test runtime: 4+ seconds → 0.02 seconds (200× faster) */
  hb_face_t *face_context = hb_test_open_font_file ("fonts/ContextFormat2-Depend-Test.ttf");
  hb_depend_t *depend_context = hb_depend_from_face_or_fail (face_context);
  g_assert_nonnull (depend_context);

  /* ContextSubst Format 2 → MultipleSubst
   * Lookup 20: Format 2 class-based context invokes Lookup 17 (MultipleSubst) */
  g_test_message ("Testing ContextSubst Format 2 → MultipleSubst: YehBarreeFin (252) → YehBarreeFin_3 (255)");
  g_assert_true (find_dependency (depend_context, 252, 255, HB_OT_TAG_GSUB,
                                  NULL, NULL));

  /* ChainContextSubst Format 2 → SingleSubst
   * Lookup 25: Format 2 class-based chain context with lookahead invokes Lookup 119 (SingleSubst)
   * When BehxIni (class 2) is followed by AlefFin (class 1), BehxIni → BehxIni.A */
  g_test_message ("Testing ChainContextSubst Format 2 → SingleSubst: BehxIni (281) → BehxIni.A (282)");
  g_assert_true (find_dependency (depend_context, 281, 282, HB_OT_TAG_GSUB,
                                  NULL, NULL));

  /* ADDITIONAL REGRESSION TESTS
   *
   * The sequential accumulation and nested context bug fixes are also
   * comprehensively tested by the depend fuzzer with these seeds:
   *   - 0x4388c9ba4973f4f9 (sequential accumulation bug)
   *   - 0x5a50e64a753f3298 (nested context bug)
   *
   * The fuzzer verifies depend closure matches subset closure across
   * 1024 randomized test cases per font.
   */

  /* Negative test: verify contextual filtering (issue #3397)
   * Lookup 10 covers {AlefFin, LamIni, LamMed} but Lookup 9's SubRuleSet for LamIni
   * only has LamIni in position 0. LamMed→LamFin.LA is unreachable from this context. */
  g_test_message ("Testing contextual filtering: LamIni (416) should NOT depend on LamFin.LA (268)");
  g_assert_false (find_dependency (depend_context, 416, 268, HB_OT_TAG_GSUB,
                                   NULL, NULL));

  hb_depend_destroy (depend_context);
  hb_face_destroy (face_context);

  /* ChainContextSubst Format 3 → MultipleSubst + feature tagging */
  face_source = hb_test_open_font_file ("fonts/SourceSansPro-Regular.otf");
  depend_source = hb_depend_from_face_or_fail (face_source);
  g_assert_nonnull (depend_source);

  /* Lookup 8: Format 3 chain context invokes Lookup 7 (MultipleSubst)
   * Also verifies contextual recursion propagates feature tags */
  g_test_message ("Testing ChainContextSubst Format 3 → MultipleSubst: Ecircumflex (92) → E (6) with ccmp");
  layout_tag = HB_TAG('c','c','m','p');
  g_assert_true (find_dependency (depend_source, 92, 6, HB_OT_TAG_GSUB,
                                  &layout_tag, NULL));

  hb_depend_destroy (depend_source);
  hb_face_destroy (face_source);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_depend_cmap);
  hb_test_add (test_depend_glyf);
  hb_test_add (test_depend_cff);
  hb_test_add (test_depend_colr);
  hb_test_add (test_depend_math);
  hb_test_add (test_depend_gsub_formats);

  return hb_test_run ();
}

#else

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_skip ("HB_DEPEND_API not enabled");
  return 0;
}

#endif
