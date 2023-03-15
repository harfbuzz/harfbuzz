/*
 * Copyright © 2021  Behdad Esfahbod
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

#include "hb.hh"
#include "hb-set.hh"

int
main (int argc, char **argv)
{

  /* Test copy constructor. */
  {
    hb_set_t v1 {1, 2};
    hb_set_t v2 {v1};
    assert (v1.get_population () == 2);
    assert (hb_len (hb_iter (v1)) == 2);
    assert (v2.get_population () == 2);
  }

  /* Test copy assignment. */
  {
    hb_set_t v1 {1, 2};
    hb_set_t v2;
    v2 = v1;
    assert (v1.get_population () == 2);
    assert (v2.get_population () == 2);
  }

  /* Test move constructor. */
  {
    hb_set_t s {1, 2};
    hb_set_t v (std::move (s));
    assert (s.get_population () == 0);
    assert (hb_len (hb_iter (s)) == 0);
    assert (v.get_population () == 2);
  }

  /* Test move assignment. */
  {
    hb_set_t s = hb_set_t {1, 2};
    hb_set_t v;
    v = std::move (s);
    assert (s.get_population () == 0);
    assert (v.get_population () == 2);
  }

  /* Test initializing from iterable. */
  {
    hb_set_t s;

    s.add (18);
    s.add (12);

    hb_vector_t<hb_codepoint_t> v (s);
    hb_set_t v0 (v);
    hb_set_t v1 (s);
    hb_set_t v2 (std::move (s));

    assert (s.get_population () == 0);
    assert (v0.get_population () == 2);
    assert (v1.get_population () == 2);
    assert (v2.get_population () == 2);
  }

  /* Test initializing from iterator. */
  {
    hb_set_t s;

    s.add (18);
    s << 12;

    /* Sink a range. */
    s << hb_pair_t<hb_codepoint_t, hb_codepoint_t> {1, 3};

    hb_set_t v (hb_iter (s));

    assert (v.get_population () == 5);
  }

  /* Test initializing from initializer list and swapping. */
  {
    hb_set_t v1 {1, 2, 3};
    hb_set_t v2 {4, 5};
    hb_swap (v1, v2);
    assert (v1.get_population () == 2);
    assert (v2.get_population () == 3);
  }

  /* Test inverted sets. */
  {
    hb_set_t s;
    s.invert();
    s.del (5);

    hb_codepoint_t start = HB_SET_VALUE_INVALID, last = HB_SET_VALUE_INVALID;
    assert (s.next_range (&start, &last));
    assert (start == 0);
    assert (last == 4);
    assert (s.next_range (&start, &last));
    assert (start == 6);
    assert (last == HB_SET_VALUE_INVALID - 1);
    assert (!s.next_range (&start, &last));

    start = HB_SET_VALUE_INVALID;
    last = HB_SET_VALUE_INVALID;
    assert (s.previous_range (&start, &last));
    assert (start == 6);
    assert (last == HB_SET_VALUE_INVALID - 1);
    assert (s.previous_range (&start, &last));
    assert (start == 0);
    assert (last == 4);
    assert (!s.previous_range (&start, &last));

    assert (s.is_inverted ());
    /* Inverted set returns true for invalid value; oh well. */
    assert (s.has (HB_SET_VALUE_INVALID));
  }

  /* Adding HB_SET_VALUE_INVALID */
  {
    hb_set_t s;

    s.add(HB_SET_VALUE_INVALID);
    assert(!s.has(HB_SET_VALUE_INVALID));

    s.clear();
    assert(!s.add_range(HB_SET_VALUE_INVALID - 2, HB_SET_VALUE_INVALID));
    assert(!s.has(HB_SET_VALUE_INVALID));

    hb_codepoint_t array[] = {(unsigned) HB_SET_VALUE_INVALID, 0, 2};
    s.clear();
    s.add_array(array, 3);
    assert(!s.has(HB_SET_VALUE_INVALID));
    assert(s.has(2));

    hb_codepoint_t sorted_array[] = {0, 2, (unsigned) HB_SET_VALUE_INVALID};
    s.clear();
    s.add_sorted_array(sorted_array, 3);
    assert(!s.has(HB_SET_VALUE_INVALID));
    assert(s.has(2));
  }

  return 0;
}
