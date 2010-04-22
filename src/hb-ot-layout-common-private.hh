/*
 * Copyright (C) 2007,2008,2009  Red Hat, Inc.
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
 */

#ifndef HB_OT_LAYOUT_COMMON_PRIVATE_HH
#define HB_OT_LAYOUT_COMMON_PRIVATE_HH

#include "hb-ot-layout-private.h"

#include "hb-open-type-private.hh"


#define NO_CONTEXT		((unsigned int) 0x110000)
#define NOT_COVERED		((unsigned int) 0x110000)
#define MAX_NESTING_LEVEL	8


/*
 *
 * OpenType Layout Common Table Formats
 *
 */


/*
 * Script, ScriptList, LangSys, Feature, FeatureList, Lookup, LookupList
 */

template <typename Type>
struct Record
{
  static inline unsigned int get_size () { return sizeof (Record<Type>); }

  inline bool sanitize (SANITIZE_ARG_DEF, void *base) {
    TRACE_SANITIZE ();
    return SANITIZE (tag) && SANITIZE_BASE (offset, base);
  }

  Tag		tag;		/* 4-byte Tag identifier */
  OffsetTo<Type>
		offset;		/* Offset from beginning of object holding
				 * the Record */
};

template <typename Type>
struct RecordArrayOf : ArrayOf<Record<Type> > {
  inline const Tag& get_tag (unsigned int i) const
  {
    if (HB_UNLIKELY (i >= this->len)) return Null(Tag);
    return (*this)[i].tag;
  }
  inline unsigned int get_tags (unsigned int start_offset,
				unsigned int *record_count /* IN/OUT */,
				hb_tag_t     *record_tags /* OUT */) const
  {
    if (record_count) {
      const Record<Type> *array = this->const_sub_array (start_offset, record_count);
      unsigned int count = *record_count;
      for (unsigned int i = 0; i < count; i++)
	record_tags[i] = array[i].tag;
    }
    return this->len;
  }
  inline bool find_index (hb_tag_t tag, unsigned int *index) const
  {
    Tag t;
    t.set (tag);
    // TODO bsearch
    const Record<Type> *a = this->array();
    unsigned int count = this->len;
    for (unsigned int i = 0; i < count; i++)
    {
      if (t == a[i].tag)
      {
        if (index) *index = i;
        return true;
      }
    }
    if (index) *index = NO_INDEX;
    return false;
  }
};

template <typename Type>
struct RecordListOf : RecordArrayOf<Type>
{
  inline const Type& operator [] (unsigned int i) const
  { return this+RecordArrayOf<Type>::operator [](i).offset; }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return RecordArrayOf<Type>::sanitize (SANITIZE_ARG, CharP(this));
  }
};


struct IndexArray : ArrayOf<USHORT>
{
  inline unsigned int operator [] (unsigned int i) const
  {
    if (HB_UNLIKELY (i >= this->len))
      return NO_INDEX;
    return this->array()[i];
  }
  inline unsigned int get_indexes (unsigned int start_offset,
				   unsigned int *_count /* IN/OUT */,
				   unsigned int *_indexes /* OUT */) const
  {
    if (_count) {
      const USHORT *array = this->const_sub_array (start_offset, _count);
      unsigned int count = *_count;
      for (unsigned int i = 0; i < count; i++)
	_indexes[i] = array[i];
    }
    return this->len;
  }
};


struct Script;
struct LangSys;
struct Feature;


struct LangSys
{
  inline unsigned int get_feature_count (void) const
  { return featureIndex.len; }
  inline hb_tag_t get_feature_index (unsigned int i) const
  { return featureIndex[i]; }
  inline unsigned int get_feature_indexes (unsigned int start_offset,
					   unsigned int *feature_count /* IN/OUT */,
					   unsigned int *feature_indexes /* OUT */) const
  { return featureIndex.get_indexes (start_offset, feature_count, feature_indexes); }

  inline bool has_required_feature (void) const { return reqFeatureIndex != 0xffff; }
  inline int get_required_feature_index (void) const
  {
    if (reqFeatureIndex == 0xffff)
      return NO_INDEX;
   return reqFeatureIndex;;
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_SELF () && SANITIZE (featureIndex);
  }

  Offset	lookupOrder;	/* = Null (reserved for an offset to a
				 * reordering table) */
  USHORT	reqFeatureIndex;/* Index of a feature required for this
				 * language system--if no required features
				 * = 0xFFFF */
  IndexArray	featureIndex;	/* Array of indices into the FeatureList */
};
ASSERT_SIZE (LangSys, 6);
DEFINE_NULL_DATA (LangSys, 6, "\0\0\xFF\xFF");


struct Script
{
  inline unsigned int get_lang_sys_count (void) const
  { return langSys.len; }
  inline const Tag& get_lang_sys_tag (unsigned int i) const
  { return langSys.get_tag (i); }
  inline unsigned int get_lang_sys_tags (unsigned int start_offset,
					 unsigned int *lang_sys_count /* IN/OUT */,
					 hb_tag_t     *lang_sys_tags /* OUT */) const
  { return langSys.get_tags (start_offset, lang_sys_count, lang_sys_tags); }
  inline const LangSys& get_lang_sys (unsigned int i) const
  {
    if (i == NO_INDEX) return get_default_lang_sys ();
    return this+langSys[i].offset;
  }
  inline bool find_lang_sys_index (hb_tag_t tag, unsigned int *index) const
  { return langSys.find_index (tag, index); }

  inline bool has_default_lang_sys (void) const { return defaultLangSys != 0; }
  inline const LangSys& get_default_lang_sys (void) const { return this+defaultLangSys; }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_THIS (defaultLangSys) && SANITIZE_THIS (langSys);
  }

  private:
  OffsetTo<LangSys>
		defaultLangSys;	/* Offset to DefaultLangSys table--from
				 * beginning of Script table--may be Null */
  RecordArrayOf<LangSys>
		langSys;	/* Array of LangSysRecords--listed
				 * alphabetically by LangSysTag */
};
ASSERT_SIZE (Script, 4);

typedef RecordListOf<Script> ScriptList;
ASSERT_SIZE (ScriptList, 2);


struct Feature
{
  inline unsigned int get_lookup_count (void) const
  { return lookupIndex.len; }
  inline hb_tag_t get_lookup_index (unsigned int i) const
  { return lookupIndex[i]; }
  inline unsigned int get_lookup_indexes (unsigned int start_index,
					  unsigned int *lookup_count /* IN/OUT */,
					  unsigned int *lookup_tags /* OUT */) const
  { return lookupIndex.get_indexes (start_index, lookup_count, lookup_tags); }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_SELF () && SANITIZE (lookupIndex);
  }

  /* TODO: implement get_feature_parameters() */
  /* TODO: implement FeatureSize and other special features? */
  Offset	featureParams;	/* Offset to Feature Parameters table (if one
				 * has been defined for the feature), relative
				 * to the beginning of the Feature Table; = Null
				 * if not required */
  IndexArray	 lookupIndex;	/* Array of LookupList indices */
};
ASSERT_SIZE (Feature, 4);

typedef RecordListOf<Feature> FeatureList;
ASSERT_SIZE (FeatureList, 2);


struct LookupFlag : USHORT
{
  enum {
    RightToLeft		= 0x0001u,
    IgnoreBaseGlyphs	= 0x0002u,
    IgnoreLigatures	= 0x0004u,
    IgnoreMarks		= 0x0008u,
    IgnoreFlags		= 0x000Eu,
    UseMarkFilteringSet	= 0x0010u,
    Reserved		= 0x00E0u,
    MarkAttachmentType	= 0xFF00u
  };
};
ASSERT_SIZE (LookupFlag, 2);

struct LookupSubTable
{
  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_SELF ();
  }

  private:
  USHORT	format;		/* Subtable format.  Different for GSUB and GPOS */
};
ASSERT_SIZE (LookupSubTable, 2);

struct Lookup
{
  inline const LookupSubTable& get_subtable (unsigned int i) const { return this+subTable[i]; }
  inline unsigned int get_subtable_count (void) const { return subTable.len; }

  inline unsigned int get_type (void) const { return lookupType; }
  inline unsigned int get_flag (void) const
  {
    unsigned int flag = lookupFlag;
    if (HB_UNLIKELY (flag & LookupFlag::UseMarkFilteringSet))
    {
      const USHORT &markFilteringSet = StructAfter<USHORT> (subTable);
      flag += (markFilteringSet << 16);
    }
    return flag;
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    if (!(SANITIZE (lookupType) && SANITIZE (lookupFlag) && SANITIZE_THIS (subTable))) return false;
    if (HB_UNLIKELY (lookupFlag & LookupFlag::UseMarkFilteringSet))
    {
      USHORT &markFilteringSet = StructAfter<USHORT> (subTable);
      if (!SANITIZE (markFilteringSet)) return false;
    }
    return true;
  }

  USHORT	lookupType;		/* Different enumerations for GSUB and GPOS */
  USHORT	lookupFlag;		/* Lookup qualifiers */
  OffsetArrayOf<LookupSubTable>
		subTable;		/* Array of SubTables */
  USHORT	markFilteringSetX[VAR];	/* Index (base 0) into GDEF mark glyph sets
					 * structure. This field is only present if bit
					 * UseMarkFilteringSet of lookup flags is set. */
};
ASSERT_SIZE_VAR (Lookup, 6, USHORT);

typedef OffsetListOf<Lookup> LookupList;
ASSERT_SIZE (LookupList, 2);


/*
 * Coverage Table
 */

struct CoverageFormat1
{
  friend struct Coverage;

  private:
  inline unsigned int get_coverage (hb_codepoint_t glyph_id) const
  {
    if (HB_UNLIKELY (glyph_id > 0xFFFF))
      return NOT_COVERED;
    GlyphID gid;
    gid.set (glyph_id);
    // TODO: bsearch
    unsigned int num_glyphs = glyphArray.len;
    for (unsigned int i = 0; i < num_glyphs; i++)
      if (gid == glyphArray[i])
        return i;
    return NOT_COVERED;
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE (glyphArray);
  }

  private:
  USHORT	coverageFormat;	/* Format identifier--format = 1 */
  ArrayOf<GlyphID>
		glyphArray;	/* Array of GlyphIDs--in numerical order */
};
ASSERT_SIZE (CoverageFormat1, 4);

struct CoverageRangeRecord
{
  friend struct CoverageFormat2;

  static inline unsigned int get_size () { return sizeof (CoverageRangeRecord); }

  private:
  inline unsigned int get_coverage (hb_codepoint_t glyph_id) const
  {
    if (glyph_id >= start && glyph_id <= end)
      return (unsigned int) startCoverageIndex + (glyph_id - start);
    return NOT_COVERED;
  }

  public:
  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_SELF ();
  }

  private:
  GlyphID	start;			/* First GlyphID in the range */
  GlyphID	end;			/* Last GlyphID in the range */
  USHORT	startCoverageIndex;	/* Coverage Index of first GlyphID in
					 * range */
};
ASSERT_SIZE (CoverageRangeRecord, 6);
DEFINE_NULL_DATA (CoverageRangeRecord, 6, "\000\001");

struct CoverageFormat2
{
  friend struct Coverage;

  private:
  inline unsigned int get_coverage (hb_codepoint_t glyph_id) const
  {
    // TODO: bsearch
    unsigned int count = rangeRecord.len;
    for (unsigned int i = 0; i < count; i++)
    {
      unsigned int coverage = rangeRecord[i].get_coverage (glyph_id);
      if (coverage != NOT_COVERED)
        return coverage;
    }
    return NOT_COVERED;
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE (rangeRecord);
  }

  private:
  USHORT	coverageFormat;	/* Format identifier--format = 2 */
  ArrayOf<CoverageRangeRecord>
		rangeRecord;	/* Array of glyph ranges--ordered by
				 * Start GlyphID. rangeCount entries
				 * long */
};
ASSERT_SIZE (CoverageFormat2, 4);

struct Coverage
{
  inline unsigned int operator () (hb_codepoint_t glyph_id) const { return get_coverage (glyph_id); }

  inline unsigned int get_coverage (hb_codepoint_t glyph_id) const
  {
    switch (u.format) {
    case 1: return u.format1->get_coverage(glyph_id);
    case 2: return u.format2->get_coverage(glyph_id);
    default:return NOT_COVERED;
    }
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    if (!SANITIZE (u.format)) return false;
    switch (u.format) {
    case 1: return u.format1->sanitize (SANITIZE_ARG);
    case 2: return u.format2->sanitize (SANITIZE_ARG);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;		/* Format identifier */
  CoverageFormat1	format1[VAR];
  CoverageFormat2	format2[VAR];
  } u;
};


/*
 * Class Definition Table
 */

struct ClassDefFormat1
{
  friend struct ClassDef;

  private:
  inline hb_ot_layout_class_t get_class (hb_codepoint_t glyph_id) const
  {
    if ((unsigned int) (glyph_id - startGlyph) < classValue.len)
      return classValue[glyph_id - startGlyph];
    return 0;
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_SELF () && SANITIZE (classValue);
  }

  USHORT	classFormat;		/* Format identifier--format = 1 */
  GlyphID	startGlyph;		/* First GlyphID of the classValueArray */
  ArrayOf<USHORT>
		classValue;		/* Array of Class Values--one per GlyphID */
};
ASSERT_SIZE (ClassDefFormat1, 6);

struct ClassRangeRecord
{
  friend struct ClassDefFormat2;

  static inline unsigned int get_size () { return sizeof (ClassRangeRecord); }

  private:
  inline hb_ot_layout_class_t get_class (hb_codepoint_t glyph_id) const
  {
    if (glyph_id >= start && glyph_id <= end)
      return classValue;
    return 0;
  }

  public:
  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_SELF ();
  }

  private:
  GlyphID	start;		/* First GlyphID in the range */
  GlyphID	end;		/* Last GlyphID in the range */
  USHORT	classValue;	/* Applied to all glyphs in the range */
};
ASSERT_SIZE (ClassRangeRecord, 6);
DEFINE_NULL_DATA (ClassRangeRecord, 6, "\000\001");

struct ClassDefFormat2
{
  friend struct ClassDef;

  private:
  inline hb_ot_layout_class_t get_class (hb_codepoint_t glyph_id) const
  {
    // TODO: bsearch
    unsigned int count = rangeRecord.len;
    for (unsigned int i = 0; i < count; i++)
    {
      int classValue = rangeRecord[i].get_class (glyph_id);
      if (classValue > 0)
        return classValue;
    }
    return 0;
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE (rangeRecord);
  }

  USHORT	classFormat;	/* Format identifier--format = 2 */
  ArrayOf<ClassRangeRecord>
		rangeRecord;	/* Array of glyph ranges--ordered by
				 * Start GlyphID */
};
ASSERT_SIZE (ClassDefFormat2, 4);

struct ClassDef
{
  inline hb_ot_layout_class_t operator () (hb_codepoint_t glyph_id) const { return get_class (glyph_id); }

  inline hb_ot_layout_class_t get_class (hb_codepoint_t glyph_id) const
  {
    switch (u.format) {
    case 1: return u.format1->get_class(glyph_id);
    case 2: return u.format2->get_class(glyph_id);
    default:return 0;
    }
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    if (!SANITIZE (u.format)) return false;
    switch (u.format) {
    case 1: return u.format1->sanitize (SANITIZE_ARG);
    case 2: return u.format2->sanitize (SANITIZE_ARG);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;		/* Format identifier */
  ClassDefFormat1	format1[VAR];
  ClassDefFormat2	format2[VAR];
  } u;
};


/*
 * Device Tables
 */

struct Device
{
  inline int operator () (unsigned int ppem_size) const { return get_delta (ppem_size); }

  inline int get_delta (unsigned int ppem_size) const
  {
    unsigned int f = deltaFormat;
    if (HB_UNLIKELY (f < 1 || f > 3))
      return 0;

    if (ppem_size < startSize || ppem_size > endSize)
      return 0;

    unsigned int s = ppem_size - startSize;

    unsigned int byte = deltaValue[s >> (4 - f)];
    unsigned int bits = (byte >> (16 - (((s & ((1 << (4 - f)) - 1)) + 1) << f)));
    unsigned int mask = (0xFFFF >> (16 - (1 << f)));

    int delta = bits & mask;

    if ((unsigned int) delta >= ((mask + 1) >> 1))
      delta -= mask + 1;

    return delta;
  }

  inline unsigned int get_size () const
  {
    unsigned int f = deltaFormat;
    if (HB_UNLIKELY (f < 1 || f > 3 || startSize > endSize)) return 3 * USHORT::get_size ();
    return 3 * USHORT::get_size () + ((endSize - startSize + (1 << (4 - f)) - 1) >> (4 - f));
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_GET_SIZE ();
  }

  private:
  USHORT	startSize;	/* Smallest size to correct--in ppem */
  USHORT	endSize;	/* Largest size to correct--in ppem */
  USHORT	deltaFormat;	/* Format of DeltaValue array data: 1, 2, or 3 */
  USHORT	deltaValue[VAR];	/* Array of compressed data */
};
ASSERT_SIZE_VAR (Device, 6, USHORT);


#endif /* HB_OT_LAYOUT_COMMON_PRIVATE_HH */
