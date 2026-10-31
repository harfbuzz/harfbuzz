/*
 * Copyright © 2023  Google, Inc.
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
 * Google Author(s): Qunxin Liu
 */

#include <math.h>
#include "hb-subset-instancer-solver.hh"
#include "hb-ot-layout-common.hh"

#define HB_SOLVER_TEST_CHECK(cond) \
  do { \
    if (!(cond)) { \
      fprintf (stderr, "[FAILED] %s:%d: %s\n", __FILE__, __LINE__, #cond); \
      exit (1); \
    } \
  } while (0)

static void
test_var_region_list_serialize_bounds (const char *which)
{
  bool ran_any = false;
  auto want = [&] (const char *name) {
    if (!strcmp (which, "all") || !strcmp (which, name)) { ran_any = true; return true; }
    return false;
  };

  hb_hashmap_t<hb_tag_t, Triple> empty_region;
  hb_hashmap_t<hb_tag_t, Triple> valid_region;
  HB_SOLVER_TEST_CHECK (valid_region.set (HB_TAG ('w', 'g', 'h', 't'),
                                      Triple (-0.5, 0.25, 1.0)));

  /* Overload 1 positive control: valid 1x1 VarRegionList succeeds cleanly. */
  if (want ("ov1_valid"))
  {
    hb_vector_t<hb_tag_t> axes;
    hb_vector_t<const hb_hashmap_t<hb_tag_t, Triple>*> regions;
    axes.push (HB_TAG ('w', 'g', 'h', 't'));
    regions.push (&valid_region);

    char small_buf[256];
    hb_serialize_context_t c (small_buf, sizeof (small_buf));
    auto *out = c.start_serialize<OT::VarRegionList> ();
    HB_SOLVER_TEST_CHECK (out);
    bool ok = out->serialize (&c, axes, regions);
    HB_SOLVER_TEST_CHECK (ok);
    HB_SOLVER_TEST_CHECK (!c.in_error ());
    if (c.errors != HB_SERIALIZE_ERROR_NONE || out->regionCount == 0) { fprintf (stderr, "[FAILED] valid VarRegionList::serialize control\n"); abort (); } HB_SOLVER_TEST_CHECK (out->regionCount != 0);
  }

  /* Overload 1: axisCount > 0xFFFF must fail with INT_OVERFLOW (not truncate to 0). */
  if (want ("ov1_axis"))
  {
    hb_vector_t<char> buf;
    HB_SOLVER_TEST_CHECK (buf.resize (1u << 20));
    hb_vector_t<hb_tag_t> axes;
    hb_vector_t<const hb_hashmap_t<hb_tag_t, Triple>*> regions;
    HB_SOLVER_TEST_CHECK (axes.resize (0x10000u));
    for (unsigned i = 0; i < axes.length; i++)
      axes.arrayZ[i] = HB_TAG ('w', 'g', 'h', 't');
    regions.push (&empty_region);

    hb_serialize_context_t c (buf.arrayZ, buf.length);
    auto *out = c.start_serialize<OT::VarRegionList> ();
    HB_SOLVER_TEST_CHECK (out);
    bool ok = out->serialize (&c, axes, regions);
    HB_SOLVER_TEST_CHECK (!ok);
    HB_SOLVER_TEST_CHECK (c.in_error ());
    if ((c.errors & HB_SERIALIZE_ERROR_INT_OVERFLOW) == 0 || (c.errors & HB_SERIALIZE_ERROR_OUT_OF_ROOM) != 0) { fprintf (stderr, "[FAILED] expected HB_SERIALIZE_ERROR_INT_OVERFLOW without OUT_OF_ROOM (errors=0x%x)\n", (unsigned) c.errors); abort (); } HB_SOLVER_TEST_CHECK ((c.errors & HB_SERIALIZE_ERROR_INT_OVERFLOW) != 0);
  }

  /* Overload 1: regionCount > 0x7FFF (HBUINT15 max) must fail with INT_OVERFLOW. */
  if (want ("ov1_region"))
  {
    hb_vector_t<char> buf;
    HB_SOLVER_TEST_CHECK (buf.resize (1u << 20));
    hb_vector_t<hb_tag_t> axes;
    hb_vector_t<const hb_hashmap_t<hb_tag_t, Triple>*> regions;
    axes.push (HB_TAG ('w', 'g', 'h', 't'));
    HB_SOLVER_TEST_CHECK (regions.resize (0x8000u));
    for (unsigned i = 0; i < regions.length; i++)
      regions.arrayZ[i] = &empty_region;

    hb_serialize_context_t c (buf.arrayZ, buf.length);
    auto *out = c.start_serialize<OT::VarRegionList> ();
    HB_SOLVER_TEST_CHECK (out);
    bool ok = out->serialize (&c, axes, regions);
    HB_SOLVER_TEST_CHECK (!ok);
    HB_SOLVER_TEST_CHECK (c.in_error ());
    if ((c.errors & HB_SERIALIZE_ERROR_INT_OVERFLOW) == 0 || (c.errors & HB_SERIALIZE_ERROR_OUT_OF_ROOM) != 0) { fprintf (stderr, "[FAILED] expected HB_SERIALIZE_ERROR_INT_OVERFLOW without OUT_OF_ROOM (errors=0x%x)\n", (unsigned) c.errors); abort (); } HB_SOLVER_TEST_CHECK ((c.errors & HB_SERIALIZE_ERROR_INT_OVERFLOW) != 0);
  }

  /* Overload 1: 32-bit product wrap (0x40000 * 0x4000 == 0 mod 2^32, with
   * regionCount = 0x4000 <= 0x7FFF so NOT preempted by region ceiling) must fail
   * up front with INT_OVERFLOW rather than buffer exhaustion (OUT_OF_ROOM). */
  if (want ("ov1_mul"))
  {
    hb_vector_t<hb_tag_t> axes;
    hb_vector_t<const hb_hashmap_t<hb_tag_t, Triple>*> regions;
    HB_SOLVER_TEST_CHECK (axes.resize (0x40000u));
    HB_SOLVER_TEST_CHECK (regions.resize (0x4000u));
    for (unsigned i = 0; i < axes.length; i++)
      axes.arrayZ[i] = HB_TAG ('w', 'g', 'h', 't');
    for (unsigned i = 0; i < regions.length; i++)
      regions.arrayZ[i] = &empty_region;

    char small_buf[16];
    hb_serialize_context_t c (small_buf, sizeof (small_buf));
    auto *out = c.start_serialize<OT::VarRegionList> ();
    HB_SOLVER_TEST_CHECK (out);
    bool ok = out->serialize (&c, axes, regions);
    HB_SOLVER_TEST_CHECK (!ok);
    HB_SOLVER_TEST_CHECK (c.in_error ());
    if ((c.errors & HB_SERIALIZE_ERROR_INT_OVERFLOW) == 0 || (c.errors & HB_SERIALIZE_ERROR_OUT_OF_ROOM) != 0) { fprintf (stderr, "[FAILED] expected HB_SERIALIZE_ERROR_INT_OVERFLOW without OUT_OF_ROOM (errors=0x%x)\n", (unsigned) c.errors); abort (); } HB_SOLVER_TEST_CHECK ((c.errors & HB_SERIALIZE_ERROR_INT_OVERFLOW) != 0);
  }

  /* Overload 2 positive control: valid 1x1 VarRegionList remap succeeds cleanly. */
  if (want ("ov2_valid"))
  {
    const unsigned char raw_src[10] = {
      0x00, 0x01, /* axisCount = 1 */
      0x00, 0x01, /* regionCount = 1 */
      0xE0, 0x00, /* startCoord = -0.5 */
      0x10, 0x00, /* peakCoord = 0.25 */
      0x40, 0x00  /* endCoord = 1.0 */
    };
    const auto *src = reinterpret_cast<const OT::VarRegionList *> (raw_src);
    hb_inc_bimap_t region_map;
    region_map.add (0);

    char small_buf[256];
    hb_serialize_context_t c (small_buf, sizeof (small_buf));
    auto *out = c.start_serialize<OT::VarRegionList> ();
    HB_SOLVER_TEST_CHECK (out);
    bool ok = out->serialize (&c, src, region_map);
    HB_SOLVER_TEST_CHECK (ok);
    HB_SOLVER_TEST_CHECK (!c.in_error ());
    if (c.errors != HB_SERIALIZE_ERROR_NONE || out->regionCount == 0) { fprintf (stderr, "[FAILED] valid VarRegionList::serialize control\n"); abort (); } HB_SOLVER_TEST_CHECK (out->regionCount != 0);
  }

  /* Overload 2: mapped_region_count = 0x8000 (> 0x7FFF HBUINT15 max) must fail
   * with INT_OVERFLOW instead of emitting illegal regionCount = 0x8000. */
  if (want ("ov2_region"))
  {
    const unsigned char raw_src[4] = {
      0x00, 0x00, /* axisCount = 0 */
      0x80, 0x00  /* regionCount = 0x8000 */
    };
    const auto *src = reinterpret_cast<const OT::VarRegionList *> (raw_src);
    hb_inc_bimap_t region_map;
    for (unsigned i = 0; i < 0x8000u; i++)
      region_map.add (i);

    char small_buf[256];
    hb_serialize_context_t c (small_buf, sizeof (small_buf));
    auto *out = c.start_serialize<OT::VarRegionList> ();
    HB_SOLVER_TEST_CHECK (out);
    bool ok = out->serialize (&c, src, region_map);
    HB_SOLVER_TEST_CHECK (!ok);
    HB_SOLVER_TEST_CHECK (c.in_error ());
    if ((c.errors & HB_SERIALIZE_ERROR_INT_OVERFLOW) == 0 || (c.errors & HB_SERIALIZE_ERROR_OUT_OF_ROOM) != 0) { fprintf (stderr, "[FAILED] expected HB_SERIALIZE_ERROR_INT_OVERFLOW without OUT_OF_ROOM (errors=0x%x)\n", (unsigned) c.errors); abort (); } HB_SOLVER_TEST_CHECK ((c.errors & HB_SERIALIZE_ERROR_INT_OVERFLOW) != 0);
  }

  /* Overload 2: mapped_region_count = 0x10000 must fail with INT_OVERFLOW instead
   * of narrowing regionCount to 0 and returning true. */
  if (want ("ov2_region_trunc"))
  {
    const unsigned char raw_src[4] = {
      0x80, 0x00, /* axisCount = 0x8000 */
      0x00, 0x00  /* regionCount = 0 */
    };
    const auto *src = reinterpret_cast<const OT::VarRegionList *> (raw_src);
    hb_inc_bimap_t region_map;
    for (unsigned i = 0; i < 0x10000u; i++)
      region_map.add (i);

    char small_buf[256];
    hb_serialize_context_t c (small_buf, sizeof (small_buf));
    auto *out = c.start_serialize<OT::VarRegionList> ();
    HB_SOLVER_TEST_CHECK (out);
    bool ok = out->serialize (&c, src, region_map);
    HB_SOLVER_TEST_CHECK (!ok);
    HB_SOLVER_TEST_CHECK (c.in_error ());
    if ((c.errors & HB_SERIALIZE_ERROR_INT_OVERFLOW) == 0 || (c.errors & HB_SERIALIZE_ERROR_OUT_OF_ROOM) != 0) { fprintf (stderr, "[FAILED] expected HB_SERIALIZE_ERROR_INT_OVERFLOW without OUT_OF_ROOM (errors=0x%x)\n", (unsigned) c.errors); abort (); } HB_SOLVER_TEST_CHECK ((c.errors & HB_SERIALIZE_ERROR_INT_OVERFLOW) != 0);
  }

  /* Overload 2: 32-bit byte-size multiplication overflow (0xFFFF * 0x7FFF * 6,
   * with mapped_region_count = 0x7FFF <= 0x7FFF so NOT preempted by region ceiling)
   * must record HB_SERIALIZE_ERROR_INT_OVERFLOW on the serializer context. */
  if (want ("ov2_mul"))
  {
    const unsigned char raw_src[4] = {
      0xFF, 0xFF, /* axisCount = 0xFFFF */
      0x7F, 0xFF  /* regionCount = 0x7FFF */
    };
    const auto *src = reinterpret_cast<const OT::VarRegionList *> (raw_src);
    hb_inc_bimap_t region_map;
    for (unsigned i = 0; i < 0x7FFFu; i++)
      region_map.add (i);

    char small_buf[64];
    hb_serialize_context_t c (small_buf, sizeof (small_buf));
    auto *out = c.start_serialize<OT::VarRegionList> ();
    HB_SOLVER_TEST_CHECK (out);
    bool ok = out->serialize (&c, src, region_map);
    HB_SOLVER_TEST_CHECK (!ok);
    HB_SOLVER_TEST_CHECK (c.in_error ());
    if ((c.errors & HB_SERIALIZE_ERROR_INT_OVERFLOW) == 0 || (c.errors & HB_SERIALIZE_ERROR_OUT_OF_ROOM) != 0) { fprintf (stderr, "[FAILED] expected HB_SERIALIZE_ERROR_INT_OVERFLOW without OUT_OF_ROOM (errors=0x%x)\n", (unsigned) c.errors); abort (); } HB_SOLVER_TEST_CHECK ((c.errors & HB_SERIALIZE_ERROR_INT_OVERFLOW) != 0);
  }
  if (!ran_any) { fprintf (stderr, "[FAILED] unknown mode %s\n", which); exit (1); }
}

static void
test_var_region_list_valid_control (void)
{
  test_var_region_list_serialize_bounds ("ov1_valid");
  test_var_region_list_serialize_bounds ("ov2_valid");
  unsigned ok = 1;
  if (ok == 0) exit (1);
  HB_SOLVER_TEST_CHECK (ok != 0);
}

static inline bool approx (Triple a, Triple b)
{
  return abs (a.minimum - b.minimum) < 0.000001f &&
         abs (a.middle - b.middle) < 0.000001f &&
         abs (a.maximum - b.maximum) < 0.000001f;
}

static inline bool approx (float a, float b)
{ return abs (a - b) < 0.000001f; }

/* tests ported from
 * https://github.com/fonttools/fonttools/blob/main/Tests/varLib/instancer/solver_test.py */
int
main (int argc, char **argv)
{
  const char *which = argc > 1 ? argv[1] : "all";
  if (strcmp (which, "all") && strcmp (which, "solver"))
  {
    test_var_region_list_serialize_bounds (which);
    return 0;
  }
  TripleDistances default_axis_distances{1.0f, 1.0f};
  /* Case 1 */
  {
    /* pin axis*/
    Triple tent (0.0f, 1.0f, 1.0f);
    Triple axis_range (0.0f, 0.0f, 0.0f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 0);
  }

  {
    /* pin axis*/
    Triple tent (0.0f, 1.0f, 1.0f);
    Triple axis_range (0.5f, 0.5f, 0.5f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 1);
    hb_always_assert (out[0].first == 0.5f);
    hb_always_assert (out[0].second == Triple ());
  }

  {
    /* tent falls outside the new axis range */
    Triple tent (0.3f, 0.5f, 0.8f);
    Triple axis_range (0.1f, 0.2f, 0.3f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 0);
  }

  {
    /* malformed axis range should be rejected, not abort */
    Triple tent (0.3f, 0.5f, 0.8f);
    Triple axis_range (0.4f, 0.2f, 0.3f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 0);
  }

  {
    /* malformed triple should not abort in renormalization */
    Triple malformed_range (0.4f, 0.2f, 0.3f);
    float normalized = renormalizeValue (0.25f, malformed_range, default_axis_distances, true);
    hb_always_assert (approx (normalized, 0.25f));
  }

  /* Case 2 */
  {
    Triple tent (0.0f, 1.0f, 1.0f);
    Triple axis_range (-1.0f, 0.0f, 0.5f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 1);
    hb_always_assert (out[0].first == 0.5f);
    hb_always_assert (out[0].second == Triple (0.0f, 1.0f, 1.0f));
  }

  /* Case 2 */
  {
    Triple tent (0.0f, 1.0f, 1.0f);
    Triple axis_range (-1.0f, 0.0f, 0.75f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 1);
    hb_always_assert (out[0].first == 0.75f);
    hb_always_assert (out[0].second == Triple (0.0f, 1.0f, 1.0f));
  }

  /* Without gain: */
  /* Case 3 */
  {
    Triple tent (0.0f, 0.2f, 1.0f);
    Triple axis_range (-1.0f, 0.0f, 0.8f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 2);
    hb_always_assert (out[0].first == 1.0f);
    hb_always_assert (out[0].second == Triple (0.0f, 0.25f, 1.0f));
    hb_always_assert (approx (out[1].first, 0.250f));
    hb_always_assert (out[1].second == Triple (0.25f, 1.0f, 1.0f));
  }

  /* Case 3 boundary */
  {
    Triple tent (0.0f, 0.4f, 1.0f);
    Triple axis_range (-1.0f, 0.0f, 0.5f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 2);
    hb_always_assert (out[0].first == 1.0f);
    hb_always_assert (out[0].second == Triple (0.0f, 0.8f, 1.0f));
    hb_always_assert (approx (out[1].first, 2.5f/3));
    hb_always_assert (out[1].second == Triple (0.8f, 1.0f, 1.0f));
  }

  /* Case 4 */
  {
    Triple tent (0.0f, 0.25f, 1.0f);
    Triple axis_range (-1.0f, 0.0f, 0.4f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 2);
    hb_always_assert (out[0].first == 1.0f);
    hb_always_assert (out[0].second == Triple (0.0f, 0.625f, 1.0f));
    hb_always_assert (approx (out[1].first, 0.80f));
    hb_always_assert (out[1].second == Triple (0.625f, 1.0f, 1.0f));
  }

  /* Case 4 */
  {
    Triple tent (0.25f, 0.3f, 1.05f);
    Triple axis_range (0.0f, 0.2f, 0.4f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 2);
    hb_always_assert (out[0].first == 1.0f);
    hb_always_assert (approx (out[0].second, Triple (0.25f, 0.5f, 1.0f)));
    hb_always_assert (approx (out[1].first, 2.6f / 3));
    hb_always_assert (approx (out[1].second, Triple (0.5f, 1.0f, 1.0f)));
  }

  /* Case 4 boundary */
  {
    Triple tent (0.25f, 0.5f, 1.0f);
    Triple axis_range (0.0f, 0.25f, 0.5f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 1);
    hb_always_assert (out[0].first == 1.0f);
    hb_always_assert (out[0].second == Triple (0.0f, 1.0f, 1.0f));
  }

  /* With gain */
  /* Case 3a/1neg */
  {
    Triple tent (0.0f, 0.5f, 1.0f);
    Triple axis_range (0.0f, 0.5f, 1.0f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 3);
    hb_always_assert (out[0].first == 1.0f);
    hb_always_assert (out[0].second == Triple ());
    hb_always_assert (out[1].first == -1.0f);
    hb_always_assert (out[1].second == Triple (0.0f, 1.0f, 1.0f));
    hb_always_assert (out[2].first == -1.0f);
    hb_always_assert (out[2].second == Triple (-1.0f, -1.0f, 0.0f));
  }

  {
    Triple tent (0.0f, 0.5f, 1.0f);
    Triple axis_range (0.0f, 0.5f, 0.75f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 3);
    hb_always_assert (out[0].first == 1.0f);
    hb_always_assert (out[0].second == Triple ());
    hb_always_assert (out[1].first == -0.5f);
    hb_always_assert (out[1].second == Triple (0.0f, 1.0f, 1.0f));
    hb_always_assert (out[2].first == -1.0f);
    hb_always_assert (out[2].second == Triple (-1.0f, -1.0f, 0.0f));
  }

  {
    Triple tent (0.0f, 0.50f, 1.0f);
    Triple axis_range (0.0f, 0.25f, 0.8f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 4);
    hb_always_assert (out[0].first == 0.5f);
    hb_always_assert (out[0].second == Triple ());
    hb_always_assert (out[1].first == 0.5f);
    hb_always_assert (approx (out[1].second, Triple (0.0f, 0.454545f, 0.909091f)));
    hb_always_assert (approx (out[2].first, -0.1f));
    hb_always_assert (approx (out[2].second, Triple (0.909091f, 1.0f, 1.0f)));
    hb_always_assert (out[3].first == -0.5f);
    hb_always_assert (out[3].second == Triple (-1.0f, -1.0f, 0.0f));
  }

  /* Case 3a/1neg */
  {
    Triple tent (0.0f, 0.5f, 2.0f);
    Triple axis_range (0.2f, 0.5f, 0.8f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 3);
    hb_always_assert (out[0].first == 1.0f);
    hb_always_assert (out[0].second == Triple ());
    hb_always_assert (approx (out[1].first, -0.2f));
    hb_always_assert (out[1].second == Triple (0.0f, 1.0f, 1.0f));
    hb_always_assert (approx (out[2].first, -0.6f));
    hb_always_assert (approx (out[2].second, Triple (-1.0f, -1.0f, 0.0f)));
  }

  /* Case 3a/1neg */
  {
    Triple tent (0.0f, 0.5f, 2.0f);
    Triple axis_range (0.2f, 0.5f, 1.0f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 3);
    hb_always_assert (out[0].first == 1.0f);
    hb_always_assert (out[0].second == Triple ());
    hb_always_assert (approx (out[1].first, -1.0f/3));
    hb_always_assert (out[1].second == Triple (0.0f, 1.0f, 1.0f));
    hb_always_assert (approx (out[2].first, -0.6f));
    hb_always_assert (approx (out[2].second, Triple (-1.0f, -1.0f, 0.0f)));
  }

  /* Case 3 */
  {
    Triple tent (0.0f, 0.5f, 1.0f);
    Triple axis_range (0.25f, 0.25f, 0.75f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 2);
    hb_always_assert (out[0].first == 0.5f);
    hb_always_assert (out[0].second == Triple ());
    hb_always_assert (out[1].first == 0.5f);
    hb_always_assert (out[1].second == Triple (0.0f, 0.5f, 1.0f));
  }

  /* Case 1neg */
  {
    Triple tent (0.0f, 0.5f, 1.0f);
    Triple axis_range (0.0f, 0.25f, 0.5f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 3);
    hb_always_assert (out[0].first == 0.5f);
    hb_always_assert (out[0].second == Triple ());
    hb_always_assert (out[1].first == 0.5f);
    hb_always_assert (out[1].second == Triple (0.0f, 1.0f, 1.0f));
    hb_always_assert (out[2].first == -0.5f);
    hb_always_assert (out[2].second == Triple (-1.0f, -1.0f, 0.0f));
  }

  /* Case 2neg */
  {
    Triple tent (0.05f, 0.55f, 1.0f);
    Triple axis_range (0.0f, 0.25f, 0.5f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 4);
    hb_always_assert (approx (out[0].first, 0.4f));
    hb_always_assert (out[0].second == Triple ());
    hb_always_assert (approx (out[1].first, 0.5f));
    hb_always_assert (out[1].second == Triple (0.0f, 1.0f, 1.0f));
    hb_always_assert (approx (out[2].first, -0.4f));
    hb_always_assert (out[2].second == Triple (-1.0f, -0.8f, 0.0f));
    hb_always_assert (approx (out[3].first, -0.4f));
    hb_always_assert (out[3].second == Triple (-1.0f, -1.0f, -0.8f));
  }

  /* Case 2neg, other side */
  {
    Triple tent (-1.0f, -0.55f, -0.05f);
    Triple axis_range (-0.5f, -0.25f, 0.0f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 4);
    hb_always_assert (approx (out[0].first, 0.4f));
    hb_always_assert (out[0].second == Triple ());
    hb_always_assert (approx (out[1].first, 0.5f));
    hb_always_assert (out[1].second == Triple (-1.0f, -1.0f, 0.0f));
    hb_always_assert (approx (out[2].first, -0.4f));
    hb_always_assert (out[2].second == Triple (0.0f, 0.8f, 1.0f));
    hb_always_assert (approx (out[3].first, -0.4f));
    hb_always_assert (out[3].second == Triple (0.8f, 1.0f, 1.0f));
  }

  /* Misc corner cases */
  {
    Triple tent (0.5f, 0.5f, 0.5f);
    Triple axis_range (0.5f, 0.5f, 0.5f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 1);
    hb_always_assert (out[0].first == 1.0f);
    hb_always_assert (out[0].second == Triple ());
  }

  {
    Triple tent (0.3f, 0.5f, 0.7f);
    Triple axis_range (0.1f, 0.5f, 0.9f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 5);
    hb_always_assert (out[0].first == 1.0f);
    hb_always_assert (out[0].second == Triple ());
    hb_always_assert (out[1].first == -1.0f);
    hb_always_assert (approx(out[1].second, Triple (0.0f, 0.5f, 1.0f)));
    hb_always_assert (out[2].first == -1.0f);
    hb_always_assert (approx(out[2].second, Triple (0.5f, 1.0f, 1.0f)));
    hb_always_assert (out[3].first == -1.0f);
    hb_always_assert (approx (out[3].second, Triple (-1.0f, -0.5f, 0.0f)));
    hb_always_assert (out[4].first == -1.0f);
    hb_always_assert (approx (out[4].second, Triple (-1.0f, -1.0f, -0.5f)));
  }

  {
    Triple tent (0.5f, 0.5f, 0.5f);
    Triple axis_range (0.25f, 0.25f, 0.5f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 1);
    hb_always_assert (out[0].first == 1.0f);
    hb_always_assert (out[0].second == Triple (1.0f, 1.0f, 1.0f));
  }

  {
    Triple tent (0.5f, 0.5f, 0.5f);
    Triple axis_range (0.25f, 0.35f, 0.5f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 1);
    hb_always_assert (out[0].first == 1.0f);
    hb_always_assert (out[0].second == Triple (1.0f, 1.0f, 1.0f));
  }

  {
    Triple tent (0.5f, 0.5f, 0.55f);
    Triple axis_range (0.25f, 0.35f, 0.5f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 1);
    hb_always_assert (out[0].first == 1.0f);
    hb_always_assert (out[0].second == Triple (1.0f, 1.0f, 1.0f));
  }

  {
    Triple tent (0.5f, 0.5f, 1.0f);
    Triple axis_range (0.5f, 0.5f, 1.0f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 2);
    hb_always_assert (out[0].first == 1.0f);
    hb_always_assert (out[0].second == Triple ());
    hb_always_assert (out[1].first == -1.0f);
    hb_always_assert (out[1].second == Triple (0.0f, 1.0f, 1.0f));
  }

  {
    Triple tent (0.25f, 0.5f, 1.0f);
    Triple axis_range (0.5f, 0.5f, 1.0f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 2);
    hb_always_assert (out[0].first == 1.0f);
    hb_always_assert (out[0].second == Triple ());
    hb_always_assert (out[1].first == -1.0f);
    hb_always_assert (out[1].second == Triple (0.0f, 1.0f, 1.0f));
  }

  {
    Triple tent (0.0f, 0.2f, 1.0f);
    Triple axis_range (0.0f, 0.0f, 0.5f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 2);
    hb_always_assert (out[0].first == 1.0f);
    hb_always_assert (out[0].second == Triple (0.0f, 0.4f, 1.0f));
    hb_always_assert (out[1].first == 0.625f);
    hb_always_assert (out[1].second == Triple (0.4f, 1.0f, 1.0f));
  }


  {
    Triple tent (0.0f, 0.5f, 1.0f);
    Triple axis_range (-1.0f, 0.25f, 1.0f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 5);
    hb_always_assert (out[0].first == 0.5f);
    hb_always_assert (out[0].second == Triple ());
    hb_always_assert (out[1].first == 0.5f);
    hb_always_assert (out[1].second == Triple (0.0f, 1.0f/3, 2.0f/3));
    hb_always_assert (out[2].first == -0.5f);
    hb_always_assert (out[2].second == Triple (2.0f/3, 1.0f, 1.0f));
    hb_always_assert (out[3].first == -0.5f);
    hb_always_assert (out[3].second == Triple (-1.0f, -0.2f, 0.0f));
    hb_always_assert (out[4].first == -0.5f);
    hb_always_assert (out[4].second == Triple (-1.0f, -1.0f, -0.2f));
  }

  {
    Triple tent (0.5f, 0.5f, 0.5f);
    Triple axis_range (0.0f, 0.5f, 1.0f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 5);
    hb_always_assert (out[0].first == 1.0f);
    hb_always_assert (out[0].second == Triple ());
    hb_always_assert (out[1].first == -1.0f);
    hb_always_assert (out[1].second == Triple (0.0f, 2/(double) (1 << 14), 1.0f));
    hb_always_assert (out[2].first == -1.0f);
    hb_always_assert (out[2].second == Triple (2/(double) (1 << 14), 1.0f, 1.0f));
    hb_always_assert (out[3].first == -1.0f);
    hb_always_assert (out[3].second == Triple (-1.0f, -2/(double) (1 << 14), 0.0f));
    hb_always_assert (out[4].first == -1.0f);
    hb_always_assert (out[4].second == Triple (-1.0f, -1.0f, -2/(double) (1 << 14)));
  }

  {
    Triple tent (0.0f, 1.0f, 1.0f);
    Triple axis_range (-1.0f, -0.5f, 1.0f);
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, default_axis_distances, out, scratch);
    hb_always_assert (out.length == 1);
    hb_always_assert (out[0].first == 1.0f);
    hb_always_assert (out[0].second == Triple (1.0f/3, 1.0f, 1.0f));
  }

  {
    Triple tent (0.0f, 1.0f, 1.0f);
    Triple axis_range (-1.0f, -0.5f, 1.0f);
    TripleDistances axis_distances{2.0f, 1.0f};
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, axis_distances, out, scratch);
    hb_always_assert (out.length == 1);
    hb_always_assert (out[0].first == 1.0f);
    hb_always_assert (out[0].second == Triple (0.5f, 1.0f, 1.0f));
  }

  {
    Triple tent (0.6f, 0.7f, 0.8f);
    Triple axis_range (-1.0f, 0.2f, 1.0f);
    TripleDistances axis_distances{1.0f, 1.0f};
    rebase_tent_result_t out, scratch;
    rebase_tent (tent, axis_range, axis_distances, out, scratch);
    hb_always_assert (out.length == 1);
    hb_always_assert (out[0].first == 1.0f);
    hb_always_assert (approx (out[0].second, Triple (0.5f, 0.625f, 0.75f)));
  }

  if (!strcmp (which, "all"))
  {
    test_var_region_list_valid_control ();
    test_var_region_list_serialize_bounds ("all");
  }
}
