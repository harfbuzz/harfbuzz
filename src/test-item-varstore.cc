/*
 * Copyright © 2020  Google, Inc.
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
 */
#include "hb-ot-var-common.hh"
#include "hb-ot-var-hvar-table.hh"
// HVAR table data from SourceSerif4Variable-Roman_subset.otf
const char hvar_data[] = "\x0\x1\x0\x0\x0\x0\x0\x14\x0\x0\x0\xc4\x0\x0\x0\x0\x0\x0\x0\x0\x0\x1\x0\x0\x0\x10\x0\x2\x0\x0\x0\x74\x0\x0\x0\x7a\x0\x2\x0\x8\xc0\x0\xc0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x40\x0\x40\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\xc0\x0\xc0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x40\x0\x40\x0\xc0\x0\xc0\x0\x0\x0\xc0\x0\xc0\x0\x0\x0\xc0\x0\xc0\x0\x0\x0\x0\x0\x40\x0\x40\x0\x0\x0\x40\x0\x40\x0\xc0\x0\xc0\x0\x0\x0\x0\x0\x40\x0\x40\x0\x0\x0\x40\x0\x40\x0\x0\x1\x0\x0\x0\x0\x0\x4\x0\x0\x0\x8\x0\x0\x0\x1\x0\x2\x0\x3\x0\x4\x0\x5\x0\x6\x0\x7\xf9\xf\x2f\xbf\xfb\xfb\x35\xf9\x4\x4\xf3\xb4\xf2\xfb\x2e\xf3\x4\x4\xe\xad\xfa\x1\x1a\x1\x15\x22\x59\xd6\xe3\xf6\x6\xf5\x0\x1\x0\x5\x0\x4\x7\x5\x6";

static void
test_item_variations ()
{
  const OT::HVAR* hvar_table = reinterpret_cast<const OT::HVAR*> (hvar_data);

  hb_tag_t axis_tag = HB_TAG ('w', 'g', 'h', 't');
  hb_map_t axis_idx_tag_map;
  axis_idx_tag_map.set (0, axis_tag);

  axis_tag = HB_TAG ('o', 'p', 's', 'z');
  axis_idx_tag_map.set (1, axis_tag);

  OT::item_variations_t item_vars;
  const OT::ItemVariationStore& src_var_store = hvar_table+(hvar_table->varStore);
  bool result = item_vars.create_from_item_varstore (src_var_store, axis_idx_tag_map);
      
  assert (result);

  /* partial instancing wght=300:800 */
  hb_hashmap_t<hb_tag_t, Triple> normalized_axes_location;
  normalized_axes_location.set (axis_tag, Triple (-0.512817, 0.0, 0.7000120));

  hb_hashmap_t<hb_tag_t, TripleDistances> axes_triple_distances;
  axes_triple_distances.set (axis_tag, TripleDistances (200.0, 500.0));

  result = item_vars.instantiate_tuple_vars (normalized_axes_location, axes_triple_distances);
  assert (result);
  result = item_vars.as_item_varstore (false);
  assert (result);
  assert (item_vars.get_region_list().length == 8);
}

int
main (int argc, char **argv)
{
  test_item_variations ();
}
