#include <stdint.h>
#include <assert.h>
#include <glib.h>

/* Public header */

typedef uint32_t hb_tag_t;
#define HB_TAG(a,b,c,d) ((hb_tag_t)(((uint8_t)a<<24)|((uint8_t)b<<16)|((uint8_t)c<<8)|(uint8_t)d))


/* Implementation */

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
struct NAME { \
  inline NAME (void) { v = 0; } \
  explicit inline NAME (TYPE i) { v = BIG_ENDIAN(i); } \
  inline NAME& operator = (TYPE i) { v = BIG_ENDIAN(i); return *this; } \
  inline operator TYPE(void) const { return BIG_ENDIAN(v); } \
  inline bool operator== (NAME o) const { return v == o.v; } \
  private: TYPE v; \
}
#define DEFINE_INT_TYPE0(NAME, type) DEFINE_INT_TYPE1 (NAME, type, hb_be_##type)
#define DEFINE_INT_TYPE(NAME, u, w)  DEFINE_INT_TYPE0 (NAME, u##int##w##_t)


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

#define DEFINE_NOT_INSTANTIABLE(Type) \
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
    assert (array[i]); \
    return *(const Type *)((const char*)this + array[i]); \
  }

#define DEFINE_RECORD_ARRAY_TYPE(Type, array, num) \
  DEFINE_RECORD_ACCESSOR(const, Type, array, num) \
  DEFINE_RECORD_ACCESSOR(     , Type, array, num) \
  DEFINE_LEN_AND_SIZE(Record, array, num)
#define DEFINE_RECORD_ACCESSOR(const, Type, array, num) \
  inline const Type& operator[] (unsigned int i) const { \
    assert (i < num); \
    assert (array[i].offset); \
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


DEFINE_INT_TYPE (BYTE,	 u, 8);		/* 8-bit unsigned integer. */
DEFINE_INT_TYPE (CHAR,	  , 8);		/* 8-bit signed integer. */
DEFINE_INT_TYPE (USHORT, u, 16);	/* 16-bit unsigned integer. */
DEFINE_INT_TYPE (SHORT,	  , 16);	/* 16-bit signed integer. */
DEFINE_INT_TYPE (ULONG,	 u, 32);	/* 32-bit unsigned integer. */
DEFINE_INT_TYPE (LONG,	  , 32);	/* 32-bit signed integer. */

/* Date represented in number of seconds since 12:00 midnight, January 1,
 * 1904. The value is represented as a signed 64-bit integer. */
DEFINE_INT_TYPE (LONGDATETIME, , 64);

/* 32-bit signed fixed-point number (16.16) */
struct Fixed : LONG {
  inline operator double(void) const { return (uint32_t) this / 65536.; }
  inline int16_t int_part (void) const { return (uint32_t) this >> 16; }
  inline int16_t frac_part (void) const { return (uint32_t) this & 0xffff; }
};

/* Smallest measurable distance in the em space. */
struct FUNIT;

/* 16-bit signed integer (SHORT) that describes a quantity in FUnits. */
struct FWORD : SHORT {
};

/* 16-bit unsigned integer (USHORT) that describes a quantity in FUnits. */
struct UFWORD : USHORT {
};

/* 16-bit signed fixed number with the low 14 bits of fraction (2.14). */
struct F2DOT14 : SHORT {
  inline operator double() const { return (uint32_t) this / 16384.; }
};

/* Array of four uint8s (length = 32 bits) used to identify a script, language
 * system, feature, or baseline */
struct Tag {
  inline Tag (void) { v[0] = v[1] = v[2] = v[3] = 0; }
  inline Tag (uint32_t v) { (ULONG&)(*this) = v; }
  inline Tag (const char *c) { v[0] = c[0]; v[1] = c[1]; v[2] = c[2]; v[3] = c[3]; }
  inline bool operator== (Tag o) const { return v[0]==o.v[0]&&v[1]==o.v[1]&&v[2]==o.v[2]&&v[3]==o.v[3]; }
  inline operator uint32_t(void) const { return (v[0]<<24)+(v[1]<<16) +(v[2]<<8)+v[3]; }
  /* What the char* converters return is NOT nul-terminated.  Print using "%.4s" */
  inline operator const char* (void) const { return (const char *)this; }
  inline operator char* (void) { return (char *)this; }

  private: char v[4];
};

/* Glyph index number, same as uint16 (length = 16 bits) */
//struct GlyphID : USHORT {
//};
DEFINE_INT_TYPE (GlyphID, u, 16);	/* 16-bit unsigned integer. */

/* Offset to a table, same as uint16 (length = 16 bits), NULL offset = 0x0000 */
struct Offset : USHORT {
};

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


/*
 * Version Numbers
 */

struct USHORT_Version : USHORT {
};

struct Fixed_Version : Fixed {
  inline int16_t major (void) const { return this->int_part(); }
  inline int16_t minor (void) const { return this->frac_part(); }
};


/*
 * Organization of an OpenType Font
 */

struct OpenTypeFontFile;
struct OffsetTable;
struct TTCHeader;

typedef struct TableDirectory {
  Tag		tag;		/* 4-byte identifier. */
  CheckSum	checkSum;	/* CheckSum for this table. */
  ULONG		offset;		/* Offset from beginning of TrueType font
				 * file. */
  ULONG		length;		/* Length of this table. */
} OpenTypeTable;

typedef struct OffsetTable {
  /* OpenTypeTables, in no particular order */
  DEFINE_ARRAY_TYPE (TableDirectory, tableDir, numTables);
  // TODO: Implement find_table

  Tag		sfnt_version;	/* '\0\001\0\00' if TrueType / 'OTTO' if CFF */
  USHORT	numTables;	/* Number of tables. */
  USHORT	searchRange;	/* (Maximum power of 2 <= numTables) x 16 */
  USHORT	entrySelector;	/* Log2(maximum power of 2 <= numTables). */
  USHORT	rangeShift;	/* NumTables x 16-searchRange. */
  TableDirectory tableDir[];	/* TableDirectory entries. numTables items */
} OpenTypeFont;

/*
 * TrueType Collections
 */

struct TTCHeader {
  /* OpenTypeFonts, in no particular order */
  DEFINE_OFFSET_ARRAY_TYPE (OffsetTable, offsetTable, numFonts);

  Tag	ttcTag;		/* TrueType Collection ID string: 'ttcf' */
  ULONG	version;	/* Version of the TTC Header (1.0 or 2.0),
				 * 0x00010000 or 0x00020000 */
  ULONG	numFonts;	/* Number of fonts in TTC */
  ULONG	offsetTable[];	/* Array of offsets to the OffsetTable for each font
			 * from the beginning of the file.
			 * numFonts entries long. */
};


/*
 * OpenType Font File
 */

struct OpenTypeFontFile {
  DEFINE_NOT_INSTANTIABLE(OpenTypeFontFile);
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

  /* Array interface sans get_size() */
  inline unsigned int get_len (void) const {
    switch (tag) {
    default: return 0;
    case TrueTypeTag: case CFFTag: return 1;
    case TTCTag: return ((const TTCHeader&)*this).get_len();
    }
  }
  inline const OpenTypeFont& operator[] (unsigned int i) const {
    assert (i < get_len ());
    switch (tag) {
    default: case TrueTypeTag: case CFFTag: return (const OffsetTable&)*this;
    case TTCTag: return ((const TTCHeader&)*this)[i];
    }
  }
  inline OpenTypeFont& operator[] (unsigned int i) {
    assert (i < get_len ());
    switch (tag) {
    default: case TrueTypeTag: case CFFTag: return (OffsetTable&)*this;
    case TTCTag: return ((TTCHeader&)*this)[i];
    }
  }

  Tag		tag;		/* 4-byte identifier. */
};



/*
 *
 * OpenType Layout Common Table Formats
 *
 */

struct Script;
struct ScriptList;
struct LangSys;
struct Feature;
struct FeatureList;
struct Lookup;
struct LookupList;
struct SubTable;


typedef struct Record {
  Tag		tag;		/* 4-byte Tag identifier */
  Offset	offset;		/* Offset from beginning of object holding
				 * the Record */
} ScriptRecord, LangSysRecord, FeatureRecord;

struct ScriptList {
  /* Scripts, in sorted alphabetical tag order */
  DEFINE_RECORD_ARRAY_TYPE (Script, scriptRecord, scriptCount);

  USHORT	scriptCount;	/* Number of ScriptRecords */
  ScriptRecord	scriptRecord[]; /* Array of ScriptRecords--listed alphabetically
				 * by ScriptTag. scriptCount entries long */
};

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

  Offset	defaultLangSys;	/* Offset to DefaultLangSys table--from
				 * beginning of Script table--may be NULL */
  USHORT	langSysCount;	/* Number of LangSysRecords for this script--
				 * excluding the DefaultLangSys */
  LangSysRecord	langSysRecord[];/* Array of LangSysRecords--listed
				 * alphabetically by LangSysTag */
};

struct LangSys {
  /* Feature indices, in no particular order */
  DEFINE_ARRAY_TYPE (USHORT, featureIndex, featureCount);
  
  /* Returns -1 if none */
  inline int get_required_feature_index (void) const {
    if (reqFeatureIndex == 0xffff)
      return -1;
    return reqFeatureIndex;;
  }

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

struct FeatureList {
  /* Feature indices, in sorted alphabetical tag order */
  DEFINE_RECORD_ARRAY_TYPE (Feature, featureRecord, featureCount);

  USHORT	featureCount;	/* Number of FeatureRecords in this table */
  FeatureRecord	featureRecord[];/* Array of FeatureRecords--zero-based (first
				 * feature has FeatureIndex = 0)--listed
				 * alphabetically by FeatureTag. featureCount
				 * entries long */
};

struct Feature {
  /* LookupList indices, in no particular order */
  DEFINE_ARRAY_TYPE (USHORT, lookupIndex, lookupCount);

  // TODO: implement get_feature_params()

  Offset	featureParams;	/* Offset to Feature Parameters table (if one
				 * has been defined for the feature), relative
				 * to the beginning of the Feature Table; = NULL
				 * if not required */
  USHORT	lookupCount;	/* Number of LookupList indices for this
				 * feature */
  USHORT	lookupIndex[];	/* Array of LookupList indices for this
				 * feature--zero-based (first lookup is
				 * LookupListIndex = 0). lookupCount
				 * entries long */
};

struct LookupList {
  /* Lookup indices, in sorted alphabetical tag order */
  DEFINE_OFFSET_ARRAY_TYPE (Lookup, lookupOffset, lookupCount);

  USHORT	lookupCount;	/* Number of lookups in this table */
  Offset	lookupOffset[];	/* Array of offsets to Lookup tables--from
				 * beginning of LookupList--zero based (first
				 * lookup is Lookup index = 0).  lookupCount
				 * entries long */
};

struct LookupFlag : USHORT {
  static const uint16_t RightToLeft		= 0x0001u;
  static const uint16_t IgnoreBaseGlyphs	= 0x0002u;
  static const uint16_t IgnoreLigatures		= 0x0004u;
  static const uint16_t IgnoreMarks		= 0x0008u;
  static const uint16_t Reserved		= 0x00F0u;
  static const uint16_t MarkAttachmentType	= 0xFF00u;
};

struct Lookup {
  /* SubTables, in the desired order */
  DEFINE_OFFSET_ARRAY_TYPE (SubTable, subTableOffset, subTableCount);

  inline bool is_right_to_left	(void) const { return lookupFlag & LookupFlag::RightToLeft; }
  inline bool ignore_base_glyphs(void) const { return lookupFlag & LookupFlag::IgnoreBaseGlyphs; }
  inline bool ignore_ligatures	(void) const { return lookupFlag & LookupFlag::IgnoreLigatures; }
  inline bool ignore_marks	(void) const { return lookupFlag & LookupFlag::IgnoreMarks; }
  inline bool get_mark_attachment_type (void) const { return (lookupFlag & LookupFlag::MarkAttachmentType) >> 8; }

  USHORT	lookupType;	/* Different enumerations for GSUB and GPOS */
  USHORT	lookupFlag;	/* Lookup qualifiers */
  USHORT	subTableCount;	/* Number of SubTables for this lookup */
  Offset	subTableOffset[];/* Array of offsets to SubTables-from
				  * beginning of Lookup table. subTableCount
				  * entries long. */
};

struct CoverageFormat1 {
  /* GlyphIDs, in sorted numerical order */
  DEFINE_ARRAY_TYPE (GlyphID, glyphArray, glyphCount);

  inline unsigned int get_coverage (uint16_t glyph_id) const {
    GlyphID gid (glyph_id);
    // TODO: bsearch
    for (int i = 0; i < glyphCount; i++)
      if (gid == glyphArray[i])
        return i;
    return -1;
  }

  USHORT	coverageFormat;		/* Format identifier--format = 1 */
  USHORT	glyphCount;		/* Number of glyphs in the GlyphArray */
  GlyphID	glyphArray[];		/* Array of GlyphIDs--in numerical
					 * order. glyphCount entries long */
};

struct RangeRecord {
  inline unsigned int get_coverage (uint16_t glyph_id) const {
    if (glyph_id >= start && glyph_id <= end)
      return startCoverageIndex + (glyph_id - start);
    return -1;
  }

  GlyphID	start;			/* First GlyphID in the range */
  GlyphID	end;			/* Last GlyphID in the range */
  USHORT	startCoverageIndex;	/* Coverage Index of first GlyphID in
					 * range */
};

struct CoverageFormat2 {
  /* RangeRecords, in sorted numerical start order */
  DEFINE_ARRAY_TYPE (RangeRecord, rangeRecord, rangeCount);

  inline unsigned int get_coverage (uint16_t glyph_id) const {
    // TODO: bsearch
    for (int i = 0; i < rangeCount; i++) {
      int coverage = rangeRecord[i].get_coverage (glyph_id);
      if (coverage >= 0)
        return coverage;
    }
    return -1;
  }

  USHORT	coverageFormat;		/* Format identifier--format = 2 */
  USHORT	rangeCount;		/* Number of RangeRecords */
  RangeRecord	rangeRecord[];		/* Array of glyph ranges--ordered by
					 * Start GlyphID. rangeCount entries
					 * long */
};

struct CoverageFormat {
  DEFINE_NOT_INSTANTIABLE(CoverageFormat);

  inline unsigned int get_size (void) const {
    switch (coverageFormat) {
    case 1: return ((const CoverageFormat1&)*this).get_size ();
    case 2: return ((const CoverageFormat2&)*this).get_size ();
    default: return sizeof (coverageFormat);
    }
  }

  /* Returns -1 if not covered. */
  inline unsigned int get_coverage (uint16_t glyph_id) const {
    switch (coverageFormat) {
    case 1: return ((const CoverageFormat1&)*this).get_coverage(glyph_id);
    case 2: return ((const CoverageFormat2&)*this).get_coverage(glyph_id);
    default: return -1;
    }
  }

  USHORT	coverageFormat;		/* Format identifier */
};






#include <stdlib.h>
#include <stdio.h>

int
main (int argc, char **argv)
{
  if (argc != 2) {
    fprintf (stderr, "usage: %s font-file.ttf\n", argv[0]);
    exit (1);
  }

  GMappedFile *mf = g_mapped_file_new (argv[1], FALSE, NULL);
  const char *font_data = g_mapped_file_get_contents (mf);
  int len = g_mapped_file_get_length (mf);
  
  printf ("Opened font file %s: %d bytes long\n", argv[1], len);
  
  const OpenTypeFontFile &ot = OpenTypeFontFile::get (font_data);

  switch (ot.tag) {
  case OpenTypeFontFile::TrueTypeTag:
    printf ("OpenType font with TrueType outlines\n");
    break;
  case OpenTypeFontFile::CFFTag:
    printf ("OpenType font with CFF (Type1) outlines\n");
    break;
  case OpenTypeFontFile::TTCTag:
    printf ("TrueType Collection of OpenType fonts\n");
    break;
  default:
    printf ("Unknown font format\n");
    break;
  }

  int num_fonts = ot.get_len ();
  printf ("%d font(s) found in file\n", num_fonts);
  for (int n_font = 0; n_font < num_fonts; n_font++) {
    const OpenTypeFont &font = ot[n_font];
    printf ("Font %d of %d:\n", n_font+1, num_fonts);

    int num_tables = font.get_len ();
    printf ("%d table(s) found in font\n", num_tables);
    for (int n_table = 0; n_table < num_tables; n_table++) {
      const OpenTypeTable &table = font[n_table];
      printf ("Table %2d of %2d: %.4s (0x%06x+0x%06x)\n", n_table+1, num_tables,
	      (const char *)table.tag, (int)table.offset, (int)table.length);
    }
  }

  Tag a;
  a == ((Tag)"1234");

  return 0;
}
