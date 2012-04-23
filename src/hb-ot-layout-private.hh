/*
 * Copyright © 2007,2008,2009  Red Hat, Inc.
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
 */

#ifndef HB_OT_LAYOUT_PRIVATE_HH
#define HB_OT_LAYOUT_PRIVATE_HH

#include "hb-private.hh"

#include "hb-ot-layout.h"

#include "hb-font-private.hh"
#include "hb-buffer-private.hh"



/*
 * GDEF
 */

/* buffer var allocations */
#define props_cache() var1.u16[1] /* glyph_props cache */

/* XXX cleanup */
typedef enum {
  HB_OT_LAYOUT_GLYPH_CLASS_UNCLASSIFIED	= 0x0001,
  HB_OT_LAYOUT_GLYPH_CLASS_BASE_GLYPH	= 0x0002,
  HB_OT_LAYOUT_GLYPH_CLASS_LIGATURE	= 0x0004,
  HB_OT_LAYOUT_GLYPH_CLASS_MARK		= 0x0008,
  HB_OT_LAYOUT_GLYPH_CLASS_COMPONENT	= 0x0010
} hb_ot_layout_glyph_class_t;


HB_INTERNAL unsigned int
_hb_ot_layout_get_glyph_property (hb_face_t       *face,
				  hb_glyph_info_t *info);

HB_INTERNAL hb_bool_t
_hb_ot_layout_check_glyph_property (hb_face_t    *face,
				    hb_glyph_info_t *ginfo,
				    unsigned int  lookup_props,
				    unsigned int *property_out);

HB_INTERNAL hb_bool_t
_hb_ot_layout_skip_mark (hb_face_t    *face,
			 hb_glyph_info_t *ginfo,
			 unsigned int  lookup_props,
			 unsigned int *property_out);



/*
 * hb_ot_layout_t
 */

struct hb_ot_layout_t
{
  hb_blob_t *gdef_blob;
  hb_blob_t *gsub_blob;
  hb_blob_t *gpos_blob;

  const struct GDEF *gdef;
  const struct GSUB *gsub;
  const struct GPOS *gpos;
};


HB_INTERNAL hb_ot_layout_t *
_hb_ot_layout_create (hb_face_t *face);

HB_INTERNAL void
_hb_ot_layout_destroy (hb_ot_layout_t *layout);



struct _hb_glyph_map_t
{
  void clear (void) {
    memset (elts, 0, sizeof elts);
  }
  bool has (hb_codepoint_t g) const {
    if (unlikely (g > MAX_G)) return false;
    return !!(elt (g) & mask (g));
  }
  bool add (hb_codepoint_t g) {
    if (unlikely (g > MAX_G)) return false;
    elt_t &e = elt (g);
    elt_t m = mask (g);
    bool ret = !!(e & m);
    e |= m;
    return ret;
  }

  private:
  typedef uint32_t elt_t;
  static const unsigned int MAX_G = 65536 - 1;
  static const unsigned int SHIFT = 5;
  static const unsigned int BITS = (1 << SHIFT);
  static const unsigned int MASK = BITS - 1;

  elt_t &elt (hb_codepoint_t g) { return elts[g >> SHIFT]; }
  elt_t elt (hb_codepoint_t g) const { return elts[g >> SHIFT]; }
  elt_t mask (hb_codepoint_t g) const { return elt_t (1) << (g & MASK); }

  elt_t elts[(MAX_G + 1 + (BITS - 1)) / BITS]; /* 8kb */
  ASSERT_STATIC (sizeof (elt_t) * 8 == BITS);
  ASSERT_STATIC (sizeof (elts) * 8 > MAX_G);
};

#endif /* HB_OT_LAYOUT_PRIVATE_HH */
