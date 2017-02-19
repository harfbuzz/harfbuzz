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
  protected:
  USHORT    baseCoordFormat;
  SHORT   coordinate;

  public:
  DEFINE_SIZE_STATIC (4);
};

struct BaseCoordFormat2 {
  protected:
  USHORT    baseCoordFormat;
  SHORT   coordinate;
  USHORT    referenceGlyph;
  USHORT    baseCoordPoint;

  public:
  DEFINE_SIZE_STATIC (8);
};

struct BaseCoordFormat3 {
  protected:
  USHORT    baseCoordFormat;
  SHORT   coordinate;
  OffsetTo<Device>    deviceTable;

  public:
  DEFINE_SIZE_STATIC (6);
};

struct BaseCoord {
  protected:
  union {
    USHORT    baseCoordFormat;
    BaseCoordFormat1  format1;
    BaseCoordFormat2  format2;
    BaseCoordFormat3  format3;
  } u;

  public:
  DEFINE_SIZE_MIN (4);
};

struct FeatMinMaxRecord {
  protected:
  Tag featureTableTag;
  OffsetTo<BaseCoord>   minCoord;
  OffsetTo<BaseCoord>   maxCoord;

  public:
  DEFINE_SIZE_STATIC (8);

};

struct MinMaxTable {
  protected:
  OffsetTo<BaseCoord>   minCoord;
  OffsetTo<BaseCoord>   maxCoord;
  USHORT    featMinMaxCount;
  ArrayOf<FeatMinMaxRecord> featMinMaxRecordTable;

  public:
  DEFINE_SIZE_ARRAY (6, featMinMaxRecordTable);

};

struct BaseLangSysRecord {
  protected:
  Tag baseLangSysTag;
  OffsetTo<MinMaxTable> minMax;

  public:
  DEFINE_SIZE_STATIC (6); 

};

struct BaseValuesTable {
  protected:
  USHORT    defaultIndex;
  USHORT    baseCoordCount;
  ArrayOf<BaseCoord> baseCoordTable;

  public:
  DEFINE_SIZE_ARRAY (4, baseCoordTable);

};

struct BaseScriptTable {
  protected:
  OffsetTo<BaseValuesTable>  baseValues;
  OffsetTo<MinMaxTable>  defaultMinMax;
  USHORT    baseLangSysCount;
  ArrayOf<BaseLangSysRecord> baseLangSysRecordTable;

  public:
    DEFINE_SIZE_ARRAY (6, baseLangSysRecordTable);
};


struct BaseScriptRecord {

  protected:
  Tag baseScriptTag;
  OffsetTo<BaseScriptTable> baseScript;

  public:
    DEFINE_SIZE_STATIC (4);
};

struct BaseScriptList {

  protected:
  USHORT    baseScriptCount;
  ArrayOf<BaseScriptRecord> baseScriptRecordTable;

  public:
  DEFINE_SIZE_ARRAY (2, baseScriptRecordTable);
  
};

struct BaselineTag {

  protected:
  Tag   tag;

  public:
  DEFINE_SIZE_STATIC (4);

};

struct BaseTagList
{

  protected:
  USHORT    baseTagCount;
  ArrayOf<BaselineTag> BaseTagListTable;

  public:
  DEFINE_SIZE_STATIC (4);
};

struct VertAxis
{

  protected:
  OffsetTo<BaseTagList> baseTagList;
  OffsetTo<BaseScriptList> baseScriptList;

  public:
  DEFINE_SIZE_STATIC (4);
};

struct HorizAxis
{

  protected:
  OffsetTo<HorizAxis> baseTagList;
  OffsetTo<VertAxis> baseScriptList;

  public:
  DEFINE_SIZE_STATIC (4);
};

struct BASE
{
  static const hb_tag_t tableTag = HB_OT_TAG_BASE;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (version.sanitize (c) &&
      likely (version.major == 1));
  }

  protected:
  FixedVersion<>version;    /* Version of the BASE table: 1.0 or 1.1 */
  OffsetTo<HorizAxis> horizAxis;
  OffsetTo<VertAxis> vertAxis;
  //LOffsetTo<ItemVarStore> itemVarStore;

  public:
  DEFINE_SIZE_MIN (6);
};


} /* namespace OT */


#endif /* HB_OT_BASE_TABLE_HH */
