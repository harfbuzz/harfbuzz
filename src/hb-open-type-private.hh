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


/*
 * Casts
 */

#define CONST_CHARP(X)		(reinterpret_cast<const char *>(X))
#define DECONST_CHARP(X)	((char *)reinterpret_cast<const char *>(X))
#define CHARP(X)		(reinterpret_cast<char *>(X))

#define CONST_CAST(T,X,Ofs)	(*(reinterpret_cast<const T *>(CONST_CHARP(&(X)) + Ofs)))
#define DECONST_CAST(T,X,Ofs)	(*(reinterpret_cast<T *>((char *)CONST_CHARP(&(X)) + Ofs)))
#define CAST(T,X,Ofs) 		(*(reinterpret_cast<T *>(CHARP(&(X)) + Ofs)))

#define CONST_NEXT(T,X)		(*(reinterpret_cast<const T *>(CONST_CHARP(&(X)) + (X).get_size ())))
#define NEXT(T,X)		(*(reinterpret_cast<T *>(CHARP(&(X)) + (X).get_size ())))

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
  static inline const Type &get () { return CONST_CAST (Type, *NullPool, 0); }
};

/* Specializaiton for arbitrary-content arbitrary-sized Null objects. */
#define DEFINE_NULL_DATA(Type, size, data) \
static const char _Null##Type[size] = data; \
template <> \
struct Null <Type> \
{ \
  static inline const Type &get () { return CONST_CAST (Type, *_Null##Type, 0); } \
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
    return CONST_CAST (Type, *data, 0); \
  }
/* Like get_for_data(), but checks major version first. */
#define STATIC_DEFINE_GET_FOR_DATA_CHECK_MAJOR_VERSION(Type, MajorMin, MajorMax) \
  static inline const Type& get_for_data (const char *data) \
  { \
    if (HB_UNLIKELY (data == NULL)) return Null(Type); \
    const Type& t = CONST_CAST (Type, *data, 0); \
    if (HB_UNLIKELY (t.version.major < MajorMin || t.version.major > MajorMax)) return Null(Type); \
    return t; \
  }


/*
 * Sanitize
 */

#ifndef HB_DEBUG_SANITIZE
#define HB_DEBUG_SANITIZE HB_DEBUG
#endif

#if HB_DEBUG_SANITIZE
#define TRACE_SANITIZE_ARG_DEF	, unsigned int sanitize_depth
#define TRACE_SANITIZE_ARG	, sanitize_depth + 1
#define TRACE_SANITIZE_ARG_INIT	, 1
#define TRACE_SANITIZE() \
	HB_STMT_START { \
	    if (sanitize_depth < HB_DEBUG_SANITIZE) \
		fprintf (stderr, "SANITIZE(%p) %-*d-> %s\n", \
			 (CONST_CHARP (this) == NullPool) ? 0 : this, \
			 sanitize_depth, sanitize_depth, \
			 __PRETTY_FUNCTION__); \
	} HB_STMT_END
#else
#define TRACE_SANITIZE_ARG_DEF
#define TRACE_SANITIZE_ARG
#define TRACE_SANITIZE_ARG_INIT
#define TRACE_SANITIZE() HB_STMT_START {} HB_STMT_END
#endif

#define SANITIZE_ARG_DEF \
	hb_sanitize_context_t *context TRACE_SANITIZE_ARG_DEF
#define SANITIZE_ARG \
	context TRACE_SANITIZE_ARG
#define SANITIZE_ARG_INIT \
	&context TRACE_SANITIZE_ARG_INIT

typedef struct _hb_sanitize_context_t hb_sanitize_context_t;
struct _hb_sanitize_context_t
{
  const char *start, *end;
  int edit_count;
  hb_blob_t *blob;
};

static HB_GNUC_UNUSED void
_hb_sanitize_init (hb_sanitize_context_t *context,
		   hb_blob_t *blob)
{
  context->blob = blob;
  context->start = hb_blob_lock (blob);
  context->end = context->start + hb_blob_get_length (blob);
  context->edit_count = 0;

#if HB_DEBUG_SANITIZE
  fprintf (stderr, "sanitize %p init [%p..%p] (%u bytes)\n",
	   context->blob, context->start, context->end, context->end - context->start);
#endif
}

static HB_GNUC_UNUSED void
_hb_sanitize_fini (hb_sanitize_context_t *context,
		   bool unlock)
{
#if HB_DEBUG_SANITIZE
  fprintf (stderr, "sanitize %p fini [%p..%p] %u edit requests\n",
	   context->blob, context->start, context->end, context->edit_count);
#endif

  if (unlock)
    hb_blob_unlock (context->blob);
}

static HB_GNUC_UNUSED inline bool
_hb_sanitize_check (SANITIZE_ARG_DEF,
		    const char *base,
		    unsigned int len)
{
  bool ret = context->start <= base &&
	     base <= context->end &&
	     (unsigned int) (context->end - base) >= len;

#if HB_DEBUG_SANITIZE
  if (sanitize_depth < HB_DEBUG_SANITIZE) \
    fprintf (stderr, "SANITIZE(%p) %-*d-> check [%p..%p] (%d bytes) in [%p..%p] -> %s\n", \
	     base,
	     sanitize_depth, sanitize_depth,
	     base, base+len, len,
	     context->start, context->end,
	     ret ? "pass" : "FAIL");
#endif
  return ret;
}

static HB_GNUC_UNUSED inline bool
_hb_sanitize_array (SANITIZE_ARG_DEF,
		    const char *base,
		    unsigned int record_size,
		    unsigned int len)
{
  bool overflows = len >= ((unsigned int) -1) / record_size;

#if HB_DEBUG_SANITIZE
  if (sanitize_depth < HB_DEBUG_SANITIZE) \
    fprintf (stderr, "SANITIZE(%p) %-*d-> array [%p..%p] (%d*%d=%ld bytes) in [%p..%p] -> %s\n", \
	     base,
	     sanitize_depth, sanitize_depth,
	     base, base + (record_size * len), record_size, len, (unsigned long) record_size * len,
	     context->start, context->end,
	     !overflows ? "does not overflow" : "OVERFLOWS FAIL");
#endif
  return HB_LIKELY (!overflows) && _hb_sanitize_check (SANITIZE_ARG, base, record_size * len);
}

static HB_GNUC_UNUSED inline bool
_hb_sanitize_edit (SANITIZE_ARG_DEF,
		   const char *base HB_GNUC_UNUSED,
		   unsigned int len HB_GNUC_UNUSED)
{
  bool perm = hb_blob_try_writable_inplace (context->blob);
  context->edit_count++;

#if HB_DEBUG_SANITIZE
  fprintf (stderr, "SANITIZE(%p) %-*d-> edit(%u) [%p..%p] (%d bytes) in [%p..%p] -> %s\n", \
	   base,
	   sanitize_depth, sanitize_depth,
	   context->edit_count,
	   base, base+len, len,
	   context->start, context->end,
	   perm ? "granted" : "REJECTED");
#endif
  return perm;
}

#define SANITIZE(X) HB_LIKELY ((X).sanitize (SANITIZE_ARG))
#define SANITIZE2(X,Y) (SANITIZE (X) && SANITIZE (Y))

#define SANITIZE_THIS(X) HB_LIKELY ((X).sanitize (SANITIZE_ARG, CONST_CHARP(this)))
#define SANITIZE_THIS2(X,Y) (SANITIZE_THIS (X) && SANITIZE_THIS (Y))
#define SANITIZE_THIS3(X,Y,Z) (SANITIZE_THIS (X) && SANITIZE_THIS (Y) && SANITIZE_THIS(Z))

#define SANITIZE_BASE(X,B) HB_LIKELY ((X).sanitize (SANITIZE_ARG, B))
#define SANITIZE_BASE2(X,Y,B) (SANITIZE_BASE (X,B) && SANITIZE_BASE (Y,B))

#define SANITIZE_SELF() SANITIZE_OBJ (*this)
#define SANITIZE_OBJ(X) SANITIZE_MEM(&(X), sizeof (X))
#define SANITIZE_GET_SIZE() SANITIZE_SELF() && SANITIZE_MEM (this, this->get_size ())

/* TODO Optimize this if L is fixed (gcc magic) */
#define SANITIZE_MEM(B,L) HB_LIKELY (_hb_sanitize_check (SANITIZE_ARG, CONST_CHARP(B), (L)))

#define SANITIZE_ARRAY(A,S,L) HB_LIKELY (_hb_sanitize_array (SANITIZE_ARG, CONST_CHARP(A), S, L))

#define NEUTER(Var, Val) \
	(SANITIZE_OBJ (Var) && \
	 _hb_sanitize_edit (SANITIZE_ARG, CONST_CHARP(&(Var)), sizeof (Var)) && \
	 ((Var) = (Val), true))


/* Template to sanitize an object. */
template <typename Type>
struct Sanitizer
{
  static hb_blob_t *sanitize (hb_blob_t *blob) {
    hb_sanitize_context_t context;
    bool sane;

    /* TODO is_sane() stuff */

  retry:
#if HB_DEBUG_SANITIZE
    fprintf (stderr, "Sanitizer %p start %s\n", blob, __PRETTY_FUNCTION__);
#endif

    _hb_sanitize_init (&context, blob);

    Type *t = &CAST (Type, *DECONST_CHARP(context.start), 0);

    sane = t->sanitize (SANITIZE_ARG_INIT);
    if (sane) {
      if (context.edit_count) {
#if HB_DEBUG_SANITIZE
	fprintf (stderr, "Sanitizer %p passed first round with %d edits; going a second round %s\n",
		 blob, context.edit_count, __PRETTY_FUNCTION__);
#endif
        /* sanitize again to ensure no toe-stepping */
        context.edit_count = 0;
	sane = t->sanitize (SANITIZE_ARG_INIT);
	if (context.edit_count) {
#if HB_DEBUG_SANITIZE
	  fprintf (stderr, "Sanitizer %p requested %d edits in second round; FAILLING %s\n",
		   blob, context.edit_count, __PRETTY_FUNCTION__);
#endif
	  sane = false;
	}
      }
      _hb_sanitize_fini (&context, true);
    } else {
      unsigned int edit_count = context.edit_count;
      _hb_sanitize_fini (&context, true);
      if (edit_count && !hb_blob_is_writable (blob) && hb_blob_try_writable (blob)) {
        /* ok, we made it writable by relocating.  try again */
#if HB_DEBUG_SANITIZE
	fprintf (stderr, "Sanitizer %p retry %s\n", blob, __PRETTY_FUNCTION__);
#endif
        goto retry;
      }
    }

#if HB_DEBUG_SANITIZE
    fprintf (stderr, "Sanitizer %p %s %s\n", blob, sane ? "passed" : "FAILED", __PRETTY_FUNCTION__);
#endif
    if (sane)
      return blob;
    else {
      hb_blob_destroy (blob);
      return hb_blob_create_empty ();
    }
  }

  static const Type& lock_instance (hb_blob_t *blob) {
    return Type::get_for_data (hb_blob_lock (blob));
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

/* TODO On machines that allow unaligned access, use this version. */
#define _DEFINE_INT_TYPE1_UNALIGNED(NAME, TYPE, BIG_ENDIAN, BYTES) \
  struct NAME \
  { \
    inline NAME& operator = (TYPE i) { (TYPE&) v = BIG_ENDIAN (i); return *this; } \
    inline operator TYPE(void) const { return BIG_ENDIAN ((TYPE&) v); } \
    inline bool operator== (NAME o) const { return (TYPE&) v == (TYPE&) o.v; } \
    inline bool sanitize (SANITIZE_ARG_DEF) { \
      TRACE_SANITIZE (); \
      return SANITIZE_SELF (); \
    } \
    private: unsigned char v[BYTES]; \
  }; \
  ASSERT_SIZE (NAME, BYTES)

#define DEFINE_INT_TYPE1(NAME, TYPE, BIG_ENDIAN, BYTES) \
  struct NAME \
  { \
    inline NAME& operator = (TYPE i) { BIG_ENDIAN##_put_unaligned(v, i); return *this; } \
    inline operator TYPE(void) const { return BIG_ENDIAN##_get_unaligned (v); } \
    inline bool operator== (NAME o) const { return BIG_ENDIAN##_cmp_unaligned (v, o.v); } \
    inline bool sanitize (SANITIZE_ARG_DEF) { \
      TRACE_SANITIZE (); \
      return SANITIZE_SELF (); \
    } \
    private: unsigned char v[BYTES]; \
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
  inline operator const char* (void) const { return CONST_CHARP(this); }
  inline operator char* (void) { return CHARP(this); }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    /* Note: Only accept ASCII-visible tags (mind DEL)
     * This is one of the few times (only time?) we check
     * for data integrity, as opposed o just boundary checks
     */
    return SANITIZE_SELF () && (((uint32_t) *this) & 0x80808080) == 0;
  }
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
    TRACE_SANITIZE ();
    return SANITIZE_SELF ();
  }

  USHORT major;
  USHORT minor;
};
ASSERT_SIZE (FixedVersion, 4);



/*
 * Template subclasses of Offset and LongOffset that do the dereferencing.
 * Use: (this+memberName)
 */

template <typename OffsetType, typename Type>
struct GenericOffsetTo : OffsetType
{
  inline const Type& operator() (const void *base) const
  {
    unsigned int offset = *this;
    if (HB_UNLIKELY (!offset)) return Null(Type);
    return CONST_CAST(Type, *CONST_CHARP(base), offset);
  }

  inline bool sanitize (SANITIZE_ARG_DEF, const void *base) {
    TRACE_SANITIZE ();
    if (!SANITIZE_SELF ()) return false;
    unsigned int offset = *this;
    if (HB_UNLIKELY (!offset)) return true;
    return SANITIZE (CAST(Type, *DECONST_CHARP(base), offset)) || NEUTER (DECONST_CAST(OffsetType,*this,0), 0);
  }
  inline bool sanitize (SANITIZE_ARG_DEF, const void *base, const void *base2) {
    TRACE_SANITIZE ();
    if (!SANITIZE_SELF ()) return false;
    unsigned int offset = *this;
    if (HB_UNLIKELY (!offset)) return true;
    return SANITIZE_BASE (CAST(Type, *DECONST_CHARP(base), offset), base2) || NEUTER (DECONST_CAST(OffsetType,*this,0), 0);
  }
  inline bool sanitize (SANITIZE_ARG_DEF, const void *base, unsigned int user_data) {
    TRACE_SANITIZE ();
    if (!SANITIZE_SELF ()) return false;
    unsigned int offset = *this;
    if (HB_UNLIKELY (!offset)) return true;
    return SANITIZE_BASE (CAST(Type, *DECONST_CHARP(base), offset), user_data) || NEUTER (DECONST_CAST(OffsetType,*this,0), 0);
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
  inline const Type& operator [] (unsigned int i) const
  {
    if (HB_UNLIKELY (i >= len)) return Null(Type);
    return array[i];
  }
  inline unsigned int get_size () const
  { return sizeof (len) + len * sizeof (array[0]); }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    if (!SANITIZE_GET_SIZE()) return false;
    /* Note:
     * for non-recursive types, this is not much needed.
     * But we keep the code to make sure the objects pointed to
     * do have a simple sanitize(). */
    return true;
    unsigned int count = len;
    for (unsigned int i = 0; i < count; i++)
      if (!SANITIZE (array[i]))
        return false;
    return true;
  }
  inline bool sanitize (SANITIZE_ARG_DEF, const void *base) {
    TRACE_SANITIZE ();
    if (!SANITIZE_GET_SIZE()) return false;
    unsigned int count = len;
    for (unsigned int i = 0; i < count; i++)
      if (!array[i].sanitize (SANITIZE_ARG, base))
        return false;
    return true;
  }
  inline bool sanitize (SANITIZE_ARG_DEF, const void *base, const void *base2) {
    TRACE_SANITIZE ();
    if (!SANITIZE_GET_SIZE()) return false;
    unsigned int count = len;
    for (unsigned int i = 0; i < count; i++)
      if (!array[i].sanitize (SANITIZE_ARG, base, base2))
        return false;
    return true;
  }
  inline bool sanitize (SANITIZE_ARG_DEF, const void *base, unsigned int user_data) {
    TRACE_SANITIZE ();
    if (!SANITIZE_GET_SIZE()) return false;
    unsigned int count = len;
    for (unsigned int i = 0; i < count; i++)
      if (!array[i].sanitize (SANITIZE_ARG, base, user_data))
        return false;
    return true;
  }

  LenType len;
  Type array[];
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
    if (HB_UNLIKELY (i >= this->len)) return Null(Type);
    return this+this->array[i];
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    TRACE_SANITIZE ();
    return OffsetArrayOf<Type>::sanitize (SANITIZE_ARG, CONST_CHARP(this));
  }
  inline bool sanitize (SANITIZE_ARG_DEF, unsigned int user_data) {
    TRACE_SANITIZE ();
    return OffsetArrayOf<Type>::sanitize (SANITIZE_ARG, CONST_CHARP(this), user_data);
  }
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
    TRACE_SANITIZE ();
    if (!SANITIZE_GET_SIZE()) return false;
    /* Note:
     * for non-recursive types, this is not much needed.
     * But we keep the code to make sure the objects pointed to
     * do have a simple sanitize(). */
    return true;
    unsigned int count = len ? len - 1 : 0;
    for (unsigned int i = 0; i < count; i++)
      if (!SANITIZE (array[i]))
        return false;
    return true;
  }

  USHORT len;
  Type array[];
};


#endif /* HB_OPEN_TYPES_PRIVATE_HH */
