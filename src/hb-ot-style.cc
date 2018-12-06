/*
 * Copyright © 2018  Ebrahim Byagowi
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

#include "hb-ot-face.hh"
#include "hb-aat-fdsc-table.hh"
#include "hb-ot-os2-table.hh"
#include "hb-ot-head-table.hh"
#include "hb-ot-stat-table.hh"


/**
 * hb_ot_style_get:
 *
 * Since: REPLACEME
 **/
float
hb_ot_style_get (hb_face_t *face, hb_ot_style_t style)
{
  /* fvar */
  {
    hb_ot_var_axis_info_t info;
    if (hb_ot_var_find_axis_info (face, style, &info))
      return info.default_value;
  }

  /* STAT */
  // {
  //   const OT::AxisValue &axis_value = face->table.STAT->get_axis_value (style);
  //   float value;
  //   if (axis_value.get_non_var_value (&value)) return value;
  // }

  /* fdsc */
  {
    const AAT::FontDescriptor &descriptor = face->table.fdsc->get_descriptor (style);
    if (descriptor.has_data ())
    {
      float value = descriptor.get_value ();
      if (style == HB_OT_STYLE_WIDTH) value *= 100.f;
      if (style == HB_OT_STYLE_WEIGHT) value *= 400.f;
      return value;
    }
  }

  /* OS/2, head */
  switch (style)
  {
  case HB_OT_STYLE_ITALIC:
    return face->table.OS2->is_italic () || face->table.head->is_italic () ? 1.f : 0.f;
  case HB_OT_STYLE_OPTICAL_SIZE:
    return face->table.OS2->v5 ().get_optical_size (); // returns 12.f if nothing found
  case HB_OT_STYLE_SLANT:
    /* What should be put when font is oblique? */
    return face->table.OS2->is_oblique () ? 6.f : 0.f;
  case HB_OT_STYLE_WIDTH:
    if (face->table.OS2->has_data ()) return face->table.OS2->get_width ();
    return face->table.head->is_condensed () ? 75.f : 100.f;
  case HB_OT_STYLE_WEIGHT:
    if (face->table.OS2->has_data ()) return face->table.OS2->usWidthClass;
    return face->table.head->is_bold () ? 700.f : 400.f;
  }
}
