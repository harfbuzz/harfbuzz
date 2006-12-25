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
  inline NAME (TYPE i) { v = BIG_ENDIAN(i); } \
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
  inline Tag (const char *c) { v[0] = c[0]; v[1] = c[1]; v[2] = c[2]; v[3] = c[3]; }
  inline operator uint32_t(void) const { return (v[0]<<24)+(v[1]<<16) +(v[2]<<8)+v[3]; } \
  private: char v[4];
};

/* Glyph index number, same as uint16 (length = 16 bits) */
struct GlyphID : USHORT {
};

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

struct TableDirectory {
  Tag		tag;		/* 4-byte identifier. */
  CheckSum	checkSum;	/* CheckSum for this table. */
  ULONG		offset;		/* Offset from beginning of TrueType font
				 * file. */
  ULONG		length;		/* Length of this table. */
};

struct OffsetTable {
  DEFINE_ARRAY_TYPE (TableDirectory, tableDir, numTables);

  Tag		sfnt_version;	/* '\0\001\0\00' if TrueType / 'OTTO' if CFF */
  USHORT	numTables;	/* Number of tables. */
  USHORT	searchRange;	/* (Maximum power of 2 <= numTables) x 16 */
  USHORT	entrySelector;	/* Log2(maximum power of 2 <= numTables). */
  USHORT	rangeShift;	/* NumTables x 16-searchRange. */
  TableDirectory tableDir[];	/* TableDirectory entries. numTables items */
};


/*
 * TrueType Collections
 */

struct TTCHeader {
  /* This works as an array type because TTCHeader is always located at the
   * beginning of the file */
  DEFINE_OFFSET_ARRAY_TYPE (OffsetTable, offsetTable, numFonts);

  Tag	TTCTag;		/* TrueType Collection ID string: 'ttcf' */
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
  Tag		tag;		/* 4-byte identifier. */

  unsigned int get_len (void) const {
    switch (tag) {
    default:
      return 0;
    case HB_TAG (0,1,0,0):
    case HB_TAG ('O','T','T','O'):
      return 1;
    case HB_TAG ('t','t','c','f'):
      const TTCHeader &ttc = (TTCHeader&)*this;
      return ttc.get_len ();
    }
  }

  inline const OffsetTable& operator[] (unsigned int i) const {
    assert (i < get_len ());
    switch (tag) {
    default:
    case HB_TAG (0,1,0,0):
    case HB_TAG ('O','T','T','O'):
      return (const OffsetTable&)*this;
    case HB_TAG ('t','t','c','f'):
      const TTCHeader &ttc = (const TTCHeader&)*this;
      return ttc[i];
    }
  }
  inline OffsetTable& operator[] (unsigned int i) {
    assert (i < get_len ());
    switch (tag) {
    default:
    case HB_TAG (0,1,0,0):
    case HB_TAG ('O','T','T','O'):
      return (OffsetTable&)*this;
    case HB_TAG ('t','t','c','f'):
      TTCHeader &ttc = (TTCHeader&)*this;
      return ttc[i];
    }
  }

  /* ::get(font_data).  This is how you get a handle to one of these */
  static inline const OpenTypeFontFile& get (const char *font_data) {
    return (const OpenTypeFontFile&)*font_data;
  }
  static inline OpenTypeFontFile& get (char *font_data) {
    return (OpenTypeFontFile&)*font_data;
  }

  /* cannot be instantiated */
  private: OpenTypeFontFile() {}
};



/*
 *
 * OpenType Layout Common Table Formats
 *
 */

struct Script;
struct LangSys;


typedef struct Record {
  Tag		tag;		/* 4-byte Tag identifier */
  Offset	offset;		/* Offset from beginning of object holding
				 * the Record */
} ScriptRecord, LangSysRecord, FeatureRecord;

struct Script;

struct ScriptList {
  DEFINE_RECORD_ARRAY_TYPE (Script, scriptRecord, scriptCount);

  USHORT	scriptCount;	/* Number of ScriptRecords */
  ScriptRecord	scriptRecord[]; /* Array of ScriptRecords--listed alphabetically
				 * by ScriptTag. scriptCount entries long */
};

struct Script {
  DEFINE_RECORD_ARRAY_TYPE (LangSys, langSysRecord, langSysCount);

  Offset	defaultLangSys;	/* Offset to DefaultLangSys table--from
				 * beginning of Script table--may be NULL */
  USHORT	langSysCount;	/* Number of LangSysRecords for this script--
				 * excluding the DefaultLangSys */
  LangSysRecord	langSysRecord[];/* Array of LangSysRecords--listed
				 * alphabetically by LangSysTag */
};

struct LangSys {
  

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
  case HB_TAG (0,1,0,0):
    printf ("OpenType font with TrueType outlines\n");
    break;
  case HB_TAG ('O','T','T','O'):
    printf ("OpenType font with CFF (Type1) outlines\n");
    break;
  case HB_TAG ('t','t','c','f'):
    printf ("TrueType Collection of OpenType fonts\n");
    break;
  default:
    fprintf (stderr, "Unknown font format\n");
    exit (1);
    break;
  }

  int num_fonts = ot.get_len ();
  printf ("%d font(s) found in file\n", num_fonts);
  for (int i = 0; i < num_fonts; i++) {
    printf ("Font %d of %d\n", i+1, num_fonts);
    const OffsetTable &offset_table = ot[i];

  }

  return 0;
}
