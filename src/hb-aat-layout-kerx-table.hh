/*
 * Copyright © 2018  Google, Inc.
 * Copyright © 2018  Ebrahim Byagowi
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

#include "hb-aat-layout-common-private.hh"

namespace AAT {


/*
 * kerx -- Kerning
 */

#define HB_AAT_TAG_kerx HB_TAG('k','e','r','x')

struct hb_glyph_pair_t
{
  hb_codepoint_t left;
  hb_codepoint_t right;
};

struct KerxPair
{
  inline int get_kerning (void) const
  { return value; }

  inline int cmp (const hb_glyph_pair_t &o) const
  {
    int ret = left.cmp (o.left);
    if (ret) return ret;
    return right.cmp (o.right);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  protected:
  GlyphID	left;
  GlyphID	right;
  FWORD		value;
  HBUINT16 pad;
  public:
  DEFINE_SIZE_STATIC (8);
};

struct KerxSubTableFormat0
{
  inline int get_kerning (hb_codepoint_t left, hb_codepoint_t right) const
  {
    //hb_glyph_pair_t pair = {left, right};
    //int i = pairs.bsearch (pair);
    //if (i == -1)
      return 0;
    //return pairs[i].get_kerning ();
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (pairs.sanitize (c));
  }

  protected:
  BinSearchArrayOf<KerxPair> pairs;	/* Array of kerning pairs. */
  //FIXME: BinSearchArrayOf and its BinSearchHeader should be
  //modified in a way to accept uint32s
  public:
  //DEFINE_SIZE_ARRAY (16, pairs);
};

struct KerxAction
{
  HBUINT16 index;
};

struct KerxSubTableFormat1
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    //TRACE_SANITIZE (this);
    //return_trace (stateHeader.sanitize (c));
    return false;
  }

  protected:
  StateTable<KerxAction> stateHeader;
  OffsetTo<ArrayOf<HBUINT16>, HBUINT32> valueTable;

  public:
  //DEFINE_SIZE_MIN (4);
};

//FIXME: Maybe this can be replaced with Lookup<HBUINT16>?
struct KerxClassTable
{
  inline unsigned int get_class (hb_codepoint_t g) const { return classes[g - firstGlyph]; }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (firstGlyph.sanitize (c) && classes.sanitize (c));
  }

  protected:
  HBUINT16		firstGlyph;	/* First glyph in class range. */
  ArrayOf<HBUINT16>	classes;	/* Glyph classes. */
  public:
  DEFINE_SIZE_ARRAY (4, classes);
};

struct KerxSubTableFormat2
{
  inline int get_kerning (hb_codepoint_t left, hb_codepoint_t right, const char *end) const
  {
    unsigned int l = (this+leftClassTable).get_class (left);
    unsigned int r = (this+leftClassTable).get_class (left);
    unsigned int offset = l * rowWidth + r * sizeof (FWORD);
    const FWORD *arr = &(this+array);
    if (unlikely ((const void *) arr < (const void *) this || (const void *) arr >= (const void *) end))
      return 0;
    const FWORD *v = &StructAtOffset<FWORD> (arr, offset);
    if (unlikely ((const void *) v < (const void *) arr || (const void *) (v + 1) > (const void *) end))
      return 0;
    return *v;
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (rowWidth.sanitize (c) &&
		  leftClassTable.sanitize (c, this) &&
		  rightClassTable.sanitize (c, this) &&
		  array.sanitize (c, this));
  }

  protected:
  HBUINT32	rowWidth;	/* The width, in bytes, of a row in the table. */
  LOffsetTo<KerxClassTable>
		leftClassTable;	/* Offset from beginning of this subtable to
				 * left-hand class table. */
  LOffsetTo<KerxClassTable>
		rightClassTable;/* Offset from beginning of this subtable to
				 * right-hand class table. */
  LOffsetTo<FWORD>
		array;		/* Offset from beginning of this subtable to
				 * the start of the kerning array. */
  public:
  DEFINE_SIZE_MIN (16);
};

struct KerxSubTableFormat4
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (rowWidth.sanitize (c) &&
		  leftClassTable.sanitize (c, this) &&
		  rightClassTable.sanitize (c, this) &&
		  array.sanitize (c, this));
  }

  protected:
  HBUINT32	rowWidth;	/* The width, in bytes, of a row in the table. */
  LOffsetTo<KerxClassTable>
		leftClassTable;	/* Offset from beginning of this subtable to
				 * left-hand class table. */
  LOffsetTo<KerxClassTable>
		rightClassTable;/* Offset from beginning of this subtable to
				 * right-hand class table. */
  LOffsetTo<FWORD>
		array;		/* Offset from beginning of this subtable to
				 * the start of the kerning array. */
  public:
  DEFINE_SIZE_MIN (16);
};

struct KerxSubTableFormat6
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    //TRACE_SANITIZE (this);
    //return_trace ;
    return false;
  }

  protected:
  HBUINT32 flags;
  HBUINT16 rowCount;
  HBUINT16 columnCount;
  HBUINT32 rowIndexTableOffset;
  HBUINT32 columnIndexTableOffset;
  HBUINT32 kerningArrayOffset;
  HBUINT32 kerningVectorOffset;

  public:
  DEFINE_SIZE_MIN (24);
};

struct KerxSubTable
{
  inline int get_kerning (hb_codepoint_t left, hb_codepoint_t right, const char *end, unsigned int format) const
  {
    switch (format) {
    case 0: return u.format0.get_kerning (left, right);
    case 2: return u.format2.get_kerning (left, right, end);
    default:return 0;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c, unsigned int format) const
  {
    TRACE_SANITIZE (this);
    switch (format) {
    case 0: return_trace (u.format0.sanitize (c));
    case 2: return_trace (u.format2.sanitize (c));
    default:return_trace (true);
    }
  }

  protected:
  union {
  KerxSubTableFormat0	format0;
  KerxSubTableFormat2	format2;
  KerxSubTableFormat4	format4;
  KerxSubTableFormat6	format6;
  } u;
  public:
  DEFINE_SIZE_MIN (0);
};



struct kerx
{
  static const hb_tag_t tableTag = HB_AAT_TAG_kerx;

  inline bool apply (hb_aat_apply_context_t *c, const AAT::ankr *ankr) const
  {
    TRACE_APPLY (this);
    /* TODO */
    return_trace (false);
  }

  struct SubTableWrapper
  {
    enum coverage_flags_t {
      COVERAGE_VERTICAL_FLAG	= 0x8000u,
      COVERAGE_CROSSSTREAM_FLAG	= 0x4000u,
      COVERAGE_VARIATION_FLAG	= 0x2000u,

      COVERAGE_OVERRIDE_FLAG	= 0x0000u, /* Not supported. */

      COVERAGE_CHECK_FLAGS	= 0x0700u, //FIXME: Where these two come from?
      COVERAGE_CHECK_HORIZONTAL	= 0x0100u
    };

    protected:
    HBUINT32	length;		/* Length of the subtable (including this header). */
    HBUINT16	coverage;	/* Coverage bits. */
    HBUINT16	format;		/* Subtable format. */
    HBUINT32	tupleIndex;	/* The tuple index (used for variations fonts).
				 * This value specifies which tuple this subtable covers. */
    KerxSubTable subtable;	/* Subtable data. */
    public:
    inline bool is_horizontal (void) const
    { return (coverage & COVERAGE_CHECK_FLAGS) == COVERAGE_CHECK_HORIZONTAL; }

    inline bool is_override (void) const
    { return bool (coverage & COVERAGE_OVERRIDE_FLAG); }

    inline int get_kerning (hb_codepoint_t left, hb_codepoint_t right, const char *end) const
    { return subtable.get_kerning (left, right, end, format); }

    inline int get_h_kerning (hb_codepoint_t left, hb_codepoint_t right, const char *end) const
    { return is_horizontal () ? get_kerning (left, right, end) : 0; }

    inline unsigned int get_size (void) const { return length; }

    inline bool sanitize (hb_sanitize_context_t *c) const
    {
      TRACE_SANITIZE (this);
      return_trace (c->check_struct (this) &&
        length >= min_size &&
        c->check_array (this, 1, length) &&
        subtable.sanitize (c, format));
    }
    DEFINE_SIZE_MIN (12);
  };

  inline int get_h_kerning (hb_codepoint_t left, hb_codepoint_t right, unsigned int table_length) const
  {
    int v = 0;
    const SubTableWrapper *st = (SubTableWrapper *) data;
    unsigned int count = nTables;
    for (unsigned int i = 0; i < count; i++)
    {
      if (st->is_override ())
        v = 0;
      v += st->get_h_kerning (left, right, table_length + (const char *) this);
      st = (SubTableWrapper *) st;
    }
    return v;
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    const SubTableWrapper *st = (SubTableWrapper *) data;
    unsigned int count = nTables;
    for (unsigned int i = 0; i < count; i++)
    {
      if (unlikely (!st->sanitize (c)))
	return_trace (false);
      st = (SubTableWrapper *) st;
    }

    return_trace (true);
  }

  struct accelerator_t
  {
    inline void init (hb_face_t *face)
    {
      blob = Sanitizer<kerx>().sanitize (face->reference_table (HB_AAT_TAG_kerx));
      table = Sanitizer<kerx>::lock_instance (blob);
      table_length = hb_blob_get_length (blob);
    }
    inline void fini (void)
    {
      hb_blob_destroy (blob);
    }

    inline int get_h_kerning (hb_codepoint_t left, hb_codepoint_t right) const
    { return table->get_h_kerning (left, right, table_length); }

    private:
    hb_blob_t *blob;
    const kerx *table;
    unsigned int table_length;
  };

  protected:
  HBUINT16		version;
  HBUINT16		padding;
  HBUINT32		nTables;	/* Number of subtables in the kerning table. */
  HBUINT8		data[VAR];
  //ArrayOf<GlyphCoverageArray> subtableGlyphCoverageArray;
  public:
  DEFINE_SIZE_ARRAY (8, data);
};

} /* namespace AAT */


#endif /* HB_AAT_LAYOUT_KERX_TABLE_HH */
