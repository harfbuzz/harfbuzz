/*
 * Copyright © 2019 Adobe Inc.
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
 * Adobe Author(s): Michiharu Ariza
 */

#ifndef HB_BIMAP_HH
#define HB_BIMAP_HH

#include "hb.hh"

/* Bi-directional map.
 * new ids are assigned incrementally & contiguously to old ids
 * which may be added randomly & sparsely
 * all mappings are 1-to-1 in both directions */
struct hb_bimap_t
{
  hb_bimap_t () { init (); }
  ~hb_bimap_t () { fini (); }

  void init (void)
  {
    count = 0;
    old_to_new.init ();
    new_to_old.init ();
  }

  void fini (void)
  {
    old_to_new.fini ();
    new_to_old.fini ();
  }

  bool has (hb_codepoint_t _old) const { return old_to_new.has (_old); }

  hb_codepoint_t add (hb_codepoint_t _old)
  {
    hb_codepoint_t	_new = old_to_new[_old];
    if (_new == HB_MAP_VALUE_INVALID)
    {
      _new = count++;
      old_to_new.set (_old, _new);
      new_to_old.resize (count);
      new_to_old[_new] = _old;
    }
    return _new;
  }

  /* returns HB_MAP_VALUE_INVALID if unmapped */
  hb_codepoint_t operator [] (hb_codepoint_t _old) const { return to_new (_old); }
  hb_codepoint_t to_new (hb_codepoint_t _old) const { return old_to_new[_old]; }
  hb_codepoint_t to_old (hb_codepoint_t _new) const { return (_new >= count)? HB_MAP_VALUE_INVALID: new_to_old[_new]; }

  bool identity (unsigned int size)
  {
    hb_codepoint_t i;
    old_to_new.clear ();
    new_to_old.resize (size);
    for (i = 0; i < size; i++)
    {
      old_to_new.set (i, i);
      new_to_old[i] = i;
    }
    count = i;
    return old_to_new.successful && !new_to_old.in_error ();
  }

  static int cmp_id (const void* a, const void* b)
  { return (int)*(const hb_codepoint_t *)a - (int)*(const hb_codepoint_t *)b; }

  /* Optional: after finished adding all mappings in a random order,
   * reassign new ids to old ids so that both are in the same order. */
  void reorder (void)
  {
    new_to_old.qsort (cmp_id);
    for (hb_codepoint_t _new = 0; _new < count; _new++)
      old_to_new.set (to_old (_new), _new);
  }

  unsigned int get_count () const { return count; }

  protected:
  unsigned int  count;
  hb_map_t	old_to_new;
  hb_vector_t<hb_codepoint_t>
  		new_to_old;
};

#endif /* HB_BIMAP_HH */
