/*
 * Copyright © 2012,2017  Google, Inc.
 * Copyright © 2021 Behdad Esfahbod
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

#ifndef HB_BIT_SET_INVERTIBLE_HH
#define HB_BIT_SET_INVERTIBLE_HH

#include "hb.hh"
#include "hb-bit-set.hh"


struct hb_bit_set_invertible_t
{
  hb_bit_set_t s;
  bool inverted;

  hb_bit_set_invertible_t () { init (); }
  ~hb_bit_set_invertible_t () { fini (); }

  void init () { s.init (); inverted = false; }
  void fini () { s.fini (); }
  void err () { s.err (); }
  bool in_error () const { return s.in_error (); }
  explicit operator bool () const { return !is_empty (); }

  void reset () { s.reset (); inverted = false; }
  void clear () { s.clear (); inverted = false; }
  void invert () { if (!s.in_error ()) inverted = !inverted; }

  bool is_empty () const { return inverted ? /*XXX*/false : s.is_empty (); }

  void add (hb_codepoint_t g) { inverted ? s.del (g) : s.add (g); }
  bool add_range (hb_codepoint_t a, hb_codepoint_t b) { return inverted ? (s.del_range (a, b), true) : s.add_range (a, b); }

  template <typename T>
  void add_array (const T *array, unsigned int count, unsigned int stride=sizeof(T))
  { /*XXX(inverted)*/s.add_array (array, count, stride); }
  template <typename T>
  void add_array (const hb_array_t<const T>& arr) { add_array (&arr, arr.len ()); }

  /* Might return false if array looks unsorted.
   * Used for faster rejection of corrupt data. */
  template <typename T>
  bool add_sorted_array (const T *array, unsigned int count, unsigned int stride=sizeof(T))
  { return /*XXX(inverted)*/s.add_sorted_array (array, count, stride); }
  template <typename T>
  bool add_sorted_array (const hb_sorted_array_t<const T>& arr) { return add_sorted_array (&arr, arr.len ()); }

  void del (hb_codepoint_t g) { inverted ? s.add (g) : s.del (g); }
  void del_range (hb_codepoint_t a, hb_codepoint_t b) { inverted ? (void) s.add_range (a, b) : s.del_range (a, b); }

  bool get (hb_codepoint_t g) const { return s.get (g) ^ inverted; }

  /* Has interface. */
  static constexpr bool SENTINEL = false;
  typedef bool value_t;
  value_t operator [] (hb_codepoint_t k) const { return get (k); }
  bool has (hb_codepoint_t k) const { return (*this)[k] != SENTINEL; }
  /* Predicate. */
  bool operator () (hb_codepoint_t k) const { return has (k); }

  /* Sink interface. */
  hb_bit_set_invertible_t& operator << (hb_codepoint_t v)
  { add (v); return *this; }
  hb_bit_set_invertible_t& operator << (const hb_pair_t<hb_codepoint_t, hb_codepoint_t>& range)
  { add_range (range.first, range.second); return *this; }

  bool intersects (hb_codepoint_t first, hb_codepoint_t last) const
  { return /*XXX(inverted)*/s.intersects (first, last); }

  void set (const hb_bit_set_invertible_t &other) { s.set (other.s); inverted = other.inverted; }

  bool is_equal (const hb_bit_set_invertible_t &other) const
  { return inverted == other.inverted /*XXX*/ && s.is_equal (other.s); }

  bool is_subset (const hb_bit_set_invertible_t &larger_set) const
  {
    if (inverted && !larger_set.inverted) return false; /*XXX*/
    if (!inverted && larger_set.inverted)
    {
      /* TODO(iter) Write as hb_all dagger. */
      for (auto c: s)
        if (larger_set.s.has (c))
	  return false;
      return true;
    }
    /* inverted == larger_set.inverted */
    return inverted ? larger_set.s.is_subset (s) : s.is_subset (larger_set.s);
  }

  template <typename Op>
  void process (const Op& op, const hb_bit_set_invertible_t &other)
  { s.process (op, other.s); }

  /*XXX(inverted)*/
  void union_ (const hb_bit_set_invertible_t &other) { process (hb_bitwise_or, other); }
  /*XXX(inverted)*/
  void intersect (const hb_bit_set_invertible_t &other) { process (hb_bitwise_and, other); }
  /*XXX(inverted)*/
  void subtract (const hb_bit_set_invertible_t &other) { process (hb_bitwise_sub, other); }
  /*XXX(inverted)*/
  void symmetric_difference (const hb_bit_set_invertible_t &other) { process (hb_bitwise_xor, other); }

  bool next (hb_codepoint_t *codepoint) const
  {
    /*XXX(inverted)*/
    return s.next (codepoint);
  }
  bool previous (hb_codepoint_t *codepoint) const
  {
    /*XXX(inverted)*/
    return s.previous (codepoint);
  }
  bool next_range (hb_codepoint_t *first, hb_codepoint_t *last) const
  {
    /*XXX(inverted)*/
    return s.next_range (first, last);
  }
  bool previous_range (hb_codepoint_t *first, hb_codepoint_t *last) const
  {
    /*XXX(inverted)*/
    return s.previous_range (first, last);
  }

  unsigned int get_population () const
  { return inverted ? INVALID - s.get_population () : s.get_population (); }

  hb_codepoint_t get_min () const
  { return /*XXX(inverted)*/s.get_min (); }
  hb_codepoint_t get_max () const
  { return /*XXX(inverted)*/s.get_max (); }

  static constexpr hb_codepoint_t INVALID = hb_bit_set_t::INVALID;

  /*
   * Iterator implementation.
   */
  /*XXX(inverted)*/
  using iter_t = hb_bit_set_t::iter_t;
  iter_t iter () const { return iter_t (this->s); }
  operator iter_t () const { return iter (); }
};


#endif /* HB_BIT_SET_INVERTIBLE_HH */
