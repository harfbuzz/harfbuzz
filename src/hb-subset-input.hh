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

struct hb_subset_input_t
{
  hb_object_header_t header;

  hb_set_t *unicodes;
  hb_set_t *glyphs;
  hb_set_t *name_ids;
  hb_set_t *name_languages;
  hb_set_t *drop_tables;
  hb_set_t *layout_features;

  //use hb_bool_t to be consistent with G option parser
  hb_bool_t drop_hints;
  hb_bool_t desubroutinize;
  hb_bool_t retain_gids;
  hb_bool_t name_legacy;
  hb_bool_t overlaps_flag;
  hb_bool_t notdef_outline;
  hb_bool_t no_prune_unicode_ranges;
  hb_bool_t retain_all_layout_features;
  /* TODO
   *
   * features
   * lookups
   * name_ids
   * ...
   */
};


#endif /* HB_SUBSET_INPUT_HH */
