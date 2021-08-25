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
 * Google Author(s): Garret Rieger, Roderick Sheeter
 */

#ifndef HB_SUBSET_INPUT_HH
#define HB_SUBSET_INPUT_HH


#include "hb.hh"

#include "hb-subset.h"
#include "hb-map.hh"
#include "hb-set.hh"

#include "hb-font.hh"

HB_MARK_AS_FLAG_T (hb_subset_flags_t);

struct hb_subset_input_t
{
  hb_object_header_t header;

  hb_hashmap_t<unsigned, hb_set_t*> sets;

  unsigned flags;

  inline hb_set_t* unicodes()
  {
    return sets.get (HB_SUBSET_SETS_UNICODE);
  }

  inline const hb_set_t* unicodes() const
  {
    return sets.get (HB_SUBSET_SETS_UNICODE);
  }

  inline hb_set_t* glyphs ()
  {
    return sets.get (HB_SUBSET_SETS_GLYPH_INDEX);
  }

  inline const hb_set_t* glyphs () const
  {
    return sets.get (HB_SUBSET_SETS_GLYPH_INDEX);
  }

  inline hb_set_t* name_ids ()
  {
    return sets.get (HB_SUBSET_SETS_NAME_ID);
  }

  inline const hb_set_t* name_ids () const
  {
    return sets.get (HB_SUBSET_SETS_NAME_ID);
  }

  inline hb_set_t* name_languages ()
  {
    return sets.get (HB_SUBSET_SETS_NAME_LANG_ID);
  }

  inline const hb_set_t* name_languages () const
  {
    return sets.get (HB_SUBSET_SETS_NAME_LANG_ID);
  }

  inline hb_set_t* no_subset_tables ()
  {
    return sets.get (HB_SUBSET_SETS_NO_SUBSET_TABLE_TAG);
  }

  inline const hb_set_t* no_subset_tables () const
  {
    return sets.get (HB_SUBSET_SETS_NO_SUBSET_TABLE_TAG);
  }

  inline hb_set_t* drop_tables ()
  {
    return sets.get (HB_SUBSET_SETS_DROP_TABLE_TAG);
  }

  inline const hb_set_t* drop_tables () const
  {
    return sets.get (HB_SUBSET_SETS_DROP_TABLE_TAG);
  }

  inline hb_set_t* layout_features ()
  {
    return sets.get (HB_SUBSET_SETS_LAYOUT_FEATURE_TAG);
  }

  inline const hb_set_t* layout_features () const
  {
    return sets.get (HB_SUBSET_SETS_LAYOUT_FEATURE_TAG);
  }

  bool in_error () const
  {
    if (sets.in_error ()) return true;
    for (const hb_set_t* set : sets.values ())
    {
      if (unlikely (set->in_error ()))
        return true;
    }
    return false;
  }
};


#endif /* HB_SUBSET_INPUT_HH */
