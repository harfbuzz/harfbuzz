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
#include "hb-ot-shape-complex-private.hh"



/*
 * GDEF
 */

/* XXX cleanup */
typedef enum {
  HB_OT_LAYOUT_GLYPH_CLASS_UNCLASSIFIED	= 0x0001,
  HB_OT_LAYOUT_GLYPH_CLASS_BASE_GLYPH	= 0x0002,
  HB_OT_LAYOUT_GLYPH_CLASS_LIGATURE	= 0x0004,
  HB_OT_LAYOUT_GLYPH_CLASS_MARK		= 0x0008,
  HB_OT_LAYOUT_GLYPH_CLASS_COMPONENT	= 0x0010
} hb_ot_layout_glyph_class_t;


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
 * GSUB/GPOS
 */

/* lig_id / lig_comp
 *
 * When a ligature is formed:
 *
 *   - The ligature glyph and any marks in between all get a unique lig_id,
 *   - The ligature glyph will get lig_comp = 0
 *   - The marks get lig_comp > 0, reflecting which component of the ligature
 *     they were applied to.
 *   - This is used in GPOS to attach marks to the right component of a ligature
 *     in MarkLigPos.
 *
 * When a multiple-substitution is done:
 *
 *   - All resulting glyphs will have lig_id = 0,
 *   - The resulting glyphs will have lig_comp = 0, 1, 2, ... respectively.
 *   - This is used in GPOS to attack marks to the first component of a
 *     multiple substitution in MarkBasePos.
 *
 * The numbers are also used in GPOS to do mark-to-mark positioning only
 * to marks that belong to the same component of a ligature in MarkMarPos.
 */
static inline void
set_lig_props (hb_glyph_info_t &info, unsigned int lig_id, unsigned int lig_comp)
{
  info.lig_props() = (lig_id << 4) | (lig_comp & 0x0F);
}
static inline unsigned int
get_lig_id (const hb_glyph_info_t &info)
{
  return info.lig_props() >> 4;
}
static inline unsigned int
get_lig_comp (const hb_glyph_info_t &info)
{
  return info.lig_props() & 0x0F;
}
static inline bool
is_a_ligature (const hb_glyph_info_t &info)
{
  return unlikely (get_lig_id (info) && ~get_lig_comp (info));
}

static inline uint8_t allocate_lig_id (hb_buffer_t *buffer) {
  uint8_t lig_id = buffer->next_serial () & 0x0F;
  if (unlikely (!lig_id))
    lig_id = allocate_lig_id (buffer); /* in case of overflow */
  return lig_id;
}



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



#endif /* HB_OT_LAYOUT_PRIVATE_HH */
