/*
 * Copyright (C) 2007,2008  Red Hat, Inc.
 *
 *  This is part of HarfBuzz, an OpenType Layout engine library.
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

#define HB_OT_LAYOUT_CC

#include "hb-ot-layout.h"
#include "hb-ot-layout-private.h"

#include "hb-ot-layout-open-private.h"
#include "hb-ot-layout-gdef-private.h"
#include "hb-ot-layout-gsub-private.h"

#include <stdlib.h>
#include <string.h>


HB_OT_Layout *
hb_ot_layout_create (const char *font_data,
                     int         face_index)
{
  HB_OT_Layout *layout = (HB_OT_Layout *) calloc (1, sizeof (HB_OT_Layout));

  const OpenTypeFontFile &font = OpenTypeFontFile::get_for_data (font_data);
  const OpenTypeFontFace &face = font.get_face (face_index);

  layout->gdef = &GDEF::get_for_data (font.get_table_data (face.get_table (GDEF::Tag)));
  layout->gsub = &GSUB::get_for_data (font.get_table_data (face.get_table (GSUB::Tag)));
//layout->gpos = &GPOS::get_for_data (font.get_table_data (face.get_table (GPOS::Tag)));

  return layout;
}

void
hb_ot_layout_destroy (HB_OT_Layout *layout)
{
  free (layout);
}

static hb_ot_layout_glyph_properties_t
_hb_ot_layout_get_glyph_properties (HB_OT_Layout         *layout,
				    hb_ot_layout_glyph_t  glyph)
{
  hb_ot_layout_class_t klass;

  /* TODO old harfbuzz doesn't always parse mark attachments as it says it was
   * introduced without a version bump, so it may not be safe */
  klass = layout->gdef->get_mark_attachment_type (glyph);
  if (klass)
    return klass << 8;

  klass = layout->gdef->get_glyph_class (glyph);

  if (!klass && glyph < layout->new_gdef.len)
    klass = layout->new_gdef.klasses[glyph];

  switch (klass) {
  default:
  case GDEF::UnclassifiedGlyph:	return HB_OT_LAYOUT_GLYPH_CLASS_UNCLASSIFIED;
  case GDEF::BaseGlyph:		return HB_OT_LAYOUT_GLYPH_CLASS_BASE_GLYPH;
  case GDEF::LigatureGlyph:	return HB_OT_LAYOUT_GLYPH_CLASS_LIGATURE;
  case GDEF::MarkGlyph:		return HB_OT_LAYOUT_GLYPH_CLASS_MARK;
  case GDEF::ComponentGlyph:	return HB_OT_LAYOUT_GLYPH_CLASS_COMPONENT;
  }
}

hb_ot_layout_glyph_class_t
hb_ot_layout_get_glyph_class (HB_OT_Layout         *layout,
			      hb_ot_layout_glyph_t  glyph)
{
  hb_ot_layout_glyph_properties_t properties;
  hb_ot_layout_class_t klass;

  properties = _hb_ot_layout_get_glyph_properties (layout, glyph);

  if (properties & 0xFF)
    return HB_OT_LAYOUT_GLYPH_CLASS_MARK;

  return (hb_ot_layout_glyph_class_t) properties;
}

void
hb_ot_layout_set_glyph_class (HB_OT_Layout               *layout,
			      hb_ot_layout_glyph_t        glyph,
			      hb_ot_layout_glyph_class_t  klass)
{
  /* TODO optimize this, similar to old harfbuzz code for example */
  /* TODO our semantics are a bit different from old harfbuzz code too */

  hb_ot_layout_class_t gdef_klass;
  int len = layout->new_gdef.len;

  if (glyph >= len) {
    int new_len;
    uint8_t *new_klasses;

    new_len = len == 0 ? 120 : 2 * len;
    if (new_len > 65535)
      new_len = 65535;
    new_klasses = (uint8_t *) realloc (layout->new_gdef.klasses, new_len * sizeof (uint8_t));

    if (G_UNLIKELY (!new_klasses))
      return;
      
    memset (new_klasses + len, 0, new_len - len);

    layout->new_gdef.klasses = new_klasses;
    layout->new_gdef.len = new_len;
  }

  switch (klass) {
  default:
  case HB_OT_LAYOUT_GLYPH_CLASS_UNCLASSIFIED:	gdef_klass = GDEF::UnclassifiedGlyph;	break;
  case HB_OT_LAYOUT_GLYPH_CLASS_BASE_GLYPH:	gdef_klass = GDEF::BaseGlyph;		break;
  case HB_OT_LAYOUT_GLYPH_CLASS_LIGATURE:	gdef_klass = GDEF::LigatureGlyph;	break;
  case HB_OT_LAYOUT_GLYPH_CLASS_MARK:		gdef_klass = GDEF::MarkGlyph;		break;
  case HB_OT_LAYOUT_GLYPH_CLASS_COMPONENT:	gdef_klass = GDEF::ComponentGlyph;	break;
  }

  layout->new_gdef.klasses[glyph] = gdef_klass;
  return;
}
