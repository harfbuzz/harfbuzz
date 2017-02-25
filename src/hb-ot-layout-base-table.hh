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

/*
 * BASE -- The BASE Table
 */

struct BaseCoordFormat1 {

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

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
      minCoord.sanitize (c, this) &&
      maxCoord.sanitize (c, this) &&
      featMinMaxRecords.sanitize (c, this));
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

  inline bool sanitize (hb_sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
      minMax.sanitize (c, base));
  }

  protected:
  Tag               baseLangSysTag;
  OffsetTo<MinMax>  minMax;

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

struct BaselineTag {

  inline bool sanitize (hb_sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  protected:
  Tag   tag;

  public:
  DEFINE_SIZE_STATIC (4);

};

struct BaseTagList
{

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
      baselineTags.sanitize (c, this));
  }

  protected:
  USHORT                baseTagCount;
  ArrayOf<BaselineTag>  baselineTags;

  public:
  DEFINE_SIZE_ARRAY (4, baselineTags);
};

struct Axis
{

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
