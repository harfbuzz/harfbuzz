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

#ifndef HB_OPEN_TYPES_PRIVATE_HH
#define HB_OPEN_TYPES_PRIVATE_HH

#include "hb-private.h"

#include "hb-blob.h"


#define NO_INDEX		((unsigned int) 0xFFFF)
#define NO_CONTEXT		((unsigned int) 0x110000)
#define NOT_COVERED		((unsigned int) 0x110000)
#define MAX_NESTING_LEVEL	8



/*
 * Sanitize
 */

typedef struct _hb_sanitize_context_t hb_sanitize_context_t;
struct _hb_sanitize_context_t
{
  const char *start, *end;
  hb_blob_t *blob;
};

#define SANITIZE_ARG_DEF \
	hb_sanitize_context_t *context
#define SANITIZE_ARG \
	context

#define SANITIZE(X) HB_LIKELY ((X).sanitize (SANITIZE_ARG))
#define SANITIZE2(X,Y) SANITIZE (X) && SANITIZE (Y)

#define SANITIZE_THIS(X) HB_LIKELY ((X).sanitize (SANITIZE_ARG, (const char *) this))
#define SANITIZE_THIS2(X,Y) SANITIZE_THIS (X) && SANITIZE_THIS (Y)
#define SANITIZE_THIS3(X,Y,Z) SANITIZE_THIS (X) && SANITIZE_THIS (Y) && SANITIZE_THIS(Z)

#define SANITIZE_SELF() SANITIZE_OBJ (*this)
#define SANITIZE_OBJ(X) SANITIZE_MEM(&(X), sizeof (X))
#define SANITIZE_GET_SIZE() SANITIZE_MEM (this, this->get_size ())

#define SANITIZE_MEM(B,L) HB_LIKELY (context->start <= (const char *)(B) && (const char *)(B) + (L) <= context->end) /* XXX overflow */

#define NEUTER(Var, Val) (false)


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
  inline const Type& operator[] (unsigned int i) const \
  { \
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
  inline const Type& operator[] (unsigned int i) const \
  { \
    if (HB_UNLIKELY (i >= num)) return Null(Type); \
    if (HB_UNLIKELY (!array[i])) return Null(Type); \
    return *(const Type)((const char*)this + array[i]); \
  }


#define DEFINE_ARRAY_INTERFACE(Type, name) \
  inline const Type& get_##name (unsigned int i) const { return (*this)[i]; } \
  inline unsigned int get_##name##_count (void) const { return this->get_len (); }
#define DEFINE_INDEX_ARRAY_INTERFACE(name) \
  inline unsigned int get_##name##_index (unsigned int i) const \
  { \
    if (HB_UNLIKELY (i >= get_len ())) return NO_INDEX; \
    return (*this)[i]; \
  } \
  inline unsigned int get_##name##_count (void) const { return get_len (); }


/*
 * List types
 */

#define DEFINE_LIST_INTERFACE(Type, name) \
  inline const Type& get_##name (unsigned int i) const { return (this+name##List)[i]; } \
  inline unsigned int get_##name##_count (void) const { return (this+name##List).len; }

/*
 * Tag types
 */

#define DEFINE_TAG_ARRAY_INTERFACE(Type, name) \
  DEFINE_ARRAY_INTERFACE (Type, name); \
  inline const Tag& get_##name##_tag (unsigned int i) const { return (*this)[i].tag; }
#define DEFINE_TAG_LIST_INTERFACE(Type, name) \
  DEFINE_LIST_INTERFACE (Type, name); \
  inline const Tag& get_##name##_tag (unsigned int i) const { return (this+name##List).get_tag (i); }

#define DEFINE_TAG_FIND_INTERFACE(Type, name) \
  inline bool find_##name##_index (hb_tag_t tag, unsigned int *index) const { \
    const Tag t = tag; \
    for (unsigned int i = 0; i < get_##name##_count (); i++) \
    { \
      if (t == get_##name##_tag (i)) \
      { \
        if (index) *index = i; \
        return true; \
      } \
    } \
    if (index) *index = NO_INDEX; \
    return false; \
  } \
  inline const Type& get_##name##_by_tag (hb_tag_t tag) const \
  { \
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
struct Null
{
  ASSERT_STATIC (sizeof (Type) <= sizeof (NullPool));
  static inline const Type &get () { return *(const Type*)NullPool; }
};

/* Specializaiton for arbitrary-content arbitrary-sized Null objects. */
#define DEFINE_NULL_DATA(Type, size, data) \
static const char _Null##Type[size] = data; \
template <> \
struct Null <Type> \
{ \
  static inline const Type &get () { return *(const Type*)_Null##Type; } \
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
  static inline const Type& get_for_data (const char *data) \
  { \
    if (HB_UNLIKELY (data == NULL)) return Null(Type); \
    return *(const Type*)data; \
  }
/* Like get_for_data(), but checks major version first. */
#define STATIC_DEFINE_GET_FOR_DATA_CHECK_MAJOR_VERSION(Type, MajorMin, MajorMax) \
  static inline const Type& get_for_data (const char *data) \
  { \
    if (HB_UNLIKELY (data == NULL)) return Null(Type); \
    const Type& t = *(const Type*)data; \
    if (HB_UNLIKELY (t.version.major < MajorMin || t.version.major > MajorMax)) return Null(Type); \
    return t; \
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

/* TODO On machines that do not allow unaligned access, fix the accessors. */
#define DEFINE_INT_TYPE1(NAME, TYPE, BIG_ENDIAN, BYTES) \
  struct NAME \
  { \
    inline NAME& operator = (TYPE i) { (TYPE&) v = BIG_ENDIAN (i); return *this; } \
    inline operator TYPE(void) const { return BIG_ENDIAN ((TYPE&) v); } \
    inline bool operator== (NAME o) const { return (TYPE&) v == (TYPE&) o.v; } \
    inline bool sanitize (SANITIZE_ARG_DEF) { return SANITIZE_SELF (); } \
    private: char v[BYTES]; \
  }; \
  ASSERT_SIZE (NAME, BYTES)
#define DEFINE_INT_TYPE0(NAME, type, b)	DEFINE_INT_TYPE1 (NAME, type##_t, hb_be_##type, b)
#define DEFINE_INT_TYPE(NAME, u, w)	DEFINE_INT_TYPE0 (NAME, u##int##w, (w / 8))


DEFINE_INT_TYPE (USHORT,  u, 16);	/* 16-bit unsigned integer. */
DEFINE_INT_TYPE (SHORT,	  , 16);	/* 16-bit signed integer. */
DEFINE_INT_TYPE (ULONG,	 u, 32);	/* 32-bit unsigned integer. */
DEFINE_INT_TYPE (LONG,	  , 32);	/* 32-bit signed integer. */

/* Array of four uint8s (length = 32 bits) used to identify a script, language
 * system, feature, or baseline */
struct Tag : ULONG
{
  inline Tag (const Tag &o) { *(ULONG*)this = (ULONG&) o; }
  inline Tag (uint32_t i) { *(ULONG*)this = i; }
  inline Tag (const char *c) { *(ULONG*)this = *(ULONG*)c; }
  inline bool operator== (const char *c) const { return *(ULONG*)this == *(ULONG*)c; }
  /* What the char* converters return is NOT nul-terminated.  Print using "%.4s" */
  inline operator const char* (void) const { return (const char *)this; }
  inline operator char* (void) { return (char *)this; }
};
ASSERT_SIZE (Tag, 4);
#define _NULL_TAG_INIT  {' ', ' ', ' ', ' '}
DEFINE_NULL_DATA (Tag, 4, _NULL_TAG_INIT);
#undef _NULL_TAG_INIT

/* Glyph index number, same as uint16 (length = 16 bits) */
typedef USHORT GlyphID;

/* Offset to a table, same as uint16 (length = 16 bits), Null offset = 0x0000 */
typedef USHORT Offset;

/* LongOffset to a table, same as uint32 (length = 32 bits), Null offset = 0x00000000 */
typedef ULONG LongOffset;

/* Template subclasses of Offset and LongOffset that do the dereferencing.  Use: (this+memberName) */

template <typename Type>
struct OffsetTo : Offset
{
  inline const Type& operator() (const void *base) const
  {
    unsigned int offset = *this;
    if (HB_UNLIKELY (!offset)) return Null(Type);
    return *(const Type*)((const char *) base + offset);
  }

  inline bool sanitize (SANITIZE_ARG_DEF, const void *base) {
    if (!SANITIZE_OBJ (*this)) return false;
    unsigned int offset = *this;
    if (HB_UNLIKELY (!offset)) return true;
    return SANITIZE (*(Type*)((char *) base + offset)) || NEUTER (*this, 0);
  }
};
template <typename Base, typename Type>
inline const Type& operator + (const Base &base, OffsetTo<Type> offset) { return offset (base); }

template <typename Type>
struct LongOffsetTo : LongOffset
{
  inline const Type& operator() (const void *base) const
  {
    unsigned int offset = *this;
    if (HB_UNLIKELY (!offset)) return Null(Type);
    return *(const Type*)((const char *) base + offset);
  }

  inline bool sanitize (SANITIZE_ARG_DEF, const void *base) {
    if (!SANITIZE (*this)) return false;
    unsigned int offset = *this;
    if (HB_UNLIKELY (!offset)) return true;
    return SANITIZE (*(Type*)((char *) base + offset)) || NEUTER (*this, 0);
  }
};
template <typename Base, typename Type>
inline const Type& operator + (const Base &base, LongOffsetTo<Type> offset) { return offset (base); }


/* CheckSum */
struct CheckSum : ULONG
{
  static uint32_t CalcTableChecksum (ULONG *Table, uint32_t Length)
  {
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

struct FixedVersion
{
  inline operator uint32_t (void) const { return (major << 16) + minor; }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    return SANITIZE_SELF ();
  }

  USHORT major;
  USHORT minor;
};
ASSERT_SIZE (FixedVersion, 4);

/*
 * Array Types
 */

/* An array with a USHORT number of elements. */
template <typename Type>
struct ArrayOf
{
  inline const Type& operator [] (unsigned int i) const
  {
    if (HB_UNLIKELY (i >= len)) return Null(Type);
    return array[i];
  }
  inline unsigned int get_size () const
  { return sizeof (len) + len * sizeof (array[0]); }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    if (!(SANITIZE (len) && SANITIZE_GET_SIZE())) return false;
    /* Note; for non-recursive types, this is not much needed
    unsigned int count = len;
    for (unsigned int i = 0; i < count; i++)
      if (!SANITIZE (array[i]))
        return false;
    */
  }

  USHORT len;
  Type array[];
};

/* An array with a USHORT number of elements,
 * starting at second element. */
template <typename Type>
struct HeadlessArrayOf
{
  inline const Type& operator [] (unsigned int i) const
  {
    if (HB_UNLIKELY (i >= len || !i)) return Null(Type);
    return array[i-1];
  }
  inline unsigned int get_size () const
  { return sizeof (len) + (len ? len - 1 : 0) * sizeof (array[0]); }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    if (!(SANITIZE_SELF () && SANITIZE_GET_SIZE())) return false;
    unsigned int count = len ? len - 1 : 0;
    /* Note; for non-recursive types, this is not much needed
    for (unsigned int i = 0; i < count; i++)
      if (!SANITIZE (array[i]))
        return false;
    */
  }

  USHORT len;
  Type array[];
};

/* An array with a ULONG number of elements. */
template <typename Type>
struct LongArrayOf
{
  inline const Type& operator [] (unsigned int i) const
  {
    if (HB_UNLIKELY (i >= len)) return Null(Type);
    return array[i];
  }
  inline unsigned int get_size () const
  { return sizeof (len) + len * sizeof (array[0]); }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    if (!(SANITIZE_SELF () && SANITIZE_GET_SIZE())) return false;
    unsigned int count = len;
    /* Note; for non-recursive types, this is not much needed
    for (unsigned int i = 0; i < count; i++)
      if (!SANITIZE (array[i]))
        return false;
    */
  }

  ULONG len;
  Type array[];
};

/* Array of Offset's */
template <typename Type>
struct OffsetArrayOf : ArrayOf<OffsetTo<Type> > {
  inline bool sanitize (SANITIZE_ARG_DEF, const char *base) {
    if (!(SANITIZE (this->len) && SANITIZE_GET_SIZE())) return false;
    unsigned int count = this->len;
    for (unsigned int i = 0; i < count; i++)
      if (!this->array[i].sanitize (SANITIZE_ARG, base))
        return false;
  }
};

/* Array of LongOffset's */
template <typename Type>
struct LongOffsetArrayOf : ArrayOf<LongOffsetTo<Type> > {
  inline bool sanitize (SANITIZE_ARG_DEF, const char *base) {
    if (!(SANITIZE (this->len) && SANITIZE_GET_SIZE())) return false;
    unsigned int count = this->len;
    for (unsigned int i = 0; i < count; i++)
      if (!this->array[i].sanitize (SANITIZE_ARG, base))
        return false;
  }
};

/* LongArray of LongOffset's */
template <typename Type>
struct LongOffsetLongArrayOf : LongArrayOf<LongOffsetTo<Type> > {
  inline bool sanitize (SANITIZE_ARG_DEF, const char *base) {
    if (!(SANITIZE (this->len) && SANITIZE_GET_SIZE())) return false;
    unsigned int count = this->len;
    for (unsigned int i = 0; i < count; i++)
      if (!this->array[i].sanitize (SANITIZE_ARG, base))
        return false;
  }
};

/* An array type is one that contains a variable number of objects
 * as its last item.  An array object is extended with get_len()
 * methods, as well as overloaded [] operator. */
#define DEFINE_ARRAY_TYPE(Type, array, num) \
  DEFINE_INDEX_OPERATOR(Type, array, num) \
  DEFINE_LEN(Type, array, num)
#define DEFINE_INDEX_OPERATOR(Type, array, num) \
  inline const Type& operator[] (unsigned int i) const \
  { \
    if (HB_UNLIKELY (i >= num)) return Null(Type); \
    return array[i]; \
  }


#endif /* HB_OPEN_TYPES_PRIVATE_HH */
