/*
 * Copyright (C) 1998-2004  David Turner and Werner Lemberg
 * Copyright (C) 2006  Behdad Esfahbod
 * Copyright (C) 2007,2008,2009  Red Hat, Inc.
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

#include "hb-ot-layout-private.h"

#include "hb-ot-layout-gdef-private.hh"
#include "hb-ot-layout-gsub-private.hh"
#include "hb-ot-layout-gpos-private.hh"


#include <stdlib.h>
#include <string.h>


void
_hb_ot_layout_init (hb_face_t *face)
{
  hb_ot_layout_t *layout = &face->ot_layout;

  layout->gdef_blob = Sanitizer<GDEF>::sanitize (hb_face_get_table (face, HB_OT_TAG_GDEF));
  layout->gdef = &Sanitizer<GDEF>::lock_instance (layout->gdef_blob);

  layout->gsub_blob = Sanitizer<GSUB>::sanitize (hb_face_get_table (face, HB_OT_TAG_GSUB));
  layout->gsub = &Sanitizer<GSUB>::lock_instance (layout->gsub_blob);

  layout->gpos_blob = Sanitizer<GPOS>::sanitize (hb_face_get_table (face, HB_OT_TAG_GPOS));
  layout->gpos = &Sanitizer<GPOS>::lock_instance (layout->gpos_blob);
}

void
_hb_ot_layout_fini (hb_face_t *face)
{
  hb_ot_layout_t *layout = &face->ot_layout;

  hb_blob_unlock (layout->gdef_blob);
  hb_blob_unlock (layout->gsub_blob);
  hb_blob_unlock (layout->gpos_blob);

  hb_blob_destroy (layout->gdef_blob);
  hb_blob_destroy (layout->gsub_blob);
  hb_blob_destroy (layout->gpos_blob);

  free (layout->new_gdef.klasses);
}

static const GDEF&
_get_gdef (hb_face_t *face)
{
  return HB_LIKELY (face->ot_layout.gdef) ? *face->ot_layout.gdef : Null(GDEF);
}

static const GSUB&
_get_gsub (hb_face_t *face)
{
  return HB_LIKELY (face->ot_layout.gsub) ? *face->ot_layout.gsub : Null(GSUB);
}

static const GPOS&
_get_gpos (hb_face_t *face)
{
  return HB_LIKELY (face->ot_layout.gpos) ? *face->ot_layout.gpos : Null(GPOS);
}


/*
 * GDEF
 */

/* TODO the public class_t is a mess */

hb_bool_t
hb_ot_layout_has_font_glyph_classes (hb_face_t *face)
{
  return _get_gdef (face).has_glyph_classes ();
}

hb_bool_t
_hb_ot_layout_has_new_glyph_classes (hb_face_t *face)
{
  return face->ot_layout.new_gdef.len > 0;
}

static unsigned int
_hb_ot_layout_get_glyph_property (hb_face_t      *face,
				  hb_codepoint_t  glyph)
{
  hb_ot_layout_class_t klass;
  const GDEF &gdef = _get_gdef (face);

  klass = gdef.get_glyph_class (glyph);

  if (!klass && glyph < face->ot_layout.new_gdef.len)
    klass = face->ot_layout.new_gdef.klasses[glyph];

  switch (klass) {
  default:
  case GDEF::UnclassifiedGlyph:	return HB_OT_LAYOUT_GLYPH_CLASS_UNCLASSIFIED;
  case GDEF::BaseGlyph:		return HB_OT_LAYOUT_GLYPH_CLASS_BASE_GLYPH;
  case GDEF::LigatureGlyph:	return HB_OT_LAYOUT_GLYPH_CLASS_LIGATURE;
  case GDEF::ComponentGlyph:	return HB_OT_LAYOUT_GLYPH_CLASS_COMPONENT;
  case GDEF::MarkGlyph:
	klass = gdef.get_mark_attachment_type (glyph);
	return HB_OT_LAYOUT_GLYPH_CLASS_MARK + (klass << 8);
  }
}

hb_bool_t
_hb_ot_layout_check_glyph_property (hb_face_t    *face,
				    hb_internal_glyph_info_t *ginfo,
				    unsigned int  lookup_flags,
				    unsigned int *property_out)
{
  unsigned int property;

  if (ginfo->gproperty == HB_BUFFER_GLYPH_PROPERTIES_UNKNOWN)
    ginfo->gproperty = _hb_ot_layout_get_glyph_property (face, ginfo->codepoint);
  property = ginfo->gproperty;
  if (property_out)
    *property_out = property;

  /* Not covered, if, for example, glyph class is ligature and
   * lookup_flags includes LookupFlags::IgnoreLigatures
   */
  if (property & lookup_flags & LookupFlag::IgnoreFlags)
    return false;

  if (property & HB_OT_LAYOUT_GLYPH_CLASS_MARK)
  {
    /* If using mark filtering sets, the high short of
     * lookup_flags has the set index.
     */
    if (lookup_flags & LookupFlag::UseMarkFilteringSet)
      return _get_gdef (face).mark_set_covers (lookup_flags >> 16, ginfo->codepoint);

    /* The second byte of lookup_flags has the meaning
     * "ignore marks of attachment type different than
     * the attachment type specified."
     */
    if (lookup_flags & LookupFlag::MarkAttachmentType && property & LookupFlag::MarkAttachmentType)
      return (lookup_flags & LookupFlag::MarkAttachmentType) == (property & LookupFlag::MarkAttachmentType);
  }

  return true;
}

hb_bool_t
_hb_ot_layout_skip_mark (hb_face_t    *face,
			 hb_internal_glyph_info_t *ginfo,
			 unsigned int  lookup_flags,
			 unsigned int *property_out)
{
  unsigned int property;

  if (ginfo->gproperty == HB_BUFFER_GLYPH_PROPERTIES_UNKNOWN)
    ginfo->gproperty = _hb_ot_layout_get_glyph_property (face, ginfo->codepoint);
  property = ginfo->gproperty;
  if (property_out)
    *property_out = property;

  if (property & HB_OT_LAYOUT_GLYPH_CLASS_MARK)
  {
    /* Skip mark if lookup_flags includes LookupFlags::IgnoreMarks */
    if (lookup_flags & LookupFlag::IgnoreMarks)
      return true;

    /* If using mark filtering sets, the high short of lookup_flags has the set index. */
    if (lookup_flags & LookupFlag::UseMarkFilteringSet)
      return !_get_gdef (face).mark_set_covers (lookup_flags >> 16, ginfo->codepoint);

    /* The second byte of lookup_flags has the meaning "ignore marks of attachment type
     * different than the attachment type specified." */
    if (lookup_flags & LookupFlag::MarkAttachmentType && property & LookupFlag::MarkAttachmentType)
      return (lookup_flags & LookupFlag::MarkAttachmentType) != (property & LookupFlag::MarkAttachmentType);
  }

  return false;
}

void
_hb_ot_layout_set_glyph_class (hb_face_t                  *face,
			       hb_codepoint_t              glyph,
			       hb_ot_layout_glyph_class_t  klass)
{
  if (HB_OBJECT_IS_INERT (face))
    return;

  /* TODO optimize this? similar to old harfbuzz code for example */

  hb_ot_layout_t *layout = &face->ot_layout;
  hb_ot_layout_class_t gdef_klass;
  unsigned int len = layout->new_gdef.len;

  if (HB_UNLIKELY (glyph > 65535))
    return;

  /* XXX this is not threadsafe */
  if (glyph >= len) {
    unsigned int new_len;
    unsigned char *new_klasses;

    new_len = len == 0 ? 120 : 2 * len;
    while (new_len <= glyph)
      new_len *= 2;

    if (new_len > 65536)
      new_len = 65536;
    new_klasses = (unsigned char *) realloc (layout->new_gdef.klasses, new_len * sizeof (unsigned char));

    if (HB_UNLIKELY (!new_klasses))
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
_hb_ot_layout_set_glyph_property (hb_face_t      *face,
				  hb_codepoint_t  glyph,
				  unsigned int    property)
{ _hb_ot_layout_set_glyph_class (face, glyph, (hb_ot_layout_glyph_class_t) (property & 0xff)); }


hb_ot_layout_glyph_class_t
hb_ot_layout_get_glyph_class (hb_face_t      *face,
			      hb_codepoint_t  glyph)
{
  return (hb_ot_layout_glyph_class_t) (_hb_ot_layout_get_glyph_property (face, glyph) & 0xff);
}

void
hb_ot_layout_set_glyph_class (hb_face_t                 *face,
			      hb_codepoint_t             glyph,
			      hb_ot_layout_glyph_class_t klass)
{
  _hb_ot_layout_set_glyph_class (face, glyph, klass);
}

void
hb_ot_layout_build_glyph_classes (hb_face_t      *face,
				  uint16_t        num_total_glyphs,
				  hb_codepoint_t *glyphs,
				  unsigned char  *klasses,
				  uint16_t        count)
{
  if (HB_OBJECT_IS_INERT (face))
    return;

  hb_ot_layout_t *layout = &face->ot_layout;

  if (HB_UNLIKELY (!count || !glyphs || !klasses))
    return;

  if (layout->new_gdef.len == 0) {
    layout->new_gdef.klasses = (unsigned char *) calloc (num_total_glyphs, sizeof (unsigned char));
    layout->new_gdef.len = count;
  }

  for (unsigned int i = 0; i < count; i++)
    _hb_ot_layout_set_glyph_class (face, glyphs[i], (hb_ot_layout_glyph_class_t) klasses[i]);
}

hb_bool_t
hb_ot_layout_get_attach_points (hb_face_t      *face,
				hb_codepoint_t  glyph,
				unsigned int   *point_count /* IN/OUT */,
				unsigned int   *point_array /* OUT */)
{
  return _get_gdef (face).get_attach_points (glyph, point_count, point_array);
}

hb_bool_t
hb_ot_layout_get_lig_carets (hb_face_t      *face,
			     hb_font_t      *font,
			     hb_codepoint_t  glyph,
			     unsigned int   *caret_count /* IN/OUT */,
			     int            *caret_array /* OUT */)
{
  hb_ot_layout_context_t context;
  context.font = font;
  context.face = face;
  return _get_gdef (face).get_lig_carets (&context, glyph, caret_count, caret_array);
}

/*
 * GSUB/GPOS
 */

static const GSUBGPOS&
get_gsubgpos_table (hb_face_t *face,
		    hb_tag_t   table_tag)
{
  switch (table_tag) {
    case HB_OT_TAG_GSUB: return _get_gsub (face);
    case HB_OT_TAG_GPOS: return _get_gpos (face);
    default:             return Null(GSUBGPOS);
  }
}


hb_bool_t
hb_ot_layout_table_get_script_tags (hb_face_t    *face,
				    hb_tag_t      table_tag,
				    unsigned int *script_count /* IN/OUT */,
				    hb_tag_t     *script_tags /* OUT */)
{
  const GSUBGPOS &g = get_gsubgpos_table (face, table_tag);

  return g.get_script_tags (script_count, script_tags);
}

hb_bool_t
hb_ot_layout_table_find_script (hb_face_t    *face,
				hb_tag_t      table_tag,
				hb_tag_t      script_tag,
				unsigned int *script_index)
{
  ASSERT_STATIC (NO_INDEX == HB_OT_LAYOUT_NO_SCRIPT_INDEX);
  const GSUBGPOS &g = get_gsubgpos_table (face, table_tag);

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

hb_bool_t
hb_ot_layout_table_get_feature_tags (hb_face_t    *face,
				     hb_tag_t      table_tag,
				     unsigned int *feature_count /* IN/OUT */,
				     hb_tag_t     *feature_tags /* OUT */)
{
  const GSUBGPOS &g = get_gsubgpos_table (face, table_tag);

  return g.get_feature_tags (feature_count, feature_tags);
}


hb_bool_t
hb_ot_layout_script_get_language_tags (hb_face_t    *face,
				       hb_tag_t      table_tag,
				       unsigned int  script_index,
				       unsigned int *language_count /* IN/OUT */,
				       hb_tag_t     *language_tags /* OUT */)
{
  const Script &s = get_gsubgpos_table (face, table_tag).get_script (script_index);

  return s.get_lang_sys_tags (language_count, language_tags);
}

hb_bool_t
hb_ot_layout_script_find_language (hb_face_t    *face,
				   hb_tag_t      table_tag,
				   unsigned int  script_index,
				   hb_tag_t      language_tag,
				   unsigned int *language_index)
{
  ASSERT_STATIC (NO_INDEX == HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX);
  const Script &s = get_gsubgpos_table (face, table_tag).get_script (script_index);

  if (s.find_lang_sys_index (language_tag, language_index))
    return TRUE;

  /* try with 'dflt'; MS site has had typos and many fonts use it now :( */
  if (s.find_lang_sys_index (HB_OT_LAYOUT_TAG_DEFAULT_LANGUAGE, language_index))
    return FALSE;

  if (language_index) *language_index = HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX;
  return FALSE;
}

hb_bool_t
hb_ot_layout_language_get_required_feature_index (hb_face_t    *face,
						  hb_tag_t      table_tag,
						  unsigned int  script_index,
						  unsigned int  language_index,
						  unsigned int *feature_index)
{
  const LangSys &l = get_gsubgpos_table (face, table_tag).get_script (script_index).get_lang_sys (language_index);

  if (feature_index) *feature_index = l.get_required_feature_index ();

  return l.has_required_feature ();
}

hb_bool_t
hb_ot_layout_language_get_feature_indexes (hb_face_t    *face,
					   hb_tag_t      table_tag,
					   unsigned int  script_index,
					   unsigned int  language_index,
					   unsigned int *feature_count /* IN/OUT */,
					   unsigned int *feature_indexes /* OUT */)
{
  const GSUBGPOS &g = get_gsubgpos_table (face, table_tag);
  const LangSys &l = g.get_script (script_index).get_lang_sys (language_index);

  return l.get_feature_indexes (feature_count, feature_indexes);
}

hb_bool_t
hb_ot_layout_language_get_feature_tags (hb_face_t    *face,
					hb_tag_t      table_tag,
					unsigned int  script_index,
					unsigned int  language_index,
					unsigned int *feature_count /* IN/OUT */,
					hb_tag_t     *feature_tags /* OUT */)
{
  const GSUBGPOS &g = get_gsubgpos_table (face, table_tag);
  const LangSys &l = g.get_script (script_index).get_lang_sys (language_index);

  ASSERT_STATIC (sizeof (unsigned int) == sizeof (hb_tag_t));
  hb_bool_t ret = l.get_feature_indexes (feature_count, (unsigned int *) feature_tags);

  if (feature_tags) {
    unsigned int count = *feature_count;
    for (unsigned int i = 0; i < count; i++)
      feature_tags[i] = g.get_feature_tag ((unsigned int) feature_tags[i]);
  }

  return ret;
}


hb_bool_t
hb_ot_layout_language_find_feature (hb_face_t    *face,
				    hb_tag_t      table_tag,
				    unsigned int  script_index,
				    unsigned int  language_index,
				    hb_tag_t      feature_tag,
				    unsigned int *feature_index)
{
  ASSERT_STATIC (NO_INDEX == HB_OT_LAYOUT_NO_FEATURE_INDEX);
  const GSUBGPOS &g = get_gsubgpos_table (face, table_tag);
  const LangSys &l = g.get_script (script_index).get_lang_sys (language_index);

  unsigned int num_features = l.get_feature_count ();
  for (unsigned int i = 0; i < num_features; i++) {
    unsigned int f_index = l.get_feature_index (i);

    if (feature_tag == g.get_feature_tag (f_index)) {
      if (feature_index) *feature_index = f_index;
      return TRUE;
    }
  }

  if (feature_index) *feature_index = HB_OT_LAYOUT_NO_FEATURE_INDEX;
  return FALSE;
}

hb_bool_t
hb_ot_layout_feature_get_lookup_indexes (hb_face_t    *face,
					 hb_tag_t      table_tag,
					 unsigned int  feature_index,
					 unsigned int *lookup_count /* IN/OUT */,
					 unsigned int *lookup_indexes /* OUT */)
{
  const GSUBGPOS &g = get_gsubgpos_table (face, table_tag);
  const Feature &f = g.get_feature (feature_index);

  return f.get_lookup_indexes (lookup_count, lookup_indexes);
}


/*
 * GSUB
 */

hb_bool_t
hb_ot_layout_has_substitution (hb_face_t *face)
{
  return &_get_gsub (face) != &Null(GSUB);
}

hb_bool_t
hb_ot_layout_substitute_lookup (hb_face_t    *face,
				hb_buffer_t  *buffer,
				unsigned int  lookup_index,
				hb_mask_t     mask)
{
  hb_ot_layout_context_t context;
  context.font = NULL;
  context.face = face;
  return _get_gsub (face).substitute_lookup (&context, buffer, lookup_index, mask);
}


/*
 * GPOS
 */

hb_bool_t
hb_ot_layout_has_positioning (hb_face_t *face)
{
  return &_get_gpos (face) != &Null(GPOS);
}

hb_bool_t
hb_ot_layout_position_lookup   (hb_face_t    *face,
				hb_font_t    *font,
				hb_buffer_t  *buffer,
				unsigned int  lookup_index,
				hb_mask_t     mask)
{
  hb_ot_layout_context_t context;
  context.font = font;
  context.face = face;
  return _get_gpos (face).position_lookup (&context, buffer, lookup_index, mask);
}
