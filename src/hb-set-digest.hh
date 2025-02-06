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
#include "hb-machinery.hh"
#include "hb-bit-page.hh"

struct hb_set_digest_t
{
  // Primes that work well modulo 512
  #define HB_SET_DIGEST_PRIMES {2654435761, 2246822519, 3266489917}


  void init () { bits.init0 (); }

  static hb_set_digest_t full ()
  {
    hb_set_digest_t d;
    d.bits.init1 ();
    return d;
  }

  void union_ (const hb_set_digest_t &o)
  { bits |= o.bits; }

  bool add_range (hb_codepoint_t a, hb_codepoint_t b)
  {
    for (hb_codepoint_t g = a; g <= b; g++)
      add (g);
    return true;
  }

  template <typename T>
  void add_array (const T *array, unsigned int count, unsigned int stride=sizeof(T))
  {
    for (unsigned int i = 0; i < count; i++)
    {
      add (*array);
      array = &StructAtOffsetUnaligned<T> ((const void *) array, stride);
    }
  }
  template <typename T>
  void add_array (const hb_array_t<const T>& arr) { add_array (&arr, arr.len ()); }
  template <typename T>
  bool add_sorted_array (const T *array, unsigned int count, unsigned int stride=sizeof(T))
  {
    add_array (array, count, stride);
    return true;
  }
  template <typename T>
  bool add_sorted_array (const hb_sorted_array_t<const T>& arr) { return add_sorted_array (&arr, arr.len ()); }

  bool operator [] (hb_codepoint_t g) const
  { return may_have (g); }


  void add (hb_codepoint_t g)
  {
    constexpr unsigned primes[] = HB_SET_DIGEST_PRIMES;
    for (unsigned i = 0; i < ARRAY_LENGTH (primes); i++)
      bits.add ((g * primes[i]) % bits.BITS);
  }

  bool may_have (hb_codepoint_t g) const
  {
    constexpr unsigned primes[] = HB_SET_DIGEST_PRIMES;
    for (unsigned i = 0; i < ARRAY_LENGTH (primes); i++)
      if (!bits.get ((g * primes[i]) % bits.BITS))
	return false;
    return true;
  }

  bool may_intersect (const hb_set_digest_t &o) const
  {
    return bits.may_intersect (o.bits);
  }

  private:

  hb_bit_page_t bits;
};


#endif /* HB_SET_DIGEST_HH */
