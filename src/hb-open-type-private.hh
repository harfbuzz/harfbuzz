/*
 * Copyright (C) 2007,2008,2009,2010  Red Hat, Inc.
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

#ifndef HB_OPEN_TYPES_PRIVATE_HH
#define HB_OPEN_TYPES_PRIVATE_HH

#include "hb-private.h"

#include "hb-blob.h"


/* Table/script/language-system/feature/... not found */
#define NO_INDEX		((unsigned int) 0xFFFF)



/*
 * Casts
 */

/* Cast to "const char *" and "char *" */
template <typename Type>
inline const char * CharP (const Type* X)
{ return reinterpret_cast<const char *>(X); }
template <typename Type>
inline char * CharP (Type* X)
{ return reinterpret_cast<char *>(X); }

/* Cast to struct T, reference to reference */
template<typename Type, typename TObject>
inline const Type& CastR(const TObject &X)
{ return reinterpret_cast<const Type&> (X); }
template<typename Type, typename TObject>
inline Type& CastR(TObject &X)
{ return reinterpret_cast<Type&> (X); }

/* Cast to struct T, pointer to pointer */
template<typename Type, typename TObject>
inline const Type* CastP(const TObject *X)
{ return reinterpret_cast<const Type*> (X); }
template<typename Type, typename TObject>
inline Type* CastP(TObject *X)
{ return reinterpret_cast<Type*> (X); }

/* StructAtOffset<T>(X,Ofs) returns the struct T& that is placed at memory
 * location of X plus Ofs bytes. */
template<typename Type, typename TObject>
inline const Type& StructAtOffset(const TObject &X, unsigned int offset)
{ return * reinterpret_cast<const Type*> (CharP(&X) + offset); }
template<typename Type, typename TObject>
inline Type& StructAtOffset(TObject &X, unsigned int offset)
{ return * reinterpret_cast<Type*> (CharP(&X) + offset); }

/* StructAfter<T>(X) returns the struct T& that is placed after X.
 * Works with X of variable size also.  X must implement get_size() */
template<typename Type, typename TObject>
inline const Type& StructAfter(const TObject &X)
{ return StructAtOffset<Type>(X, X.get_size()); }
template<typename Type, typename TObject>
inline Type& StructAfter(TObject &X)
{ return StructAtOffset<Type>(X, X.get_size()); }



/*
 * Null objects
 */

/* Global nul-content Null pool.  Enlarge as necessary. */
static const void *_NullPool[32 / sizeof (void *)];

/* Generic template for nul-content sizeof-sized Null objects. */
template <typename Type>
static inline const Type& Null () {
  ASSERT_STATIC (sizeof (Type) <= sizeof (_NullPool));
  return *CastP<Type> (_NullPool);
}

/* Specializaiton for arbitrary-content arbitrary-sized Null objects. */
#define DEFINE_NULL_DATA(Type, size, data) \
static const char _Null##Type[size + 1] = data; /* +1 is for nul-termination in data */ \
template <> \
inline const Type& Null<Type> () { \
  return *CastP<Type> (_Null##Type); \
} /* The following line really exists such that we end in a place needing semicolon */ \
ASSERT_STATIC (sizeof (Type) + 1 <= sizeof (_Null##Type))

/* Accessor macro. */
#define Null(Type) Null<Type>()



/*
 * Sanitize
 */

#ifndef HB_DEBUG_SANITIZE
#define HB_DEBUG_SANITIZE HB_DEBUG+0
#endif

#define TRACE_SANITIZE() \
	HB_STMT_START { \
	  if (HB_DEBUG_SANITIZE) \
		  _hb_trace ("SANITIZE", HB_FUNC, this, sanitize_depth, HB_DEBUG_SANITIZE); \
	} HB_STMT_END


#define SANITIZE_ARG_DEF \
	hb_sanitize_context_t *context, \
	unsigned int sanitize_depth HB_UNUSED
#define SANITIZE_ARG \
	context, \
	(HB_DEBUG_SANITIZE ? sanitize_depth + 1 : 0)

struct hb_sanitize_context_t
{
  const char *start, *end;
  hb_bool_t writable;
  unsigned int edit_count;
};


static inline void
_hb_sanitize_init (hb_sanitize_context_t *context,
		   hb_blob_t *blob)
{
  context->start = hb_blob_lock (blob);
  context->end = context->start + hb_blob_get_length (blob);
  context->writable = hb_blob_is_writable (blob);
  context->edit_count = 0;

  if (HB_DEBUG_SANITIZE)
    fprintf (stderr, "sanitize %p init [%p..%p] (%u bytes)\n",
	     blob, context->start, context->end, context->end - context->start);
}

static inline void
_hb_sanitize_fini (hb_sanitize_context_t *context HB_UNUSED,
		   hb_blob_t *blob)
{
  if (HB_DEBUG_SANITIZE)
    fprintf (stderr, "sanitize %p fini [%p..%p] %u edit requests\n",
	     blob, context->start, context->end, context->edit_count);

  hb_blob_unlock (blob);
}

static inline bool
_hb_sanitize_check (SANITIZE_ARG_DEF,
		    const char *base,
		    unsigned int len)
{
  bool ret = context->start <= base &&
	     base <= context->end &&
	     (unsigned int) (context->end - base) >= len;

  if (HB_DEBUG_SANITIZE && (int) sanitize_depth < (int) HB_DEBUG_SANITIZE) \
    fprintf (stderr, "SANITIZE(%p) %-*d-> check [%p..%p] (%d bytes) in [%p..%p] -> %s\n", \
	     base,
	     sanitize_depth, sanitize_depth,
	     base, base+len, len,
	     context->start, context->end,
	     ret ? "pass" : "FAIL");

  return ret;
}

static inline bool
_hb_sanitize_array (SANITIZE_ARG_DEF,
		    const char *base,
		    unsigned int record_size,
		    unsigned int len)
{
  bool overflows = len >= ((unsigned int) -1) / record_size;


  if (HB_DEBUG_SANITIZE && (int) sanitize_depth < (int) HB_DEBUG_SANITIZE)
    fprintf (stderr, "SANITIZE(%p) %-*d-> array [%p..%p] (%d*%d=%ld bytes) in [%p..%p] -> %s\n", \
	     base,
	     sanitize_depth, sanitize_depth,
	     base, base + (record_size * len), record_size, len, (unsigned long) record_size * len,
	     context->start, context->end,
	     !overflows ? "does not overflow" : "OVERFLOWS FAIL");

  return likely (!overflows) && _hb_sanitize_check (SANITIZE_ARG, base, record_size * len);
}

static inline bool
_hb_sanitize_edit (SANITIZE_ARG_DEF,
		   const char *base HB_UNUSED,
		   unsigned int len HB_UNUSED)
{
  context->edit_count++;

  if (HB_DEBUG_SANITIZE && (int) sanitize_depth < (int) HB_DEBUG_SANITIZE)
    fprintf (stderr, "SANITIZE(%p) %-*d-> edit(%u) [%p..%p] (%d bytes) in [%p..%p] -> %s\n", \
	     base,
	     sanitize_depth, sanitize_depth,
	     context->edit_count,
	     base, base+len, len,
	     context->start, context->end,
	     context->writable ? "granted" : "REJECTED");

  return context->writable;
}

#define SANITIZE(X) likely ((X).sanitize (SANITIZE_ARG))

#define SANITIZE_THIS(X) likely ((X).sanitize (SANITIZE_ARG, CharP(this)))
#define SANITIZE_THIS2(X,Y) (SANITIZE_THIS (X) && SANITIZE_THIS (Y))
#define SANITIZE_THIS3(X,Y,Z) (SANITIZE_THIS (X) && SANITIZE_THIS (Y) && SANITIZE_THIS(Z))

#define SANITIZE_BASE(X,B) likely ((X).sanitize (SANITIZE_ARG, B))
#define SANITIZE_BASE2(X,Y,B) (SANITIZE_BASE (X,B) && SANITIZE_BASE (Y,B))

#define SANITIZE_SELF() SANITIZE_OBJ (*this)
#define SANITIZE_OBJ(X) SANITIZE_MEM(&(X), sizeof (X))

#define SANITIZE_MEM(B,L) likely (_hb_sanitize_check (SANITIZE_ARG, CharP(B), (L)))

#define SANITIZE_ARRAY(A,S,L) likely (_hb_sanitize_array (SANITIZE_ARG, CharP(A), S, L))


/* Template to sanitize an object. */
template <typename Type>
struct Sanitizer
{
  static hb_blob_t *sanitize (hb_blob_t *blob) {
    hb_sanitize_context_t context[1];
    unsigned int sanitize_depth = 0;
    bool sane;

    /* TODO is_sane() stuff */

  retry:
    if (HB_DEBUG_SANITIZE)
      fprintf (stderr, "Sanitizer %p start %s\n", blob, HB_FUNC);

    _hb_sanitize_init (context, blob);

    Type *t = CastP<Type> (const_cast<char *> (context->start));

    sane = t->sanitize (SANITIZE_ARG);
    if (sane) {
      if (context->edit_count) {
	if (HB_DEBUG_SANITIZE)
	  fprintf (stderr, "Sanitizer %p passed first round with %d edits; doing a second round %s\n",
		   blob, context->edit_count, HB_FUNC);

        /* sanitize again to ensure no toe-stepping */
        context->edit_count = 0;
	sane = t->sanitize (SANITIZE_ARG);
	if (context->edit_count) {
	  if (HB_DEBUG_SANITIZE)
	    fprintf (stderr, "Sanitizer %p requested %d edits in second round; FAILLING %s\n",
		     blob, context->edit_count, HB_FUNC);
	  sane = false;
	}
      }
      _hb_sanitize_fini (context, blob);
    } else {
      unsigned int edit_count = context->edit_count;
      _hb_sanitize_fini (context, blob);
      if (edit_count && !hb_blob_is_writable (blob) && hb_blob_try_writable (blob)) {
        /* ok, we made it writable by relocating.  try again */
	if (HB_DEBUG_SANITIZE)
	  fprintf (stderr, "Sanitizer %p retry %s\n", blob, HB_FUNC);
        goto retry;
      }
    }

    if (HB_DEBUG_SANITIZE)
      fprintf (stderr, "Sanitizer %p %s %s\n", blob, sane ? "passed" : "FAILED", HB_FUNC);
    if (sane)
      return blob;
    else {
      hb_blob_destroy (blob);
      return hb_blob_create_empty ();
    }
  }
};




/*
 *
 * The OpenType Font File: Data Types
 */


/* "The following data types are used in the OpenType font file.
 *  All OpenType fonts use Motorola-style byte ordering (Big Endian):" */

/*
 * Int types
 */


template <typename Type, int Bytes> class BEInt;

/* LONGTERMTODO: On machines allowing unaligned access, we can make the
 * following tighter by using byteswap instructions on ints directly. */
template <typename Type>
class BEInt<Type, 2>
{
  public:
  inline class BEInt<Type,2>& operator = (Type i) { hb_be_uint16_put (v,i); return *this; }
  inline operator Type () const { return hb_be_uint16_get (v); }
  inline bool operator == (const BEInt<Type, 2>& o) const { return hb_be_uint16_cmp (v, o.v); }
  inline bool operator != (const BEInt<Type, 2>& o) const { return !(*this == o); }
  private: uint8_t v[2];
};
template <typename Type>
class BEInt<Type, 4>
{
  public:
  inline class BEInt<Type,4>& operator = (Type i) { hb_be_uint32_put (v,i); return *this; }
  inline operator Type () const { return hb_be_uint32_get (v); }
  inline bool operator == (const BEInt<Type, 4>& o) const { return hb_be_uint32_cmp (v, o.v); }
  inline bool operator != (const BEInt<Type, 4>& o) const { return !(*this == o); }
  private: uint8_t v[4];
};

/* Integer types in big-endian order and no alignment requirement */
template <typename Type>
struct IntType
{
  static inline unsigned int get_size () { return sizeof (Type); }
  inline void set (Type i) { v = i; }
  inline operator Type(void) const { return v; }
  inline bool operator == (const IntType<Type> &o) const { return v == o.v; }
  inline bool operator != (const IntType<Type> &o) const { return v != o.v; }
  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_SELF ();
  }
  private: BEInt<Type, sizeof (Type)> v;
};

typedef IntType<uint16_t> USHORT;	/* 16-bit unsigned integer. */
typedef IntType<int16_t>  SHORT;	/* 16-bit signed integer. */
typedef IntType<uint32_t> ULONG;	/* 32-bit unsigned integer. */
typedef IntType<int32_t>  LONG;		/* 32-bit signed integer. */

ASSERT_SIZE (USHORT, 2);
ASSERT_SIZE (SHORT, 2);
ASSERT_SIZE (ULONG, 4);
ASSERT_SIZE (LONG, 4);

/* Array of four uint8s (length = 32 bits) used to identify a script, language
 * system, feature, or baseline */
struct Tag : ULONG
{
  /* What the char* converters return is NOT nul-terminated.  Print using "%.4s" */
  inline operator const char* (void) const { return CharP(this); }
  inline operator char* (void) { return CharP(this); }
};
ASSERT_SIZE (Tag, 4);
DEFINE_NULL_DATA (Tag, 4, "    ");

/* Glyph index number, same as uint16 (length = 16 bits) */
typedef USHORT GlyphID;

/* Offset to a table, same as uint16 (length = 16 bits), Null offset = 0x0000 */
typedef USHORT Offset;

/* LongOffset to a table, same as uint32 (length = 32 bits), Null offset = 0x00000000 */
typedef ULONG LongOffset;


/* CheckSum */
struct CheckSum : ULONG
{
  static uint32_t CalcTableChecksum (ULONG *Table, uint32_t Length)
  {
    uint32_t Sum = 0L;
    ULONG *EndPtr = Table+((Length+3) & ~3) / ULONG::get_size ();

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
    TRACE_SANITIZE ();
    return SANITIZE_SELF ();
  }

  USHORT major;
  USHORT minor;
};
ASSERT_SIZE (FixedVersion, 4);



/*
 * Template subclasses of Offset and LongOffset that do the dereferencing.
 * Use: (base+offset)
 */

template <typename OffsetType, typename Type>
struct GenericOffsetTo : OffsetType
{
  inline const Type& operator () (const void *base) const
  {
    unsigned int offset = *this;
    if (unlikely (!offset)) return Null(Type);
    return StructAtOffset<Type> (*CharP(base), offset);
  }

  inline bool sanitize (SANITIZE_ARG_DEF, void *base) {
    TRACE_SANITIZE ();
    if (!SANITIZE_SELF ()) return false;
    unsigned int offset = *this;
    if (unlikely (!offset)) return true;
    return SANITIZE (StructAtOffset<Type> (*CharP(base), offset)) || neuter (SANITIZE_ARG);
  }
  inline bool sanitize (SANITIZE_ARG_DEF, void *base, void *base2) {
    TRACE_SANITIZE ();
    if (!SANITIZE_SELF ()) return false;
    unsigned int offset = *this;
    if (unlikely (!offset)) return true;
    return SANITIZE_BASE (StructAtOffset<Type> (*CharP(base), offset), base2) || neuter (SANITIZE_ARG);
  }
  inline bool sanitize (SANITIZE_ARG_DEF, void *base, unsigned int user_data) {
    TRACE_SANITIZE ();
    if (!SANITIZE_SELF ()) return false;
    unsigned int offset = *this;
    if (unlikely (!offset)) return true;
    return SANITIZE_BASE (StructAtOffset<Type> (*CharP(base), offset), user_data) || neuter (SANITIZE_ARG);
  }

  private:
  /* Set the offset to Null */
  inline bool neuter (SANITIZE_ARG_DEF) {
    if (_hb_sanitize_edit (SANITIZE_ARG, CharP(this), this->get_size ())) {
      this->set (0); /* 0 is Null offset */
      return true;
    }
    return false;
  }
};
template <typename Base, typename OffsetType, typename Type>
inline const Type& operator + (const Base &base, GenericOffsetTo<OffsetType, Type> offset) { return offset (base); }

template <typename Type>
struct OffsetTo : GenericOffsetTo<Offset, Type> {};

template <typename Type>
struct LongOffsetTo : GenericOffsetTo<LongOffset, Type> {};


/*
 * Array Types
 */

template <typename LenType, typename Type>
struct GenericArrayOf
{
  const Type *array(void) const { return &StructAfter<Type> (len); }
  Type *array(void) { return &StructAfter<Type> (len); }

  const Type *sub_array (unsigned int start_offset, unsigned int *pcount /* IN/OUT */) const
  {
    unsigned int count = len;
    if (unlikely (start_offset > count))
      count = 0;
    else
      count -= start_offset;
    count = MIN (count, *pcount);
    *pcount = count;
    return array() + start_offset;
  }

  inline const Type& operator [] (unsigned int i) const
  {
    if (unlikely (i >= len)) return Null(Type);
    return array()[i];
  }
  inline unsigned int get_size () const
  { return len.get_size () + len * Type::get_size (); }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    if (!likely (sanitize_shallow (SANITIZE_ARG))) return false;
    /* Note: for structs that do not reference other structs,
     * we do not need to call their sanitize() as we already did
     * a bound check on the aggregate array size, hence the return.
     */
    return true;
    /* We do keep this code though to make sure the structs pointed
     * to do have a simple sanitize(), ie. they do not reference
     * other structs. */
    unsigned int count = len;
    for (unsigned int i = 0; i < count; i++)
      if (!SANITIZE (array()[i]))
        return false;
    return true;
  }
  inline bool sanitize (SANITIZE_ARG_DEF, void *base) {
    TRACE_SANITIZE ();
    if (!likely (sanitize_shallow (SANITIZE_ARG))) return false;
    unsigned int count = len;
    for (unsigned int i = 0; i < count; i++)
      if (!array()[i].sanitize (SANITIZE_ARG, base))
        return false;
    return true;
  }
  inline bool sanitize (SANITIZE_ARG_DEF, void *base, void *base2) {
    TRACE_SANITIZE ();
    if (!likely (sanitize_shallow (SANITIZE_ARG))) return false;
    unsigned int count = len;
    for (unsigned int i = 0; i < count; i++)
      if (!array()[i].sanitize (SANITIZE_ARG, base, base2))
        return false;
    return true;
  }
  inline bool sanitize (SANITIZE_ARG_DEF, void *base, unsigned int user_data) {
    TRACE_SANITIZE ();
    if (!likely (sanitize_shallow (SANITIZE_ARG))) return false;
    unsigned int count = len;
    for (unsigned int i = 0; i < count; i++)
      if (!array()[i].sanitize (SANITIZE_ARG, base, user_data))
        return false;
    return true;
  }

  private:
  inline bool sanitize_shallow (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_SELF() && SANITIZE_ARRAY (this, Type::get_size (), len);
  }

  public:
  LenType len;
/*Type array[VAR];*/
};

/* An array with a USHORT number of elements. */
template <typename Type>
struct ArrayOf : GenericArrayOf<USHORT, Type> {};

/* An array with a ULONG number of elements. */
template <typename Type>
struct LongArrayOf : GenericArrayOf<ULONG, Type> {};

/* Array of Offset's */
template <typename Type>
struct OffsetArrayOf : ArrayOf<OffsetTo<Type> > {};

/* Array of LongOffset's */
template <typename Type>
struct LongOffsetArrayOf : ArrayOf<LongOffsetTo<Type> > {};

/* LongArray of LongOffset's */
template <typename Type>
struct LongOffsetLongArrayOf : LongArrayOf<LongOffsetTo<Type> > {};

/* Array of offsets relative to the beginning of the array itself. */
template <typename Type>
struct OffsetListOf : OffsetArrayOf<Type>
{
  inline const Type& operator [] (unsigned int i) const
  {
    if (unlikely (i >= this->len)) return Null(Type);
    return this+this->array()[i];
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return OffsetArrayOf<Type>::sanitize (SANITIZE_ARG, CharP(this));
  }
  inline bool sanitize (SANITIZE_ARG_DEF, unsigned int user_data) {
    TRACE_SANITIZE ();
    return OffsetArrayOf<Type>::sanitize (SANITIZE_ARG, CharP(this), user_data);
  }
};


/* An array with a USHORT number of elements,
 * starting at second element. */
template <typename Type>
struct HeadlessArrayOf
{
  const Type *array(void) const { return &StructAfter<Type> (len); }
  Type *array(void) { return &StructAfter<Type> (len); }

  inline const Type& operator [] (unsigned int i) const
  {
    if (unlikely (i >= len || !i)) return Null(Type);
    return array()[i-1];
  }
  inline unsigned int get_size () const
  { return len.get_size () + (len ? len - 1 : 0) * Type::get_size (); }

  inline bool sanitize_shallow (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return SANITIZE_SELF() && SANITIZE_ARRAY (this, Type::get_size (), len);
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    if (!likely (sanitize_shallow (SANITIZE_ARG))) return false;
    /* Note: for structs that do not reference other structs,
     * we do not need to call their sanitize() as we already did
     * a bound check on the aggregate array size, hence the return.
     */
    return true;
    /* We do keep this code though to make sure the structs pointed
     * to do have a simple sanitize(), ie. they do not reference
     * other structs. */
    unsigned int count = len ? len - 1 : 0;
    Type *a = array();
    for (unsigned int i = 0; i < count; i++)
      if (!SANITIZE (a[i]))
        return false;
    return true;
  }

  USHORT len;
/*Type array[VAR];*/
};


#endif /* HB_OPEN_TYPE_PRIVATE_HH */
