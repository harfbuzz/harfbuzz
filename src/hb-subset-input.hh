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

#include "hb-font.hh"

HB_MARK_AS_FLAG_T (hb_subset_flags_t);

struct hb_subset_input_t
{
  hb_object_header_t header;

  hb_set_t *unicodes;         // invert safe
  hb_set_t *glyphs;           // invert safe
  hb_set_t *name_ids;         // invert safe
  hb_set_t *name_languages;   // invert safe
  hb_set_t *no_subset_tables; // invert safe
  hb_set_t *drop_tables;      // invert safe
  hb_set_t *layout_features;  // invert safe

  unsigned flags;

  /* TODO
   *
   * features
   * lookups
   * name_ids
   * ...
   */
};


#endif /* HB_SUBSET_INPUT_HH */
