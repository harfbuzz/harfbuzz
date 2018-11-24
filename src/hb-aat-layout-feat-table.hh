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

#ifndef HB_AAT_LAYOUT_FEAT_TABLE_HH
#define HB_AAT_LAYOUT_FEAT_TABLE_HH

#include "hb-aat-layout-common.hh"

/*
 * feat -- Feature Name
 * https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6feat.html
 */
#define HB_AAT_TAG_feat HB_TAG('f','e','a','t')


namespace AAT {


struct SettingName
{
  int cmp (hb_aat_layout_feature_selector_t key) const
  { return (int) key - (int) setting; }

  inline hb_aat_layout_feature_selector_t get_selector () const
  { return (hb_aat_layout_feature_selector_t) (unsigned int) setting; }

  inline hb_ot_name_id_t get_name_id () const { return nameIndex; }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this)));
  }

  protected:
  HBUINT16	setting;	/* The setting. */
  NameID	nameIndex;	/* The name table index for the setting's name. */
  public:
  DEFINE_SIZE_STATIC (4);
};
DECLARE_NULL_NAMESPACE_BYTES (AAT, SettingName);

struct feat;

struct FeatureName
{
  int cmp (hb_aat_layout_feature_type_t key) const
  { return (int) key - (int) feature; }

  enum {
    Exclusive	= 0x8000,	/* If set, the feature settings are mutually exclusive. */
    NotDefault	= 0x4000,	/* If clear, then the setting with an index of 0 in
				 * the setting name array for this feature should
				 * be taken as the default for the feature
				 * (if one is required). If set, then bits 0-15 of this
				 * featureFlags field contain the index of the setting
				 * which is to be taken as the default. */
    IndexMask	= 0x00FF	/* If bits 30 and 31 are set, then these sixteen bits
				 * indicate the index of the setting in the setting name
				 * array for this feature which should be taken
				 * as the default. */
  };

  inline unsigned int get_selectors (const feat                       *feat,
				     hb_aat_layout_feature_selector_t *default_selector,
				     unsigned int                      start_offset,
				     unsigned int                     *count,
				     hb_aat_layout_feature_selector_t *selectors) const
  {
    const UnsizedArrayOf<SettingName>& settings_table = feat+settingTableZ;
    unsigned int settings_count = nSettings;
    if (count && *count)
    {
      unsigned int len = MIN (settings_count - start_offset, *count);
      for (unsigned int i = 0; i < len; i++)
        selectors[i] = settings_table[start_offset + i].get_selector ();
      *count = len;
    }
    if (default_selector)
    {
      unsigned int index = (featureFlags & NotDefault) ? featureFlags & IndexMask : 0;
      *default_selector = ((featureFlags & Exclusive) && index < settings_count)
			  ? settings_table[index].get_selector ()
			  : HB_AAT_LAYOUT_FEATURE_SELECTOR_INVALID;
    }
    return settings_count;
  }

  inline hb_aat_layout_feature_type_t get_feature_type () const
  { return (hb_aat_layout_feature_type_t) (unsigned int) feature; }

  inline hb_ot_name_id_t get_feature_name_id () const { return nameIndex; }

  inline hb_ot_name_id_t get_feature_selector_name_id (const feat                       *feat,
						       hb_aat_layout_feature_selector_t  key) const
  {
    return (feat+settingTableZ).lsearch (nSettings, key).get_name_id ();
  }

  inline bool sanitize (hb_sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) &&
			  (base+settingTableZ).sanitize (c, nSettings)));
  }

  protected:
  HBUINT16	feature;	/* Feature type. */
  HBUINT16	nSettings;	/* The number of records in the setting name array. */
  LOffsetTo<UnsizedArrayOf<SettingName>, false>
		settingTableZ;	/* Offset in bytes from the beginning of this table to
				 * this feature's setting name array. The actual type of
				 * record this offset refers to will depend on the
				 * exclusivity value, as described below. */
  HBUINT16	featureFlags;	/* Single-bit flags associated with the feature type. */
  HBINT16	nameIndex;	/* The name table index for the feature's name.
				 * This index has values greater than 255 and
				 * less than 32768. */
  public:
  DEFINE_SIZE_STATIC (12);
};

struct feat
{
  static const hb_tag_t tableTag = HB_AAT_TAG_feat;

  inline bool has_data (void) const { return version.to_int (); }

  inline unsigned int get_feature_types (unsigned int                  start_offset,
					 unsigned int                 *count,
					 hb_aat_layout_feature_type_t *features) const
  {
    unsigned int feature_count = featureNameCount;
    if (count && *count)
    {
      unsigned int len = MIN (feature_count - start_offset, *count);
      for (unsigned int i = 0; i < len; i++)
	features[i] = namesZ[i + start_offset].get_feature_type ();
      *count = len;
    }
    return featureNameCount;
  }

  inline const FeatureName& get_feature (hb_aat_layout_feature_type_t key) const
  {
    return namesZ.bsearch (featureNameCount, key);
  }

  inline hb_ot_name_id_t get_feature_name_id (hb_aat_layout_feature_type_t feature) const
  { return get_feature (feature).get_feature_name_id (); }

  inline hb_ot_name_id_t get_feature_selector_name_id (hb_aat_layout_feature_type_t    feature,
						      hb_aat_layout_feature_selector_t selector) const
  { return get_feature (feature).get_feature_selector_name_id (this, selector); }

  inline unsigned int get_selectors (hb_aat_layout_feature_type_t     key,
				    hb_aat_layout_feature_selector_t *default_selector,
				    unsigned int                      start_offset,
				    unsigned int                     *count,
				    hb_aat_layout_feature_selector_t *selectors) const
  {
    return get_feature (key).get_selectors (this, default_selector, start_offset,
					    count, selectors);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) &&
			  namesZ.sanitize (c, featureNameCount, this)));
  }

  protected:
  FixedVersion<>version;	/* Version number of the feature name table
				 * (0x00010000 for the current version). */
  HBUINT16	featureNameCount;
				/* The number of entries in the feature name array. */
  HBUINT16	reserved1;	/* Reserved (set to zero). */
  HBUINT32	reserved2;	/* Reserved (set to zero). */
  SortedUnsizedArrayOf<FeatureName>
		namesZ;		/* The feature name array. */
  public:
  DEFINE_SIZE_STATIC (24);
};

} /* namespace AAT */

#endif /* HB_AAT_LAYOUT_FEAT_TABLE_HH */
