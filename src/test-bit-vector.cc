/*
 * Copyright © 2026  Behdad Esfahbod
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
 * Author(s): Behdad Esfahbod
 */

#define hb_calloc bit_vector_test_calloc

#include "hb-bit-vector.hh"

static bool fail_allocation;
static size_t last_allocation;

extern "C" void *
bit_vector_test_calloc (size_t count, size_t size)
{
  last_allocation = count * size;
  return fail_allocation ? nullptr : calloc (count, size);
}

int
main ()
{
  /* Check every bit offset, including ranges that wrap within a word
   * or cross a word boundary relative to an unaligned minimum. */
  for (unsigned bias = 0; bias < 64; bias++)
    for (unsigned first = 0; first < 130; first++)
      for (unsigned last = first; last < 130; last++)
      {
        hb_bit_vector_t<> bits (bias, bias + 129);
        hb_always_assert (!bits.in_error ());
        hb_always_assert (bits.add_range (bias + first, bias + last));
        for (unsigned i = 0; i < 130; i++)
          hb_always_assert (bits.has (bias + i) == (first <= i && i <= last));
        bits.del_range (bias + first, bias + last);
        for (unsigned i = 0; i < 130; i++)
          hb_always_assert (!bits.has (bias + i));
        bits.add_range (bias, bias + 129);
        bits.del_range (bias + first, bias + last);
        for (unsigned i = 0; i < 130; i++)
          hb_always_assert (bits.has (bias + i) == (i < first || i > last));
      }

  for (unsigned value : {0u, 37u, UINT_MAX})
  {
    hb_bit_vector_t<> bits (value, value);
    hb_always_assert (!bits.in_error ());
    hb_always_assert (bits.count == 1 && last_allocation == 8);
    hb_always_assert (!bits.has (value));
    bits.add (value);
    hb_always_assert (bits.has (value));
    bits.del (value);
    hb_always_assert (!bits.has (value));
  }

  for (unsigned size : {1u, 64u, 65u, 512u})
  {
    hb_bit_vector_t<> bits (37, 37 + size - 1);
    hb_always_assert (last_allocation == (size + 63) / 64 * 8);
    bits.add_range (37, 37 + size - 1);
    hb_always_assert (!bits.has (36) && !bits.has (37 + size));
    hb_always_assert (!bits.has (UINT_MAX));
    bits.add (36);
    bits.del (37 + size);
    hb_always_assert (bits.has (37) && bits.has (37 + size - 1));
    hb_bit_vector_t<> moved (std::move (bits));
    hb_always_assert (bits.in_error () && moved.has (37));
    bits = std::move (moved);
    hb_always_assert (bits.has (37) && moved.in_error ());
  }

  /* Empty/reversed and unrepresentable inclusive ranges must not allocate. */
  last_allocation = 0;
  hb_bit_vector_t<> empty (1, 0);
  hb_bit_vector_t<> overflow (0, UINT_MAX);
  hb_always_assert (empty.in_error () && overflow.in_error ());
  hb_always_assert (!last_allocation);

  fail_allocation = true;
  hb_bit_vector_t<> failed (37, 102);
  hb_always_assert (failed.in_error () && !failed.count);
  hb_always_assert (!failed.has (37));
  failed.add (37);
  hb_always_assert (!failed.has (37));
  hb_always_assert (!failed.add_range (37, 102));
  fail_allocation = false;

  const unsigned values[] = {37, 63, 64, 65, 100};
  auto array = hb_sorted_array (values);
  hb_min_max_t extents;
  hb_always_assert (extents.add_range (100, 37));
  hb_always_assert (extents.get_min () == HB_CODEPOINT_INVALID);
  hb_always_assert (extents.add_sorted_array (array));
  hb_always_assert (extents.get_min () == 37 && extents.get_max () == 100);
  hb_bit_vector_t<> bits (extents.get_min (), extents.get_max ());
  hb_always_assert (bits.add_sorted_array (array));
  for (unsigned i = 0; i < 110; i++)
    hb_always_assert (bits.has (i) == (i == 37 || i == 63 || i == 64 || i == 65 || i == 100));
}
