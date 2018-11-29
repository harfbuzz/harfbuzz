/*
 * Copyright © 2007,2008,2009,2010  Red Hat, Inc.
 * Copyright © 2012  Google, Inc.
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
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_OPEN_TYPE_HH
#define HB_OPEN_TYPE_HH

#include "hb.hh"
#include "hb-blob.hh"
#include "hb-face.hh"
#include "hb-machinery.hh"
#include "hb-subset.hh"


namespace OT {


/*
 *
 * The OpenType Font File: Data Types
 */


/* "The following data types are used in the OpenType font file.
 *  All OpenType fonts use Motorola-style byte ordering (Big Endian):" */

/*
 * Int types
 */

/* Integer types in big-endian order and no alignment requirement */
template <typename Type, unsigned int Size>
struct IntType
{
  typedef Type type;
  inline void set (Type i) { v.set (i); }
  inline operator Type (void) const { return v; }
  inline bool operator == (const IntType<Type,Size> &o) const { return (Type) v == (Type) o.v; }
  inline bool operator != (const IntType<Type,Size> &o) const { return !(*this == o); }
  static inline int cmp (const IntType<Type,Size> *a, const IntType<Type,Size> *b) { return b->cmp (*a); }
  template <typename Type2>
  inline int cmp (Type2 a) const
  {
    Type b = v;
    if (sizeof (Type) < sizeof (int) && sizeof (Type2) < sizeof (int))
      return (int) a - (int) b;
    else
      return a < b ? -1 : a == b ? 0 : +1;
  }
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this)));
  }
  protected:
  BEInt<Type, Size> v;
  public:
  DEFINE_SIZE_STATIC (Size);
};

typedef IntType<uint8_t,  1> HBUINT8;	/* 8-bit unsigned integer. */
typedef IntType<int8_t,   1> HBINT8;	/* 8-bit signed integer. */
typedef IntType<uint16_t, 2> HBUINT16;	/* 16-bit unsigned integer. */
typedef IntType<int16_t,  2> HBINT16;	/* 16-bit signed integer. */
typedef IntType<uint32_t, 4> HBUINT32;	/* 32-bit unsigned integer. */
typedef IntType<int32_t,  4> HBINT32;	/* 32-bit signed integer. */
typedef IntType<uint32_t, 3> HBUINT24;	/* 24-bit unsigned integer. */

/* 16-bit signed integer (HBINT16) that describes a quantity in FUnits. */
typedef HBINT16 FWORD;

/* 32-bit signed integer (HBINT32) that describes a quantity in FUnits. */
typedef HBINT32 FWORD32;

/* 16-bit unsigned integer (HBUINT16) that describes a quantity in FUnits. */
typedef HBUINT16 UFWORD;

/* 16-bit signed fixed number with the low 14 bits of fraction (2.14). */
struct F2DOT14 : HBINT16
{
  // 16384 means 1<<14
  inline float to_float (void) const { return ((int32_t) v) / 16384.f; }
  inline void set_float (float f) { v.set (round (f * 16384.f)); }
  public:
  DEFINE_SIZE_STATIC (2);
};

/* 32-bit signed fixed-point number (16.16). */
struct Fixed : HBINT32
{
  // 65536 means 1<<16
  inline float to_float (void) const { return ((int32_t) v) / 65536.f; }
  inline void set_float (float f) { v.set (round (f * 65536.f)); }
  public:
  DEFINE_SIZE_STATIC (4);
};

/* Date represented in number of seconds since 12:00 midnight, January 1,
 * 1904. The value is represented as a signed 64-bit integer. */
struct LONGDATETIME
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this)));
  }
  protected:
  HBINT32 major;
  HBUINT32 minor;
  public:
  DEFINE_SIZE_STATIC (8);
};

/* Array of four uint8s (length = 32 bits) used to identify a script, language
 * system, feature, or baseline */
struct Tag : HBUINT32
{
  /* What the char* converters return is NOT nul-terminated.  Print using "%.4s" */
  inline operator const char* (void) const { return reinterpret_cast<const char *> (&this->v); }
  inline operator char* (void) { return reinterpret_cast<char *> (&this->v); }
  public:
  DEFINE_SIZE_STATIC (4);
};

/* Glyph index number, same as uint16 (length = 16 bits) */
typedef HBUINT16 GlyphID;

/* Script/language-system/feature index */
struct Index : HBUINT16 {
  enum { NOT_FOUND_INDEX = 0xFFFFu };
};
DECLARE_NULL_NAMESPACE_BYTES (OT, Index);

typedef Index NameID;

/* Offset, Null offset = 0 */
template <typename Type, bool has_null=true>
struct Offset : Type
{
  typedef Type type;

  inline bool is_null (void) const { return has_null && 0 == *this; }

  inline void *serialize (hb_serialize_context_t *c, const void *base)
  {
    void *t = c->start_embed<void> ();
    this->set ((char *) t - (char *) base); /* TODO(serialize) Overflow? */
    return t;
  }

  public:
  DEFINE_SIZE_STATIC (sizeof (Type));
};

typedef Offset<HBUINT16> Offset16;
typedef Offset<HBUINT32> Offset32;


/* CheckSum */
struct CheckSum : HBUINT32
{
  /* This is reference implementation from the spec. */
  static inline uint32_t CalcTableChecksum (const HBUINT32 *Table, uint32_t Length)
  {
    uint32_t Sum = 0L;
    assert (0 == (Length & 3));
    const HBUINT32 *EndPtr = Table + Length / HBUINT32::static_size;

    while (Table < EndPtr)
      Sum += *Table++;
    return Sum;
  }

  /* Note: data should be 4byte aligned and have 4byte padding at the end. */
  inline void set_for_data (const void *data, unsigned int length)
  { set (CalcTableChecksum ((const HBUINT32 *) data, length)); }

  public:
  DEFINE_SIZE_STATIC (4);
};


/*
 * Version Numbers
 */

template <typename FixedType=HBUINT16>
struct FixedVersion
{
  inline uint32_t to_int (void) const { return (major << (sizeof (FixedType) * 8)) + minor; }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  FixedType major;
  FixedType minor;
  public:
  DEFINE_SIZE_STATIC (2 * sizeof (FixedType));
};


/*
 * Template subclasses of Offset that do the dereferencing.
 * Use: (base+offset)
 */

template <typename Type, bool has_null>
struct _hb_has_null
{
  static inline const Type *get_null (void) { return nullptr; }
  static inline Type *get_crap (void) { return nullptr; }
};
template <typename Type>
struct _hb_has_null<Type, true>
{
  static inline const Type *get_null (void) { return &Null(Type); }
  static inline Type *get_crap (void) { return &Crap(Type); }
};

template <typename Type, typename OffsetType=HBUINT16, bool has_null=true>
struct OffsetTo : Offset<OffsetType, has_null>
{
  inline const Type& operator () (const void *base) const
  {
    if (unlikely (this->is_null ())) return *_hb_has_null<Type, has_null>::get_null ();
    return StructAtOffset<const Type> (base, *this);
  }
  inline Type& operator () (void *base) const
  {
    if (unlikely (this->is_null ())) return Crap (Type);
    if (unlikely (this->is_null ())) return *_hb_has_null<Type, has_null>::get_crap ();
    return StructAtOffset<Type> (base, *this);
  }

  inline Type& serialize (hb_serialize_context_t *c, const void *base)
  {
    return * (Type *) Offset<OffsetType>::serialize (c, base);
  }

  template <typename T>
  inline void serialize_subset (hb_subset_context_t *c, const T &src, const void *base)
  {
    if (&src == &Null (T))
    {
      this->set (0);
      return;
    }
    serialize (c->serializer, base);
    if (!src.subset (c))
      this->set (0);
  }

  inline bool sanitize_shallow (hb_sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!c->check_struct (this))) return_trace (false);
    if (unlikely (this->is_null ())) return_trace (true);
    if (unlikely (!c->check_range (base, *this))) return_trace (false);
    return_trace (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    return_trace (sanitize_shallow (c, base) &&
		  (this->is_null () ||
		   StructAtOffset<Type> (base, *this).sanitize (c) ||
		   neuter (c)));
  }
  template <typename T1>
  inline bool sanitize (hb_sanitize_context_t *c, const void *base, T1 d1) const
  {
    TRACE_SANITIZE (this);
    return_trace (sanitize_shallow (c, base) &&
		  (this->is_null () ||
		   StructAtOffset<Type> (base, *this).sanitize (c, d1) ||
		   neuter (c)));
  }
  template <typename T1, typename T2>
  inline bool sanitize (hb_sanitize_context_t *c, const void *base, T1 d1, T2 d2) const
  {
    TRACE_SANITIZE (this);
    return_trace (sanitize_shallow (c, base) &&
		  (this->is_null () ||
		   StructAtOffset<Type> (base, *this).sanitize (c, d1, d2) ||
		   neuter (c)));
  }
  template <typename T1, typename T2, typename T3>
  inline bool sanitize (hb_sanitize_context_t *c, const void *base, T1 d1, T2 d2, T3 d3) const
  {
    TRACE_SANITIZE (this);
    return_trace (sanitize_shallow (c, base) &&
		  (this->is_null () ||
		   StructAtOffset<Type> (base, *this).sanitize (c, d1, d2, d3) ||
		   neuter (c)));
  }

  /* Set the offset to Null */
  inline bool neuter (hb_sanitize_context_t *c) const
  {
    if (!has_null) return false;
    return c->try_set (this, 0);
  }
  DEFINE_SIZE_STATIC (sizeof (OffsetType));
};
template <typename Type, bool has_null=true> struct LOffsetTo : OffsetTo<Type, HBUINT32, has_null> {};

template <typename Base, typename OffsetType, bool has_null, typename Type>
static inline const Type& operator + (const Base &base, const OffsetTo<Type, OffsetType, has_null> &offset) { return offset (base); }
template <typename Base, typename OffsetType, bool has_null, typename Type>
static inline Type& operator + (Base &base, OffsetTo<Type, OffsetType, has_null> &offset) { return offset (base); }


/*
 * Array Types
 */

template <typename Type>
struct UnsizedArrayOf
{
  static_assert ((bool) (unsigned) hb_static_size (Type), "");

  enum { item_size = Type::static_size };

  HB_NO_CREATE_COPY_ASSIGN_TEMPLATE (UnsizedArrayOf, Type);

  inline const Type& operator [] (unsigned int i) const
  {
    const Type *p = &arrayZ[i];
    if (unlikely (p < arrayZ)) return Null (Type); /* Overflowed. */
    return *p;
  }
  inline Type& operator [] (unsigned int i)
  {
    Type *p = &arrayZ[i];
    if (unlikely (p < arrayZ)) return Crap (Type); /* Overflowed. */
    return *p;
  }

  template <typename T> inline operator T * (void) { return arrayZ; }
  template <typename T> inline operator const T * (void) const { return arrayZ; }

  inline unsigned int get_size (unsigned int len) const
  { return len * Type::static_size; }

  inline hb_array_t<Type> as_array (unsigned int len)
  { return hb_array (arrayZ, len); }
  inline hb_array_t<const Type> as_array (unsigned int len) const
  { return hb_array (arrayZ, len); }

  template <typename T>
  inline Type &lsearch (unsigned int len, const T &x, Type &not_found = Crap (Type))
  { return *as_array (len).lsearch (x, &not_found); }
  template <typename T>
  inline const Type &lsearch (unsigned int len, const T &x, const Type &not_found = Null (Type)) const
  { return *as_array (len).lsearch (x, &not_found); }

  inline void qsort (unsigned int len, unsigned int start = 0, unsigned int end = (unsigned int) -1)
  { as_array (len).qsort (start, end); }

  inline bool sanitize (hb_sanitize_context_t *c, unsigned int count) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!sanitize_shallow (c, count))) return_trace (false);

    /* Note: for structs that do not reference other structs,
     * we do not need to call their sanitize() as we already did
     * a bound check on the aggregate array size.  We just include
     * a small unreachable expression to make sure the structs
     * pointed to do have a simple sanitize(), ie. they do not
     * reference other structs via offsets.
     */
    (void) (false && arrayZ[0].sanitize (c));

    return_trace (true);
  }
  inline bool sanitize (hb_sanitize_context_t *c, unsigned int count, const void *base) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!sanitize_shallow (c, count))) return_trace (false);
    for (unsigned int i = 0; i < count; i++)
      if (unlikely (!arrayZ[i].sanitize (c, base)))
	return_trace (false);
    return_trace (true);
  }
  template <typename T>
  inline bool sanitize (hb_sanitize_context_t *c, unsigned int count, const void *base, T user_data) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!sanitize_shallow (c, count))) return_trace (false);
    for (unsigned int i = 0; i < count; i++)
      if (unlikely (!arrayZ[i].sanitize (c, base, user_data)))
	return_trace (false);
    return_trace (true);
  }

  inline bool sanitize_shallow (hb_sanitize_context_t *c, unsigned int count) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_array (arrayZ, count));
  }

  public:
  Type		arrayZ[VAR];
  public:
  DEFINE_SIZE_UNBOUNDED (0);
};

/* Unsized array of offset's */
template <typename Type, typename OffsetType, bool has_null=true>
struct UnsizedOffsetArrayOf : UnsizedArrayOf<OffsetTo<Type, OffsetType, has_null> > {};

/* Unsized array of offsets relative to the beginning of the array itself. */
template <typename Type, typename OffsetType, bool has_null=true>
struct UnsizedOffsetListOf : UnsizedOffsetArrayOf<Type, OffsetType, has_null>
{
  inline const Type& operator [] (unsigned int i) const
  {
    const OffsetTo<Type, OffsetType, has_null> *p = &this->arrayZ[i];
    if (unlikely (p < this->arrayZ)) return Null (Type); /* Overflowed. */
    return this+*p;
  }
  inline Type& operator [] (unsigned int i)
  {
    const OffsetTo<Type, OffsetType, has_null> *p = &this->arrayZ[i];
    if (unlikely (p < this->arrayZ)) return Crap (Type); /* Overflowed. */
    return this+*p;
  }


  inline bool sanitize (hb_sanitize_context_t *c, unsigned int count) const
  {
    TRACE_SANITIZE (this);
    return_trace ((UnsizedOffsetArrayOf<Type, OffsetType, has_null>::sanitize (c, count, this)));
  }
  template <typename T>
  inline bool sanitize (hb_sanitize_context_t *c, unsigned int count, T user_data) const
  {
    TRACE_SANITIZE (this);
    return_trace ((UnsizedOffsetArrayOf<Type, OffsetType, has_null>::sanitize (c, count, this, user_data)));
  }
};

/* An array with sorted elements.  Supports binary searching. */
template <typename Type>
struct SortedUnsizedArrayOf : UnsizedArrayOf<Type>
{
  inline hb_sorted_array_t<Type> as_array (unsigned int len)
  { return hb_sorted_array (this->arrayZ, len); }
  inline hb_sorted_array_t<const Type> as_array (unsigned int len) const
  { return hb_sorted_array (this->arrayZ, len); }

  template <typename T>
  inline Type &bsearch (unsigned int len, const T &x, Type &not_found = Crap (Type))
  { return *as_array (len).bsearch (x, &not_found); }
  template <typename T>
  inline const Type &bsearch (unsigned int len, const T &x, const Type &not_found = Null (Type)) const
  { return *as_array (len).bsearch (x, &not_found); }
  template <typename T>
  inline bool bfind (unsigned int len, const T &x, unsigned int *i = nullptr,
		     hb_bfind_not_found_t not_found = HB_BFIND_NOT_FOUND_DONT_STORE,
		     unsigned int to_store = (unsigned int) -1) const
  { return as_array (len).bfind (x, i, not_found, to_store); }
};


/* An array with a number of elements. */
template <typename Type, typename LenType=HBUINT16>
struct ArrayOf
{
  static_assert ((bool) (unsigned) hb_static_size (Type), "");

  enum { item_size = Type::static_size };

  HB_NO_CREATE_COPY_ASSIGN_TEMPLATE2 (ArrayOf, Type, LenType);

  inline const Type& operator [] (unsigned int i) const
  {
    if (unlikely (i >= len)) return Null (Type);
    return arrayZ[i];
  }
  inline Type& operator [] (unsigned int i)
  {
    if (unlikely (i >= len)) return Crap (Type);
    return arrayZ[i];
  }

  inline unsigned int get_size (void) const
  { return len.static_size + len * Type::static_size; }

  inline hb_array_t<Type> as_array (void)
  { return hb_array (arrayZ, len); }
  inline hb_array_t<const Type> as_array (void) const
  { return hb_array (arrayZ, len); }

  inline hb_array_t<const Type> sub_array (unsigned int start_offset, unsigned int count) const
  { return as_array ().sub_array (start_offset, count);}
  inline hb_array_t<const Type> sub_array (unsigned int start_offset, unsigned int *count /* IN/OUT */) const
  { return as_array ().sub_array (start_offset, count);}
  inline hb_array_t<Type> sub_array (unsigned int start_offset, unsigned int count)
  { return as_array ().sub_array (start_offset, count);}
  inline hb_array_t<Type> sub_array (unsigned int start_offset, unsigned int *count /* IN/OUT */)
  { return as_array ().sub_array (start_offset, count);}

  inline bool serialize (hb_serialize_context_t *c,
			 unsigned int items_len)
  {
    TRACE_SERIALIZE (this);
    if (unlikely (!c->extend_min (*this))) return_trace (false);
    len.set (items_len); /* TODO(serialize) Overflow? */
    if (unlikely (!c->extend (*this))) return_trace (false);
    return_trace (true);
  }
  inline bool serialize (hb_serialize_context_t *c,
			 Supplier<Type> &items,
			 unsigned int items_len)
  {
    TRACE_SERIALIZE (this);
    if (unlikely (!serialize (c, items_len))) return_trace (false);
    for (unsigned int i = 0; i < items_len; i++)
      arrayZ[i] = items[i];
    items += items_len;
    return_trace (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!sanitize_shallow (c))) return_trace (false);

    /* Note: for structs that do not reference other structs,
     * we do not need to call their sanitize() as we already did
     * a bound check on the aggregate array size.  We just include
     * a small unreachable expression to make sure the structs
     * pointed to do have a simple sanitize(), ie. they do not
     * reference other structs via offsets.
     */
    (void) (false && arrayZ[0].sanitize (c));

    return_trace (true);
  }
  inline bool sanitize (hb_sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!sanitize_shallow (c))) return_trace (false);
    unsigned int count = len;
    for (unsigned int i = 0; i < count; i++)
      if (unlikely (!arrayZ[i].sanitize (c, base)))
	return_trace (false);
    return_trace (true);
  }
  template <typename T>
  inline bool sanitize (hb_sanitize_context_t *c, const void *base, T user_data) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!sanitize_shallow (c))) return_trace (false);
    unsigned int count = len;
    for (unsigned int i = 0; i < count; i++)
      if (unlikely (!arrayZ[i].sanitize (c, base, user_data)))
	return_trace (false);
    return_trace (true);
  }

  template <typename T>
  inline Type &lsearch (const T &x, Type &not_found = Crap (Type))
  { return *as_array ().lsearch (x, &not_found); }
  template <typename T>
  inline const Type &lsearch (const T &x, const Type &not_found = Null (Type)) const
  { return *as_array ().lsearch (x, &not_found); }

  inline void qsort (unsigned int start = 0, unsigned int end = (unsigned int) -1)
  { as_array ().qsort (start, end); }

  inline bool sanitize_shallow (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (len.sanitize (c) && c->check_array (arrayZ, len));
  }

  public:
  LenType	len;
  Type		arrayZ[VAR];
  public:
  DEFINE_SIZE_ARRAY (sizeof (LenType), arrayZ);
};
template <typename Type> struct LArrayOf : ArrayOf<Type, HBUINT32> {};
typedef ArrayOf<HBUINT8, HBUINT8> PString;

/* Array of Offset's */
template <typename Type>
struct OffsetArrayOf : ArrayOf<OffsetTo<Type, HBUINT16> > {};
template <typename Type>
struct LOffsetArrayOf : ArrayOf<OffsetTo<Type, HBUINT32> > {};
template <typename Type>
struct LOffsetLArrayOf : ArrayOf<OffsetTo<Type, HBUINT32>, HBUINT32> {};

/* Array of offsets relative to the beginning of the array itself. */
template <typename Type>
struct OffsetListOf : OffsetArrayOf<Type>
{
  inline const Type& operator [] (unsigned int i) const
  {
    if (unlikely (i >= this->len)) return Null (Type);
    return this+this->arrayZ[i];
  }
  inline const Type& operator [] (unsigned int i)
  {
    if (unlikely (i >= this->len)) return Crap (Type);
    return this+this->arrayZ[i];
  }

  inline bool subset (hb_subset_context_t *c) const
  {
    TRACE_SUBSET (this);
    struct OffsetListOf<Type> *out = c->serializer->embed (*this);
    if (unlikely (!out)) return_trace (false);
    unsigned int count = this->len;
    for (unsigned int i = 0; i < count; i++)
      out->arrayZ[i].serialize_subset (c, (*this)[i], out);
    return_trace (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (OffsetArrayOf<Type>::sanitize (c, this));
  }
  template <typename T>
  inline bool sanitize (hb_sanitize_context_t *c, T user_data) const
  {
    TRACE_SANITIZE (this);
    return_trace (OffsetArrayOf<Type>::sanitize (c, this, user_data));
  }
};

/* An array starting at second element. */
template <typename Type, typename LenType=HBUINT16>
struct HeadlessArrayOf
{
  enum { item_size = Type::static_size };

  HB_NO_CREATE_COPY_ASSIGN_TEMPLATE2 (HeadlessArrayOf, Type, LenType);

  inline const Type& operator [] (unsigned int i) const
  {
    if (unlikely (i >= lenP1 || !i)) return Null (Type);
    return arrayZ[i-1];
  }
  inline Type& operator [] (unsigned int i)
  {
    if (unlikely (i >= lenP1 || !i)) return Crap (Type);
    return arrayZ[i-1];
  }
  inline unsigned int get_size (void) const
  { return lenP1.static_size + (lenP1 ? lenP1 - 1 : 0) * Type::static_size; }

  inline bool serialize (hb_serialize_context_t *c,
			 Supplier<Type> &items,
			 unsigned int items_len)
  {
    TRACE_SERIALIZE (this);
    if (unlikely (!c->extend_min (*this))) return_trace (false);
    lenP1.set (items_len); /* TODO(serialize) Overflow? */
    if (unlikely (!items_len)) return_trace (true);
    if (unlikely (!c->extend (*this))) return_trace (false);
    for (unsigned int i = 0; i < items_len - 1; i++)
      arrayZ[i] = items[i];
    items += items_len - 1;
    return_trace (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!sanitize_shallow (c))) return_trace (false);

    /* Note: for structs that do not reference other structs,
     * we do not need to call their sanitize() as we already did
     * a bound check on the aggregate array size.  We just include
     * a small unreachable expression to make sure the structs
     * pointed to do have a simple sanitize(), ie. they do not
     * reference other structs via offsets.
     */
    (void) (false && arrayZ[0].sanitize (c));

    return_trace (true);
  }

  private:
  inline bool sanitize_shallow (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (lenP1.sanitize (c) &&
		  (!lenP1 || c->check_array (arrayZ, lenP1 - 1)));
  }

  public:
  LenType	lenP1;
  Type		arrayZ[VAR];
  public:
  DEFINE_SIZE_ARRAY (sizeof (LenType), arrayZ);
};

/* An array storing length-1. */
template <typename Type, typename LenType=HBUINT16>
struct ArrayOfM1
{
  HB_NO_CREATE_COPY_ASSIGN_TEMPLATE2 (ArrayOfM1, Type, LenType);

  inline const Type& operator [] (unsigned int i) const
  {
    if (unlikely (i > lenM1)) return Null (Type);
    return arrayZ[i];
  }
  inline Type& operator [] (unsigned int i)
  {
    if (unlikely (i > lenM1)) return Crap (Type);
    return arrayZ[i];
  }
  inline unsigned int get_size (void) const
  { return lenM1.static_size + (lenM1 + 1) * Type::static_size; }

  template <typename T>
  inline bool sanitize (hb_sanitize_context_t *c, const void *base, T user_data) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!sanitize_shallow (c))) return_trace (false);
    unsigned int count = lenM1 + 1;
    for (unsigned int i = 0; i < count; i++)
      if (unlikely (!arrayZ[i].sanitize (c, base, user_data)))
	return_trace (false);
    return_trace (true);
  }

  private:
  inline bool sanitize_shallow (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (lenM1.sanitize (c) &&
		  (c->check_array (arrayZ, lenM1 + 1)));
  }

  public:
  LenType	lenM1;
  Type		arrayZ[VAR];
  public:
  DEFINE_SIZE_ARRAY (sizeof (LenType), arrayZ);
};

/* An array with sorted elements.  Supports binary searching. */
template <typename Type, typename LenType=HBUINT16>
struct SortedArrayOf : ArrayOf<Type, LenType>
{
  inline hb_sorted_array_t<Type> as_array (void)
  { return hb_sorted_array (this->arrayZ, this->len); }
  inline hb_sorted_array_t<const Type> as_array (void) const
  { return hb_sorted_array (this->arrayZ, this->len); }

  inline hb_array_t<const Type> sub_array (unsigned int start_offset, unsigned int count) const
  { return as_array ().sub_array (start_offset, count);}
  inline hb_array_t<const Type> sub_array (unsigned int start_offset, unsigned int *count /* IN/OUT */) const
  { return as_array ().sub_array (start_offset, count);}
  inline hb_array_t<Type> sub_array (unsigned int start_offset, unsigned int count)
  { return as_array ().sub_array (start_offset, count);}
  inline hb_array_t<Type> sub_array (unsigned int start_offset, unsigned int *count /* IN/OUT */)
  { return as_array ().sub_array (start_offset, count);}

  template <typename T>
  inline Type &bsearch (const T &x, Type &not_found = Crap (Type))
  { return *as_array ().bsearch (x, &not_found); }
  template <typename T>
  inline const Type &bsearch (const T &x, const Type &not_found = Null (Type)) const
  { return *as_array ().bsearch (x, &not_found); }
  template <typename T>
  inline bool bfind (const T &x, unsigned int *i = nullptr,
		     hb_bfind_not_found_t not_found = HB_BFIND_NOT_FOUND_DONT_STORE,
		     unsigned int to_store = (unsigned int) -1) const
  { return as_array ().bfind (x, i, not_found, to_store); }
};

/*
 * Binary-search arrays
 */

template <typename LenType=HBUINT16>
struct BinSearchHeader
{
  inline operator uint32_t (void) const { return len; }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  inline void set (unsigned int v)
  {
    len.set (v);
    assert (len == v);
    entrySelector.set (MAX (1u, hb_bit_storage (v)) - 1);
    searchRange.set (16 * (1u << entrySelector));
    rangeShift.set (v * 16 > searchRange
		    ? 16 * v - searchRange
		    : 0);
  }

  protected:
  LenType	len;
  LenType	searchRange;
  LenType	entrySelector;
  LenType	rangeShift;

  public:
  DEFINE_SIZE_STATIC (8);
};

template <typename Type, typename LenType=HBUINT16>
struct BinSearchArrayOf : SortedArrayOf<Type, BinSearchHeader<LenType> > {};


struct VarSizedBinSearchHeader
{

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  HBUINT16	unitSize;	/* Size of a lookup unit for this search in bytes. */
  HBUINT16	nUnits;		/* Number of units of the preceding size to be searched. */
  HBUINT16	searchRange;	/* The value of unitSize times the largest power of 2
				 * that is less than or equal to the value of nUnits. */
  HBUINT16	entrySelector;	/* The log base 2 of the largest power of 2 less than
				 * or equal to the value of nUnits. */
  HBUINT16	rangeShift;	/* The value of unitSize times the difference of the
				 * value of nUnits minus the largest power of 2 less
				 * than or equal to the value of nUnits. */
  public:
  DEFINE_SIZE_STATIC (10);
};

template <typename Type>
struct VarSizedBinSearchArrayOf
{
  enum { item_size = Type::static_size };

  HB_NO_CREATE_COPY_ASSIGN_TEMPLATE (VarSizedBinSearchArrayOf, Type);

  inline bool last_is_terminator (void) const
  {
    if (unlikely (!header.nUnits)) return false;

    /* Gah.
     *
     * "The number of termination values that need to be included is table-specific.
     * The value that indicates binary search termination is 0xFFFF." */
    const HBUINT16 *words = &StructAtOffset<HBUINT16> (&bytesZ, (header.nUnits - 1) * header.unitSize);
    unsigned int count = Type::TerminationWordCount;
    for (unsigned int i = 0; i < count; i++)
      if (words[i] != 0xFFFFu)
        return false;
    return true;
  }

  inline const Type& operator [] (unsigned int i) const
  {
    if (unlikely (i >= get_length ())) return Null (Type);
    return StructAtOffset<Type> (&bytesZ, i * header.unitSize);
  }
  inline Type& operator [] (unsigned int i)
  {
    if (unlikely (i >= get_length ())) return Crap (Type);
    return StructAtOffset<Type> (&bytesZ, i * header.unitSize);
  }
  inline unsigned int get_length (void) const
  {
    return header.nUnits - last_is_terminator ();
  }
  inline unsigned int get_size (void) const
  { return header.static_size + header.nUnits * header.unitSize; }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!sanitize_shallow (c))) return_trace (false);

    /* Note: for structs that do not reference other structs,
     * we do not need to call their sanitize() as we already did
     * a bound check on the aggregate array size.  We just include
     * a small unreachable expression to make sure the structs
     * pointed to do have a simple sanitize(), ie. they do not
     * reference other structs via offsets.
     */
    (void) (false && StructAtOffset<Type> (&bytesZ, 0).sanitize (c));

    return_trace (true);
  }
  inline bool sanitize (hb_sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!sanitize_shallow (c))) return_trace (false);
    unsigned int count = get_length ();
    for (unsigned int i = 0; i < count; i++)
      if (unlikely (!(*this)[i].sanitize (c, base)))
	return_trace (false);
    return_trace (true);
  }
  template <typename T>
  inline bool sanitize (hb_sanitize_context_t *c, const void *base, T user_data) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!sanitize_shallow (c))) return_trace (false);
    unsigned int count = get_length ();
    for (unsigned int i = 0; i < count; i++)
      if (unlikely (!(*this)[i].sanitize (c, base, user_data)))
	return_trace (false);
    return_trace (true);
  }

  template <typename T>
  inline const Type *bsearch (const T &key) const
  {
    unsigned int size = header.unitSize;
    int min = 0, max = (int) get_length () - 1;
    while (min <= max)
    {
      int mid = ((unsigned int) min + (unsigned int) max) / 2;
      const Type *p = (const Type *) (((const char *) &bytesZ) + (mid * size));
      int c = p->cmp (key);
      if (c < 0) max = mid - 1;
      else if (c > 0) min = mid + 1;
      else return p;
    }
    return nullptr;
  }

  private:
  inline bool sanitize_shallow (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (header.sanitize (c) &&
		  Type::static_size <= header.unitSize &&
		  c->check_range (bytesZ.arrayZ,
				  header.nUnits,
				  header.unitSize));
  }

  protected:
  VarSizedBinSearchHeader	header;
  UnsizedArrayOf<HBUINT8>	bytesZ;
  public:
  DEFINE_SIZE_ARRAY (10, bytesZ);
};


} /* namespace OT */


#endif /* HB_OPEN_TYPE_HH */
