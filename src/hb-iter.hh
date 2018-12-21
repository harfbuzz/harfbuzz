/*
 * Copyright © 2018  Google, Inc.
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

#ifndef HB_ITER_HH
#define HB_ITER_HH

#include "hb.hh"


/* Unified iterator object.
 *
 * The goal of this template is to make the same iterator interface
 * available to all types, and make it very easy and compact to use.
 * hb_iter_tator objects are small, light-weight, objects that can be
 * copied by value.  If the collection / object being iterated on
 * is writable, then the iterator returns lvalues, otherwise it
 * returns rvalues.
 */

/* Base class for all iterators. */
template <typename Iter, typename Item = typename Iter::__item_type__>
struct hb_iter_t
{
  typedef Iter type_t;
  typedef Item item_type_t;

  /* https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern */ 
  const type_t* thiz () const { return static_cast<const type_t *> (this); }
        type_t* thiz ()       { return static_cast<      type_t *> (this); }

  /* Operators. */
  operator type_t () { return iter(); }
  explicit_operator bool () const { return more (); }
  item_type_t& operator * () { return item (); }
  item_type_t& operator [] (unsigned i) { return item (i); }
  type_t& operator += (unsigned count) { forward (count); return *thiz(); }
  type_t& operator ++ () { next (); return *thiz(); }
  type_t& operator -= (unsigned count) { rewind (count); return *thiz(); }
  type_t& operator -- () { prev (); return *thiz(); }
  type_t operator + (unsigned count) { type_t c (*thiz()); c += count; return c; }
  type_t operator ++ (int) { type_t c (*thiz()); ++*thiz(); return c; }
  type_t operator - (unsigned count) { type_t c (*thiz()); c -= count; return c; }
  type_t operator -- (int) { type_t c (*thiz()); --*thiz(); return c; }

  /* Methods. */
  type_t iter () const { return *thiz(); }
  item_type_t& item () const { return thiz()->__item__ (); }
  item_type_t& item_at (unsigned i) const { return thiz()->__item_at__ (i); }
  bool more () const { return thiz()->__more__ (); }
  void next () { thiz()->__next__ (); }
  void forward (unsigned n) { thiz()->__forward__ (n); }
  void prev () { thiz()->__prev__ (); }
  void rewind (unsigned n) { thiz()->__rewind__ (n); }
  unsigned len () const { return thiz()->__len__ (); }

  /*
   * Subclasses overrides:
   */

  /* Access: Implement __item__(), or __item_at__() if random-access. */
  item_type_t& __item__ () const { return thiz()->item_at (0); }
  item_type_t& __item_at__ (unsigned i) const { return *(thiz() + i); }

  /* Termination: Implement __more__() or __end__(). */
  bool __more__ () const { return item () != thiz()->__end__ (); }
  const item_type_t& __end__ () const { return type_t::__sentinel__; }

  /* Advancing: Implement __next__(), or __forward__() if random-access. */
  void __next__ () { thiz()->forward (1); }
  void __forward__ (unsigned n) { while (n--) next (); }

  /* Rewinding: Implement __prev__() or __rewind__() if bidirectional. */
  void __prev__ () { thiz()->rewind (1); }
  void __rewind__ (unsigned n) { while (n--) prev (); }

  /* Population: Implement __len__() if known. */
  unsigned __len__ () const
  { type_t c (*thiz()); unsigned l = 0; while (c) { c++; l++; }; return l; }
};


#endif /* HB_ITER_HH */
