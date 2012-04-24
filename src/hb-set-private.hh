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

#ifndef HB_SET_PRIVATE_HH
#define HB_SET_PRIVATE_HH

#include "hb-private.hh"
#include "hb-set.h"
#include "hb-object-private.hh"



struct _hb_set_t
{
  inline void clear (void) {
    memset (elts, 0, sizeof elts);
  }
  inline bool add (hb_codepoint_t g)
  {
    if (unlikely (g > MAX_G)) return false;
    elt_t &e = elt (g);
    elt_t m = mask (g);
    bool ret = !(e & m);
    e |= m;
    return ret;
  }
  inline bool del (hb_codepoint_t g)
  {
    if (unlikely (g > MAX_G)) return false;
    elt_t &e = elt (g);
    elt_t m = mask (g);
    bool ret = !!(e & m);
    e &= ~m;
    return ret;
  }
  inline bool has (hb_codepoint_t g) const
  {
    if (unlikely (g > MAX_G)) return false;
    return !!(elt (g) & mask (g));
  }
  inline bool intersects (hb_codepoint_t first,
			  hb_codepoint_t last) const
  {
    if (unlikely (first > MAX_G)) return false;
    if (unlikely (last  > MAX_G)) last = MAX_G;
    unsigned int end = last + 1;
    for (hb_codepoint_t i = first; i < end; i++)
      if (has (i))
        return true;
    return false;
  }

  typedef uint32_t elt_t;
  static const unsigned int MAX_G = 65536 - 1;
  static const unsigned int SHIFT = 5;
  static const unsigned int BITS = (1 << SHIFT);
  static const unsigned int MASK = BITS - 1;

  elt_t &elt (hb_codepoint_t g) { return elts[g >> SHIFT]; }
  elt_t elt (hb_codepoint_t g) const { return elts[g >> SHIFT]; }
  elt_t mask (hb_codepoint_t g) const { return elt_t (1) << (g & MASK); }

  hb_object_header_t header;
  elt_t elts[(MAX_G + 1 + (BITS - 1)) / BITS]; /* 8kb */

  ASSERT_STATIC (sizeof (elt_t) * 8 == BITS);
  ASSERT_STATIC (sizeof (elts) * 8 > MAX_G);
};



#endif /* HB_SET_PRIVATE_HH */
