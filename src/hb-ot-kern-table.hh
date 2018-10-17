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

#ifndef HB_OT_KERN_TABLE_HH
#define HB_OT_KERN_TABLE_HH

#include "hb-open-type.hh"
#include "hb-ot-shape.hh"
#include "hb-ot-layout-gsubgpos.hh"


template <typename Driver>
struct hb_kern_machine_t
{
  hb_kern_machine_t (const Driver &driver_) : driver (driver_) {}

  HB_NO_SANITIZE_SIGNED_INTEGER_OVERFLOW
  inline void kern (hb_font_t   *font,
		    hb_buffer_t *buffer,
		    hb_mask_t    kern_mask,
		    bool         scale = true) const
  {
    OT::hb_ot_apply_context_t c (1, font, buffer);
    c.set_lookup_mask (kern_mask);
    c.set_lookup_props (OT::LookupFlag::IgnoreMarks);
    OT::hb_ot_apply_context_t::skipping_iterator_t &skippy_iter = c.iter_input;
    skippy_iter.init (&c);

    bool horizontal = HB_DIRECTION_IS_HORIZONTAL (buffer->props.direction);
    unsigned int count = buffer->len;
    hb_glyph_info_t *info = buffer->info;
    hb_glyph_position_t *pos = buffer->pos;
    for (unsigned int idx = 0; idx < count;)
    {
      if (!(info[idx].mask & kern_mask))
      {
	idx++;
	continue;
      }

      skippy_iter.reset (idx, 1);
      if (!skippy_iter.next ())
      {
	idx++;
	continue;
      }

      unsigned int i = idx;
      unsigned int j = skippy_iter.idx;

      hb_position_t kern = driver.get_kerning (info[i].codepoint,
					       info[j].codepoint);


      if (likely (!kern))
        goto skip;


      if (horizontal)
      {
        if (scale)
	  kern = font->em_scale_x (kern);
	hb_position_t kern1 = kern >> 1;
	hb_position_t kern2 = kern - kern1;
	pos[i].x_advance += kern1;
	pos[j].x_advance += kern2;
	pos[j].x_offset += kern2;
      }
      else
      {
        if (scale)
	  kern = font->em_scale_y (kern);
	hb_position_t kern1 = kern >> 1;
	hb_position_t kern2 = kern - kern1;
	pos[i].y_advance += kern1;
	pos[j].y_advance += kern2;
	pos[j].y_offset += kern2;
      }

      buffer->unsafe_to_break (i, j + 1);

    skip:
      idx = skippy_iter.idx;
    }
  }

  const Driver &driver;
};


/*
 * kern -- Kerning
 * https://docs.microsoft.com/en-us/typography/opentype/spec/kern
 * https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6kern.html
 */
#define HB_OT_TAG_kern HB_TAG('k','e','r','n')


namespace OT {


struct hb_glyph_pair_t
{
  hb_codepoint_t left;
  hb_codepoint_t right;
};

struct KernPair
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
  public:
  DEFINE_SIZE_STATIC (6);
};

struct KernSubTableFormat0
{
  inline int get_kerning (hb_codepoint_t left, hb_codepoint_t right) const
  {
    hb_glyph_pair_t pair = {left, right};
    int i = pairs.bsearch (pair);
    if (i == -1)
      return 0;
    return pairs[i].get_kerning ();
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (pairs.sanitize (c));
  }

  protected:
  BinSearchArrayOf<KernPair> pairs;	/* Array of kerning pairs. */
  public:
  DEFINE_SIZE_ARRAY (8, pairs);
};

struct KernClassTable
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

struct KernSubTableFormat2
{
  inline int get_kerning (hb_codepoint_t left, hb_codepoint_t right, const char *end) const
  {
    /* This subtable is disabled.  It's not cleaer to me *exactly* where the offests are
     * based from.  I *think* they should be based from beginning of kern subtable wrapper,
     * *NOT* "this".  Since we know of no fonts that use this subtable, we are disabling
     * it.  Someday fix it and re-enable.  Better yet, find fonts that use it... Meh,
     * Windows doesn't implement it.  Maybe just remove... */
    return 0;
    unsigned int l = (this+leftClassTable).get_class (left);
    unsigned int r = (this+rightClassTable).get_class (right);
    unsigned int offset = l + r;
    const FWORD *v = &StructAtOffset<FWORD> (&(this+array), offset);
    if (unlikely ((const char *) v < (const char *) &array ||
		  (const char *) v > (const char *) end - 2))
      return 0;
    return *v;
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (true); /* Disabled.  See above. */
    return_trace (rowWidth.sanitize (c) &&
		  leftClassTable.sanitize (c, this) &&
		  rightClassTable.sanitize (c, this) &&
		  array.sanitize (c, this));
  }

  protected:
  HBUINT16	rowWidth;	/* The width, in bytes, of a row in the table. */
  OffsetTo<KernClassTable>
		leftClassTable;	/* Offset from beginning of this subtable to
				 * left-hand class table. */
  OffsetTo<KernClassTable>
		rightClassTable;/* Offset from beginning of this subtable to
				 * right-hand class table. */
  OffsetTo<FWORD>
		array;		/* Offset from beginning of this subtable to
				 * the start of the kerning array. */
  public:
  DEFINE_SIZE_MIN (8);
};

struct KernSubTable
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
  KernSubTableFormat0	format0;
  KernSubTableFormat2	format2;
  } u;
  public:
  DEFINE_SIZE_MIN (0);
};


template <typename T>
struct KernSubTableWrapper
{
  /* https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern */
  inline const T* thiz (void) const { return static_cast<const T *> (this); }

  inline bool is_horizontal (void) const
  { return (thiz()->coverage & T::CheckFlags) == T::CheckHorizontal; }

  inline bool is_override (void) const
  { return bool (thiz()->coverage & T::Override); }

  inline int get_kerning (hb_codepoint_t left, hb_codepoint_t right, const char *end) const
  { return thiz()->subtable.get_kerning (left, right, end, thiz()->format); }

  inline int get_h_kerning (hb_codepoint_t left, hb_codepoint_t right, const char *end) const
  { return is_horizontal () ? get_kerning (left, right, end) : 0; }

  inline unsigned int get_size (void) const { return thiz()->length; }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (thiz()) &&
		  thiz()->length >= T::min_size &&
		  c->check_range (thiz(), thiz()->length) &&
		  thiz()->subtable.sanitize (c, thiz()->format));
  }
};

template <typename T>
struct KernTable
{
  /* https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern */
  inline const T* thiz (void) const { return static_cast<const T *> (this); }

  inline int get_h_kerning (hb_codepoint_t left, hb_codepoint_t right) const
  {
    int v = 0;
    const typename T::SubTableWrapper *st = CastP<typename T::SubTableWrapper> (&thiz()->dataZ);
    unsigned int count = thiz()->nTables;
    for (unsigned int i = 0; i < count; i++)
    {
      if (st->is_override ())
        v = 0;
      v += st->get_h_kerning (left, right, st->length + (const char *) st);
      st = &StructAfter<typename T::SubTableWrapper> (*st);
    }
    return v;
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!c->check_struct (thiz()) ||
		  thiz()->version != T::VERSION))
      return_trace (false);

    const typename T::SubTableWrapper *st = CastP<typename T::SubTableWrapper> (&thiz()->dataZ);
    unsigned int count = thiz()->nTables;
    for (unsigned int i = 0; i < count; i++)
    {
      if (unlikely (!st->sanitize (c)))
	return_trace (false);
      st = &StructAfter<typename T::SubTableWrapper> (*st);
    }

    return_trace (true);
  }
};

struct KernOT : KernTable<KernOT>
{
  friend struct KernTable<KernOT>;

  static const uint16_t VERSION = 0x0000u;

  struct SubTableWrapper : KernSubTableWrapper<SubTableWrapper>
  {
    friend struct KernTable<KernOT>;
    friend struct KernSubTableWrapper<SubTableWrapper>;

    enum Coverage
    {
      Direction		= 0x01u,
      Minimum		= 0x02u,
      CrossStream	= 0x04u,
      Override		= 0x08u,

      Variation		= 0x00u, /* Not supported. */

      CheckFlags	= 0x07u,
      CheckHorizontal	= 0x01u
    };

    protected:
    HBUINT16	versionZ;	/* Unused. */
    HBUINT16	length;		/* Length of the subtable (including this header). */
    HBUINT8	format;		/* Subtable format. */
    HBUINT8	coverage;	/* Coverage bits. */
    KernSubTable subtable;	/* Subtable data. */
    public:
    DEFINE_SIZE_MIN (6);
  };

  protected:
  HBUINT16			version;	/* Version--0x0000u */
  HBUINT16			nTables;	/* Number of subtables in the kerning table. */
  UnsizedArrayOf<HBUINT8>	dataZ;
  public:
  DEFINE_SIZE_ARRAY (4, dataZ);
};

struct KernAAT : KernTable<KernAAT>
{
  friend struct KernTable<KernAAT>;

  static const uint32_t VERSION = 0x00010000u;

  struct SubTableWrapper : KernSubTableWrapper<SubTableWrapper>
  {
    friend struct KernTable<KernAAT>;
    friend struct KernSubTableWrapper<SubTableWrapper>;

    enum Coverage
    {
      Direction		= 0x80u,
      CrossStream	= 0x40u,
      Variation		= 0x20u,

      Override		= 0x00u, /* Not supported. */

      CheckFlags	= 0xE0u,
      CheckHorizontal	= 0x00u
    };

    protected:
    HBUINT32	length;		/* Length of the subtable (including this header). */
    HBUINT8	coverage;	/* Coverage bits. */
    HBUINT8	format;		/* Subtable format. */
    HBUINT16	tupleIndex;	/* The tuple index (used for variations fonts).
				 * This value specifies which tuple this subtable covers. */
    KernSubTable subtable;	/* Subtable data. */
    public:
    DEFINE_SIZE_MIN (8);
  };

  protected:
  HBUINT32			version;	/* Version--0x00010000u */
  HBUINT32			nTables;	/* Number of subtables in the kerning table. */
  UnsizedArrayOf<HBUINT8>	dataZ;
  public:
  DEFINE_SIZE_ARRAY (8, dataZ);
};

struct kern
{
  static const hb_tag_t tableTag = HB_OT_TAG_kern;

  inline bool has_data (void) const
  { return u.version32 != 0; }

  inline int get_h_kerning (hb_codepoint_t left, hb_codepoint_t right) const
  {
    switch (u.major) {
    case 0: return u.ot.get_h_kerning (left, right);
    case 1: return u.aat.get_h_kerning (left, right);
    default:return 0;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (!u.version32.sanitize (c)) return_trace (false);
    switch (u.major) {
    case 0: return_trace (u.ot.sanitize (c));
    case 1: return_trace (u.aat.sanitize (c));
    default:return_trace (true);
    }
  }

  struct accelerator_t
  {
    inline void init (hb_face_t *face)
    {
      blob = hb_sanitize_context_t().reference_table<kern> (face);
      table = blob->as<kern> ();
    }
    inline void fini (void)
    {
      hb_blob_destroy (blob);
    }

    inline bool has_data (void) const
    { return table->has_data (); }

    inline int get_h_kerning (hb_codepoint_t left, hb_codepoint_t right) const
    { return table->get_h_kerning (left, right); }

    inline int get_kerning (hb_codepoint_t first, hb_codepoint_t second) const
    { return get_h_kerning (first, second); }

    inline void apply (hb_font_t *font,
		       hb_buffer_t  *buffer,
		       hb_mask_t kern_mask) const
    {
      /* We only apply horizontal kerning in this table. */
      if (!HB_DIRECTION_IS_HORIZONTAL (buffer->props.direction))
        return;

      hb_kern_machine_t<accelerator_t> machine (*this);

      machine.kern (font, buffer, kern_mask);
    }

    private:
    hb_blob_t *blob;
    const kern *table;
  };

  protected:
  union {
  HBUINT32		version32;
  HBUINT16		major;
  KernOT		ot;
  KernAAT		aat;
  } u;
  public:
  DEFINE_SIZE_UNION (4, version32);
};

struct kern_accelerator_t : kern::accelerator_t {};

} /* namespace OT */


#endif /* HB_OT_KERN_TABLE_HH */
