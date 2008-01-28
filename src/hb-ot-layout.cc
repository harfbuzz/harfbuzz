/*
 * Copyright (C) 1998-2004  David Turner and Werner Lemberg
 * Copyright (C) 2006  Behdad Esfahbod
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


struct _hb_ot_layout_t {
  const GDEF *gdef;
  const GSUB *gsub;
  const /*XXX*/GSUBGPOS *gpos;

  struct {
    unsigned char *klasses;
    unsigned int len;
  } new_gdef;

};

hb_ot_layout_t *
hb_ot_layout_create_for_data (const char *font_data,
			      int         face_index)
{
  hb_ot_layout_t *layout = (hb_ot_layout_t *) calloc (1, sizeof (hb_ot_layout_t));

  const OpenTypeFontFile &font = OpenTypeFontFile::get_for_data (font_data);
  const OpenTypeFontFace &face = font.get_face (face_index);

  layout->gdef = &GDEF::get_for_data (font.get_table_data (face.get_table_by_tag (GDEF::Tag)));
  layout->gsub = &GSUB::get_for_data (font.get_table_data (face.get_table_by_tag (GSUB::Tag)));
  layout->gpos = &/*XXX*/GSUBGPOS::get_for_data (font.get_table_data (face.get_table_by_tag (/*XXX*/GSUBGPOS::GPOSTag)));

  return layout;
}

void
hb_ot_layout_destroy (hb_ot_layout_t *layout)
{
  free (layout);
}

/*
 * GDEF
 */

hb_bool_t
hb_ot_layout_has_font_glyph_classes (hb_ot_layout_t *layout)
{
  return layout->gdef->has_glyph_classes ();
}

HB_OT_LAYOUT_INTERNAL hb_bool_t
_hb_ot_layout_has_new_glyph_classes (hb_ot_layout_t *layout)
{
  return layout->new_gdef.len > 0;
}

HB_OT_LAYOUT_INTERNAL hb_ot_layout_glyph_properties_t
_hb_ot_layout_get_glyph_properties (hb_ot_layout_t *layout,
				    hb_glyph_t      glyph)
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

HB_OT_LAYOUT_INTERNAL hb_bool_t
_hb_ot_layout_check_glyph_properties (hb_ot_layout_t                  *layout,
				      HB_GlyphItem                     gitem,
				      hb_ot_layout_lookup_flags_t      lookup_flags,
				      hb_ot_layout_glyph_properties_t *property)
{
  hb_ot_layout_glyph_class_t basic_glyph_class;
  hb_ot_layout_glyph_properties_t desired_attachment_class;

  if (gitem->gproperties == HB_BUFFER_GLYPH_PROPERTIES_UNKNOWN)
  {
    gitem->gproperties = *property = _hb_ot_layout_get_glyph_properties (layout, gitem->gindex);
    if (gitem->gproperties == HB_OT_LAYOUT_GLYPH_CLASS_UNCLASSIFIED)
      return false;
  }

  *property = gitem->gproperties;

  /* If the glyph was found in the MarkAttachmentClass table,
   * then that class value is the high byte of the result,
   * otherwise the low byte contains the basic type of the glyph
   * as defined by the GlyphClassDef table.
   */
  if (*property & LookupFlag::MarkAttachmentType)
    basic_glyph_class = HB_OT_LAYOUT_GLYPH_CLASS_MARK;
  else
    basic_glyph_class = (hb_ot_layout_glyph_class_t) *property;

  /* Not covered, if, for example, basic_glyph_class
   * is HB_GDEF_LIGATURE and lookup_flags includes LookupFlags::IgnoreLigatures
   */
  if (lookup_flags & basic_glyph_class)
    return false;

  /* The high byte of lookup_flags has the meaning
   * "ignore marks of attachment type different than
   * the attachment type specified."
   */
  desired_attachment_class = lookup_flags & LookupFlag::MarkAttachmentType;
  if (desired_attachment_class)
  {
    if (basic_glyph_class == HB_OT_LAYOUT_GLYPH_CLASS_MARK &&
        *property != desired_attachment_class )
      return false;
  }

  return true;
}


hb_ot_layout_glyph_class_t
hb_ot_layout_get_glyph_class (hb_ot_layout_t *layout,
			      hb_glyph_t      glyph)
{
  hb_ot_layout_glyph_properties_t properties;
  hb_ot_layout_class_t klass;

  properties = _hb_ot_layout_get_glyph_properties (layout, glyph);

  if (properties & 0xFF00)
    return HB_OT_LAYOUT_GLYPH_CLASS_MARK;

  return (hb_ot_layout_glyph_class_t) properties;
}

void
hb_ot_layout_set_glyph_class (hb_ot_layout_t             *layout,
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
hb_ot_layout_build_glyph_classes (hb_ot_layout_t *layout,
				  uint16_t        num_total_glyphs,
				  hb_glyph_t     *glyphs,
				  unsigned char  *klasses,
				  uint16_t        count)
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

/*
 * GSUB/GPOS
 */

static const GSUBGPOS&
get_gsubgpos_table (hb_ot_layout_t            *layout,
		    hb_ot_layout_table_type_t  table_type)
{
  switch (table_type) {
    case HB_OT_LAYOUT_TABLE_TYPE_GSUB: return *(layout->gsub);
    case HB_OT_LAYOUT_TABLE_TYPE_GPOS: return *(layout->gpos);
    default:                           return NullGSUBGPOS;
  }
}


unsigned int
hb_ot_layout_get_script_count (hb_ot_layout_t            *layout,
			       hb_ot_layout_table_type_t  table_type)
{
  const GSUBGPOS &g = get_gsubgpos_table (layout, table_type);

  return g.get_script_count ();
}

hb_tag_t
hb_ot_layout_get_script_tag (hb_ot_layout_t            *layout,
			     hb_ot_layout_table_type_t  table_type,
			     unsigned int               script_index)
{
  const GSUBGPOS &g = get_gsubgpos_table (layout, table_type);

  return g.get_script_tag (script_index);
}

hb_bool_t
hb_ot_layout_find_script (hb_ot_layout_t            *layout,
			  hb_ot_layout_table_type_t  table_type,
			  hb_tag_t                   script_tag,
			  unsigned int              *script_index)
{
  unsigned int i;
  const GSUBGPOS &g = get_gsubgpos_table (layout, table_type);

  if (g.find_script_index (script_tag, script_index))
    return TRUE;

  /* try finding 'DFLT' */
  if (g.find_script_index (HB_OT_LAYOUT_TAG_DEFAULT_SCRIPT, script_index))
    return FALSE;

  /* try with 'dflt'; MS site has had typos and many fonts use it now :( */
  if (g.find_script_index (HB_OT_LAYOUT_TAG_DEFAULT_LANGUAGE, script_index))
    return FALSE;

  if (script_index) *script_index = HB_OT_LAYOUT_NO_SCRIPT_INDEX;
  return FALSE;
}


unsigned int
hb_ot_layout_get_language_count (hb_ot_layout_t            *layout,
				 hb_ot_layout_table_type_t  table_type,
				 unsigned int               script_index)
{
  const Script &s = get_gsubgpos_table (layout, table_type).get_script (script_index);

  return s.get_lang_sys_count ();
}

hb_tag_t
hb_ot_layout_get_language_tag (hb_ot_layout_t            *layout,
			       hb_ot_layout_table_type_t  table_type,
			       unsigned int               script_index,
			       unsigned int               language_index)
{
  const Script &s = get_gsubgpos_table (layout, table_type).get_script (script_index);

  return s.get_lang_sys_tag (language_index);
}

hb_bool_t
hb_ot_layout_find_language (hb_ot_layout_t            *layout,
			    hb_ot_layout_table_type_t  table_type,
			    unsigned int               script_index,
			    hb_tag_t                   language_tag,
			    unsigned int              *language_index,
			    unsigned int              *required_features_index)
{
  unsigned int i;
  const Script &s = get_gsubgpos_table (layout, table_type).get_script (script_index);

#if 0
  if (s.find_script_index (script_tag, script_index))
    return TRUE;

  /* try with 'dflt'; MS site has had typos and many fonts use it now :( */
  if (s.find_script_index (HB_OT_LAYOUT_TAG_DEFAULT_LANGUAGE, script_index))
    return FALSE;

  ////////////////////////
  if (script_index) *script_index = HB_OT_LAYOUT_NO_SCRIPT_INDEX;
  return FALSE;

  if (language_index)
    *language_index = PANGO_OT_DEFAULT_LANGUAGE;
  if (required_feature_index)
    *required_feature_index = PANGO_OT_NO_FEATURE;

  if (script_index == PANGO_OT_NO_SCRIPT)
    return FALSE;


  /* DefaultLangSys */
  if (language_index)
    *language_index = PANGO_OT_DEFAULT_LANGUAGE;
  if (required_feature_index)
    *required_feature_index = script->DefaultLangSys.ReqFeatureIndex;
#endif

  return FALSE;
}
