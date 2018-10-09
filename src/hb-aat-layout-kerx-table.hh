/*
 * Copyright © 2018  Ebrahim Byagowi
 * Copyright © 2018  Google, Inc.
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

#ifndef HB_AAT_LAYOUT_KERX_TABLE_HH
#define HB_AAT_LAYOUT_KERX_TABLE_HH

#include "hb-open-type.hh"
#include "hb-aat-layout-common.hh"
#include "hb-ot-kern-table.hh"

/*
 * kerx -- Extended Kerning
 * https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6kerx.html
 */
#define HB_AAT_TAG_kerx HB_TAG('k','e','r','x')


namespace AAT {

using namespace OT;


struct KerxSubTableFormat0
{
  inline int get_kerning (hb_codepoint_t left, hb_codepoint_t right) const
  {
    hb_glyph_pair_t pair = {left, right};
    int i = pairs.bsearch (pair);
    if (i == -1)
      return 0;
    return pairs[i].get_kerning ();
  }

  inline bool apply (hb_aat_apply_context_t *c) const
  {
    TRACE_APPLY (this);

    /* TODO */

    return_trace (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (pairs.sanitize (c)));
  }

  protected:
  BinSearchArrayOf<KernPair, HBUINT32>
		pairs;	/* Sorted kern records. */
  public:
  DEFINE_SIZE_ARRAY (16, pairs);
};

struct KerxSubTableFormat1
{
  inline bool apply (hb_aat_apply_context_t *c) const
  {
    TRACE_APPLY (this);

    /* TODO */

    return_trace (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) &&
			  stateHeader.sanitize (c)));
  }

  protected:
  StateTable<HBUINT16>		stateHeader;
  LOffsetTo<ArrayOf<HBUINT16> >	valueTable;
  public:
  DEFINE_SIZE_STATIC (20);
};

struct KerxSubTableFormat2
{
  inline int get_kerning (hb_codepoint_t left, hb_codepoint_t right,
			  const char *end, unsigned int num_glyphs) const
  {
    unsigned int l = *(this+leftClassTable).get_value (left, num_glyphs);
    unsigned int r = *(this+rightClassTable).get_value (right, num_glyphs);
    unsigned int offset = l + r;
    const FWORD *arr = &(this+array);
    if (unlikely ((const void *) arr < (const void *) this || (const void *) arr >= (const void *) end))
      return 0;
    const FWORD *v = &StructAtOffset<FWORD> (arr, offset);
    if (unlikely ((const void *) v < (const void *) arr || (const void *) (v + 1) > (const void *) end))
      return 0;
    return *v;
  }

  inline bool apply (hb_aat_apply_context_t *c) const
  {
    TRACE_APPLY (this);

    /* TODO */

    return_trace (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) &&
			  rowWidth.sanitize (c) &&
			  leftClassTable.sanitize (c, this) &&
			  rightClassTable.sanitize (c, this) &&
			  array.sanitize (c, this)));
  }

  protected:
  HBUINT32	rowWidth;	/* The width, in bytes, of a row in the table. */
  LOffsetTo<Lookup<HBUINT16> >
		leftClassTable;	/* Offset from beginning of this subtable to
				 * left-hand class table. */
  LOffsetTo<Lookup<HBUINT16> >
		rightClassTable;/* Offset from beginning of this subtable to
				 * right-hand class table. */
  LOffsetTo<FWORD>
		array;		/* Offset from beginning of this subtable to
				 * the start of the kerning array. */
  public:
  DEFINE_SIZE_STATIC (16);
};

struct KerxSubTableFormat4
{
  inline bool apply (hb_aat_apply_context_t *c) const
  {
    TRACE_APPLY (this);

    /* TODO */

    return_trace (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);

    /* TODO */
    return_trace (likely (c->check_struct (this)));
  }

  protected:
  public:
  DEFINE_SIZE_STATIC (1);
};

struct KerxSubTableFormat6
{
  inline bool apply (hb_aat_apply_context_t *c) const
  {
    TRACE_APPLY (this);

    /* TODO */

    return_trace (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) &&
			  rowIndexTable.sanitize (c, this) &&
			  columnIndexTable.sanitize (c, this) &&
			  kerningArray.sanitize (c, this) &&
			  kerningVector.sanitize (c, this)));
  }

  protected:
  HBUINT32	flags;
  HBUINT16	rowCount;
  HBUINT16	columnCount;
  LOffsetTo<Lookup<HBUINT16> >	rowIndexTable;
  LOffsetTo<Lookup<HBUINT16> >	columnIndexTable;
  LOffsetTo<Lookup<HBUINT16> >	kerningArray;
  LOffsetTo<Lookup<HBUINT16> >	kerningVector;
  public:
  DEFINE_SIZE_STATIC (24);
};

struct KerxTable
{
  friend kerx;

  inline unsigned int get_size (void) const { return length; }
  inline unsigned int get_type (void) const { return coverage & SubtableType; }

  enum Coverage
  {
    Vertical		= 0x80000000,	/* Set if table has vertical kerning values. */
    CrossStream		= 0x40000000,	/* Set if table has cross-stream kerning values. */
    Variation		= 0x20000000,	/* Set if table has variation kerning values. */
    ProcessDirection	= 0x10000000,	/* If clear, process the glyphs forwards, that
					 * is, from first to last in the glyph stream.
					 * If we, process them from last to first.
					 * This flag only applies to state-table based
					 * 'kerx' subtables (types 1 and 4). */
    Reserved		= 0x0FFFFF00,	/* Reserved, set to zero. */
    SubtableType	= 0x000000FF,	/* Subtable type. */
  };

  template <typename context_t>
  inline typename context_t::return_t dispatch (context_t *c) const
  {
    unsigned int subtable_type = get_type ();
    TRACE_DISPATCH (this, subtable_type);
    switch (subtable_type) {
    case 0	:		return_trace (c->dispatch (u.format0));
    case 1	:		return_trace (c->dispatch (u.format1));
    case 2	:		return_trace (c->dispatch (u.format2));
    case 4	:		return_trace (c->dispatch (u.format4));
    case 6	:		return_trace (c->dispatch (u.format6));
    default:			return_trace (c->default_return_value ());
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (!length.sanitize (c) ||
	length < min_size ||
	!c->check_range (this, length))
      return_trace (false);

    return_trace (dispatch (c));
  }

protected:
  HBUINT32	length;
  HBUINT32	coverage;
  HBUINT32	tupleCount;
  union {
  KerxSubTableFormat0	format0;
  KerxSubTableFormat1	format1;
  KerxSubTableFormat2	format2;
  KerxSubTableFormat4	format4;
  KerxSubTableFormat6	format6;
  } u;
public:
  DEFINE_SIZE_MIN (12);
};

struct SubtableXXX
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this)));
  }

  protected:
  HBUINT32	length;
  HBUINT32	coverage;
  HBUINT32	tupleCount;
  public:
  DEFINE_SIZE_STATIC (12);
};


/*
 * The 'kerx' Table
 */

struct kerx
{
  static const hb_tag_t tableTag = HB_AAT_TAG_kerx;

  inline bool has_data (void) const { return version != 0; }

  inline void apply (hb_aat_apply_context_t *c) const
  {
    c->set_lookup_index (0);
    const KerxTable *table = &firstTable;
    unsigned int count = tableCount;
    for (unsigned int i = 0; i < count; i++)
    {
      bool reverse;

      if (HB_DIRECTION_IS_VERTICAL (c->buffer->props.direction) !=
	  bool (table->coverage & KerxTable::Vertical))
        goto skip;

      if (table->coverage & KerxTable::CrossStream)
        goto skip; /* We do NOT handle cross-stream kerning. */

      reverse = bool (table->coverage & KerxTable::ProcessDirection) !=
		HB_DIRECTION_IS_BACKWARD (c->buffer->props.direction);

      if (!c->buffer->message (c->font, "start kerx subtable %d", c->lookup_index))
        goto skip;

      if (reverse)
        c->buffer->reverse ();

      table->dispatch (c);

      if (reverse)
        c->buffer->reverse ();

      (void) c->buffer->message (c->font, "end kerx subtable %d", c->lookup_index);

    skip:
      table = &StructAfter<KerxTable> (*table);
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (!version.sanitize (c) || version < 2 ||
	!tableCount.sanitize (c))
      return_trace (false);

    const KerxTable *table = &firstTable;
    unsigned int count = tableCount;
    for (unsigned int i = 0; i < count; i++)
    {
      if (!table->sanitize (c))
	return_trace (false);
      table = &StructAfter<KerxTable> (*table);
    }

    return_trace (true);
  }

  protected:
  HBUINT16	version;	/* The version number of the extended kerning table
				 * (currently 2, 3, or 4). */
  HBUINT16	unused;		/* Set to 0. */
  HBUINT32	tableCount;	/* The number of subtables included in the extended kerning
				 * table. */
  KerxTable	firstTable;	/* Subtables. */
/*subtableGlyphCoverageArray*/	/* Only if version >= 3. We don't use. */

  public:
  DEFINE_SIZE_MIN (8);
};

} /* namespace AAT */


#endif /* HB_AAT_LAYOUT_KERX_TABLE_HH */
