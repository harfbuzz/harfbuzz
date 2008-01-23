#ifndef HARFBUZZ_OPEN_PRIVATE_H
#define HARFBUZZ_OPEN_PRIVATE_H

#include "harfbuzz-private.h"
#include "harfbuzz-open.h"

#include <glib.h>

/* Macros to convert to/from BigEndian */
#define hb_be_uint8_t
#define hb_be_int8_t
#define hb_be_uint16_t	GUINT16_TO_BE
#define hb_be_int16_t	GINT16_TO_BE
#define hb_be_uint32_t	GUINT32_TO_BE
#define hb_be_int32_t	GINT32_TO_BE
#define hb_be_uint64_t	GUINT64_TO_BE
#define hb_be_int64_t	GINT64_TO_BE

/*
 * Int types
 */

#define DEFINE_INT_TYPE1(NAME, TYPE, BIG_ENDIAN) \
  inline NAME& operator = (TYPE i) { v = BIG_ENDIAN(i); return *this; } \
  inline operator TYPE(void) const { return BIG_ENDIAN(v); } \
  inline bool operator== (NAME o) const { return v == o.v; } \
  private: TYPE v; \
  public:
#define DEFINE_INT_TYPE0(NAME, type) DEFINE_INT_TYPE1 (NAME, type, hb_be_##type)
#define DEFINE_INT_TYPE(NAME, u, w)  DEFINE_INT_TYPE0 (NAME, u##int##w##_t)
#define DEFINE_INT_TYPE_STRUCT(NAME, u, w) \
  struct NAME { \
    DEFINE_INT_TYPE(NAME, u, w) \
  }

/*
 * Array types
 */

/* get_len() is a method returning the number of items in an array-like object */
#define DEFINE_LEN(Type, array, num) \
  inline unsigned int get_len(void) const { return num; } \

/* get_size() is a method returning the size in bytes of an array-like object */
#define DEFINE_SIZE(Type, array, num) \
  inline unsigned int get_size(void) const { return sizeof (*this) + sizeof (Type) * num; }

#define DEFINE_LEN_AND_SIZE(Type, array, num) \
  DEFINE_LEN(Type, array, num) \
  DEFINE_SIZE(Type, array, num)

#define DEFINE_NON_INSTANTIABLE(Type) \
  private: inline Type() {} /* cannot be instantiated */ \
  public:

/* An array type is one that contains a variable number of objects
 * as its last item.  An array object is extended with len() and size()
 * methods, as well as overloaded [] operator. */
#define DEFINE_ARRAY_TYPE(Type, array, num) \
  DEFINE_INDEX_OPERATOR(const, Type, array, num) \
  DEFINE_INDEX_OPERATOR(     , Type, array, num) \
  DEFINE_LEN_AND_SIZE(Type, array, num)
#define DEFINE_INDEX_OPERATOR(const, Type, array, num) \
  inline const Type& operator[] (unsigned int i) const { \
    assert (i < num); \
    return array[i]; \
  }

/* An offset array type is like an array type, but it contains a table
 * of offsets to the objects, relative to the beginning of the current
 * object. */
#define DEFINE_OFFSET_ARRAY_TYPE(Type, array, num) \
  DEFINE_OFFSET_INDEX_OPERATOR(const, Type, array, num) \
  DEFINE_OFFSET_INDEX_OPERATOR(     , Type, array, num) \
  DEFINE_LEN_AND_SIZE(Offset, array, num)
#define DEFINE_OFFSET_INDEX_OPERATOR(const, Type, array, num) \
  inline const Type& operator[] (unsigned int i) const { \
    assert (i < num); \
    assert (array[i]); /* TODO: should just skip them */ \
    return *(const Type *)((const char*)this + array[i]); \
  }

#define DEFINE_RECORD_ARRAY_TYPE(Type, array, num) \
  DEFINE_RECORD_ACCESSOR(const, Type, array, num) \
  DEFINE_RECORD_ACCESSOR(     , Type, array, num) \
  DEFINE_LEN_AND_SIZE(Record, array, num)
#define DEFINE_RECORD_ACCESSOR(const, Type, array, num) \
  inline const Type& operator[] (unsigned int i) const { \
    assert (i < num); \
    assert (array[i].offset); /* TODO: should just skip them */ \
    return *(const Type *)((const char*)this + array[i].offset); \
  } \
  inline const Tag& get_tag (unsigned int i) const { \
    assert (i < num); \
    return array[i].tag; \
  } \
  /* TODO: implement find_tag() */



/*
 *
 * The OpenType Font File
 *
 */



/*
 * Data Types
 */


/* "The following data types are used in the OpenType font file.
 *  All OpenType fonts use Motorola-style byte ordering (Big Endian):" */


DEFINE_INT_TYPE_STRUCT (BYTE,	 u,  8);	/*  8-bit unsigned integer. */
ASSERT_SIZE (BYTE, 1);
DEFINE_INT_TYPE_STRUCT (CHAR,	  ,  8);	/*  8-bit signed integer. */
ASSERT_SIZE (CHAR, 1);
DEFINE_INT_TYPE_STRUCT (USHORT,  u, 16);	/* 16-bit unsigned integer. */
ASSERT_SIZE (USHORT, 2);
DEFINE_INT_TYPE_STRUCT (SHORT,	  , 16);	/* 16-bit signed integer. */
ASSERT_SIZE (SHORT, 2);
DEFINE_INT_TYPE_STRUCT (ULONG,	 u, 32);	/* 32-bit unsigned integer. */
ASSERT_SIZE (ULONG, 4);
DEFINE_INT_TYPE_STRUCT (LONG,	  , 32);	/* 32-bit signed integer. */
ASSERT_SIZE (LONG, 4);

/* Date represented in number of seconds since 12:00 midnight, January 1,
 * 1904. The value is represented as a signed 64-bit integer. */
DEFINE_INT_TYPE_STRUCT (LONGDATETIME, , 64);

/* 32-bit signed fixed-point number (16.16) */
struct Fixed {
  inline Fixed& operator = (int32_t v) { i = (int16_t) (v >> 16); f = (uint16_t) v; return *this; } \
  inline operator int32_t(void) const { return (((int32_t) i) << 16) + (uint16_t) f; } \
  inline bool operator== (Fixed o) const { return i == o.i && f == o.f; } \

  inline operator double(void) const { return (uint32_t) this / 65536.; }
  inline int16_t int_part (void) const { return i; }
  inline uint16_t frac_part (void) const { return f; }

  private:
  SHORT i;
  USHORT f;
};
ASSERT_SIZE (Fixed, 4);

/* Smallest measurable distance in the em space. */
struct FUNIT;

/* 16-bit signed integer (SHORT) that describes a quantity in FUnits. */
struct FWORD : SHORT {
};
ASSERT_SIZE (FWORD, 2);

/* 16-bit unsigned integer (USHORT) that describes a quantity in FUnits. */
struct UFWORD : USHORT {
};
ASSERT_SIZE (UFWORD, 2);

/* 16-bit signed fixed number with the low 14 bits of fraction (2.14). */
struct F2DOT14 : SHORT {
  inline operator double() const { return (uint32_t) this / 16384.; }
};
ASSERT_SIZE (F2DOT14, 2);

/* Array of four uint8s (length = 32 bits) used to identify a script, language
 * system, feature, or baseline */
struct Tag {
  inline Tag (void) { v[0] = v[1] = v[2] = v[3] = 0; }
  inline Tag (uint32_t v) { (ULONG&)(*this) = v; }
  inline Tag (const char *c) { v[0] = c[0]; v[1] = c[1]; v[2] = c[2]; v[3] = c[3]; }
  inline bool operator== (Tag o) const { return v[0]==o.v[0]&&v[1]==o.v[1]&&v[2]==o.v[2]&&v[3]==o.v[3]; }
  inline bool operator== (const char *c) const { return v[0]==c[0]&&v[1]==c[1]&&v[2]==c[2]&&v[3]==c[3]; }
  inline bool operator== (uint32_t i) const { return i == (uint32_t) *this; }
  inline operator uint32_t(void) const { return (v[0]<<24)+(v[1]<<16) +(v[2]<<8)+v[3]; }
  /* What the char* converters return is NOT nul-terminated.  Print using "%.4s" */
  inline operator const char* (void) const { return (const char *)this; }
  inline operator char* (void) { return (char *)this; }

  private:
  char v[4];
};
ASSERT_SIZE (Tag, 4);

/* Glyph index number, same as uint16 (length = 16 bits) */
DEFINE_INT_TYPE_STRUCT (GlyphID, u, 16);
ASSERT_SIZE (GlyphID, 2);

/* Offset to a table, same as uint16 (length = 16 bits), NULL offset = 0x0000 */
DEFINE_INT_TYPE_STRUCT (Offset, u, 16);
ASSERT_SIZE (Offset, 2);

/* CheckSum */
struct CheckSum : ULONG {
  static uint32_t CalcTableChecksum (ULONG *Table, uint32_t Length) {
    uint32_t Sum = 0L;
    ULONG *EndPtr = Table+((Length+3) & ~3) / sizeof(ULONG);

    while (Table < EndPtr)
      Sum += *Table++;
    return Sum;
  }
};
ASSERT_SIZE (CheckSum, 4);


/*
 * Version Numbers
 */

struct USHORT_Version : USHORT {
};
ASSERT_SIZE (USHORT_Version, 2);

struct Fixed_Version : Fixed {
  inline int16_t major (void) const { return this->int_part(); }
  inline int16_t minor (void) const { return this->frac_part(); }
};
ASSERT_SIZE (Fixed_Version, 4);


/*
 * Organization of an OpenType Font
 */

struct OpenTypeFontFile;
struct OffsetTable;
struct TTCHeader;

typedef struct TableDirectory {

  friend struct OpenTypeFontFile;

  inline const Tag& get_tag (void) const { return tag; }
  inline unsigned long get_checksum (void) const { return checkSum; }
  inline unsigned long get_offset (void) const { return offset; }
  inline unsigned long get_length (void) const { return length; }

  private:
  Tag		tag;		/* 4-byte identifier. */
  CheckSum	checkSum;	/* CheckSum for this table. */
  ULONG		offset;		/* Offset from beginning of TrueType font
				 * file. */
  ULONG		length;		/* Length of this table. */
} OpenTypeTable;
ASSERT_SIZE (TableDirectory, 16);

typedef struct OffsetTable {
  /* OpenTypeTables, in no particular order */
  DEFINE_ARRAY_TYPE (TableDirectory, tableDir, numTables);
  // TODO: Implement find_table

  private:
  Tag		sfnt_version;	/* '\0\001\0\00' if TrueType / 'OTTO' if CFF */
  USHORT	numTables;	/* Number of tables. */
  USHORT	searchRange;	/* (Maximum power of 2 <= numTables) x 16 */
  USHORT	entrySelector;	/* Log2(maximum power of 2 <= numTables). */
  USHORT	rangeShift;	/* NumTables x 16-searchRange. */
  TableDirectory tableDir[];	/* TableDirectory entries. numTables items */
} OpenTypeFontFace;
ASSERT_SIZE (OffsetTable, 12);

/*
 * TrueType Collections
 */

struct TTCHeader {
  /* OpenTypeFontFaces, in no particular order */
  DEFINE_OFFSET_ARRAY_TYPE (OffsetTable, offsetTable, numFonts);

  private:
  Tag	ttcTag;		/* TrueType Collection ID string: 'ttcf' */
  ULONG	version;	/* Version of the TTC Header (1.0 or 2.0),
			 * 0x00010000 or 0x00020000 */
  ULONG	numFonts;	/* Number of fonts in TTC */
  ULONG	offsetTable[];	/* Array of offsets to the OffsetTable for each font
			 * from the beginning of the file */
};
ASSERT_SIZE (TTCHeader, 12);


/*
 * OpenType Font File
 */

struct OpenTypeFontFile {
  DEFINE_NON_INSTANTIABLE(OpenTypeFontFile);
  static const hb_tag_t TrueTypeTag	= HB_TAG ( 0 , 1 , 0 , 0 );
  static const hb_tag_t CFFTag		= HB_TAG ('O','T','T','O');
  static const hb_tag_t TTCTag		= HB_TAG ('t','t','c','f');

  /* Factory: ::get(font_data)
   * This is how you get a handle to one of these
   */
  static inline const OpenTypeFontFile& get (const char *font_data) {
    return (const OpenTypeFontFile&)*font_data;
  }
  static inline OpenTypeFontFile& get (char *font_data) {
    return (OpenTypeFontFile&)*font_data;
  }

  /* This is how you get a table */
  inline const char* get_table (const OpenTypeTable& table) const {
    return ((const char*)this) + table.offset;
  }
  inline char* get_table (const OpenTypeTable& table) {
    return ((char*)this) + table.offset;
  }
  inline const char* operator[] (const OpenTypeTable& table) const {
    return ((const char*)this) + table.offset;
  }
  inline char* operator[] (const OpenTypeTable& table) {
    return ((char*)this) + table.offset;
  }

  /* Array interface sans get_size() */
  inline unsigned int get_len (void) const {
    switch (tag) {
    default: return 0;
    case TrueTypeTag: case CFFTag: return 1;
    case TTCTag: return ((const TTCHeader&)*this).get_len();
    }
  }
  inline const OpenTypeFontFace& operator[] (unsigned int i) const {
    assert (i < get_len ());
    switch (tag) {
    default: case TrueTypeTag: case CFFTag: return (const OffsetTable&)*this;
    case TTCTag: return ((const TTCHeader&)*this)[i];
    }
  }
  inline OpenTypeFontFace& operator[] (unsigned int i) {
    assert (i < get_len ());
    switch (tag) {
    default: case TrueTypeTag: case CFFTag: return (OffsetTable&)*this;
    case TTCTag: return ((TTCHeader&)*this)[i];
    }
  }
  inline const Tag& get_tag (void) const { return tag; }

  private:
  Tag		tag;		/* 4-byte identifier. */
};
ASSERT_SIZE (OpenTypeFontFile, 4);



/*
 *
 * OpenType Layout Common Table Formats
 *
 */

/*
 * Script, ScriptList, LangSys, Feature, FeatureList, Lookup, LookupList
 */

struct Script;
struct ScriptList;
struct LangSys;
struct Feature;
struct FeatureList;
struct Lookup;
struct LookupList;

typedef struct Record {
  Tag		tag;		/* 4-byte Tag identifier */
  Offset	offset;		/* Offset from beginning of object holding
				 * the Record */
} ScriptRecord, LangSysRecord, FeatureRecord;
ASSERT_SIZE (Record, 6);

struct ScriptList {
  /* Scripts, in sorted alphabetical tag order */
  DEFINE_RECORD_ARRAY_TYPE (Script, scriptRecord, scriptCount);

  private:
  USHORT	scriptCount;	/* Number of ScriptRecords */
  ScriptRecord	scriptRecord[]; /* Array of ScriptRecords--listed alphabetically
				 * by ScriptTag */
};
ASSERT_SIZE (ScriptList, 2);

struct Script {
  /* LangSys', in sorted alphabetical tag order */
  DEFINE_RECORD_ARRAY_TYPE (LangSys, langSysRecord, langSysCount);

  /* Return NULL if none */
  inline const LangSys* get_default_language_system (void) const {
    if (!defaultLangSys)
      return NULL;
    return (const LangSys *)((const char*)this + defaultLangSys);
  }
  inline LangSys* get_default_language_system (void) {
    if (!defaultLangSys)
      return NULL;
    return (LangSys *)((char*)this + defaultLangSys);
  }

  /* TODO implement find_language_system based on find_tag */

  private:
  Offset	defaultLangSys;	/* Offset to DefaultLangSys table--from
				 * beginning of Script table--may be NULL */
  USHORT	langSysCount;	/* Number of LangSysRecords for this script--
				 * excluding the DefaultLangSys */
  LangSysRecord	langSysRecord[];/* Array of LangSysRecords--listed
				 * alphabetically by LangSysTag */
};
ASSERT_SIZE (Script, 4);

struct LangSys {
  /* Feature indices, in no particular order */
  DEFINE_ARRAY_TYPE (USHORT, featureIndex, featureCount);
  
  /* Returns -1 if none */
  inline int get_required_feature_index (void) const {
    if (reqFeatureIndex == 0xffff)
      return -1;
    return reqFeatureIndex;;
  }

  /* TODO implement find_feature */

  private:
  Offset	lookupOrder;	/* = NULL (reserved for an offset to a
				 * reordering table) */
  USHORT	reqFeatureIndex;/* Index of a feature required for this
				 * language system--if no required features
				 * = 0xFFFF */
  USHORT	featureCount;	/* Number of FeatureIndex values for this
				 * language system--excludes the required
				 * feature */
  USHORT	featureIndex[];	/* Array of indices into the FeatureList--in
				 * arbitrary order. featureCount entires long */
};
ASSERT_SIZE (LangSys, 6);

struct FeatureList {
  /* Feature indices, in sorted alphabetical tag order */
  DEFINE_RECORD_ARRAY_TYPE (Feature, featureRecord, featureCount);

  private:
  USHORT	featureCount;	/* Number of FeatureRecords in this table */
  FeatureRecord	featureRecord[];/* Array of FeatureRecords--zero-based (first
				 * feature has FeatureIndex = 0)--listed
				 * alphabetically by FeatureTag */
};
ASSERT_SIZE (FeatureList, 2);

struct Feature {
  /* LookupList indices, in no particular order */
  DEFINE_ARRAY_TYPE (USHORT, lookupIndex, lookupCount);

  // TODO: implement get_feature_params()

  private:
  Offset	featureParams;	/* Offset to Feature Parameters table (if one
				 * has been defined for the feature), relative
				 * to the beginning of the Feature Table; = NULL
				 * if not required */
  USHORT	lookupCount;	/* Number of LookupList indices for this
				 * feature */
  USHORT	lookupIndex[];	/* Array of LookupList indices for this
				 * feature--zero-based (first lookup is
				 * LookupListIndex = 0) */
};
ASSERT_SIZE (Feature, 4);

struct LookupList {
  /* Lookup indices, in sorted alphabetical tag order */
  DEFINE_OFFSET_ARRAY_TYPE (Lookup, lookupOffset, lookupCount);

  private:
  USHORT	lookupCount;	/* Number of lookups in this table */
  Offset	lookupOffset[];	/* Array of offsets to Lookup tables--from
				 * beginning of LookupList--zero based (first
				 * lookup is Lookup index = 0) */
};
ASSERT_SIZE (LookupList, 2);

struct LookupFlag : USHORT {
  static const uint16_t RightToLeft		= 0x0001u;
  static const uint16_t IgnoreBaseGlyphs	= 0x0002u;
  static const uint16_t IgnoreLigatures		= 0x0004u;
  static const uint16_t IgnoreMarks		= 0x0008u;
  static const uint16_t Reserved		= 0x00F0u;
  static const uint16_t MarkAttachmentType	= 0xFF00u;
};
ASSERT_SIZE (LookupFlag, 2);

struct Lookup {
  /* SubTables, in the desired order */
  DEFINE_OFFSET_ARRAY_TYPE (char*, subTableOffset, subTableCount);

  inline bool is_right_to_left	(void) const { return lookupFlag & LookupFlag::RightToLeft; }
  inline bool ignore_base_glyphs(void) const { return lookupFlag & LookupFlag::IgnoreBaseGlyphs; }
  inline bool ignore_ligatures	(void) const { return lookupFlag & LookupFlag::IgnoreLigatures; }
  inline bool ignore_marks	(void) const { return lookupFlag & LookupFlag::IgnoreMarks; }
  inline bool get_mark_attachment_type (void) const { return lookupFlag & LookupFlag::MarkAttachmentType; }

  inline uint16_t get_type (void) const { return lookupType; }
  inline uint16_t get_flag (void) const { return lookupFlag; }

  private:
  USHORT	lookupType;	/* Different enumerations for GSUB and GPOS */
  USHORT	lookupFlag;	/* Lookup qualifiers */
  USHORT	subTableCount;	/* Number of SubTables for this lookup */
  Offset	subTableOffset[];/* Array of offsets to SubTables-from
				  * beginning of Lookup table */
};
ASSERT_SIZE (Lookup, 6);

/*
 * Coverage Table
 */

struct CoverageFormat1 {
  /* GlyphIDs, in sorted numerical order */
  DEFINE_ARRAY_TYPE (GlyphID, glyphArray, glyphCount);

  inline int get_coverage (uint16_t glyph_id) const {
    GlyphID gid;
    gid = glyph_id;
    // TODO: bsearch
    for (int i = 0; i < glyphCount; i++)
      if (gid == glyphArray[i])
        return i;
    return -1;
  }

  private:
  USHORT	coverageFormat;	/* Format identifier--format = 1 */
  USHORT	glyphCount;	/* Number of glyphs in the GlyphArray */
  GlyphID	glyphArray[];	/* Array of GlyphIDs--in numerical order */
};
ASSERT_SIZE (CoverageFormat1, 4);

struct CoverageRangeRecord {

  inline int get_coverage (uint16_t glyph_id) const {
    if (glyph_id >= start && glyph_id <= end)
      return startCoverageIndex + (glyph_id - start);
    return -1;
  }

  private:
  GlyphID	start;			/* First GlyphID in the range */
  GlyphID	end;			/* Last GlyphID in the range */
  USHORT	startCoverageIndex;	/* Coverage Index of first GlyphID in
					 * range */
};
ASSERT_SIZE (CoverageRangeRecord, 6);

struct CoverageFormat2 {
  /* CoverageRangeRecords, in sorted numerical start order */
  DEFINE_ARRAY_TYPE (CoverageRangeRecord, rangeRecord, rangeCount);

  inline int get_coverage (uint16_t glyph_id) const {
    // TODO: bsearch
    for (int i = 0; i < rangeCount; i++) {
      int coverage = rangeRecord[i].get_coverage (glyph_id);
      if (coverage >= 0)
        return coverage;
    }
    return -1;
  }

  private:
  USHORT		coverageFormat;	/* Format identifier--format = 2 */
  USHORT		rangeCount;	/* Number of CoverageRangeRecords */
  CoverageRangeRecord	rangeRecord[];	/* Array of glyph ranges--ordered by
					 * Start GlyphID. rangeCount entries
					 * long */
};
ASSERT_SIZE (CoverageFormat2, 4);

struct Coverage {
  DEFINE_NON_INSTANTIABLE(Coverage);

  inline unsigned int get_size (void) const {
    switch (u.coverageFormat) {
    case 1: return u.format1.get_size ();
    case 2: return u.format2.get_size ();
    default:return sizeof (u.coverageFormat);
    }
  }

  /* Returns -1 if not covered. */
  inline int get_coverage (uint16_t glyph_id) const {
    switch (u.coverageFormat) {
    case 1: return u.format1.get_coverage(glyph_id);
    case 2: return u.format2.get_coverage(glyph_id);
    default:return -1;
    }
  }

  private:
  union {
  USHORT		coverageFormat;	/* Format identifier */
  CoverageFormat1	format1;
  CoverageFormat2	format2;
  } u;
};

/*
 * Class Definition Table
 */

struct ClassDefFormat1 {
  /* GlyphIDs, in sorted numerical order */
  DEFINE_ARRAY_TYPE (USHORT, classValueArray, glyphCount);

  inline int get_class (uint16_t glyph_id) const {
    if (glyph_id >= startGlyph && glyph_id < startGlyph + glyphCount)
      return classValueArray[glyph_id - startGlyph];
    return 0;
  }

  private:
  USHORT	classFormat;		/* Format identifier--format = 1 */
  GlyphID	startGlyph;		/* First GlyphID of the classValueArray */
  USHORT	glyphCount;		/* Size of the classValueArray */
  USHORT	classValueArray[];	/* Array of Class Values--one per GlyphID */
};
ASSERT_SIZE (ClassDefFormat1, 6);

struct ClassRangeRecord {

  inline int get_class (uint16_t glyph_id) const {
    if (glyph_id >= start && glyph_id <= end)
      return classValue;
    return 0;
  }

  private:
  GlyphID	start;		/* First GlyphID in the range */
  GlyphID	end;		/* Last GlyphID in the range */
  USHORT	classValue;	/* Applied to all glyphs in the range */
};
ASSERT_SIZE (ClassRangeRecord, 6);

struct ClassDefFormat2 {
  /* ClassRangeRecords, in sorted numerical start order */
  DEFINE_ARRAY_TYPE (ClassRangeRecord, rangeRecord, rangeCount);

  inline int get_class (uint16_t glyph_id) const {
    // TODO: bsearch
    for (int i = 0; i < rangeCount; i++) {
      int classValue = rangeRecord[i].get_class (glyph_id);
      if (classValue > 0)
        return classValue;
    }
    return 0;
  }

  private:
  USHORT		classFormat;	/* Format identifier--format = 2 */
  USHORT		rangeCount;	/* Number of Number of ClassRangeRecords */
  ClassRangeRecord	rangeRecord[];	/* Array of glyph ranges--ordered by
					 * Start GlyphID */
};
ASSERT_SIZE (ClassDefFormat2, 4);

struct ClassDef {
  DEFINE_NON_INSTANTIABLE(ClassDef);

  inline unsigned int get_size (void) const {
    switch (u.classFormat) {
    case 1: return u.format1.get_size ();
    case 2: return u.format2.get_size ();
    default:return sizeof (u.classFormat);
    }
  }

  /* Returns 0 if not found. */
  inline int get_class (uint16_t glyph_id) const {
    switch (u.classFormat) {
    case 1: return u.format1.get_class(glyph_id);
    case 2: return u.format2.get_class(glyph_id);
    default:return 0;
    }
  }

  private:
  union {
  USHORT		classFormat;	/* Format identifier */
  ClassDefFormat1	format1;
  ClassDefFormat2	format2;
  } u;
};

/*
 * Device Tables
 */

struct Device {

  inline unsigned int get_size (void) const {
    int count = endSize - startSize + 1;
    if (count < 0) count = 0;
    switch (deltaFormat) {
    case 1: return sizeof (Device) + sizeof (USHORT) * ((count+7)/8);
    case 2: return sizeof (Device) + sizeof (USHORT) * ((count+3)/4);
    case 3: return sizeof (Device) + sizeof (USHORT) * ((count+1)/2);
    default:return sizeof (Device);
    }
  }

  inline int get_delta (int ppem_size) const {
    if (ppem_size >= startSize && ppem_size <= endSize &&
        deltaFormat >= 1 && deltaFormat <= 3) {
      int s = ppem_size - startSize;
      int f = deltaFormat;

      uint16_t byte = deltaValue[s >> (4 - f)];
      uint16_t bits = byte >> (16 - (((s & ((1 << (4 - f)) - 1)) + 1) << f));
      uint16_t mask = 0xFFFF >> (16 - (1 << f));
      
      int delta = bits & mask;

      if (delta >= ((mask + 1) >> 1))
        delta -= mask + 1;

      return delta;
    }
    return 0;
  }

  private:
  USHORT	startSize;	/* Smallest size to correct--in ppem */
  USHORT	endSize;	/* Largest size to correct--in ppem */
  USHORT	deltaFormat;	/* Format of DeltaValue array data: 1, 2, or 3 */
  USHORT	deltaValue[];	/* Array of compressed data */
};
ASSERT_SIZE (Device, 6);



#define DEFINE_LIST_ACCESSOR0(const, Type, name) \
  inline const Type##List* get_##name##_list (void) const { \
    assert (name##List); \
    return (const Type##List *)((const char*)this + name##List); \
  } \
  inline const Type& get_##name (unsigned int i) const { \
    return (*get_##name##_list())[i]; \
  }
#define DEFINE_LIST_ACCESSOR(Type, name) \
	DEFINE_LIST_ACCESSOR0(const, Type, name) \
	DEFINE_LIST_ACCESSOR0(     , Type, name)

struct GSUBGPOSHeader {

  DEFINE_LIST_ACCESSOR(Script, script);	 /* get_script_list, get_script(i) */
  DEFINE_LIST_ACCESSOR(Feature, feature);/* get_feature_list, get_feature(i) */
  DEFINE_LIST_ACCESSOR(Lookup, lookup);	 /* get_lookup_list, get_lookup(i) */

  /* TODO implement find_script */

  private:
  Fixed_Version	version;	/* Version of the GSUB/GPOS table--initially set
				 * to 0x00010000 */
  Offset	scriptList;  	/* Offset to ScriptList table--from beginning of
				 * GSUB/GPOS table */
  Offset	featureList; 	/* Offset to FeatureList table--from beginning of
				 * GSUB/GPOS table */
  Offset	lookupList; 	/* Offset to LookupList table--from beginning of
				 * GSUB/GPOS table */
};
ASSERT_SIZE (GSUBGPOSHeader, 10);

#endif /* HARFBUZZ_OPEN_PRIVATE_H */
