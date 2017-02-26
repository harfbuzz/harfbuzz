/*
 * Copyright © 2016 Elie Roux <elie.roux@telecom-bretagne.eu>
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
 */

#ifndef HB_OT_BASE_TABLE_HH
#define HB_OT_BASE_TABLE_HH

#include "hb-open-type-private.hh"
#include "hb-ot-layout-common-private.hh"

#define HB_OT_TAG_BASE      HB_TAG('B','A','S','E')

// https://www.microsoft.com/typography/otspec/baselinetags.htm

#define HB_OT_TAG_BASE_HANG HB_TAG('h','a','n','g')
#define HB_OT_TAG_BASE_ICFB HB_TAG('i','c','f','b')
#define HB_OT_TAG_BASE_ICFT HB_TAG('i','c','f','t')
#define HB_OT_TAG_BASE_IDEO HB_TAG('i','d','e','o')
#define HB_OT_TAG_BASE_IDTB HB_TAG('i','d','t','b')
#define HB_OT_TAG_BASE_MATH HB_TAG('m','a','t','h')
#define HB_OT_TAG_BASE_ROMN HB_TAG('r','o','m','n')

namespace OT {

#define NO_COORD Null(SHORT)//SHORT((short int) -32767)

#define NOT_INDEXED   ((unsigned int) -1)

/*
 * BASE -- The BASE Table
 */

struct BaseCoordFormat1 {

  inline SHORT get_coord (void) const
  { return coordinate; }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  protected:
  USHORT    baseCoordFormat;
  SHORT     coordinate;

  public:
  DEFINE_SIZE_STATIC (4);
};

struct BaseCoordFormat2 {

  inline SHORT get_coord (void) const
  { return coordinate; }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  protected:
  USHORT    baseCoordFormat;
  SHORT     coordinate;
  USHORT    referenceGlyph;
  USHORT    baseCoordPoint;

  public:
  DEFINE_SIZE_STATIC (8);
};

struct BaseCoordFormat3 {

  inline SHORT get_coord (void) const
  { return coordinate; }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) && deviceTable.sanitize (c, this));
  }

  protected:
  USHORT           baseCoordFormat;
  SHORT            coordinate;
  OffsetTo<Device> deviceTable;

  public:
  DEFINE_SIZE_STATIC (6);
};

struct BaseCoord {

  inline SHORT get_coord (void) const
  { return u.format1.get_coord(); }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (!u.baseCoordFormat.sanitize (c)) return_trace (false);
    switch (u.baseCoordFormat) {
    case 1: return_trace (u.format1.sanitize (c));
    case 2: return_trace (u.format2.sanitize (c));
    case 3: return_trace (u.format3.sanitize (c));
    default:return_trace (false);
    }
  }

  protected:
  union {
    USHORT            baseCoordFormat;
    BaseCoordFormat1  format1;
    BaseCoordFormat2  format2;
    BaseCoordFormat3  format3;
  } u;

  public:
  DEFINE_SIZE_MIN (4);
};

struct FeatMinMaxRecord {

  inline SHORT get_min_value (void) const
  {
    if (minCoord == Null(OffsetTo<BaseCoord>)) return NO_COORD;
      return (this+minCoord).get_coord();
  }

  inline SHORT get_max_value (void) const
  {
    if (minCoord == Null(OffsetTo<BaseCoord>)) return NO_COORD;
      return (this+maxCoord).get_coord();
  }

  inline Tag get_tag () const
  { return featureTableTag; }

  inline bool sanitize (hb_sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
      minCoord.sanitize (c, base) &&
      maxCoord.sanitize (c, base));
  }

  protected:
  Tag                   featureTableTag;
  OffsetTo<BaseCoord>   minCoord;
  OffsetTo<BaseCoord>   maxCoord;

  public:
  DEFINE_SIZE_STATIC (8);

};

struct MinMax {

  inline unsigned int get_feature_tag_index (Tag featureTableTag) const
  {
    Tag tag;
    int cmp;
    // taking advantage of alphabetical order
    for (unsigned int i = 0; i < featMinMaxCount; i++) {
      tag = featMinMaxRecords[i].get_tag();
      cmp = tag.cmp(featureTableTag);
      if (cmp == 0) return i;
      if (cmp > 0)  return NOT_INDEXED;
    }
    return NOT_INDEXED;
  }

  inline SHORT get_min_value (unsigned int featureTableTagIndex) const
  {
    if (featureTableTagIndex == NOT_INDEXED) {
      if (minCoord == Null(OffsetTo<BaseCoord>)) return NO_COORD;
      return (this+minCoord).get_coord();
    }
    if (unlikely(featureTableTagIndex >= featMinMaxCount)) return NO_COORD;
    return featMinMaxRecords[featureTableTagIndex].get_min_value();
  }

  inline SHORT get_max_value (unsigned int featureTableTagIndex) const
  {
    if (featureTableTagIndex == NOT_INDEXED) {
      if (minCoord == Null(OffsetTo<BaseCoord>)) return NO_COORD;
      return (this+maxCoord).get_coord();
    }
    if (unlikely(featureTableTagIndex >= featMinMaxCount)) return NO_COORD;
    return featMinMaxRecords[featureTableTagIndex].get_max_value();
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
      minCoord.sanitize (c, this) &&
      maxCoord.sanitize (c, this) &&
      featMinMaxRecords.sanitize (c, this));
    // TODO: test alphabetical order?
  }

  protected:
  OffsetTo<BaseCoord>       minCoord;
  OffsetTo<BaseCoord>       maxCoord;
  USHORT                    featMinMaxCount;
  ArrayOf<FeatMinMaxRecord> featMinMaxRecords;

  public:
  DEFINE_SIZE_ARRAY (8, featMinMaxRecords);

};

struct BaseLangSysRecord {

  inline Tag get_tag(void) const
  { return baseLangSysTag; }

  inline unsigned int get_feature_tag_index (Tag featureTableTag) const
  { (this+minMax).get_feature_tag_index(featureTableTag); }

  inline SHORT get_min_value (unsigned int featureTableTagIndex) const
  { (this+minMax).get_min_value(featureTableTagIndex); }

  inline SHORT get_max_value (unsigned int featureTableTagIndex) const
  { (this+minMax).get_max_value(featureTableTagIndex); }

  inline bool sanitize (hb_sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
      minMax != Null(OffsetTo<MinMax>) &&
      minMax.sanitize (c, base));
  }

  protected:
  Tag               baseLangSysTag;
  OffsetTo<MinMax>  minMax; // not supposed to be NULL

  public:
  DEFINE_SIZE_STATIC (6); 

};

struct BaseValues {

  inline const unsigned int get_default_base_tag_index (void) const
  { return defaultIndex; }

  inline SHORT get_base_coord (unsigned int baselineTagIndex) const
  {
    if (unlikely(baselineTagIndex >= baseCoordCount)) return NO_COORD;
    return (this+baseCoords[baselineTagIndex]).get_coord();
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
      defaultIndex <= baseCoordCount &&
      baseCoords.sanitize (c, this));
  }

  protected:
  Index                     defaultIndex;
  USHORT                    baseCoordCount;
  OffsetArrayOf<BaseCoord>  baseCoords;

  public:
  DEFINE_SIZE_ARRAY (6, baseCoords);

};

struct BaseScript {

  inline unsigned int get_lang_tag_index (Tag baseLangSysTag) const
  {
    Tag tag;
    int cmp;
    for (unsigned int i = 0; i < baseLangSysCount; i++) {
      tag = baseLangSysRecords[i].get_tag();
      // taking advantage of alphabetical order
      cmp = tag.cmp(baseLangSysTag);
      if (cmp == 0) return i;
      if (cmp > 0)  return NOT_INDEXED;
    }
    return NOT_INDEXED;
  }

  inline unsigned int get_feature_tag_index (unsigned int baseLangSysIndex, Tag featureTableTag) const
  {
    if (baseLangSysIndex == NOT_INDEXED) {
      if (unlikely(defaultMinMax)) return NOT_INDEXED;
      return (this+defaultMinMax).get_feature_tag_index(featureTableTag);
    }
    if (unlikely(baseLangSysIndex >= baseLangSysCount)) return NOT_INDEXED;
    return baseLangSysRecords[baseLangSysIndex].get_feature_tag_index(featureTableTag);
  }

  inline SHORT get_min_value (unsigned int baseLangSysIndex, unsigned int featureTableTagIndex) const
  {
    if (baseLangSysIndex == NOT_INDEXED) {
      if (unlikely(defaultMinMax == Null(OffsetTo<MinMax>))) return NO_COORD;
      return (this+defaultMinMax).get_min_value(featureTableTagIndex);
    }
    if (unlikely(baseLangSysIndex >= baseLangSysCount)) return NO_COORD;
    return baseLangSysRecords[baseLangSysIndex].get_max_value(featureTableTagIndex); 
  }

  inline SHORT get_max_value (unsigned int baseLangSysIndex, unsigned int featureTableTagIndex) const
  {
    if (baseLangSysIndex == NOT_INDEXED) {
      if (unlikely(defaultMinMax == Null(OffsetTo<MinMax>))) return NO_COORD;
      return (this+defaultMinMax).get_min_value(featureTableTagIndex);
    }
    if (unlikely(baseLangSysIndex >= baseLangSysCount)) return NO_COORD;
    return baseLangSysRecords[baseLangSysIndex].get_max_value(featureTableTagIndex);
  }

  inline unsigned int get_default_base_tag_index (void) const
  { return (this+baseValues).get_default_base_tag_index(); }

  inline SHORT get_base_coord (unsigned int baselineTagIndex) const
  { return (this+baseValues).get_base_coord(baselineTagIndex); }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
      baseValues.sanitize (c, this) &&
      defaultMinMax.sanitize (c, this) &&
      baseLangSysRecords.sanitize (c, this));
  }

  protected:
  OffsetTo<BaseValues>        baseValues;
  OffsetTo<MinMax>            defaultMinMax;
  USHORT                      baseLangSysCount;
  ArrayOf<BaseLangSysRecord>  baseLangSysRecords;

  public:
    DEFINE_SIZE_ARRAY (8, baseLangSysRecords);
};


struct BaseScriptRecord {

  inline bool get_tag (void) const
  { return baseScriptTag; }

  inline unsigned int get_default_base_tag_index(void) const
  { return (this+baseScript).get_default_base_tag_index(); }

  inline SHORT get_base_coord(unsigned int baselineTagIndex) const
  { return (this+baseScript).get_base_coord(baselineTagIndex); }

  inline unsigned int get_lang_tag_index (Tag baseLangSysTag) const
  { return (this+baseScript).get_lang_tag_index(baseLangSysTag); }

  inline unsigned int get_feature_tag_index (unsigned int baseLangSysIndex, Tag featureTableTag) const
  { return (this+baseScript).get_feature_tag_index(baseLangSysIndex, featureTableTag); }

  inline SHORT get_max_value (unsigned int baseLangSysIndex, unsigned int featureTableTagIndex) const
  { return (this+baseScript).get_max_value(baseLangSysIndex, featureTableTagIndex); }

  inline SHORT get_min_value (unsigned int baseLangSysIndex, unsigned int featureTableTagIndex) const
  { return (this+baseScript).get_min_value(baseLangSysIndex, featureTableTagIndex); }

  inline bool sanitize (hb_sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
      baseScript != Null(OffsetTo<BaseScript>) &&
      baseScript.sanitize (c, base));
  }

  protected:
  Tag                   baseScriptTag;
  OffsetTo<BaseScript>  baseScript;

  public:
    DEFINE_SIZE_STATIC (6);
};

struct BaseScriptList {

  inline unsigned int get_base_script_index (Tag baseScriptTag) const
  {
    for (unsigned int i = 0; i < baseScriptCount; i++)
      if (baseScriptRecords[i].get_tag() == baseScriptTag)
        return i;
    return NOT_INDEXED;
  }

  inline unsigned int get_default_base_tag_index (unsigned int baseScriptIndex) const
  { 
    if (unlikely(baseScriptIndex >= baseScriptCount)) return NOT_INDEXED;
    return baseScriptRecords[baseScriptIndex].get_default_base_tag_index();
  }

  inline SHORT get_base_coord(unsigned int baseScriptIndex, unsigned int baselineTagIndex) const
  {
    if (unlikely(baseScriptIndex >= baseScriptCount)) return NO_COORD;
    return baseScriptRecords[baseScriptIndex].get_base_coord(baselineTagIndex);
  }

  inline unsigned int get_lang_tag_index (unsigned int baseScriptIndex, Tag baseLangSysTag) const
  {
    if (unlikely(baseScriptIndex >= baseScriptCount)) return NOT_INDEXED;
    return baseScriptRecords[baseScriptIndex].get_lang_tag_index(baseLangSysTag);
  }

  inline unsigned int get_feature_tag_index (unsigned int baseScriptIndex, unsigned int baseLangSysIndex, Tag featureTableTag) const
  {
    if (unlikely(baseScriptIndex >= baseScriptCount)) return NOT_INDEXED;
    return baseScriptRecords[baseScriptIndex].get_feature_tag_index(baseLangSysIndex, featureTableTag);
  }

  inline SHORT get_max_value (unsigned int baseScriptIndex, unsigned int baseLangSysIndex, unsigned int featureTableTagIndex) const
  {
    if (unlikely(baseScriptIndex >= baseScriptCount)) return NO_COORD;
    return baseScriptRecords[baseScriptIndex].get_max_value(baseLangSysIndex, featureTableTagIndex);
  }

  inline SHORT get_min_value (unsigned int baseScriptIndex, unsigned int baseLangSysIndex, unsigned int featureTableTagIndex) const
  {
    if (unlikely(baseScriptIndex >= baseScriptCount)) return NO_COORD;
    return baseScriptRecords[baseScriptIndex].get_min_value(baseLangSysIndex, featureTableTagIndex);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
      baseScriptRecords.sanitize (c, this));
  }

  protected:
  USHORT                    baseScriptCount;
  ArrayOf<BaseScriptRecord> baseScriptRecords;

  public:
  DEFINE_SIZE_ARRAY (4, baseScriptRecords);
  
};

struct BaseTagList
{

  inline unsigned int get_tag_index(Tag baselineTag) const
  {
    for (unsigned int i = 0; i < baseTagCount; i++)
      if (baselineTags[i] == baselineTag)
        return i;
    return NOT_INDEXED;
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  protected:
  USHORT        baseTagCount;
  SortedArrayOf<Tag>  baselineTags;

  public:
  DEFINE_SIZE_ARRAY (4, baselineTags);
};

struct Axis
{

  inline unsigned int get_base_tag_index(Tag baselineTag) const
  {
    if (unlikely(baseTagList == Null(OffsetTo<BaseTagList>))) return NOT_INDEXED;
    return (this+baseTagList).get_tag_index(baselineTag);
  }

  inline unsigned int get_default_base_tag_index_for_script_index (unsigned int baseScriptIndex) const
  { 
    if (unlikely(baseScriptList == Null(OffsetTo<BaseScriptList>))) return NOT_INDEXED;
    return (this+baseScriptList).get_default_base_tag_index(baseScriptIndex);
  }

  inline SHORT get_base_coord(unsigned int baseScriptIndex, unsigned int baselineTagIndex) const
  {
    if (unlikely(baseScriptList == Null(OffsetTo<BaseScriptList>))) return NO_COORD;
    return (this+baseScriptList).get_base_coord(baseScriptIndex, baselineTagIndex);
  }

  inline unsigned int get_lang_tag_index (unsigned int baseScriptIndex, Tag baseLangSysTag) const
  {
    if (unlikely(baseScriptList == Null(OffsetTo<BaseScriptList>))) return NOT_INDEXED;
    return (this+baseScriptList).get_lang_tag_index(baseScriptIndex, baseLangSysTag);
  }

  inline unsigned int get_feature_tag_index (unsigned int baseScriptIndex, unsigned int baseLangSysIndex, Tag featureTableTag) const
  {
    if (unlikely(baseScriptList == Null(OffsetTo<BaseScriptList>))) return NOT_INDEXED;
    return (this+baseScriptList).get_feature_tag_index(baseScriptIndex, baseLangSysIndex, featureTableTag);
  }

  inline SHORT get_max_value (unsigned int baseScriptIndex, unsigned int baseLangSysIndex, unsigned int featureTableTagIndex) const
  {
    if (unlikely(baseScriptList == Null(OffsetTo<BaseScriptList>))) return NO_COORD;
    return (this+baseScriptList).get_max_value(baseScriptIndex, baseLangSysIndex, featureTableTagIndex);
  }

  inline SHORT get_min_value (unsigned int baseScriptIndex, unsigned int baseLangSysIndex, unsigned int featureTableTagIndex) const
  {
    if (unlikely(baseScriptList == Null(OffsetTo<BaseScriptList>))) return NO_COORD;
    return (this+baseScriptList).get_min_value(baseScriptIndex, baseLangSysIndex, featureTableTagIndex);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
      baseTagList.sanitize (c, this) &&
      baseScriptList.sanitize (c, this));
  }

  protected:
  OffsetTo<BaseTagList>     baseTagList;
  OffsetTo<BaseScriptList>  baseScriptList;

  public:
  DEFINE_SIZE_STATIC (4);
};

struct BASEFormat1_1
{

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
      horizAxis.sanitize (c, this) &&
      vertAxis.sanitize (c, this) && 
      itemVarStore.sanitize (c, this));
  }

  protected:
  FixedVersion<>            version;
  OffsetTo<Axis>            horizAxis;
  OffsetTo<Axis>            vertAxis;
  LOffsetTo<VariationStore> itemVarStore;

  public:
  DEFINE_SIZE_STATIC (12);
};

struct BASEFormat1_0
{

  inline bool has_vert_axis(void)
  { return vertAxis != Null(OffsetTo<Axis>); }

  inline bool has_horiz_axis(void)
  { return horizAxis != Null(OffsetTo<Axis>); }

  // horizontal axis base coords:

  inline unsigned int get_horiz_base_tag_index(Tag baselineTag) const
  {
    if (unlikely(horizAxis == Null(OffsetTo<Axis>))) return NOT_INDEXED;
    return (this+horizAxis).get_base_tag_index(baselineTag);
  }

  inline unsigned int get_horiz_default_base_tag_index_for_script_index (unsigned int baseScriptIndex) const
  { 
    if (unlikely(horizAxis == Null(OffsetTo<Axis>))) return NOT_INDEXED;
    return (this+horizAxis).get_default_base_tag_index_for_script_index(baseScriptIndex);
  }

  inline SHORT get_horiz_base_coord(unsigned int baseScriptIndex, unsigned int baselineTagIndex) const
  {
    if (unlikely(horizAxis == Null(OffsetTo<Axis>))) return NO_COORD;
    return (this+horizAxis).get_base_coord(baseScriptIndex, baselineTagIndex);
  }

  // vertical axis base coords:

  inline unsigned int get_vert_base_tag_index(Tag baselineTag) const
  {
    if (unlikely(vertAxis == Null(OffsetTo<Axis>))) return NOT_INDEXED;
    return (this+vertAxis).get_base_tag_index(baselineTag);
  }

  inline unsigned int get_vert_default_base_tag_index_for_script_index (unsigned int baseScriptIndex) const
  { 
    if (unlikely(vertAxis == Null(OffsetTo<Axis>))) return NOT_INDEXED;
    return (this+vertAxis).get_default_base_tag_index_for_script_index(baseScriptIndex);
  }

  inline SHORT get_vert_base_coord(unsigned int baseScriptIndex, unsigned int baselineTagIndex) const
  {
    if (vertAxis == Null(OffsetTo<Axis>)) return NO_COORD;
    return (this+vertAxis).get_base_coord(baseScriptIndex, baselineTagIndex);
  }

  // horizontal axis min/max coords:

  inline unsigned int get_horiz_lang_tag_index (unsigned int baseScriptIndex, Tag baseLangSysTag) const
  {
    if (unlikely(horizAxis == Null(OffsetTo<Axis>))) return NOT_INDEXED;
    return (this+horizAxis).get_lang_tag_index (baseScriptIndex, baseLangSysTag);
  }

  inline unsigned int get_horiz_feature_tag_index (unsigned int baseScriptIndex, unsigned int baseLangSysIndex, Tag featureTableTag) const
  {
    if (unlikely(horizAxis == Null(OffsetTo<Axis>))) return NOT_INDEXED;
    return (this+horizAxis).get_feature_tag_index (baseScriptIndex, baseLangSysIndex, featureTableTag);
  }

  inline SHORT get_horiz_max_value (unsigned int baseScriptIndex, unsigned int baseLangSysIndex, unsigned int featureTableTagIndex) const
  {
    if (unlikely(horizAxis == Null(OffsetTo<Axis>))) return NO_COORD;
    return (this+horizAxis).get_max_value (baseScriptIndex, baseLangSysIndex, featureTableTagIndex);
  }

  inline SHORT get_horiz_min_value (unsigned int baseScriptIndex, unsigned int baseLangSysIndex, unsigned int featureTableTagIndex) const
  {
    if (unlikely(horizAxis == Null(OffsetTo<Axis>))) return NO_COORD;
    return (this+horizAxis).get_min_value (baseScriptIndex, baseLangSysIndex, featureTableTagIndex);
  }

    // vertical axis min/max coords:

  inline unsigned int get_vert_lang_tag_index (unsigned int baseScriptIndex, Tag baseLangSysTag) const
  {
    if (unlikely(vertAxis == Null(OffsetTo<Axis>))) return NOT_INDEXED;
    return (this+vertAxis).get_lang_tag_index (baseScriptIndex, baseLangSysTag);
  }

  inline unsigned int get_vert_feature_tag_index (unsigned int baseScriptIndex, unsigned int baseLangSysIndex, Tag featureTableTag) const
  {
    if (unlikely(vertAxis == Null(OffsetTo<Axis>))) return NOT_INDEXED;
    return (this+vertAxis).get_feature_tag_index (baseScriptIndex, baseLangSysIndex, featureTableTag);
  }

  inline SHORT get_vert_max_value (unsigned int baseScriptIndex, unsigned int baseLangSysIndex, unsigned int featureTableTagIndex) const
  {
    if (unlikely(vertAxis == Null(OffsetTo<Axis>))) return NO_COORD;
    return (this+vertAxis).get_max_value (baseScriptIndex, baseLangSysIndex, featureTableTagIndex);
  }

  inline SHORT get_vert_min_value (unsigned int baseScriptIndex, unsigned int baseLangSysIndex, unsigned int featureTableTagIndex) const
  {
    if (unlikely(vertAxis == Null(OffsetTo<Axis>))) return NO_COORD;
    return (this+vertAxis).get_min_value (baseScriptIndex, baseLangSysIndex, featureTableTagIndex);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
      horizAxis.sanitize (c, this) &&
      vertAxis.sanitize (c, this));
  }

  protected:
  FixedVersion<>  version;
  OffsetTo<Axis>  horizAxis;
  OffsetTo<Axis>  vertAxis;

  public:
  DEFINE_SIZE_STATIC (8);
};

struct BASE
{
  static const hb_tag_t tableTag = HB_OT_TAG_BASE;

  inline bool has_vert_axis(void)
  { return u.format1_0.has_vert_axis(); }

  inline bool has_horiz_axis(void)
  { return u.format1_0.has_horiz_axis(); }

  inline unsigned int get_horiz_base_tag_index(Tag baselineTag) const
  { return u.format1_0.get_horiz_base_tag_index(baselineTag); }

  inline unsigned int get_horiz_default_base_tag_index_for_script_index (unsigned int baseScriptIndex) const
  { return u.format1_0.get_horiz_default_base_tag_index_for_script_index(baseScriptIndex); }

  inline SHORT get_horiz_base_coord(unsigned int baseScriptIndex, unsigned int baselineTagIndex) const
  { return u.format1_0.get_horiz_base_coord(baseScriptIndex, baselineTagIndex); }

  inline unsigned int get_vert_base_tag_index(Tag baselineTag) const
  { return u.format1_0.get_vert_base_tag_index(baselineTag); }

  inline unsigned int get_vert_default_base_tag_index_for_script_index (unsigned int baseScriptIndex) const
  { return u.format1_0.get_vert_default_base_tag_index_for_script_index(baseScriptIndex); }

  inline SHORT get_vert_base_coord(unsigned int baseScriptIndex, unsigned int baselineTagIndex) const
  { return u.format1_0.get_vert_base_coord(baseScriptIndex, baselineTagIndex); }

  inline bool sanitize (hb_sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    if (!u.version.sanitize (c)) return_trace (false);
    if (!likely(u.version.major == 1)) return_trace (false);
    switch (u.version.minor) {
    case 0: return_trace (u.format1_0.sanitize (c));
    case 1: return_trace (u.format1_1.sanitize (c));
    default:return_trace (false);
    }
  }

  protected:
  union {
    FixedVersion<>  version;    /* Version of the BASE table: 1.0 or 1.1 */
    BASEFormat1_0   format1_0;
    BASEFormat1_1   format1_1;
  } u;

  public:
  DEFINE_SIZE_UNION (4, version);
};


} /* namespace OT */


#endif /* HB_OT_BASE_TABLE_HH */
