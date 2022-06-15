/*
 * Copyright © 2012  Google, Inc.
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
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_SET_DIGEST_HH
#define HB_SET_DIGEST_HH

#include "hb.hh"

#include <arm_neon.h>

/*
 * The set-digests here implement various "filters" that support
 * "approximate member query".  Conceptually these are like Bloom
 * Filter and Quotient Filter, however, much smaller, faster, and
 * designed to fit the requirements of our uses for glyph coverage
 * queries.
 *
 * Our filters are highly accurate if the lookup covers fairly local
 * set of glyphs, but fully flooded and ineffective if coverage is
 * all over the place.
 *
 * The way these are used is that the filter is first populated by
 * a lookup's or subtable's Coverage table(s), and then when we
 * want to apply the lookup or subtable to a glyph, before trying
 * to apply, we ask the filter if the glyph may be covered. If it's
 * not, we return early.
 *
 * We use these filters both at the lookup-level, and then again,
 * at the subtable-level. Both have performance win.
 *
 * The main filter we use is a combination of three bits-pattern
 * filters. A bits-pattern filter checks a number of bits (5 or 6)
 * of the input number (glyph-id in this case) and checks whether
 * its pattern is amongst the patterns of any of the accepted values.
 * The accepted patterns are represented as a "long" integer. The
 * check is done using four bitwise operations only.
 */

template <typename mask_t, unsigned int shift>
struct hb_set_digest_bits_pattern_t
{
  static constexpr unsigned mask_bytes = sizeof (mask_t);
  static constexpr unsigned mask_bits = sizeof (mask_t) * 8;
  static constexpr unsigned num_bits = 0
				     + (mask_bytes >= 1 ? 3 : 0)
				     + (mask_bytes >= 2 ? 1 : 0)
				     + (mask_bytes >= 4 ? 1 : 0)
				     + (mask_bytes >= 8 ? 1 : 0)
				     + (mask_bytes >= 16? 1 : 0)
				     + 0;

  static_assert ((shift < sizeof (hb_codepoint_t) * 8), "");
  static_assert ((shift + num_bits <= sizeof (hb_codepoint_t) * 8), "");

  void init () { mask = 0; }

  void add (hb_codepoint_t g) { mask |= mask_for (g); }

  bool add_range (hb_codepoint_t a, hb_codepoint_t b)
  {
    if ((b >> shift) - (a >> shift) >= mask_bits - 1)
      mask = (mask_t) -1;
    else {
      mask_t ma = mask_for (a);
      mask_t mb = mask_for (b);
      mask |= mb + (mb - ma) - (mb < ma);
    }
    return true;
  }

  template <typename T>
  void add_array (const T *array, unsigned int count, unsigned int stride=sizeof(T))
  {
    for (unsigned int i = 0; i < count; i++)
    {
      add (*array);
      array = (const T *) (stride + (const char *) array);
    }
  }
  template <typename T>
  void add_array (const hb_array_t<const T>& arr) { add_array (&arr, arr.len ()); }
  template <typename T>
  bool add_sorted_array (const T *array, unsigned int count, unsigned int stride=sizeof(T))
  {
    for (unsigned int i = 0; i < count; i++)
    {
      add (*array);
      array = (const T *) (stride + (const char *) array);
    }
    return true;
  }
  template <typename T>
  bool add_sorted_array (const hb_sorted_array_t<const T>& arr) { return add_sorted_array (&arr, arr.len ()); }

  bool may_have (hb_codepoint_t g) const
  { return mask & mask_for (g); }

  static inline bool uint32x4_is_not_zero(uint32x4_t v)
  {
    // https://stackoverflow.com/questions/15389539/fastest-way-to-test-a-128-bit-neon-register-for-a-value-of-0-using-intrinsics
    uint32x2_t tmp = vorr_u32(vget_low_u32(v), vget_high_u32(v));
    return vget_lane_u32(vpmax_u32(tmp, tmp), 0);
  }

  bool may_have (const uint32x4_t &g) const
  { return uint32x4_is_not_zero (vandq_u32 (vdupq_n_u32 (mask), mask_for (g))); }

  private:

  static mask_t mask_for (hb_codepoint_t g)
  { return ((mask_t) 1) << ((g >> shift) & (mask_bits - 1)); }

  template <int u = shift,
	    hb_enable_if (u == 0)>
  static uint32x4_t shifted (uint32x4_t v) { return v; }
  template <int u = shift,
	    hb_enable_if (u != 0)>
  static uint32x4_t shifted (uint32x4_t v) { return vshrq_n_u32 (v, shift); }

  static uint32x4_t mask_for (const uint32x4_t &g)
  {
    uint32x4_t a = vandq_u32 (shifted (g), vdupq_n_u32 (mask_bits - 1));
    return vshlq_u32 (vdupq_n_u32 (1), a);
  }

  mask_t mask;
};

template <typename head_t, typename tail_t>
struct hb_set_digest_combiner_t
{
  void init ()
  {
    head.init ();
    tail.init ();
  }

  void add (hb_codepoint_t g)
  {
    head.add (g);
    tail.add (g);
  }

  bool add_range (hb_codepoint_t a, hb_codepoint_t b)
  {
    head.add_range (a, b);
    tail.add_range (a, b);
    return true;
  }
  template <typename T>
  void add_array (const T *array, unsigned int count, unsigned int stride=sizeof(T))
  {
    head.add_array (array, count, stride);
    tail.add_array (array, count, stride);
  }
  template <typename T>
  void add_array (const hb_array_t<const T>& arr) { add_array (&arr, arr.len ()); }
  template <typename T>
  bool add_sorted_array (const T *array, unsigned int count, unsigned int stride=sizeof(T))
  {
    head.add_sorted_array (array, count, stride);
    tail.add_sorted_array (array, count, stride);
    return true;
  }
  template <typename T>
  bool add_sorted_array (const hb_sorted_array_t<const T>& arr) { return add_sorted_array (&arr, arr.len ()); }

  template <typename T>
  bool may_have (const T &g) const
  {
    return head.may_have (g) && tail.may_have (g);
  }

  private:
  head_t head;
  tail_t tail;
};


/*
 * hb_set_digest_t
 *
 * This is a combination of digests that performs "best".
 * There is not much science to this: it's a result of intuition
 * and testing.
 */
using hb_set_digest_t =
  hb_set_digest_combiner_t
  <
    hb_set_digest_bits_pattern_t<uint32_t, 4>,
    hb_set_digest_combiner_t
    <
      hb_set_digest_bits_pattern_t<uint32_t, 0>,
      hb_set_digest_bits_pattern_t<uint32_t, 9>
    >
  >
;


#endif /* HB_SET_DIGEST_HH */
