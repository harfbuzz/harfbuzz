#include <stdint.h>

/*
 *
 * The OpenType Font File
 *
 */

/*
 * Data Types:
 */


/* "All OpenType fonts use Motorola-style byte ordering (Big Endian)"
 * So we define the following hb_be_* types as structs/unions, to
 * disallow accidental access to them as an integer. */

typedef union {
  uint8_t u8;
  uint8_t v;
} hb_be_uint8_t;

typedef union {
  int8_t i8;
  int8_t v;
} hb_be_int8_t;

typedef union {
  uint8_t u8x2[2];
  uint16_t u16;
  uint16_t v;
} hb_be_uint16_t;

typedef union {
  uint8_t u8x2[2];
  int16_t i16;
  int16_t v;
} hb_be_int16_t;

typedef union {
  uint8_t u8x4[4];
  uint16_t u16x2[2];
  uint32_t u32;
  uint32_t v;
  inline operator int () { return (u8x4[0]<<24)+(u8x4[1]<<16)+(u8x4[2]<<8)+u8x4[3]; }
} hb_be_uint32_t;

typedef union {
  uint8_t u8x4[4];
  uint16_t u16x2[2];
  int32_t i32;
  int32_t v;
} hb_be_int32_t;

typedef union {
  uint8_t u8x8[8];
  uint16_t u16x4[4];
  uint32_t u32x2[2];
  uint64_t u64;
  uint64_t v;
} hb_be_uint64_t;

typedef union {
  uint8_t u8x8[8];
  uint16_t u16x4[4];
  uint32_t u32x2[2];
  int64_t i64;
  int64_t v;
} hb_be_int64_t;


/* "The following data types are used in the OpenType font file." */

typedef hb_be_uint8_t	HB_BYTE;	/* 8-bit unsigned integer. */
typedef hb_be_int8_t	HB_CHAR;	/* 8-bit signed integer. */
typedef hb_be_uint16_t	HB_USHORT;	/* 16-bit unsigned integer. */
typedef hb_be_int16_t	HB_SHORT;	/* 16-bit signed integer. */
typedef hb_be_uint32_t	HB_ULONG;	/* 32-bit unsigned integer. */
typedef hb_be_int32_t	HB_LONG;	/* 32-bit signed integer. */
typedef hb_be_int32_t	HB_Fixed;	/* 32-bit signed fixed-point number
					 * (16.16) */
typedef struct _FUNIT	HB_FUNIT;	/* Smallest measurable distance
					 * in the em space. */
typedef HB_SHORT	HB_FWORD;	/* 16-bit signed integer (SHORT) that
					 * describes a quantity in FUnits. */
typedef HB_USHORT	HB_UFWORD;	/* 16-bit unsigned integer (USHORT)
					 * that describes a quantity in
					 * FUnits. */
typedef hb_be_int16_t	HB_F2DOT14;	/* 16-bit signed fixed number with the
					 * low 14 bits of fraction (2.14). */
typedef hb_be_int64_t	HB_LONGDATETIME;/* Date represented in number of
					 * seconds since 12:00 midnight,
					 * January 1, 1904. The value is
					 * represented as a signed 64-bit
					 * integer. */
typedef hb_be_uint32_t	HB_Tag;		/* Array of four uint8s (length = 32
					 * bits) used to identify a script,
					 * language system, feature, or
					 * baseline */
typedef hb_be_uint16_t	HB_GlyphID;	/* Glyph index number, same as
					 * uint16(length = 16 bits) */
typedef hb_be_uint16_t	HB_Offset;	/* Offset to a table, same as uint16
					 * (length = 16 bits), NULL offset =
					 * 0x0000 */

#include <stdio.h>

int
main (void)
{
  HB_Tag x = {{'a', 'b', 'c', 'd'}};
  HB_ULONG y = x;

  printf ("%d\n", x+0);
  return 0;
}
