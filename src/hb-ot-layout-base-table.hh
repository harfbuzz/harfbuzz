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

#define NO_COORD -32767

/*
 * BASE -- The BASE Table
 */

struct BaseCoordFormat1 {

  inline SHORT get_coord () const
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

  inline SHORT get_coord () const
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

  inline SHORT get_coord (void ) const
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

  inline hb_position_t get_value (hb_font_t *font, hb_direction_t dir) const
  { 
    SHORT coord;
    switch (u.baseCoordFormat) {
    case 1: coord = u.format1.get_coord();
    case 2: coord = u.format2.get_coord();
    case 3: coord = u.format3.get_coord();
    default: return 0;
    }
    return font->em_scale (coord, dir); 
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (!u.baseCoordFormat.sanitize (c)) return_trace (false);
    switch (u.baseCoordFormat) {
    case 1: return_trace (u.format1.sanitize (c));
    case 2: return_trace (u.format2.sanitize (c));
    case 3: return_trace (u.format3.sanitize (c));
    default:return_trace (true);
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

  inline hb_position_t get_min_value (hb_font_t *font, hb_direction_t dir) const
  { 
    if (unlikely(minCoord == Null(OffsetTo<BaseCoord>))) return NO_COORD;
    return (this+minCoord).get_value(font, dir); 
  }

  inline hb_position_t get_max_value (hb_font_t *font, hb_direction_t dir) const
  { 
    if (unlikely(maxCoord == Null(OffsetTo<BaseCoord>))) return NO_COORD;
    return (this+maxCoord).get_value(font, dir); 
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

  inline hb_position_t get_min_value (hb_font_t *font, hb_direction_t dir) const
  { 
    if (unlikely(minCoord == Null(OffsetTo<BaseCoord>))) return NO_COORD;
    return (this+minCoord).get_value(font, dir);
  }

  inline hb_position_t get_max_value (hb_font_t *font, hb_direction_t dir) const
  { 
    if (unlikely(maxCoord == Null(OffsetTo<BaseCoord>))) return NO_COORD;
    return (this+maxCoord).get_value(font, dir);
  }

  inline hb_position_t get_min_value (hb_font_t *font, hb_direction_t dir, Tag featureTableTag) const
  {
    for (unsigned int i = 0; i < featMinMaxCount; i++) 
    {
      if (featMinMaxRecords[i].get_tag() == featureTableTag)
        return featMinMaxRecords[i].get_min_value(font, dir);
      // we could take advantage of alphabetical order by comparing Tags, not currently possible
      //if (featMinMaxRecords[i].get_tag() > featureTableTag)
      //  break;
    }
    return get_min_value (font, dir);
  }

  inline hb_position_t get_max_value (hb_font_t *font, hb_direction_t dir, Tag featureTableTag) const
  {
    for (unsigned int i = 0; i < featMinMaxCount; i++) 
    {
      if (featMinMaxRecords[i].get_tag() == featureTableTag)
        return featMinMaxRecords[i].get_max_value(font, dir);
      // we could use alphabetical order to break here
    }
    return get_min_value (font, dir);
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
  ArrayOf<FeatMinMaxRecord> featMinMaxRecords; /* All FeatMinMaxRecords are listed alphabetically */

  public:
  DEFINE_SIZE_ARRAY (8, featMinMaxRecords);

};

struct BaseLangSysRecord {

  inline Tag get_tag(void) const
  { return baseLangSysTag; }

  inline hb_position_t get_min_value (hb_font_t *font, hb_direction_t dir) const
  { return (this+minMax).get_min_value(font, dir); }

  inline hb_position_t get_max_value (hb_font_t *font, hb_direction_t dir) const
  { return (this+minMax).get_max_value(font, dir); }

  inline hb_position_t get_min_value (hb_font_t *font, hb_direction_t dir, Tag featureTableTag) const
  { return (this+minMax).get_min_value(font, dir, featureTableTag); }

  inline hb_position_t get_max_value (hb_font_t *font, hb_direction_t dir, Tag featureTableTag) const
  { return (this+minMax).get_max_value(font, dir, featureTableTag); }  

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

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
      baseCoords.sanitize (c, this));
  }

  protected:
  USHORT                    defaultIndex;
  USHORT                    baseCoordCount;
  OffsetArrayOf<BaseCoord>  baseCoords;

  public:
  DEFINE_SIZE_ARRAY (6, baseCoords);

};

struct BaseScript {

  inline hb_position_t get_min_value (hb_font_t *font, hb_direction_t dir) const 
  {
    if (unlikely(defaultMinMax == Null(OffsetTo<MinMax>))) return NO_COORD;
    return (this+defaultMinMax).get_min_value(font, dir);
  }

  inline hb_position_t get_max_value (hb_font_t *font, hb_direction_t dir) const 
  {
    if (unlikely(defaultMinMax == Null(OffsetTo<MinMax>))) return NO_COORD;
    return (this+defaultMinMax).get_max_value(font, dir);
  }

  inline hb_position_t get_min_value (hb_font_t *font, hb_direction_t dir, Tag baseLangSysTag) const
  { 
    for (unsigned int i = 0; i < baseLangSysCount; i++) 
    {
      if (baseLangSysRecords[i].get_tag() == baseLangSysTag)
        return baseLangSysRecords[i].get_min_value(font, dir);
      // we could use alphabetical order to break here
    }
    return get_min_value (font, dir);
  }

  inline hb_position_t get_max_value (hb_font_t *font, hb_direction_t dir, Tag baseLangSysTag) const
  {
    for (unsigned int i = 0; i < baseLangSysCount; i++) 
    {
      if (baseLangSysRecords[i].get_tag() == baseLangSysTag)
        return baseLangSysRecords[i].get_max_value(font, dir);
      // we could use alphabetical order to break here
    }
    return get_max_value (font, dir);
  }

  inline hb_position_t get_min_value (hb_font_t *font, hb_direction_t dir, Tag baseLangSysTag, Tag featureTableTag) const
  {
    for (unsigned int i = 0; i < baseLangSysCount; i++) 
    {
      if (baseLangSysRecords[i].get_tag() == baseLangSysTag)
        return baseLangSysRecords[i].get_min_value(font, dir, featureTableTag);
      // we could use alphabetical order to break here
    }
    return get_min_value (font, dir);
  }

  inline hb_position_t get_max_value (hb_font_t *font, hb_direction_t dir, Tag baseLangSysTag, Tag featureTableTag) const
  {
    for (unsigned int i = 0; i < baseLangSysCount; i++)
    {
      if (baseLangSysRecords[i].get_tag() == baseLangSysTag)
        return baseLangSysRecords[i].get_max_value(font, dir);
      // we could use alphabetical order to break here
    }
    return get_max_value (font, dir);
  }

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

  inline const BaseScript *get_baseScript (void) const
  {
    return &(this+baseScript);
  }

  inline bool get_tag (void) const
  {
    return baseScriptTag;
  }

  inline bool sanitize (hb_sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
      baseScript.sanitize (c, base));
  }

  protected:
  Tag                   baseScriptTag;
  OffsetTo<BaseScript>  baseScript;

  public:
    DEFINE_SIZE_STATIC (6);
};

struct BaseScriptList {

  inline const BaseScript *get_baseScript_from_tag (Tag baseScriptTag) const
  {
    for (unsigned int i = 0; i < baseScriptCount; i++)
      if (baseScriptRecords[i].get_tag() == baseScriptTag)
        return baseScriptRecords[i].get_baseScript();
      // we could use alphabetical order to break here
    return NULL;
  }

  inline hb_position_t get_min_value (hb_font_t *font, hb_direction_t dir, Tag baseScriptTag) const 
  {
    const BaseScript *baseScript = get_baseScript_from_tag (baseScriptTag);
    if (baseScript == NULL) return NO_COORD;
    return baseScript->get_min_value(font, dir);
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

  inline bool hasTag(Tag tag) const
  {
    for (unsigned int i = 0; i < baseTagCount; i++)
      if (baselineTags[i] == tag)
        return true;
    return false;
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  protected:
  USHORT        baseTagCount;
  ArrayOf<Tag>  baselineTags; // must be in alphabetical order

  public:
  DEFINE_SIZE_ARRAY (4, baselineTags);
};

struct Axis
{

  inline bool hasTag(Tag tag) const
  {
    if (unlikely(baseTagList == Null(OffsetTo<BaseTagList>))) return false;
    return (this+baseTagList).hasTag(tag);
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

  inline bool sanitize (hb_sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    if (!u.version.sanitize (c)) return_trace (false);
    if (!likely(u.version.major == 1)) return_trace (false);
    switch (u.version.minor) {
    case 0: return_trace (u.format1_0.sanitize (c));
    case 1: return_trace (u.format1_1.sanitize (c));
    default:return_trace (true);
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
