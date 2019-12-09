/*
 * Copyright © 2019  Facebook, Inc.
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
 * Facebook Author(s): Behdad Esfahbod
 */

#include "hb.hh"
#include "hb-simd.hh"



struct U16
{
  U16 (unsigned v_)
  {
    v[0] = v_ >> 8;
    v[1] = v_ & 0xFF;
  }

  uint8_t v[2];
};

int
main (int argc, char **argv)
{
#ifndef HB_NO_SIMD

  const U16 a[] = {1, 2, 5, 10, 16, 19};

#define TEST(k, f, p) \
  { \
    unsigned pos = 123456789; \
    bool found = hb_simd_ksearch_glyphid_range (&pos, \
						k, \
						a, \
						ARRAY_LENGTH (a) / 2, \
						sizeof (a[0]) * 2); \
    /*printf ("key %d found %d pos %d\n", k, found, pos);*/ \
    assert (found == f && pos == p); \
  }

  TEST (0, false, 0);
  TEST (1, true , 0);
  TEST (2, true , 0);
  TEST (3, false, 1);
  TEST (4, false, 1);
  TEST (5, true , 1);
  TEST (6, true , 1);
  TEST (7, true , 1);
  TEST (8, true , 1);
  TEST (9, true , 1);
  TEST (10, true , 1);
  TEST (11, false, 2);
  TEST (12, false, 2);
  TEST (13, false, 2);
  TEST (14, false, 2);
  TEST (15, false, 2);
  TEST (16, true , 2);
  TEST (17, true , 2);
  TEST (18, true , 2);
  TEST (19, true , 2);
  TEST (20, false, 3);

#endif
  return 0;
}
