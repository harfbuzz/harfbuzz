/*
 * Copyright © 2016 Elie Roux <elie.roux@telecom-bretagne.eu>
 * Copyright © 2018  Google, Inc.
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
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_OT_LAYOUT_BASE_TABLE_HH
#define HB_OT_LAYOUT_BASE_TABLE_HH

#include "hb-open-type.hh"
#include "hb-ot-layout-common.hh"

namespace OT {

/*
 * BASE -- Baseline
 * https://docs.microsoft.com/en-us/typography/opentype/spec/base
 */


/* XXX Review this. */
#define NOT_INDEXED		((unsigned int) -1)


struct BaseCoordFormat1
{
  inline hb_position_t get_coord () const { return coordinate; }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  protected:
  HBUINT16	format;		/* Format identifier--format = 1 */
  FWORD		coordinate;	/* X or Y value, in design units */
  public:
  DEFINE_SIZE_STATIC (4);
};

struct BaseCoordFormat2
{
  inline hb_position_t get_coord () const
  {
    /* TODO */
    return coordinate;
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  protected:
  HBUINT16	format;		/* Format identifier--format = 2 */
  FWORD		coordinate;	/* X or Y value, in design units */
  GlyphID	referenceGlyph;	/* Glyph ID of control glyph */
  HBUINT16	coordPoint;	/* Index of contour point on the
				 * reference glyph */
  public:
  DEFINE_SIZE_STATIC (8);
};

struct BaseCoordFormat3
{
  inline hb_position_t get_coord () const
  {
    /* TODO */
    return coordinate;
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) && deviceTable.sanitize (c, this));
  }

  protected:
  HBUINT16	format;		/* Format identifier--format = 3 */
  FWORD		coordinate;	/* X or Y value, in design units */
  OffsetTo<Device>
		deviceTable;	/* Offset to Device table for X or
				 * Y value, from beginning of
				 * BaseCoord table (may be NULL). */
  public:
  DEFINE_SIZE_STATIC (6);
};

struct BaseCoord
{
  inline hb_position_t get_coord () const
  {
    /* XXX wire up direction and font. */
    switch (u.format) {
    case 1: return u.format1.get_coord ();
    case 2: return u.format2.get_coord ();
    case 3: return u.format3.get_coord ();
    default:return 0;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (!u.format.sanitize (c)) return_trace (false);
    switch (u.format) {
    case 1: return_trace (u.format1.sanitize (c));
    case 2: return_trace (u.format2.sanitize (c));
    case 3: return_trace (u.format3.sanitize (c));
    default:return_trace (false);
    }
  }

  protected:
  union {
    HBUINT16		format;
    BaseCoordFormat1	format1;
    BaseCoordFormat2	format2;
    BaseCoordFormat3	format3;
  } u;
  public:
  DEFINE_SIZE_UNION (2, format);
};

struct FeatMinMaxRecord
{
  static int cmp (const void *key_, const void *entry_)
  {
    hb_ot_layout_baseline_t key = * (hb_ot_layout_baseline_t *) key_;
    const FeatMinMaxRecord &entry = * (const FeatMinMaxRecord *) entry_;
    return key < (unsigned int) entry.tag ? -1 :
	   key > (unsigned int) entry.tag ? 1 :
	   0;
  }

  inline hb_position_t get_min_value (void) const { return (this+minCoord).get_coord (); }
  inline hb_position_t get_max_value (void) const { return (this+maxCoord).get_coord (); }

  inline bool sanitize (hb_sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  minCoord.sanitize (c, base) &&
		  maxCoord.sanitize (c, base));
  }

  protected:
  Tag		tag;		/* 4-byte feature identification tag--must
				 * match feature tag in FeatureList */
  OffsetTo<BaseCoord>
		minCoord;	/* Offset to BaseCoord table that defines
				 * the minimum extent value, from beginning
				 * of MinMax table (may be NULL) */
  OffsetTo<BaseCoord>
		maxCoord;	/* Offset to BaseCoord table that defines
				 * the maximum extent value, from beginning
				 * of MinMax table (may be NULL) */
  public:
  DEFINE_SIZE_STATIC (8);

};

struct MinMax
{
  private:
  inline const FeatMinMaxRecord *find_record (hb_ot_layout_baseline_t baseline) const
  {
    return (FeatMinMaxRecord *) hb_bsearch (&baseline, featMinMaxRecords.arrayZ,
					    featMinMaxRecords.len,
					    FeatMinMaxRecord::static_size,
					    FeatMinMaxRecord::cmp);
  }

  public:
  inline hb_position_t get_min_value (hb_ot_layout_baseline_t baseline) const
  {
    const FeatMinMaxRecord *minMaxCoord = find_record (baseline);
    if (!minMaxCoord) return (this+minCoord).get_coord ();
    return minMaxCoord->get_min_value ();
  }

  inline int get_max_value (hb_ot_layout_baseline_t baseline) const
  {
    const FeatMinMaxRecord *minMaxCoord = find_record (baseline);
    if (!minMaxCoord) return (this+maxCoord).get_coord ();
    return minMaxCoord->get_max_value ();
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  minCoord.sanitize (c, this) &&
		  maxCoord.sanitize (c, this) &&
		  featMinMaxRecords.sanitize (c, this));
  }

  protected:
  OffsetTo<BaseCoord>
		minCoord;	/* Offset to BaseCoord table that defines
				 * minimum extent value, from the beginning
				 * of MinMax table (may be NULL) */
  OffsetTo<BaseCoord>
		maxCoord;	/* Offset to BaseCoord table that defines
				 * maximum extent value, from the beginning
				 * of MinMax table (may be NULL) */
  ArrayOf<FeatMinMaxRecord>
		featMinMaxRecords;
				/* Array of FeatMinMaxRecords, in alphabetical
				 * order by featureTableTag */
  public:
  DEFINE_SIZE_ARRAY (6, featMinMaxRecords);
};

struct BaseValues
{
  inline unsigned int get_default_base_tag_index () const
  { return defaultIndex; }

  inline hb_position_t get_base_coord (unsigned int baselineTagIndex) const
  { return (this+baseCoords[baselineTagIndex]).get_coord (); }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  baseCoords.sanitize (c, this));
  }

  protected:
  Index		defaultIndex;	/* Index number of default baseline for this
				 * script — equals index position of baseline tag
				 * in baselineTags array of the BaseTagList */
  OffsetArrayOf<BaseCoord>
		baseCoords;	/* Number of BaseCoord tables defined — should equal
				 * baseTagCount in the BaseTagList
				 *
				 * Array of offsets to BaseCoord tables, from beginning of
				 * BaseValues table — order matches baselineTags array in
				 * the BaseTagList */
  public:
  DEFINE_SIZE_ARRAY (4, baseCoords);
};

struct BaseLangSysRecord
{
  static int cmp (const void *key_, const void *entry_)
  {
    hb_tag_t key = * (hb_tag_t *) key_;
    const BaseLangSysRecord &entry = * (const BaseLangSysRecord *) entry_;
    return key < (unsigned int) entry.baseLangSysTag ? -1 :
	   key > (unsigned int) entry.baseLangSysTag ? 1 :
	   0;
  }

  inline int get_min_value (hb_ot_layout_baseline_t baseline) const
  { return (this+minMax).get_min_value (baseline); }

  inline int get_max_value (hb_ot_layout_baseline_t baseline) const
  { return (this+minMax).get_max_value (baseline); }

  inline bool sanitize (hb_sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  minMax.sanitize (c, base));
  }

  protected:
  Tag		baseLangSysTag;	/* 4-byte language system identification tag */
  OffsetTo<MinMax>
		minMax;		/* Offset to MinMax table, from beginning
				 * of BaseScript table */
  public:
  DEFINE_SIZE_STATIC (6);
};

struct BaseScript
{
  private:
  inline const BaseLangSysRecord *find_record (hb_tag_t baseline) const
  {
    return (BaseLangSysRecord *) hb_bsearch (&baseline, baseLangSysRecords.arrayZ,
					     baseLangSysRecords.len,
					     BaseLangSysRecord::static_size,
					     BaseLangSysRecord::cmp);
  }

  public:
  inline hb_position_t get_min_value (hb_tag_t language_tag, hb_ot_layout_baseline_t baseline) const
  {
    const BaseLangSysRecord* record = find_record (language_tag);
    if (record) record->get_min_value (baseline);
    else return (this+defaultMinMax).get_min_value (baseline);
  }

  inline hb_position_t get_max_value (hb_tag_t language_tag, hb_ot_layout_baseline_t baseline) const
  {
    const BaseLangSysRecord* record = find_record (language_tag);
    if (record) record->get_max_value (baseline);
    else return (this+defaultMinMax).get_max_value (baseline);
  }

  inline hb_position_t get_base_coord (unsigned int baselineTagIndex) const
  { return (this+baseValues).get_base_coord (baselineTagIndex); }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  baseValues.sanitize (c, this) &&
		  defaultMinMax.sanitize (c, this) &&
		  baseLangSysRecords.sanitize (c, this));
  }

  protected:
  OffsetTo<BaseValues>
		baseValues;	/* Offset to BaseValues table, from beginning
				 * of BaseScript table (may be NULL) */
  OffsetTo<MinMax>
		defaultMinMax;	/* Offset to MinMax table, from beginning of
				 * BaseScript table (may be NULL) */
  ArrayOf<BaseLangSysRecord>
		baseLangSysRecords;
				/* Number of BaseLangSysRecords
				 * defined — may be zero (0) */

  public:
  DEFINE_SIZE_ARRAY (6, baseLangSysRecords);
};


struct BaseScriptRecord
{
  inline const BaseScript &get_base_script () const
  { return (this+baseScript); }

  inline bool sanitize (hb_sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  baseScript.sanitize (c, base));
  }

  protected:
  Tag		baseScriptTag;	/* 4-byte script identification tag */
  OffsetTo<BaseScript>
		baseScript;	/* Offset to BaseScript table, from beginning
				 * of BaseScriptList */

  public:
  DEFINE_SIZE_STATIC (6);
};

struct Axis
{
  inline const BaseScript &get_base_record (hb_ot_layout_baseline_t baseline) const
  { return (this+baseScriptList)[(this+baseTagList).bsearch (baseline)].get_base_script (); }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  (this+baseTagList).sanitize (c) &&
		  (this+baseScriptList).sanitize (c, this));
  }

  protected:
  OffsetTo<SortedArrayOf<Tag> >
		baseTagList;	/* Offset to BaseTagList table, from beginning
				 * of Axis table (may be NULL)
				 * Array of 4-byte baseline identification tags — must
				 * be in alphabetical order */
  OffsetTo<ArrayOf<BaseScriptRecord> >
		baseScriptList;	/* Offset to BaseScriptList table, from beginning
				 * of Axis table
				 * Array of BaseScriptRecords, in alphabetical order
				 * by baseScriptTag */

  public:
  DEFINE_SIZE_STATIC (4);
};

struct BASE
{
  static const hb_tag_t tableTag = HB_OT_TAG_BASE;

  inline const BaseScript& get_base_script (hb_direction_t direction,
					    hb_ot_layout_baseline_t baseline) const
  {
    const Axis &axis = HB_DIRECTION_IS_HORIZONTAL (direction) ? (this+hAxis) : (this+vAxis);
    return axis.get_base_record (baseline);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  likely (version.major == 1) &&
		  hAxis.sanitize (c, this) &&
		  vAxis.sanitize (c, this) &&
		  (version.to_int () < 0x00010001u || varStore.sanitize (c, this)));
  }

  protected:
  FixedVersion<> version;	/* Version of the BASE table */
  OffsetTo<Axis> hAxis;		/* Offset to horizontal Axis table, from beginning
				 * of BASE table (may be NULL) */
  OffsetTo<Axis> vAxis;		/* Offset to vertical Axis table, from beginning
				 * of BASE table (may be NULL) */
  LOffsetTo<VariationStore>
		varStore;	/* Offset to the table of Item Variation
				 * Store--from beginning of BASE
				 * header (may be NULL).  Introduced
				 * in version 0x00010001. */
  public:
  DEFINE_SIZE_MIN (8);
};


} /* namespace OT */


#endif /* HB_OT_LAYOUT_BASE_TABLE_HH */
