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


/*
 * Binary Searching Tables
 */

struct BinSearchHeader
{
  friend struct BinSearchArray;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  protected:
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
  inline Type *bsearch (const T &key) const
  {
    return ::bsearch (&key, bytes, header.nUnits, header.unitSize, Type::cmp);
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


} /* namespace AAT */


#endif /* HB_AAT_LAYOUT_COMMON_PRIVATE_HH */
