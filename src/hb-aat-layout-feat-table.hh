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
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this)));
  }

  HBUINT16	setting;	/* The setting. */
  NameID	nameIndex;	/* The name table index for the setting's name. */
  public:
  DEFINE_SIZE_STATIC (4);
};

struct feat;

struct FeatureName
{
<<<<<<< HEAD
=======
  static int cmp (const void *key_, const void *entry_)
  {
    hb_aat_feature_type_t key = * (hb_aat_feature_type_t *) key_;
    const FeatureName * entry = (const FeatureName *) entry_;
    return key < entry->feature ? -1 :
	   key > entry->feature ? 1 :
	   0;
  }

>>>>>>> 08982bb4... [feat] Expose public API
  enum {
    Exclusive = 0x8000,		/* If set, the feature settings are mutually exclusive. */
    NotDefault = 0x4000,	/* If clear, then the setting with an index of 0 in
				 * the setting name array for this feature should
				 * be taken as the default for the feature
				 * (if one is required). If set, then bits 0-15 of this
				 * featureFlags field contain the index of the setting
				 * which is to be taken as the default. */
    IndexMask = 0x00FF		/* If bits 30 and 31 are set, then these sixteen bits
				 * indicate the index of the setting in the setting name
				 * array for this feature which should be taken
				 * as the default. */
  };

<<<<<<< HEAD
=======
  inline unsigned int get_settings (const feat                     *feat,
				    hb_bool_t                      *is_exclusive,
				    unsigned int                    start_offset,
				    unsigned int                   *records_count,
				    hb_aat_feature_option_record_t *records_buffer) const
  {
    bool exclusive = featureFlags & Exclusive;
    bool not_default = featureFlags & NotDefault;
    if (is_exclusive) *is_exclusive = exclusive;
    const UnsizedArrayOf<SettingName>& settings = feat+settingTable;
    unsigned int len = 0;
    unsigned int settings_count = nSettings;
    if (records_count && records_buffer)
    {
      len = MIN (settings_count - start_offset, *records_count);
      for (unsigned int i = 0; i < len; i++)
      {
	records_buffer[i].is_default = exclusive && not_default &&
				       i + start_offset == (featureFlags & IndexMask);
	records_buffer[i].name_id = settings[start_offset + i].nameIndex;
	records_buffer[i].setting = settings[start_offset + i].setting;
      }
      if (exclusive && !not_default && start_offset == 0 && len != 0)
        records_buffer[0].is_default = true;
    }
    if (is_exclusive) *is_exclusive = exclusive;
    if (records_count) *records_count = len;
    return settings_count;
  }

>>>>>>> 08982bb4... [feat] Expose public API
  inline bool sanitize (hb_sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) &&
			  (base+settingTable).sanitize (c, nSettings)));
  }

  protected:
  HBUINT16	feature;	/* Feature type. */
  HBUINT16	nSettings;	/* The number of records in the setting name array. */
  LOffsetTo<UnsizedArrayOf<SettingName>, false>
		settingTable;	/* Offset in bytes from the beginning of this table to
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

  inline const FeatureName& get_feature (hb_aat_feature_type_t key) const
  {
    const FeatureName* feature = (FeatureName*) hb_bsearch (&key, &names,
							    FeatureName::static_size,
							    sizeof (FeatureName),
							    FeatureName::cmp);

    return feature ? *feature : Null (FeatureName);
  }

  inline unsigned int get_settings (hb_aat_feature_type_t           key,
				    hb_bool_t                      *is_exclusive,
				    unsigned int                    start_offset,
				    unsigned int                   *records_count,
				    hb_aat_feature_option_record_t *records_buffer) const
  {
    return get_feature (key).get_settings (this, is_exclusive, start_offset,
					   records_count, records_buffer);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) &&
			  names.sanitize (c, featureNameCount, this)));
  }

  protected:
  FixedVersion<>version;	/* Version number of the feature name table
				 * (0x00010000 for the current version). */
  HBUINT16	featureNameCount;
				/* The number of entries in the feature name array. */
  HBUINT16	reserved1;	/* Reserved (set to zero). */
  HBUINT32	reserved2;	/* Reserved (set to zero). */
  UnsizedArrayOf<FeatureName>
		names;		/* The feature name array. */
  public:
  DEFINE_SIZE_STATIC (24);
};

} /* namespace AAT */

#endif /* HB_AAT_LAYOUT_FEAT_TABLE_HH */
