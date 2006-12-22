#include <stdint.h>
#include <glib.h>

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


/* Macros to convert to/from BigEndian */
#define hb_be_uint8_t
#define hb_be_int8_t
#define hb_be_uint16_t	GUINT16_TO_BE
#define hb_be_int16_t	GINT16_TO_BE
#define hb_be_uint32_t	GUINT32_TO_BE
#define hb_be_int32_t	GINT32_TO_BE
#define hb_be_uint64_t	GUINT64_TO_BE
#define hb_be_int64_t	GINT64_TO_BE

#define DEFINE_INT_TYPE1(NAME, TYPE, BIG_ENDIAN) \
struct NAME { \
  inline NAME (TYPE i) { v = BIG_ENDIAN(i); } \
  inline TYPE operator = (TYPE i) { v = BIG_ENDIAN(i); return i; } \
  inline operator TYPE(void) { return BIG_ENDIAN(v); } \
  private: TYPE v; \
}
#define DEFINE_INT_TYPE0(NAME, type) DEFINE_INT_TYPE1 (NAME, type, hb_be_##type)
#define DEFINE_INT_TYPE(NAME, u, w)  DEFINE_INT_TYPE0 (NAME, u##int##w##_t)

DEFINE_INT_TYPE (HB_BYTE,	 u, 8);		/* 8-bit unsigned integer. */
DEFINE_INT_TYPE (HB_CHAR,	  , 8);		/* 8-bit signed integer. */
DEFINE_INT_TYPE (HB_USHORT,	 u, 16);	/* 16-bit unsigned integer. */
DEFINE_INT_TYPE (HB_SHORT,	  , 16);	/* 16-bit signed integer. */
DEFINE_INT_TYPE (HB_ULONG,	 u, 32);	/* 32-bit unsigned integer. */
DEFINE_INT_TYPE (HB_LONG,	  , 32);	/* 32-bit signed integer. */

/* Date represented in number of seconds since 12:00 midnight, January 1,
 * 1904. The value is represented as a signed 64-bit integer. */
DEFINE_INT_TYPE (HB_LONGDATETIME, , 64);

/* 32-bit signed fixed-point number (16.16) */
struct HB_Fixed : HB_LONG {
  inline operator double(void) { return (uint32_t) this / 65536.; }
  inline int16_t int_part (void) { return (uint32_t) this >> 16; }
  inline int16_t frac_part (void) { return (uint32_t) this & 0xffff; }
};

/* Smallest measurable distance in the em space. */
struct HB_FUNIT;

/* 16-bit signed integer (SHORT) that describes a quantity in FUnits. */
struct HB_FWORD : HB_SHORT {
};

/* 16-bit unsigned integer (USHORT) that describes a quantity in FUnits. */
struct HB_UFWORD : HB_USHORT {
};

/* 16-bit signed fixed number with the low 14 bits of fraction (2.14). */
struct HB_F2DOT14 : HB_SHORT {
  inline operator double() { return (uint32_t) this / 16384.; }
};

/* Array of four uint8s (length = 32 bits) used to identify a script, language
 * system, feature, or baseline */
struct HB_Tag : public HB_ULONG {
  inline HB_Tag (char *c) : HB_ULONG(c ? *(uint32_t*)c : 0) {}
};

/* Glyph index number, same as uint16 (length = 16 bits) */
struct HB_GlyphID : HB_USHORT {
};

/* Offset to a table, same as uint16 (length = 16 bits), NULL offset = 0x0000 */
struct HB_Offset : HB_USHORT {
};

/* CheckSum */
struct HB_CheckSum : HB_ULONG {
  static uint32_t CalcTableChecksum (HB_ULONG *Table, uint32_t Length) {
    uint32_t Sum = 0L;
    HB_ULONG *EndPtr = Table+((Length+3) & ~3) / sizeof(HB_ULONG);

    while (Table < EndPtr)
      Sum += *Table++;
    return Sum;
  }
};



/*
 * Version Numbers
 */


struct HB_USHORT_Version : HB_USHORT {
};

struct HB_Fixed_Version : HB_Fixed {
  inline int16_t major (void) { return this->int_part(); }
  inline int16_t minor (void) { return this->frac_part(); }
};



/*
 * Organization of an OpenType Font
 */


/* Offset Table */
struct HB_OffsetTable {
  HB_Fixed_Version	sfnt_version;	/* 0x00010000 for version 1.0. */
  HB_USHORT		numTables;	/* Number of tables. */
  HB_USHORT		searchRange;	/* (Maximum power of 2 <= numTables)
					 * x 16 */
  HB_USHORT		entrySelector;	/* Log2(maximum power of 2 <=
					 * numTables). */
  HB_USHORT		rangeShift;	/* NumTables x 16-searchRange. */
};


/* Table Directory */
struct HB_TableDirectory {
  HB_Tag	tag;		/* 4-byte identifier. */
  HB_CheckSum	checkSum;	/* CheckSum for this table. */
  HB_ULONG	offset;		/* Offset from beginning of TrueType font
				 * file. */
  HB_ULONG	length;		/* Length of this table. */
};



/*
 * TrueType Collections
 */


/* TTC Header Version 1.0 */
struct HB_TTCHeader {
  HB_Tag	TTCTag;		/* TrueType Collection ID string: 'ttcf' */
  HB_ULONG	version;	/* Version of the TTC Header (1.0 or 2.0),
				 * 0x00010000 or 0x00020000 */
  HB_ULONG	numFonts;	/* Number of fonts in TTC */
  HB_ULONG	offsetTable[0];	/* Array of offsets to the OffsetTable for
				 * each font from the beginning of the file.
				 * numFonts entries long. */

  inline int len(void) { return sizeof (HB_TTCHeader)
			      + sizeof (HB_ULONG) * numFonts; }
};


/* TTC Header Version 2.0 tail
 * Follows after HB_TTCHeader with appropriate size for the offsetTable. */
struct HB_TTCHeaderVersion2Tail {
  HB_ULONG	ulDsigTag;	/* Tag indicating that a DSIG table exists,
				 * 0x44534947 ('DSIG') (null if no signature) */
  HB_ULONG	ulDsigLength;	/* The length (in bytes) of the DSIG table
				 * (null if no signature) */
  HB_ULONG	ulDsigOffset;	/* The offset (in bytes) of the DSIG table
				 * from the beginning of the TTC file (null if
				 * no signature) */
};






/*
 *
 * OpenType Layout Common Table Formats
 *
 */


/*
 * Script List Table and Script Record
 */

struct ScriptRecord {
  Tag		scriptTag;	/* 4-byte ScriptTag identifier */
  Offset	scriptOffset;	/* Offset to Script table--from beginning of
				 * ScriptList */
};

struct ScriptList {
  USHORT	scriptCount;	/* Number of ScriptRecords */
  ScriptRecord	scriptRecord[]; /* Array of ScriptRecords--listed alphabetically
				 * by ScriptTag. scriptCount entries long */

  inline int len(void) { return sizeof (ScriptList)
			      + sizeof (ScriptRecord) * scriptCount; }
};







#include <stdio.h>

int
main (void)
{
  HB_Tag y("abcd");
  HB_Tag &x = y;
  HB_BYTE b(0);

  printf ("%d %04x %04x\n", sizeof (x), x+0, y+0);
  return 0;
}
