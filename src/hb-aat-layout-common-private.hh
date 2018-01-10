/*
 * Copyright © 2017  Google, Inc.
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
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_AAT_LAYOUT_COMMON_PRIVATE_HH
#define HB_AAT_LAYOUT_COMMON_PRIVATE_HH

#include "hb-aat-layout-private.hh"


namespace AAT {

using namespace OT;
using OT::UINT8;
using OT::UINT16;
using OT::UINT32;


/*
 * Binary Searching Tables
 */

struct BinSearchHeader
{

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  UINT16	unitSize;	/* Size of a lookup unit for this search in bytes. */
  UINT16	nUnits;		/* Number of units of the preceding size to be searched. */
  UINT16	searchRange;	/* The value of unitSize times the largest power of 2
				 * that is less than or equal to the value of nUnits. */
  UINT16	entrySelector;	/* The log base 2 of the largest power of 2 less than
				 * or equal to the value of nUnits. */
  UINT16	rangeShift;	/* The value of unitSize times the difference of the
				 * value of nUnits minus the largest power of 2 less
				 * than or equal to the value of nUnits. */
  public:
  DEFINE_SIZE_STATIC (10);
};

template <typename Type>
struct BinSearchArrayOf
{
  inline const Type& operator [] (unsigned int i) const
  {
    if (unlikely (i >= header.nUnits)) return Null(Type);
    return StructAtOffset<Type> (bytes, i * header.unitSize);
  }
  inline Type& operator [] (unsigned int i)
  {
    return StructAtOffset<Type> (bytes, i * header.unitSize);
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
    (void) (false && StructAtOffset<Type> (bytes, 0).sanitize (c));

    return_trace (true);
  }
  inline bool sanitize (hb_sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!sanitize_shallow (c))) return_trace (false);
    unsigned int count = header.nUnits;
    for (unsigned int i = 0; i < count; i++)
      if (unlikely (!(*this)[i].sanitize (c, base)))
        return_trace (false);
    return_trace (true);
  }

  template <typename T>
  inline const Type *bsearch (const T &key) const
  {
    unsigned int size = header.unitSize;
    int min = 0, max = (int) header.nUnits - 1;
    while (min <= max)
    {
      int mid = (min + max) / 2;
      const Type *p = (const Type *) (((const char *) bytes) + (mid * size));
      int c = p->cmp (key);
      if (c < 0)
	max = mid - 1;
      else if (c > 0)
	min = mid + 1;
      else
	return p;
    }
    return NULL;
  }

  private:
  inline bool sanitize_shallow (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (header.sanitize (c) &&
		  Type::static_size >= header.unitSize &&
		  c->check_array (bytes, header.unitSize, header.nUnits));
  }

  protected:
  BinSearchHeader	header;
  UINT8			bytes[VAR];
  public:
  DEFINE_SIZE_ARRAY (10, bytes);
};


/* TODO Move this to hb-open-type-private.hh and use it in ArrayOf, HeadlessArrayOf,
 * and other places around the code base?? */
template <typename Type>
struct UnsizedArrayOf
{
  inline const Type& operator [] (unsigned int i) const { return arrayZ[i]; }
  inline Type& operator [] (unsigned int i) { return arrayZ[i]; }

  inline bool sanitize (hb_sanitize_context_t *c, unsigned int count) const
  {
    TRACE_SANITIZE (this);

    /* Note: for structs that do not reference other structs,
     * we do not need to call their sanitize() as we already did
     * a bound check on the aggregate array size.  We just include
     * a small unreachable expression to make sure the structs
     * pointed to do have a simple sanitize(), ie. they do not
     * reference other structs via offsets.
     */
    (void) (false && count && arrayZ->sanitize (c));

    return_trace (c->check_array (arrayZ, arrayZ[0].static_size, count));
  }

  protected:
  Type	arrayZ[VAR];
  public:
  DEFINE_SIZE_ARRAY (0, arrayZ);
};


/*
 * Lookup Table
 */

template <typename T> struct Lookup;

template <typename T>
struct LookupFormat0
{
  friend struct Lookup<T>;

  private:
  inline const T* get_value (hb_codepoint_t glyph_id, unsigned int num_glyphs) const
  {
    if (unlikely (glyph_id >= num_glyphs)) return nullptr;
    return &arrayZ[glyph_id];
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (arrayZ.sanitize (c, c->num_glyphs));
  }

  protected:
  UINT16	format;		/* Format identifier--format = 0 */
  UnsizedArrayOf<T>
		arrayZ;		/* Array of lookup values, indexed by glyph index. */
  public:
  DEFINE_SIZE_ARRAY (2, arrayZ);
};


template <typename T>
struct LookupSegmentSingle
{
  inline int cmp (hb_codepoint_t g) const {
    return g < first ? -1 : g <= last ? 0 : +1 ;
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) && value.sanitize (c));
  }

  GlyphID	last;		/* Last GlyphID in this segment */
  GlyphID	first;		/* First GlyphID in this segment */
  T		value;		/* The lookup value (only one) */
  public:
  DEFINE_SIZE_STATIC (4 + sizeof (T));
};

template <typename T>
struct LookupFormat2
{
  friend struct Lookup<T>;

  private:
  inline const T* get_value (hb_codepoint_t glyph_id) const
  {
    const LookupSegmentSingle<T> *v = segments.bsearch (glyph_id);
    return v ? &v->value : nullptr;
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (segments.sanitize (c));
  }

  protected:
  UINT16	format;		/* Format identifier--format = 2 */
  BinSearchArrayOf<LookupSegmentSingle<T> >
		segments;	/* The actual segments. These must already be sorted,
				 * according to the first word in each one (the last
				 * glyph in each segment). */
  public:
  DEFINE_SIZE_ARRAY (8, segments);
};

template <typename T>
struct LookupSegmentArray
{
  inline const T* get_value (hb_codepoint_t glyph_id, const void *base) const
  {
    return first <= glyph_id && glyph_id <= last ? &(base+valuesZ)[glyph_id - first] : nullptr;
  }

  inline int cmp (hb_codepoint_t g) const {
    return g < first ? -1 : g <= last ? 0 : +1 ;
  }

  inline bool sanitize (hb_sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  first <= last &&
		  valuesZ.sanitize (c, base, last - first + 1));
  }

  GlyphID	last;		/* Last GlyphID in this segment */
  GlyphID	first;		/* First GlyphID in this segment */
  OffsetTo<UnsizedArrayOf<T> >
		valuesZ;	/* A 16-bit offset from the start of
				 * the table to the data. */
  public:
  DEFINE_SIZE_STATIC (6);
};

template <typename T>
struct LookupFormat4
{
  friend struct Lookup<T>;

  private:
  inline const T* get_value (hb_codepoint_t glyph_id) const
  {
    const LookupSegmentArray<T> *v = segments.bsearch (glyph_id);
    return v ? v->get_value (glyph_id, this) : nullptr;
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (segments.sanitize (c, this));
  }

  protected:
  UINT16	format;		/* Format identifier--format = 2 */
  BinSearchArrayOf<LookupSegmentArray<T> >
		segments;	/* The actual segments. These must already be sorted,
				 * according to the first word in each one (the last
				 * glyph in each segment). */
  public:
  DEFINE_SIZE_ARRAY (8, segments);
};

template <typename T>
struct LookupSingle
{
  inline int cmp (hb_codepoint_t g) const { return glyph.cmp (g); }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) && value.sanitize (c));
  }

  GlyphID	glyph;		/* Last GlyphID */
  T		value;		/* The lookup value (only one) */
  public:
  DEFINE_SIZE_STATIC (4 + sizeof (T));
};

template <typename T>
struct LookupFormat6
{
  friend struct Lookup<T>;

  private:
  inline const T* get_value (hb_codepoint_t glyph_id) const
  {
    const LookupSingle<T> *v = entries.bsearch (glyph_id);
    return v ? &v->value : nullptr;
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (entries.sanitize (c));
  }

  protected:
  UINT16	format;		/* Format identifier--format = 6 */
  BinSearchArrayOf<LookupSingle<T> >
		entries;	/* The actual entries, sorted by glyph index. */
  public:
  DEFINE_SIZE_ARRAY (8, entries);
};

template <typename T>
struct LookupFormat8
{
  friend struct Lookup<T>;

  private:
  inline const T* get_value (hb_codepoint_t glyph_id) const
  {
    return firstGlyph <= glyph_id && glyph_id - firstGlyph < glyphCount ? &valueArrayZ[glyph_id - firstGlyph] : nullptr;
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) && valueArrayZ.sanitize (c, glyphCount));
  }

  protected:
  UINT16	format;		/* Format identifier--format = 6 */
  GlyphID	firstGlyph;	/* First glyph index included in the trimmed array. */
  UINT16	glyphCount;	/* Total number of glyphs (equivalent to the last
				 * glyph minus the value of firstGlyph plus 1). */
  UnsizedArrayOf<T>
		valueArrayZ;	/* The lookup values (indexed by the glyph index
				 * minus the value of firstGlyph). */
  public:
  DEFINE_SIZE_ARRAY (6, valueArrayZ);
};

template <typename T>
struct Lookup
{
  inline const T* get_value (hb_codepoint_t glyph_id, unsigned int num_glyphs) const
  {
    switch (u.format) {
    case 0: return u.format0.get_value (glyph_id, num_glyphs);
    case 2: return u.format2.get_value (glyph_id);
    case 4: return u.format4.get_value (glyph_id);
    case 6: return u.format6.get_value (glyph_id);
    case 8: return u.format8.get_value (glyph_id);
    default:return nullptr;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (!u.format.sanitize (c)) return_trace (false);
    switch (u.format) {
    case 0: return_trace (u.format0.sanitize (c));
    case 2: return_trace (u.format2.sanitize (c));
    case 4: return_trace (u.format4.sanitize (c));
    case 6: return_trace (u.format6.sanitize (c));
    case 8: return_trace (u.format8.sanitize (c));
    default:return_trace (true);
    }
  }

  protected:
  union {
  UINT16		format;		/* Format identifier */
  LookupFormat0<T>	format0;
  LookupFormat2<T>	format2;
  LookupFormat4<T>	format4;
  LookupFormat6<T>	format6;
  LookupFormat8<T>	format8;
  } u;
  public:
  DEFINE_SIZE_UNION (2, format);
};


} /* namespace AAT */


#endif /* HB_AAT_LAYOUT_COMMON_PRIVATE_HH */
