/*
 * Copyright © 2009,2010  Red Hat, Inc.
 * Copyright © 2010  Google, Inc.
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
 * Red Hat Author(s): Behdad Esfahbod
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_OT_MAP_PRIVATE_HH
#define HB_OT_MAP_PRIVATE_HH

#include "hb-buffer-private.hh"

#include "hb-ot-layout.h"

HB_BEGIN_DECLS


static const hb_tag_t table_tags[2] = {HB_OT_TAG_GSUB, HB_OT_TAG_GPOS};

struct hb_ot_map_builder_t
{
  public:

  inline void add_feature (hb_tag_t tag, unsigned int value, bool global)
  {
    feature_info_t *info = feature_infos.push();
    if (unlikely (!info)) return;
    info->tag = tag;
    info->seq = feature_infos.len;
    info->max_value = value;
    info->global = global;
    info->default_value = global ? value : 0;
  }

  inline void add_bool_feature (hb_tag_t tag, bool global = true)
  { add_feature (tag, 1, global); }

  HB_INTERNAL void compile (hb_face_t *face,
			    const hb_segment_properties_t *props,
			    struct hb_ot_map_t &m);

  inline void finish (void) {
    feature_infos.finish ();
  }

  private:

  struct feature_info_t {
    hb_tag_t tag;
    unsigned int seq; /* sequence#, used for stable sorting only */
    unsigned int max_value;
    bool global; /* whether the feature applies value to every glyph in the buffer */
    unsigned int default_value; /* for non-global features, what should the unset glyphs take */

    static int cmp (const feature_info_t *a, const feature_info_t *b)
    { return (a->tag != b->tag) ?  (a->tag < b->tag ? -1 : 1) : (a->seq < b->seq ? -1 : 1); }
  };

  hb_prealloced_array_t<feature_info_t,16> feature_infos; /* used before compile() only */
};


struct hb_ot_map_t
{
  friend struct hb_ot_map_builder_t;

  public:

  inline hb_mask_t get_global_mask (void) const { return global_mask; }

  inline hb_mask_t get_mask (hb_tag_t tag, unsigned int *shift = NULL) const {
    const feature_map_t *map = features.bsearch (&tag);
    if (shift) *shift = map ? map->shift : 0;
    return map ? map->mask : 0;
  }

  inline hb_mask_t get_1_mask (hb_tag_t tag) const {
    const feature_map_t *map = features.bsearch (&tag);
    return map ? map->_1_mask : 0;
  }

  inline void substitute (hb_face_t *face, hb_buffer_t *buffer) const {
    for (unsigned int i = 0; i < lookups[0].len; i++)
      hb_ot_layout_substitute_lookup (face, buffer, lookups[0][i].index, lookups[0][i].mask);
  }

  inline void position (hb_font_t *font, hb_face_t *face, hb_buffer_t *buffer) const {
    for (unsigned int i = 0; i < lookups[1].len; i++)
      hb_ot_layout_position_lookup (font, buffer, lookups[1][i].index, lookups[1][i].mask);
  }

  inline void finish (void) {
    features.finish ();
    lookups[0].finish ();
    lookups[1].finish ();
  }

  private:

  struct feature_map_t {
    hb_tag_t tag; /* should be first for our bsearch to work */
    unsigned int index[2]; /* GSUB, GPOS */
    unsigned int shift;
    hb_mask_t mask;
    hb_mask_t _1_mask; /* mask for value=1, for quick access */

    static int cmp (const feature_map_t *a, const feature_map_t *b)
    { return a->tag < b->tag ? -1 : a->tag > b->tag ? 1 : 0; }
  };

  struct lookup_map_t {
    unsigned int index;
    hb_mask_t mask;

    static int cmp (const lookup_map_t *a, const lookup_map_t *b)
    { return a->index < b->index ? -1 : a->index > b->index ? 1 : 0; }
  };

  HB_INTERNAL void add_lookups (hb_face_t    *face,
				unsigned int  table_index,
				unsigned int  feature_index,
				hb_mask_t     mask);


  hb_mask_t global_mask;

  hb_prealloced_array_t<feature_map_t, 8> features;
  hb_prealloced_array_t<lookup_map_t, 32> lookups[2]; /* GSUB/GPOS */
};


HB_END_DECLS

#endif /* HB_OT_MAP_PRIVATE_HH */
