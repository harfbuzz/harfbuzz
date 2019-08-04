/*
 * Copyright © 2019  Ebrahim Byagowi
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
 */

#include "hb.hh"

#ifndef HB_NO_STYLE

#include "hb-aat-fdsc-table.hh"
#include "hb-ot-var-avar-table.hh"
#include "hb-ot-var-fvar-table.hh"
#include "hb-ot-stat-table.hh"
#include "hb-ot-os2-table.hh"
#include "hb-ot-head-table.hh"
#include "hb-ot-post-table.hh"
#include "hb-ot-face.hh"

/**
 * hb_style_get_value:
 * @font: a #hb_font_t object.
 * @style_tag: a style tag.
 *
 * Searches variation axes of a hb_font_t object for a specific axis first,
 * if not set, then tries to get default style values from different
 * tables of the font.
 *
 * Returns: Corresponding axis or default value to a style tag.
 *
 * Since: REPLACEME
 **/
float
hb_style_get_value (hb_font_t *font, hb_style_tag_t style_tag)
{
  hb_face_t *face = font->face;

#ifndef HB_NO_VAR
  hb_ot_var_axis_info_t axis;
  if (hb_ot_var_find_axis_info (face, style_tag, &axis))
  {
    if (axis.axis_index < font->num_coords) return font->design_coords[axis.axis_index];
    /* If a face is variable, fvar's default_value is better than STAT records */
    return axis.default_value;
  }
#endif

  /* STAT */
  float value;
  if (face->table.STAT->get_value (style_tag, &value))
    return value;

  /* Check Apple's fdsc as OS2 table is optional in AAT */
  const AAT::FontDescriptor &descriptor = face->table.fdsc->get_descriptor (style_tag);
  if (descriptor.has_data ())
  {
    float value = descriptor.get_value ();
    /* https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6fdsc.html */
    /* Percent weight relative to regular weight. */
    if (style_tag == HB_STYLE_TAG_WEIGHT) value *= 400.f;
    /* Percent width relative to regular width. */
    if (style_tag == HB_STYLE_TAG_WIDTH) value *= 100.f;
    return value;
  }

  switch ((unsigned) style_tag)
  {
  case HB_STYLE_TAG_ITALIC:
    return face->table.OS2->is_italic () || face->table.head->is_italic () ? 1 : 0;
  case HB_STYLE_TAG_OPTICAL_SIZE:
  {
    unsigned int lower, upper;
    return face->table.OS2->v5 ().get_optical_size (&lower, &upper)
	   ? (float) (lower + upper) / 2.f
	   : 12.f;
  }
  case HB_STYLE_TAG_SLANT:
    return face->table.post->table->italicAngle.to_float ();
  case HB_STYLE_TAG_WIDTH:
    return face->table.OS2->has_data ()
	   ? face->table.OS2->get_width ()
	   : (face->table.head->is_condensed () ? 75 : 100);
  case HB_STYLE_TAG_WEIGHT:
    return face->table.OS2->has_data ()
	   ? face->table.OS2->usWeightClass
	   : (face->table.head->is_bold () ? 700 : 400);
  default:
    return 0;
  }
}

#endif
