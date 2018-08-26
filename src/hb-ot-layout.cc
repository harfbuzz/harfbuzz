/*
 * Copyright © 1998-2004  David Turner and Werner Lemberg
 * Copyright © 2006  Behdad Esfahbod
 * Copyright © 2007,2008,2009  Red Hat, Inc.
 * Copyright © 2012,2013  Google, Inc.
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

#include "hb-open-type.hh"
#include "hb-ot-layout.hh"
#include "hb-ot-face.hh"
#include "hb-ot-map.hh"
#include "hb-map.hh"

#include "hb-ot-layout-gdef-table.hh"
#include "hb-ot-layout-gsub-table.hh"
#include "hb-ot-layout-gpos-table.hh"

// Just so we compile them; unused otherwise:
#include "hb-ot-layout-base-table.hh"
#include "hb-ot-layout-jstf-table.hh"
#include "hb-ot-color-colr-table.hh"
#include "hb-ot-color-cpal-table.hh"
#include "hb-ot-color-sbix-table.hh"
#include "hb-ot-color-svg-table.hh"
#include "hb-ot-name-table.hh"


// static inline const OT::BASE&
// _get_base (hb_face_t *face)
// {
//   if (unlikely (!hb_ot_shaper_face_data_ensure (face))) return Null(OT::BASE);
//   hb_ot_face_data_t *data = hb_ot_face_data (face);
//   return *(data->base.get ());
// }

inline const OT::GDEF&
_get_gdef (hb_face_t *face)
{
  if (unlikely (!hb_ot_shaper_face_data_ensure (face))) return Null(OT::GDEF);
  return *hb_ot_face_data (face)->table.GDEF;
}
inline const OT::GSUB&
_get_gsub (hb_face_t *face)
{
  if (unlikely (!hb_ot_shaper_face_data_ensure (face))) return Null(OT::GSUB);
  return *hb_ot_face_data (face)->table.GSUB->table;
}
inline const OT::GPOS&
_get_gpos (hb_face_t *face)
{
  if (unlikely (!hb_ot_shaper_face_data_ensure (face))) return Null(OT::GPOS);
  return *hb_ot_face_data (face)->table.GPOS->table;
}

/*
 * GDEF
 */

hb_bool_t
hb_ot_layout_has_glyph_classes (hb_face_t *face)
{
  return _get_gdef (face).has_glyph_classes ();
}

/**
 * hb_ot_layout_get_glyph_class:
 *
 * Since: 0.9.7
 **/
hb_ot_layout_glyph_class_t
hb_ot_layout_get_glyph_class (hb_face_t      *face,
			      hb_codepoint_t  glyph)
{
  return (hb_ot_layout_glyph_class_t) _get_gdef (face).get_glyph_class (glyph);
}

/**
 * hb_ot_layout_get_glyphs_in_class:
 *
 * Since: 0.9.7
 **/
void
hb_ot_layout_get_glyphs_in_class (hb_face_t                  *face,
				  hb_ot_layout_glyph_class_t  klass,
				  hb_set_t                   *glyphs /* OUT */)
{
  return _get_gdef (face).get_glyphs_in_class (klass, glyphs);
}

unsigned int
hb_ot_layout_get_attach_points (hb_face_t      *face,
				hb_codepoint_t  glyph,
				unsigned int    start_offset,
				unsigned int   *point_count /* IN/OUT */,
				unsigned int   *point_array /* OUT */)
{
  return _get_gdef (face).get_attach_points (glyph, start_offset, point_count, point_array);
}

unsigned int
hb_ot_layout_get_ligature_carets (hb_font_t      *font,
				  hb_direction_t  direction,
				  hb_codepoint_t  glyph,
				  unsigned int    start_offset,
				  unsigned int   *caret_count /* IN/OUT */,
				  hb_position_t  *caret_array /* OUT */)
{
  return _get_gdef (font->face).get_lig_carets (font, direction, glyph, start_offset, caret_count, caret_array);
}


/*
 * GSUB/GPOS
 */

static const OT::GSUBGPOS&
get_gsubgpos_table (hb_face_t *face,
		    hb_tag_t   table_tag)
{
  switch (table_tag) {
    case HB_OT_TAG_GSUB: return _get_gsub (face);
    case HB_OT_TAG_GPOS: return _get_gpos (face);
    default:             return Null(OT::GSUBGPOS);
  }
}


unsigned int
hb_ot_layout_table_get_script_tags (hb_face_t    *face,
				    hb_tag_t      table_tag,
				    unsigned int  start_offset,
				    unsigned int *script_count /* IN/OUT */,
				    hb_tag_t     *script_tags /* OUT */)
{
  const OT::GSUBGPOS &g = get_gsubgpos_table (face, table_tag);

  return g.get_script_tags (start_offset, script_count, script_tags);
}

#define HB_OT_TAG_LATIN_SCRIPT		HB_TAG ('l', 'a', 't', 'n')

hb_bool_t
hb_ot_layout_table_find_script (hb_face_t    *face,
				hb_tag_t      table_tag,
				hb_tag_t      script_tag,
				unsigned int *script_index)
{
  static_assert ((OT::Index::NOT_FOUND_INDEX == HB_OT_LAYOUT_NO_SCRIPT_INDEX), "");
  const OT::GSUBGPOS &g = get_gsubgpos_table (face, table_tag);

  if (g.find_script_index (script_tag, script_index))
    return true;

  /* try finding 'DFLT' */
  if (g.find_script_index (HB_OT_TAG_DEFAULT_SCRIPT, script_index))
    return false;

  /* try with 'dflt'; MS site has had typos and many fonts use it now :(.
   * including many versions of DejaVu Sans Mono! */
  if (g.find_script_index (HB_OT_TAG_DEFAULT_LANGUAGE, script_index))
    return false;

  /* try with 'latn'; some old fonts put their features there even though
     they're really trying to support Thai, for example :( */
  if (g.find_script_index (HB_OT_TAG_LATIN_SCRIPT, script_index))
    return false;

  if (script_index) *script_index = HB_OT_LAYOUT_NO_SCRIPT_INDEX;
  return false;
}

hb_bool_t
hb_ot_layout_table_choose_script (hb_face_t      *face,
				  hb_tag_t        table_tag,
				  const hb_tag_t *script_tags,
				  unsigned int   *script_index,
				  hb_tag_t       *chosen_script)
{
  static_assert ((OT::Index::NOT_FOUND_INDEX == HB_OT_LAYOUT_NO_SCRIPT_INDEX), "");
  const OT::GSUBGPOS &g = get_gsubgpos_table (face, table_tag);

  while (*script_tags)
  {
    if (g.find_script_index (*script_tags, script_index)) {
      if (chosen_script)
        *chosen_script = *script_tags;
      return true;
    }
    script_tags++;
  }

  /* try finding 'DFLT' */
  if (g.find_script_index (HB_OT_TAG_DEFAULT_SCRIPT, script_index)) {
    if (chosen_script)
      *chosen_script = HB_OT_TAG_DEFAULT_SCRIPT;
    return false;
  }

  /* try with 'dflt'; MS site has had typos and many fonts use it now :( */
  if (g.find_script_index (HB_OT_TAG_DEFAULT_LANGUAGE, script_index)) {
    if (chosen_script)
      *chosen_script = HB_OT_TAG_DEFAULT_LANGUAGE;
    return false;
  }

  /* try with 'latn'; some old fonts put their features there even though
     they're really trying to support Thai, for example :( */
  if (g.find_script_index (HB_OT_TAG_LATIN_SCRIPT, script_index)) {
    if (chosen_script)
      *chosen_script = HB_OT_TAG_LATIN_SCRIPT;
    return false;
  }

  if (script_index) *script_index = HB_OT_LAYOUT_NO_SCRIPT_INDEX;
  if (chosen_script)
    *chosen_script = HB_OT_LAYOUT_NO_SCRIPT_INDEX;
  return false;
}

unsigned int
hb_ot_layout_table_get_feature_tags (hb_face_t    *face,
				     hb_tag_t      table_tag,
				     unsigned int  start_offset,
				     unsigned int *feature_count /* IN/OUT */,
				     hb_tag_t     *feature_tags /* OUT */)
{
  const OT::GSUBGPOS &g = get_gsubgpos_table (face, table_tag);

  return g.get_feature_tags (start_offset, feature_count, feature_tags);
}

hb_bool_t
hb_ot_layout_table_find_feature (hb_face_t    *face,
				 hb_tag_t      table_tag,
				 hb_tag_t      feature_tag,
				 unsigned int *feature_index)
{
  static_assert ((OT::Index::NOT_FOUND_INDEX == HB_OT_LAYOUT_NO_FEATURE_INDEX), "");
  const OT::GSUBGPOS &g = get_gsubgpos_table (face, table_tag);

  unsigned int num_features = g.get_feature_count ();
  for (unsigned int i = 0; i < num_features; i++)
  {
    if (feature_tag == g.get_feature_tag (i)) {
      if (feature_index) *feature_index = i;
      return true;
    }
  }

  if (feature_index) *feature_index = HB_OT_LAYOUT_NO_FEATURE_INDEX;
  return false;
}


unsigned int
hb_ot_layout_script_get_language_tags (hb_face_t    *face,
				       hb_tag_t      table_tag,
				       unsigned int  script_index,
				       unsigned int  start_offset,
				       unsigned int *language_count /* IN/OUT */,
				       hb_tag_t     *language_tags /* OUT */)
{
  const OT::Script &s = get_gsubgpos_table (face, table_tag).get_script (script_index);

  return s.get_lang_sys_tags (start_offset, language_count, language_tags);
}

hb_bool_t
hb_ot_layout_script_find_language (hb_face_t    *face,
				   hb_tag_t      table_tag,
				   unsigned int  script_index,
				   hb_tag_t      language_tag,
				   unsigned int *language_index)
{
  static_assert ((OT::Index::NOT_FOUND_INDEX == HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX), "");
  const OT::Script &s = get_gsubgpos_table (face, table_tag).get_script (script_index);

  if (s.find_lang_sys_index (language_tag, language_index))
    return true;

  /* try with 'dflt'; MS site has had typos and many fonts use it now :( */
  if (s.find_lang_sys_index (HB_OT_TAG_DEFAULT_LANGUAGE, language_index))
    return false;

  if (language_index) *language_index = HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX;
  return false;
}

hb_bool_t
hb_ot_layout_language_get_required_feature_index (hb_face_t    *face,
						  hb_tag_t      table_tag,
						  unsigned int  script_index,
						  unsigned int  language_index,
						  unsigned int *feature_index)
{
  return hb_ot_layout_language_get_required_feature (face,
						     table_tag,
						     script_index,
						     language_index,
						     feature_index,
						     nullptr);
}

/**
 * hb_ot_layout_language_get_required_feature:
 *
 * Since: 0.9.30
 **/
hb_bool_t
hb_ot_layout_language_get_required_feature (hb_face_t    *face,
					    hb_tag_t      table_tag,
					    unsigned int  script_index,
					    unsigned int  language_index,
					    unsigned int *feature_index,
					    hb_tag_t     *feature_tag)
{
  const OT::GSUBGPOS &g = get_gsubgpos_table (face, table_tag);
  const OT::LangSys &l = g.get_script (script_index).get_lang_sys (language_index);

  unsigned int index = l.get_required_feature_index ();
  if (feature_index) *feature_index = index;
  if (feature_tag) *feature_tag = g.get_feature_tag (index);

  return l.has_required_feature ();
}

static void
_hb_ot_layout_language_add_feature_indexes_to (hb_face_t    *face,
                                               hb_tag_t      table_tag,
                                               unsigned int  script_index,
                                               unsigned int  language_index,
                                               hb_set_t     *feature_indexes /* OUT */)
{
  const OT::GSUBGPOS &g = get_gsubgpos_table (face, table_tag);
  const OT::LangSys &l = g.get_script (script_index).get_lang_sys (language_index);
  l.add_feature_indexes_to (feature_indexes);
}


unsigned int
hb_ot_layout_language_get_feature_indexes (hb_face_t    *face,
					   hb_tag_t      table_tag,
					   unsigned int  script_index,
					   unsigned int  language_index,
					   unsigned int  start_offset,
					   unsigned int *feature_count /* IN/OUT */,
					   unsigned int *feature_indexes /* OUT */)
{
  const OT::GSUBGPOS &g = get_gsubgpos_table (face, table_tag);
  const OT::LangSys &l = g.get_script (script_index).get_lang_sys (language_index);

  return l.get_feature_indexes (start_offset, feature_count, feature_indexes);
}

unsigned int
hb_ot_layout_language_get_feature_tags (hb_face_t    *face,
					hb_tag_t      table_tag,
					unsigned int  script_index,
					unsigned int  language_index,
					unsigned int  start_offset,
					unsigned int *feature_count /* IN/OUT */,
					hb_tag_t     *feature_tags /* OUT */)
{
  const OT::GSUBGPOS &g = get_gsubgpos_table (face, table_tag);
  const OT::LangSys &l = g.get_script (script_index).get_lang_sys (language_index);

  static_assert ((sizeof (unsigned int) == sizeof (hb_tag_t)), "");
  unsigned int ret = l.get_feature_indexes (start_offset, feature_count, (unsigned int *) feature_tags);

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
  static_assert ((OT::Index::NOT_FOUND_INDEX == HB_OT_LAYOUT_NO_FEATURE_INDEX), "");
  const OT::GSUBGPOS &g = get_gsubgpos_table (face, table_tag);
  const OT::LangSys &l = g.get_script (script_index).get_lang_sys (language_index);

  unsigned int num_features = l.get_feature_count ();
  for (unsigned int i = 0; i < num_features; i++) {
    unsigned int f_index = l.get_feature_index (i);

    if (feature_tag == g.get_feature_tag (f_index)) {
      if (feature_index) *feature_index = f_index;
      return true;
    }
  }

  if (feature_index) *feature_index = HB_OT_LAYOUT_NO_FEATURE_INDEX;
  return false;
}

/**
 * hb_ot_layout_feature_get_lookups:
 *
 * Since: 0.9.7
 **/
unsigned int
hb_ot_layout_feature_get_lookups (hb_face_t    *face,
				  hb_tag_t      table_tag,
				  unsigned int  feature_index,
				  unsigned int  start_offset,
				  unsigned int *lookup_count /* IN/OUT */,
				  unsigned int *lookup_indexes /* OUT */)
{
  return hb_ot_layout_feature_with_variations_get_lookups (face,
							   table_tag,
							   feature_index,
							   HB_OT_LAYOUT_NO_VARIATIONS_INDEX,
							   start_offset,
							   lookup_count,
							   lookup_indexes);
}

/**
 * hb_ot_layout_table_get_lookup_count:
 *
 * Since: 0.9.22
 **/
unsigned int
hb_ot_layout_table_get_lookup_count (hb_face_t    *face,
				     hb_tag_t      table_tag)
{
  if (unlikely (!hb_ot_shaper_face_data_ensure (face))) return 0;
  switch (table_tag)
  {
    case HB_OT_TAG_GSUB:
    {
      return hb_ot_face_data (face)->table.GSUB->lookup_count;
    }
    case HB_OT_TAG_GPOS:
    {
      return hb_ot_face_data (face)->table.GPOS->lookup_count;
    }
  }
  return 0;
}

static void
_hb_ot_layout_collect_lookups_lookups (hb_face_t      *face,
				       hb_tag_t        table_tag,
				       unsigned int    feature_index,
				       hb_set_t       *lookup_indexes /* OUT */)
{
  unsigned int lookup_indices[32];
  unsigned int offset, len;

  offset = 0;
  do {
    len = ARRAY_LENGTH (lookup_indices);
    hb_ot_layout_feature_get_lookups (face,
				      table_tag,
				      feature_index,
				      offset, &len,
				      lookup_indices);

    for (unsigned int i = 0; i < len; i++)
      lookup_indexes->add (lookup_indices[i]);

    offset += len;
  } while (len == ARRAY_LENGTH (lookup_indices));
}

static void
_hb_ot_layout_collect_features_features (hb_face_t      *face,
                                         hb_tag_t        table_tag,
                                         unsigned int    script_index,
                                         unsigned int    language_index,
                                         const hb_tag_t *features,
                                         hb_set_t       *feature_indexes /* OUT */)
{
  if (!features)
  {
    unsigned int required_feature_index;
    if (hb_ot_layout_language_get_required_feature (face,
						    table_tag,
						    script_index,
						    language_index,
						    &required_feature_index,
						    nullptr))
      feature_indexes->add (required_feature_index);

    /* All features */
    _hb_ot_layout_language_add_feature_indexes_to (face,
                                                   table_tag,
                                                   script_index,
                                                   language_index,
                                                   feature_indexes);
  }
  else
  {
    for (; *features; features++)
    {
      unsigned int feature_index;
      if (hb_ot_layout_language_find_feature (face,
					      table_tag,
					      script_index,
					      language_index,
					      *features,
					      &feature_index))
        feature_indexes->add (feature_index);
    }
  }
}

static void
_hb_ot_layout_collect_features_languages (hb_face_t      *face,
                                          hb_tag_t        table_tag,
                                          unsigned int    script_index,
                                          const hb_tag_t *languages,
                                          const hb_tag_t *features,
                                          hb_set_t       *feature_indexes /* OUT */)
{
  _hb_ot_layout_collect_features_features (face,
                                           table_tag,
                                           script_index,
                                           HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX,
                                           features,
                                           feature_indexes);

  if (!languages)
  {
    /* All languages */
    unsigned int count = hb_ot_layout_script_get_language_tags (face,
								table_tag,
								script_index,
								0, nullptr, nullptr);
    for (unsigned int language_index = 0; language_index < count; language_index++)
      _hb_ot_layout_collect_features_features (face,
                                               table_tag,
                                               script_index,
                                               language_index,
                                               features,
                                               feature_indexes);
  }
  else
  {
    for (; *languages; languages++)
    {
      unsigned int language_index;
      if (hb_ot_layout_script_find_language (face,
					     table_tag,
					     script_index,
					     *languages,
					     &language_index))
        _hb_ot_layout_collect_features_features (face,
                                                 table_tag,
                                                 script_index,
                                                 language_index,
                                                 features,
                                                 feature_indexes);
    }
  }
}

/**
 * hb_ot_layout_collect_features:
 *
 * Since: 1.8.5
 **/
void
hb_ot_layout_collect_features (hb_face_t      *face,
                               hb_tag_t        table_tag,
                               const hb_tag_t *scripts,
                               const hb_tag_t *languages,
                               const hb_tag_t *features,
                               hb_set_t       *feature_indexes /* OUT */)
{
  if (!scripts)
  {
    /* All scripts */
    unsigned int count = hb_ot_layout_table_get_script_tags (face,
							     table_tag,
							     0, nullptr, nullptr);
    for (unsigned int script_index = 0; script_index < count; script_index++)
      _hb_ot_layout_collect_features_languages (face,
                                                table_tag,
                                                script_index,
                                                languages,
                                                features,
                                                feature_indexes);
  }
  else
  {
    for (; *scripts; scripts++)
    {
      unsigned int script_index;
      if (hb_ot_layout_table_find_script (face,
					  table_tag,
					  *scripts,
					  &script_index))
        _hb_ot_layout_collect_features_languages (face,
                                                  table_tag,
                                                  script_index,
                                                  languages,
                                                  features,
                                                  feature_indexes);
    }
  }
}

/**
 * hb_ot_layout_collect_lookups:
 *
 * Since: 0.9.8
 **/
void
hb_ot_layout_collect_lookups (hb_face_t      *face,
			      hb_tag_t        table_tag,
			      const hb_tag_t *scripts,
			      const hb_tag_t *languages,
			      const hb_tag_t *features,
			      hb_set_t       *lookup_indexes /* OUT */)
{
  hb_auto_t<hb_set_t> feature_indexes;
  hb_ot_layout_collect_features (face, table_tag, scripts, languages, features, &feature_indexes);
  for (hb_codepoint_t feature_index = HB_SET_VALUE_INVALID; hb_set_next (&feature_indexes, &feature_index);)
    _hb_ot_layout_collect_lookups_lookups (face, table_tag, feature_index, lookup_indexes);
}

/**
 * hb_ot_layout_lookup_collect_glyphs:
 *
 * Since: 0.9.7
 **/
void
hb_ot_layout_lookup_collect_glyphs (hb_face_t    *face,
				    hb_tag_t      table_tag,
				    unsigned int  lookup_index,
				    hb_set_t     *glyphs_before, /* OUT. May be nullptr */
				    hb_set_t     *glyphs_input,  /* OUT. May be nullptr */
				    hb_set_t     *glyphs_after,  /* OUT. May be nullptr */
				    hb_set_t     *glyphs_output  /* OUT. May be nullptr */)
{
  if (unlikely (!hb_ot_shaper_face_data_ensure (face))) return;

  OT::hb_collect_glyphs_context_t c (face,
				     glyphs_before,
				     glyphs_input,
				     glyphs_after,
				     glyphs_output);

  switch (table_tag)
  {
    case HB_OT_TAG_GSUB:
    {
      const OT::SubstLookup& l = hb_ot_face_data (face)->table.GSUB->table->get_lookup (lookup_index);
      l.collect_glyphs (&c);
      return;
    }
    case HB_OT_TAG_GPOS:
    {
      const OT::PosLookup& l = hb_ot_face_data (face)->table.GPOS->table->get_lookup (lookup_index);
      l.collect_glyphs (&c);
      return;
    }
  }
}


/* Variations support */

hb_bool_t
hb_ot_layout_table_find_feature_variations (hb_face_t    *face,
					    hb_tag_t      table_tag,
					    const int    *coords,
					    unsigned int  num_coords,
					    unsigned int *variations_index /* out */)
{
  const OT::GSUBGPOS &g = get_gsubgpos_table (face, table_tag);

  return g.find_variations_index (coords, num_coords, variations_index);
}

unsigned int
hb_ot_layout_feature_with_variations_get_lookups (hb_face_t    *face,
						  hb_tag_t      table_tag,
						  unsigned int  feature_index,
						  unsigned int  variations_index,
						  unsigned int  start_offset,
						  unsigned int *lookup_count /* IN/OUT */,
						  unsigned int *lookup_indexes /* OUT */)
{
  static_assert ((OT::FeatureVariations::NOT_FOUND_INDEX == HB_OT_LAYOUT_NO_VARIATIONS_INDEX), "");
  const OT::GSUBGPOS &g = get_gsubgpos_table (face, table_tag);

  const OT::Feature &f = g.get_feature_variation (feature_index, variations_index);

  return f.get_lookup_indexes (start_offset, lookup_count, lookup_indexes);
}


/*
 * OT::GSUB
 */

hb_bool_t
hb_ot_layout_has_substitution (hb_face_t *face)
{
  return _get_gsub (face).has_data ();
}

/**
 * hb_ot_layout_lookup_would_substitute:
 *
 * Since: 0.9.7
 **/
hb_bool_t
hb_ot_layout_lookup_would_substitute (hb_face_t            *face,
				      unsigned int          lookup_index,
				      const hb_codepoint_t *glyphs,
				      unsigned int          glyphs_length,
				      hb_bool_t             zero_context)
{
  if (unlikely (!hb_ot_shaper_face_data_ensure (face))) return false;
  return hb_ot_layout_lookup_would_substitute_fast (face, lookup_index, glyphs, glyphs_length, zero_context);
}

hb_bool_t
hb_ot_layout_lookup_would_substitute_fast (hb_face_t            *face,
					   unsigned int          lookup_index,
					   const hb_codepoint_t *glyphs,
					   unsigned int          glyphs_length,
					   hb_bool_t             zero_context)
{
  if (unlikely (lookup_index >= hb_ot_face_data (face)->table.GSUB->lookup_count)) return false;
  OT::hb_would_apply_context_t c (face, glyphs, glyphs_length, (bool) zero_context);

  const OT::SubstLookup& l = hb_ot_face_data (face)->table.GSUB->table->get_lookup (lookup_index);

  return l.would_apply (&c, &hb_ot_face_data (face)->table.GSUB->accels[lookup_index]);
}

void
hb_ot_layout_substitute_start (hb_font_t *font, hb_buffer_t *buffer)
{
  OT::GSUB::substitute_start (font, buffer);
}

/**
 * hb_ot_layout_lookup_substitute_closure:
 *
 * Since: 0.9.7
 **/
void
hb_ot_layout_lookup_substitute_closure (hb_face_t    *face,
				        unsigned int  lookup_index,
				        hb_set_t     *glyphs)
{
  hb_auto_t<hb_map_t> done_lookups;
  OT::hb_closure_context_t c (face, glyphs, &done_lookups);

  const OT::SubstLookup& l = _get_gsub (face).get_lookup (lookup_index);

  l.closure (&c, lookup_index);
}

/**
 * hb_ot_layout_lookups_substitute_closure:
 *
 * Compute the transitive closure of glyphs needed for all of the
 * provided lookups.
 *
 * Since: 1.8.1
 **/
void
hb_ot_layout_lookups_substitute_closure (hb_face_t      *face,
                                         const hb_set_t *lookups,
                                         hb_set_t       *glyphs)
{
  hb_auto_t<hb_map_t> done_lookups;
  OT::hb_closure_context_t c (face, glyphs, &done_lookups);
  const OT::GSUB& gsub = _get_gsub (face);

  unsigned int iteration_count = 0;
  unsigned int glyphs_length;
  do
  {
    glyphs_length = glyphs->get_population ();
    if (lookups != nullptr)
    {
      for (hb_codepoint_t lookup_index = HB_SET_VALUE_INVALID; hb_set_next (lookups, &lookup_index);)
        gsub.get_lookup (lookup_index).closure (&c, lookup_index);
    }
    else
    {
      for (unsigned int i = 0; i < gsub.get_lookup_count (); i++)
        gsub.get_lookup (i).closure (&c, i);
    }
    iteration_count++;
  } while (iteration_count <= HB_CLOSURE_MAX_STAGES
           && glyphs_length != glyphs->get_population ());
}

/*
 * OT::GPOS
 */

hb_bool_t
hb_ot_layout_has_positioning (hb_face_t *face)
{
  return _get_gpos (face).has_data ();
}

void
hb_ot_layout_position_start (hb_font_t *font, hb_buffer_t *buffer)
{
  OT::GPOS::position_start (font, buffer);
}

void
hb_ot_layout_position_finish_advances (hb_font_t *font, hb_buffer_t *buffer)
{
  OT::GPOS::position_finish_advances (font, buffer);
}

void
hb_ot_layout_position_finish_offsets (hb_font_t *font, hb_buffer_t *buffer)
{
  OT::GPOS::position_finish_offsets (font, buffer);
}

/**
 * hb_ot_layout_get_size_params:
 *
 * Since: 0.9.10
 **/
hb_bool_t
hb_ot_layout_get_size_params (hb_face_t    *face,
			      unsigned int *design_size,       /* OUT.  May be nullptr */
			      unsigned int *subfamily_id,      /* OUT.  May be nullptr */
			      unsigned int *subfamily_name_id, /* OUT.  May be nullptr */
			      unsigned int *range_start,       /* OUT.  May be nullptr */
			      unsigned int *range_end          /* OUT.  May be nullptr */)
{
  const OT::GPOS &gpos = _get_gpos (face);
  const hb_tag_t tag = HB_TAG ('s','i','z','e');

  unsigned int num_features = gpos.get_feature_count ();
  for (unsigned int i = 0; i < num_features; i++)
  {
    if (tag == gpos.get_feature_tag (i))
    {
      const OT::Feature &f = gpos.get_feature (i);
      const OT::FeatureParamsSize &params = f.get_feature_params ().get_size_params (tag);

      if (params.designSize)
      {
#define PARAM(a, A) if (a) *a = params.A
	PARAM (design_size, designSize);
	PARAM (subfamily_id, subfamilyID);
	PARAM (subfamily_name_id, subfamilyNameID);
	PARAM (range_start, rangeStart);
	PARAM (range_end, rangeEnd);
#undef PARAM

	return true;
      }
    }
  }

#define PARAM(a, A) if (a) *a = 0
  PARAM (design_size, designSize);
  PARAM (subfamily_id, subfamilyID);
  PARAM (subfamily_name_id, subfamilyNameID);
  PARAM (range_start, rangeStart);
  PARAM (range_end, rangeEnd);
#undef PARAM

  return false;
}


/*
 * Parts of different types are implemented here such that they have direct
 * access to GSUB/GPOS lookups.
 */


struct GSUBProxy
{
  static const unsigned int table_index = 0;
  static const bool inplace = false;
  typedef OT::SubstLookup Lookup;

  GSUBProxy (hb_face_t *face) :
    table (*hb_ot_face_data (face)->table.GSUB->table),
    accels (hb_ot_face_data (face)->table.GSUB->accels) {}

  const OT::GSUB &table;
  const hb_ot_layout_lookup_accelerator_t *accels;
};

struct GPOSProxy
{
  static const unsigned int table_index = 1;
  static const bool inplace = true;
  typedef OT::PosLookup Lookup;

  GPOSProxy (hb_face_t *face) :
    table (*hb_ot_face_data (face)->table.GPOS->table),
    accels (hb_ot_face_data (face)->table.GPOS->accels) {}

  const OT::GPOS &table;
  const hb_ot_layout_lookup_accelerator_t *accels;
};


struct hb_get_subtables_context_t :
       hb_dispatch_context_t<hb_get_subtables_context_t, hb_void_t, HB_DEBUG_APPLY>
{
  template <typename Type>
  static inline bool apply_to (const void *obj, OT::hb_ot_apply_context_t *c)
  {
    const Type *typed_obj = (const Type *) obj;
    return typed_obj->apply (c);
  }

  typedef bool (*hb_apply_func_t) (const void *obj, OT::hb_ot_apply_context_t *c);

  struct hb_applicable_t
  {
    inline void init (const void *obj_, hb_apply_func_t apply_func_)
    {
      obj = obj_;
      apply_func = apply_func_;
    }

    inline bool apply (OT::hb_ot_apply_context_t *c) const { return apply_func (obj, c); }

    private:
    const void *obj;
    hb_apply_func_t apply_func;
  };

  typedef hb_auto_t<hb_vector_t<hb_applicable_t> > array_t;

  /* Dispatch interface. */
  inline const char *get_name (void) { return "GET_SUBTABLES"; }
  template <typename T>
  inline return_t dispatch (const T &obj)
  {
    hb_applicable_t *entry = array.push();
    entry->init (&obj, apply_to<T>);
    return HB_VOID;
  }
  static return_t default_return_value (void) { return HB_VOID; }
  bool stop_sublookup_iteration (return_t r HB_UNUSED) const { return false; }

  hb_get_subtables_context_t (array_t &array_) :
			      array (array_),
			      debug_depth (0) {}

  array_t &array;
  unsigned int debug_depth;
};

static inline bool
apply_forward (OT::hb_ot_apply_context_t *c,
	       const hb_ot_layout_lookup_accelerator_t &accel,
	       const hb_get_subtables_context_t::array_t &subtables)
{
  bool ret = false;
  hb_buffer_t *buffer = c->buffer;
  while (buffer->idx < buffer->len && buffer->successful)
  {
    bool applied = false;
    if (accel.may_have (buffer->cur().codepoint) &&
	(buffer->cur().mask & c->lookup_mask) &&
	c->check_glyph_property (&buffer->cur(), c->lookup_props))
     {
       for (unsigned int i = 0; i < subtables.len; i++)
         if (subtables[i].apply (c))
	 {
	   applied = true;
	   break;
	 }
     }

    if (applied)
      ret = true;
    else
      buffer->next_glyph ();
  }
  return ret;
}

static inline bool
apply_backward (OT::hb_ot_apply_context_t *c,
	       const hb_ot_layout_lookup_accelerator_t &accel,
	       const hb_get_subtables_context_t::array_t &subtables)
{
  bool ret = false;
  hb_buffer_t *buffer = c->buffer;
  do
  {
    if (accel.may_have (buffer->cur().codepoint) &&
	(buffer->cur().mask & c->lookup_mask) &&
	c->check_glyph_property (&buffer->cur(), c->lookup_props))
    {
     for (unsigned int i = 0; i < subtables.len; i++)
       if (subtables[i].apply (c))
       {
	 ret = true;
	 break;
       }
    }
    /* The reverse lookup doesn't "advance" cursor (for good reason). */
    buffer->idx--;

  }
  while ((int) buffer->idx >= 0);
  return ret;
}

template <typename Proxy>
static inline void
apply_string (OT::hb_ot_apply_context_t *c,
	      const typename Proxy::Lookup &lookup,
	      const hb_ot_layout_lookup_accelerator_t &accel)
{
  hb_buffer_t *buffer = c->buffer;

  if (unlikely (!buffer->len || !c->lookup_mask))
    return;

  c->set_lookup_props (lookup.get_props ());

  hb_get_subtables_context_t::array_t subtables;
  hb_get_subtables_context_t c_get_subtables (subtables);
  lookup.dispatch (&c_get_subtables);

  if (likely (!lookup.is_reverse ()))
  {
    /* in/out forward substitution/positioning */
    if (Proxy::table_index == 0)
      buffer->clear_output ();
    buffer->idx = 0;

    bool ret;
    ret = apply_forward (c, accel, subtables);
    if (ret)
    {
      if (!Proxy::inplace)
	buffer->swap_buffers ();
      else
	assert (!buffer->has_separate_output ());
    }
  }
  else
  {
    /* in-place backward substitution/positioning */
    if (Proxy::table_index == 0)
      buffer->remove_output ();
    buffer->idx = buffer->len - 1;

    apply_backward (c, accel, subtables);
  }
}

template <typename Proxy>
inline void hb_ot_map_t::apply (const Proxy &proxy,
				const hb_ot_shape_plan_t *plan,
				hb_font_t *font,
				hb_buffer_t *buffer) const
{
  const unsigned int table_index = proxy.table_index;
  unsigned int i = 0;
  OT::hb_ot_apply_context_t c (table_index, font, buffer);
  c.set_recurse_func (Proxy::Lookup::apply_recurse_func);

  for (unsigned int stage_index = 0; stage_index < stages[table_index].len; stage_index++) {
    const stage_map_t *stage = &stages[table_index][stage_index];
    for (; i < stage->last_lookup; i++)
    {
      unsigned int lookup_index = lookups[table_index][i].index;
      if (!buffer->message (font, "start lookup %d", lookup_index)) continue;
      c.set_lookup_index (lookup_index);
      c.set_lookup_mask (lookups[table_index][i].mask);
      c.set_auto_zwj (lookups[table_index][i].auto_zwj);
      c.set_auto_zwnj (lookups[table_index][i].auto_zwnj);
      apply_string<Proxy> (&c,
			   proxy.table.get_lookup (lookup_index),
			   proxy.accels[lookup_index]);
      (void) buffer->message (font, "end lookup %d", lookup_index);
    }

    if (stage->pause_func)
    {
      buffer->clear_output ();
      stage->pause_func (plan, font, buffer);
    }
  }
}

void hb_ot_map_t::substitute (const hb_ot_shape_plan_t *plan, hb_font_t *font, hb_buffer_t *buffer) const
{
  GSUBProxy proxy (font->face);
  apply (proxy, plan, font, buffer);
}

void hb_ot_map_t::position (const hb_ot_shape_plan_t *plan, hb_font_t *font, hb_buffer_t *buffer) const
{
  GPOSProxy proxy (font->face);
  apply (proxy, plan, font, buffer);
}

void
hb_ot_layout_substitute_lookup (OT::hb_ot_apply_context_t *c,
				const OT::SubstLookup &lookup,
				const hb_ot_layout_lookup_accelerator_t &accel)
{
  apply_string<GSUBProxy> (c, lookup, accel);
}




/*
 * OT::BASE
 */

// /**
//  * hb_ot_base_has_data:
//  * @face: #hb_face_t to test
//  *
//  * This function allows to verify the presence of an OpenType BASE table on the
//  * face.
//  *
//  * Return value: true if face has a BASE table, false otherwise
//  *
//  * Since: XXX
//  **/
// hb_bool_t
// hb_ot_base_has_data (hb_face_t *face)
// {
//   return _get_base (face).has_data ();
// }
