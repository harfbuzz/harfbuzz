/*
 * Copyright (C) 2007,2008,2009  Red Hat, Inc.
 *
 *  This is part of HarfBuzz, an OpenType Layout engine library.
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

#ifndef HB_OT_LAYOUT_OPEN_PRIVATE_H
#define HB_OT_LAYOUT_OPEN_PRIVATE_H

#ifndef HB_OT_LAYOUT_CC
#error "This file should only be included from hb-ot-layout.cc"
#endif

#include "hb-ot-layout-private.h"


#define NO_INDEX		((unsigned int) 0xFFFF)
#define NO_CONTEXT		((unsigned int) -1)
#define NOT_COVERED		((unsigned int) -1)
#define MAX_NESTING_LEVEL	32


/*
 * Array types
 */

/* get_len() is a method returning the number of items in an array-like object */
#define DEFINE_LEN(Type, array, num) \
  inline unsigned int get_len(void) const { return num; } \

/* An array type is one that contains a variable number of objects
 * as its last item.  An array object is extended with get_len()
 * methods, as well as overloaded [] operator. */
#define DEFINE_ARRAY_TYPE(Type, array, num) \
  DEFINE_INDEX_OPERATOR(Type, array, num) \
  DEFINE_LEN(Type, array, num)
#define DEFINE_INDEX_OPERATOR(Type, array, num) \
  inline const Type& operator[] (unsigned int i) const { \
    if (HB_UNLIKELY (i >= num)) return Null(Type); \
    return array[i]; \
  }

/* An offset array type is like an array type, but it contains a table
 * of offsets to the objects, relative to the beginning of the current
 * object. */
#define DEFINE_OFFSET_ARRAY_TYPE(Type, array, num) \
  DEFINE_OFFSET_INDEX_OPERATOR(Type, array, num) \
  DEFINE_LEN(Offset, array, num)
#define DEFINE_OFFSET_INDEX_OPERATOR(Type, array, num) \
  inline const Type& operator[] (unsigned int i) const { \
    if (HB_UNLIKELY (i >= num)) return Null(Type); \
    if (HB_UNLIKELY (!array[i])) return Null(Type); \
    return *(const Type *)((const char*)this + array[i]); \
  }


#define DEFINE_ARRAY_INTERFACE(Type, name) \
  inline const Type& get_##name (unsigned int i) const { \
    return (*this)[i]; \
  } \
  inline unsigned int get_##name##_count (void) const { \
    return this->get_len (); \
  }
#define DEFINE_INDEX_ARRAY_INTERFACE(name) \
  inline unsigned int get_##name##_index (unsigned int i) const { \
    if (HB_UNLIKELY (i >= get_len ())) return NO_INDEX; \
    return (*this)[i]; \
  } \
  inline unsigned int get_##name##_count (void) const { \
    return get_len (); \
  }


/*
 * List types
 */

#define DEFINE_LIST_INTERFACE(Type, name) \
  inline const Type& get_##name (unsigned int i) const { \
    return (this+name##List)[i]; \
  } \
  inline unsigned int get_##name##_count (void) const { \
    return (this+name##List).len; \
  }

/*
 * Tag types
 */

#define DEFINE_TAG_ARRAY_INTERFACE(Type, name) \
  DEFINE_ARRAY_INTERFACE (Type, name); \
  inline const Tag& get_##name##_tag (unsigned int i) const { \
    return (*this)[i].tag; \
  }
#define DEFINE_TAG_LIST_INTERFACE(Type, name) \
  DEFINE_LIST_INTERFACE (Type, name); \
  inline const Tag& get_##name##_tag (unsigned int i) const { \
    return (this+name##List).get_tag (i); \
  }

#define DEFINE_TAG_FIND_INTERFACE(Type, name) \
  inline bool find_##name##_index (hb_tag_t tag, unsigned int *name##_index) const { \
    const Tag t = tag; \
    for (unsigned int i = 0; i < get_##name##_count (); i++) { \
      if (t == get_##name##_tag (i)) { \
        if (name##_index) *name##_index = i; \
        return true; \
      } \
    } \
    if (name##_index) *name##_index = NO_INDEX; \
    return false; \
  } \
  inline const Type& get_##name##_by_tag (hb_tag_t tag) const { \
    unsigned int i; \
    if (find_##name##_index (tag, &i)) \
      return get_##name (i); \
    else \
      return Null(Type); \
  }

/*
 * Class features
 */


/* Null objects */

/* Global nul-content Null pool.  Enlarge as necessary. */
static const char NullPool[16] = "";

/* Generic template for nul-content sizeof-sized Null objects. */
template <typename Type>
struct Null {
  ASSERT_STATIC (sizeof (Type) <= sizeof (NullPool));
  static inline const Type &get () { return (const Type&) *(const Type*) NullPool; }
};

/* Specializaiton for arbitrary-content arbitrary-sized Null objects. */
#define DEFINE_NULL_DATA(Type, size, data) \
template <> \
struct Null <Type> { \
  static inline const Type &get () { static const char bytes[size] = data; return (const Type&) *(const Type*) bytes; } \
}

/* Accessor macro. */
#define Null(Type) (Null<Type>::get())


#define ASSERT_SIZE_DATA(Type, size, data) \
  ASSERT_SIZE (Type, size); \
  DEFINE_NULL_DATA (Type, size, data)

/* get_for_data() is a static class method returning a reference to an
 * instance of Type located at the input data location.  It's just a
 * fancy, NULL-safe, cast! */
#define STATIC_DEFINE_GET_FOR_DATA(Type) \
  static inline const Type& get_for_data (const char *data) { \
    if (HB_UNLIKELY (data == NULL)) return Null(Type); \
    return *(const Type*)data; \
  } \
  static inline Type& get_for_data (char *data) { \
    return *(Type*)data; \
  }



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

/*
 * Int types
 */

/* XXX define these as structs of chars on machines that do not allow
 * unaligned access (using templates?). */
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
  }; \
  ASSERT_SIZE (NAME, w / 8)


DEFINE_INT_TYPE_STRUCT (BYTE,	 u,  8);	/*  8-bit unsigned integer. */
DEFINE_INT_TYPE_STRUCT (CHAR,	  ,  8);	/*  8-bit signed integer. */
DEFINE_INT_TYPE_STRUCT (USHORT,  u, 16);	/* 16-bit unsigned integer. */
DEFINE_INT_TYPE_STRUCT (SHORT,	  , 16);	/* 16-bit signed integer. */
DEFINE_INT_TYPE_STRUCT (ULONG,	 u, 32);	/* 32-bit unsigned integer. */
DEFINE_INT_TYPE_STRUCT (LONG,	  , 32);	/* 32-bit signed integer. */

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
#define _NULL_TAG_INIT  {' ', ' ', ' ', ' '}
DEFINE_NULL_DATA (Tag, 4, _NULL_TAG_INIT);
#undef _NULL_TAG_INIT

/* Glyph index number, same as uint16 (length = 16 bits) */
DEFINE_INT_TYPE_STRUCT (GlyphID, u, 16);

/* Offset to a table, same as uint16 (length = 16 bits), Null offset = 0x0000 */
DEFINE_INT_TYPE_STRUCT (Offset, u, 16);

/* Template subclass of Offset that does the dereferencing.  Use: (this+memberName) */
template <typename Type>
struct OffsetTo : Offset {
  inline const Type& operator() (const void *base) const {
    unsigned int offset = *this;
    if (HB_UNLIKELY (!offset)) return Null(Type);
    return * (const Type *) ((const char *) base + offset);
  }
};
template <typename Base, typename Type>
inline const Type& operator + (const Base &base, OffsetTo<Type> offset) {
  return offset(base);
}


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
 * Array Types
 */

/* An array with a USHORT number of elements. */
template <typename Type>
struct ArrayOf {
  inline const Type& operator [] (unsigned int i) const {
    if (HB_UNLIKELY (i >= len)) return Null(Type);
    return array[i];
  }
  inline unsigned int get_size () const {
    return sizeof (len) + len * sizeof (array[0]);
  }

  USHORT len;
  Type array[];
};

/* An array with a USHORT number of elements,
 * starting at second element. */
template <typename Type>
struct HeadlessArrayOf {
  inline const Type& operator [] (unsigned int i) const {
    if (HB_UNLIKELY (i >= len || !i)) return Null(Type);
    return array[i-1];
  }
  inline unsigned int get_size () const {
    return sizeof (len) + (len ? len - 1 : 0) * sizeof (array[0]);
  }

  USHORT len;
  Type array[];
};

/* Array of Offset's */
template <typename Type>
struct OffsetArrayOf : ArrayOf<OffsetTo<Type> > {
};

/* An array type is one that contains a variable number of objects
 * as its last item.  An array object is extended with get_len()
 * methods, as well as overloaded [] operator. */
#define DEFINE_ARRAY_TYPE(Type, array, num) \
  DEFINE_INDEX_OPERATOR(Type, array, num) \
  DEFINE_LEN(Type, array, num)
#define DEFINE_INDEX_OPERATOR(Type, array, num) \
  inline const Type& operator[] (unsigned int i) const { \
    if (HB_UNLIKELY (i >= num)) return Null(Type); \
    return array[i]; \
  }


/*
 * Organization of an OpenType Font
 */

struct OpenTypeFontFile;
struct OffsetTable;
struct TTCHeader;

typedef struct TableDirectory {

  friend struct OpenTypeFontFile;
  friend struct OffsetTable;

  inline bool is_null (void) const { return length == 0; }
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

  friend struct OpenTypeFontFile;
  friend struct TTCHeader;

  DEFINE_TAG_ARRAY_INTERFACE (OpenTypeTable, table);	/* get_table_count(), get_table(i), get_table_tag(i) */
  DEFINE_TAG_FIND_INTERFACE  (OpenTypeTable, table);	/* find_table_index(tag), get_table_by_tag(tag) */

  private:
  /* OpenTypeTables, in no particular order */
  DEFINE_ARRAY_TYPE (TableDirectory, tableDir, numTables);

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

  friend struct OpenTypeFontFile;

  private:
  /* OpenTypeFontFaces, in no particular order */
  DEFINE_OFFSET_ARRAY_TYPE (OffsetTable, offsetTable, numFonts);
  /* XXX check version here? */

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

  static const hb_tag_t TrueTypeTag	= HB_TAG ( 0 , 1 , 0 , 0 );
  static const hb_tag_t CFFTag		= HB_TAG ('O','T','T','O');
  static const hb_tag_t TTCTag		= HB_TAG ('t','t','c','f');

  STATIC_DEFINE_GET_FOR_DATA (OpenTypeFontFile);

  DEFINE_ARRAY_INTERFACE (OpenTypeFontFace, face);	/* get_face_count(), get_face(i) */

  inline const Tag& get_tag (void) const { return tag; }

  /* This is how you get a table */
  inline const char* get_table_data (const OpenTypeTable& table) const {
    return (*this)[table];
  }
  inline char* get_table_data (const OpenTypeTable& table) {
    return (*this)[table];
  }

  private:
  inline const char* operator[] (const OpenTypeTable& table) const {
    if (G_UNLIKELY (table.offset == 0)) return NULL;
    return ((const char*)this) + table.offset;
  }
  inline char* operator[] (const OpenTypeTable& table) {
    if (G_UNLIKELY (table.offset == 0)) return NULL;
    return ((char*)this) + table.offset;
  }

  unsigned int get_len (void) const {
    switch (tag) {
    default: return 0;
    case TrueTypeTag: case CFFTag: return 1;
    case TTCTag: return ((const TTCHeader&)*this).get_len();
    }
  }
  const OpenTypeFontFace& operator[] (unsigned int i) const {
    if (HB_UNLIKELY (i >= get_len ())) return Null(OpenTypeFontFace);
    switch (tag) {
    default: case TrueTypeTag: case CFFTag: return (const OffsetTable&)*this;
    case TTCTag: return ((const TTCHeader&)*this)[i];
    }
  }

  private:
  Tag		tag;		/* 4-byte identifier. */
};
ASSERT_SIZE (OpenTypeFontFile, 4);


#endif /* HB_OT_LAYOUT_OPEN_PRIVATE_H */
