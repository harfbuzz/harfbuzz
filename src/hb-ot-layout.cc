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


struct _HB_OT_Layout {
  const GDEF *gdef;
  const GSUB *gsub;
//const GPOS *gpos;

  struct {
    unsigned char *klasses;
    unsigned int len;
  } new_gdef;

};

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

/*
 * GDEF
 */

hb_bool_t
hb_ot_layout_has_font_glyph_classes (HB_OT_Layout *layout)
{
  return layout->gdef->has_glyph_classes ();
}

static hb_bool_t
_hb_ot_layout_has_new_glyph_classes (HB_OT_Layout *layout)
{
  return layout->new_gdef.len > 0;
}

static hb_ot_layout_glyph_properties_t
_hb_ot_layout_get_glyph_properties (HB_OT_Layout *layout,
				    hb_glyph_t    glyph)
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

#if 0
static bool
_hb_ot_layout_check_glyph_properties (HB_OT_Layout *layout,
				      HB_GlyphItem  gitem,
				      HB_UShort     flags,
				      HB_UShort*    property)
{
  HB_Error  error;

  if ( gdef )
  {
    HB_UShort basic_glyph_class;
    HB_UShort desired_attachment_class;

    if ( gitem->gproperties == HB_GLYPH_PROPERTIES_UNKNOWN )
    {
      error = HB_GDEF_Get_Glyph_Property( gdef, gitem->gindex, &gitem->gproperties );
      if ( error )
	return error;
    }

    *property = gitem->gproperties;

    /* If the glyph was found in the MarkAttachmentClass table,
     * then that class value is the high byte of the result,
     * otherwise the low byte contains the basic type of the glyph
     * as defined by the GlyphClassDef table.
     */
    if ( *property & HB_LOOKUP_FLAG_IGNORE_SPECIAL_MARKS  )
      basic_glyph_class = HB_GDEF_MARK;
    else
      basic_glyph_class = *property;

    /* Return Not_Covered, if, for example, basic_glyph_class
     * is HB_GDEF_LIGATURE and LookFlags includes HB_LOOKUP_FLAG_IGNORE_LIGATURES
     */
    if ( flags & basic_glyph_class )
      return HB_Err_Not_Covered;

    /* The high byte of LookupFlags has the meaning
     * "ignore marks of attachment type different than
     * the attachment type specified."
     */
    desired_attachment_class = flags & HB_LOOKUP_FLAG_IGNORE_SPECIAL_MARKS;
    if ( desired_attachment_class )
    {
      if ( basic_glyph_class == HB_GDEF_MARK &&
	   *property != desired_attachment_class )
	return HB_Err_Not_Covered;
    }
  } else {
      *property = 0;
  }

  return HB_Err_Ok;
}
#endif


hb_ot_layout_glyph_class_t
hb_ot_layout_get_glyph_class (HB_OT_Layout *layout,
			      hb_glyph_t    glyph)
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
			      hb_glyph_t                  glyph,
			      hb_ot_layout_glyph_class_t  klass)
{
  /* TODO optimize this, similar to old harfbuzz code for example */

  hb_ot_layout_class_t gdef_klass;
  int len = layout->new_gdef.len;

  if (glyph >= len) {
    int new_len;
    unsigned char *new_klasses;

    new_len = len == 0 ? 120 : 2 * len;
    if (new_len > 65535)
      new_len = 65535;
    new_klasses = (unsigned char *) realloc (layout->new_gdef.klasses, new_len * sizeof (unsigned char));

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

void
hb_ot_layout_build_glyph_classes (HB_OT_Layout  *layout,
				  uint16_t       num_total_glyphs,
				  hb_glyph_t    *glyphs,
				  unsigned char *klasses,
				  uint16_t       count)
{
  int i;

  if (G_UNLIKELY (!count || !glyphs || !klasses))
    return;

  if (layout->new_gdef.len == 0) {
    layout->new_gdef.klasses = (unsigned char *) calloc (num_total_glyphs, sizeof (unsigned char));
    layout->new_gdef.len = count;
  }

  for (i = 0; i < count; i++)
    hb_ot_layout_set_glyph_class (layout, glyphs[i], (hb_ot_layout_glyph_class_t) klasses[i]);
}
