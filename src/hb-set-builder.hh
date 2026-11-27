/*
 * Copyright © 2026  The HarfBuzz Project Authors
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

#ifndef HB_SET_BUILDER_HH
#define HB_SET_BUILDER_HH

#include "hb-map.hh"

/* While collecting an unordered stream, keep cheap inserts in the set.
 * Once its page directory is large, staging backward inserts by word
 * avoids shifting the directory once per new page. Repeated values
 * cost no additional storage. Flush before iterating the destination. */
template <typename Set = hb_set_t>
struct hb_set_builder_t
{
  explicit hb_set_builder_t (Set &set) : set (set), largest (set.get_max ())
  { if (largest == HB_SET_VALUE_INVALID) largest = 0; }

  void add (hb_codepoint_t g)
  {
    if (unlikely (g == HB_SET_VALUE_INVALID || in_error ())) return;
    /* A 16-bit glyph space occupies at most 128 pages. */
    if (likely (g >= largest || set.get_page_count () <= 128))
    {
      set.add (g);
      largest = hb_max (g, largest);
      return;
    }
    unsigned word = g >> 6;
    uint64_t bit = uint64_t (1) << (g & 63);
    uint64_t *mask;
    if (pending.has (word, &mask))
      *mask |= bit;
    else if (unlikely (!pending.set (word, bit)))
      set.err ();
  }

  bool has (hb_codepoint_t g) const
  {
    return set.has (g) ||
           (pending.get (g >> 6) & (uint64_t (1) << (g & 63)));
  }

  bool in_error () const { return set.in_error (); }

  void clear ()
  {
    set.clear ();
    pending.reset ();
    largest = 0;
  }

  bool flush ()
  {
    if (unlikely (in_error ())) return false;
    if (!pending) return true;

    hb_vector_t<unsigned> words (pending.keys ());
    if (unlikely (words.in_error ()))
    {
      set.err ();
      return false;
    }
    words.qsort (_hb_cmp_operator<unsigned, unsigned>);
    Set sorted;
    for (unsigned word : words)
      sorted.add_bits (word << 6, pending.get (word));
    if (unlikely (sorted.in_error ())) set.err ();
    else set.union_ (sorted);
    pending.reset ();
    return !in_error ();
  }

  private:
  Set &set;
  hb_codepoint_t largest;
  hb_hashmap_t<unsigned, uint64_t> pending;
};

#endif /* HB_SET_BUILDER_HH */
