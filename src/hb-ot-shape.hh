/*
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
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_OT_SHAPE_HH
#define HB_OT_SHAPE_HH

#include "hb.hh"

#include "hb-ot-map.hh"
#include "hb-shape-plan.hh"



struct hb_ot_shape_plan_t
{
  hb_segment_properties_t props;
  const struct hb_ot_complex_shaper_t *shaper;
  hb_ot_map_t map;
  const void *data;
  hb_mask_t rtlm_mask, frac_mask, numr_mask, dnom_mask;
  hb_mask_t kern_mask;

  bool requested_kerning : 1;
  bool has_frac : 1;
  bool has_gpos_mark : 1;
  bool fallback_glyph_classes : 1;
  bool fallback_kerning : 1;
  bool fallback_mark_positioning : 1;

  bool apply_gpos : 1;
  bool apply_kerx : 1;
  bool apply_kern : 1;
  bool apply_morx : 1;
  bool apply_trak : 1;


  inline void collect_lookups (hb_tag_t table_tag, hb_set_t *lookups) const
  {
    unsigned int table_index;
    switch (table_tag) {
      case HB_OT_TAG_GSUB: table_index = 0; break;
      case HB_OT_TAG_GPOS: table_index = 1; break;
      default: return;
    }
    map.collect_lookups (table_index, lookups);
  }
  inline void substitute (hb_font_t *font, hb_buffer_t *buffer) const { map.substitute (this, font, buffer); }
  inline void position (hb_font_t *font, hb_buffer_t *buffer) const { map.position (this, font, buffer); }

  void init (void)
  {
    memset (this, 0, sizeof (*this));
    map.init ();
  }
  void fini (void) {
    map.fini ();
  }
};

struct hb_ot_shape_planner_t
{
  /* In the order that they are filled in. */
  hb_face_t *face;
  hb_segment_properties_t props;
  const struct hb_ot_complex_shaper_t *shaper;
  hb_ot_map_builder_t map;

  hb_ot_shape_planner_t (const hb_shape_plan_t *master_plan) :
			 face (master_plan->face_unsafe),
			 props (master_plan->props),
			 shaper (nullptr),
			 map (face, &props) {}

  HB_INTERNAL void compile (hb_ot_shape_plan_t &plan,
			    const int          *coords,
			    unsigned int        num_coords);

  private:
  HB_DISALLOW_COPY_AND_ASSIGN (hb_ot_shape_planner_t);
};


#endif /* HB_OT_SHAPE_HH */
