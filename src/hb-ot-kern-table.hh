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
#include "hb-aat-layout-common.hh"


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

template <typename KernSubTableHeader>
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

  inline bool apply (AAT::hb_aat_apply_context_t *c) const
  {
    TRACE_APPLY (this);

    if (!c->plan->requested_kerning)
      return false;

    if (header.coverage & header.CrossStream)
      return false;

    hb_kern_machine_t<KernSubTableFormat0> machine (*this);
    machine.kern (c->font, c->buffer, c->plan->kern_mask);

    return_trace (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (pairs.sanitize (c));
  }

  protected:
  KernSubTableHeader		header;
  BinSearchArrayOf<KernPair>	pairs;	/* Array of kerning pairs. */
  public:
  DEFINE_SIZE_ARRAY (KernSubTableHeader::static_size + 8, pairs);
};

template <typename KernSubTableHeader>
struct KernSubTableFormat1
{
  typedef void EntryData;

  struct driver_context_t
  {
    static const bool in_place = true;
    enum Flags
    {
      Push		= 0x8000,	/* If set, push this glyph on the kerning stack. */
      DontAdvance	= 0x4000,	/* If set, don't advance to the next glyph
					 * before going to the new state. */
      Offset		= 0x3FFF,	/* Byte offset from beginning of subtable to the
					 * value table for the glyphs on the kerning stack. */
    };

    inline driver_context_t (const KernSubTableFormat1 *table_,
			     AAT::hb_aat_apply_context_t *c_) :
	c (c_),
	table (table_),
	/* Apparently the offset kernAction is from the beginning of the state-machine,
	 * similar to offsets in morx table, NOT from beginning of this table, like
	 * other subtables in kerx.  Discovered via testing. */
	kernAction (&table->machine + table->kernAction),
	depth (0),
	crossStream (table->header.coverage & table->header.CrossStream),
	crossOffset (0) {}

    inline bool is_actionable (AAT::StateTableDriver<AAT::MortTypes, EntryData> *driver HB_UNUSED,
			       const AAT::Entry<EntryData> *entry)
    {
      return entry->flags & Offset;
    }
    inline bool transition (AAT::StateTableDriver<AAT::MortTypes, EntryData> *driver,
			    const AAT::Entry<EntryData> *entry)
    {
      hb_buffer_t *buffer = driver->buffer;
      unsigned int flags = entry->flags;

      if (flags & Push)
      {
	if (likely (depth < ARRAY_LENGTH (stack)))
	  stack[depth++] = buffer->idx;
	else
	  depth = 0; /* Probably not what CoreText does, but better? */
      }

      if (entry->flags & Offset)
      {
	unsigned int kernIndex = AAT::MortTypes::offsetToIndex (entry->flags & Offset,
								&table->machine,
								kernAction.arrayZ);
	const FWORD *actions = &kernAction[kernIndex];
	if (!c->sanitizer.check_array (actions, depth))
	{
	  depth = 0;
	  return false;
	}

	hb_mask_t kern_mask = c->plan->kern_mask;
	for (unsigned int i = 0; i < depth; i++)
	{
	  /* Apparently, when spec says "Each pops one glyph from the kerning stack
	   * and applies the kerning value to it.", it doesn't mean it in that order.
	   * The deepest item in the stack corresponds to the first item in the action
	   * list.  Discovered by testing. */
	  unsigned int idx = stack[i];
	  int v = *actions++;

	  /* The following two flags are undocumented in the spec, but described
	   * in the example. */
	  bool last = v & 1;
	  v = v & ~1;
	  if (v == 0x8000)
	  {
	    crossOffset = 0;
	    v = 0;
	  }

	  if (idx < buffer->len && buffer->info[idx].mask & kern_mask)
	  {
	    if (HB_DIRECTION_IS_HORIZONTAL (buffer->props.direction))
	    {
	      if (crossStream)
	      {
	        /* XXX Why negative, not positive?!?! */
	        crossOffset -= v;
		buffer->pos[idx].y_offset += c->font->em_scale_y (crossOffset);
	      }
	      else
	      {
		buffer->pos[idx].x_advance += c->font->em_scale_x (v);
		if (HB_DIRECTION_IS_BACKWARD (buffer->props.direction))
		  buffer->pos[idx].x_offset += c->font->em_scale_x (v);
	      }
	    }
	    else
	    {
	      if (crossStream)
	      {
	        crossOffset += v;
		buffer->pos[idx].x_offset += c->font->em_scale_x (crossOffset);
	      }
	      else
	      {
		buffer->pos[idx].y_advance += c->font->em_scale_y (v);
		if (HB_DIRECTION_IS_BACKWARD (buffer->props.direction))
		  buffer->pos[idx].y_offset += c->font->em_scale_y (v);
	      }
	    }
	  }
	  if (last)
	    break;
	}
	depth = 0;
      }

      return true;
    }

    private:
    AAT::hb_aat_apply_context_t *c;
    const KernSubTableFormat1 *table;
    const UnsizedArrayOf<FWORD> &kernAction;
    unsigned int stack[8];
    unsigned int depth;
    bool crossStream;
    int crossOffset;
  };

  inline bool apply (AAT::hb_aat_apply_context_t *c) const
  {
    TRACE_APPLY (this);

    if (!c->plan->requested_kerning)
      return false;

    driver_context_t dc (this, c);

    AAT::StateTableDriver<AAT::MortTypes, EntryData> driver (machine, c->buffer, c->font->face);
    driver.drive (&dc);

    return_trace (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    /* The rest of array sanitizations are done at run-time. */
    return_trace (likely (c->check_struct (this) &&
			  machine.sanitize (c)));
  }

  protected:
  KernSubTableHeader					header;
  AAT::StateTable<AAT::MortTypes, EntryData>		machine;
  OffsetTo<UnsizedArrayOf<FWORD>, HBUINT16, false>	kernAction;
  public:
  DEFINE_SIZE_STATIC (KernSubTableHeader::static_size + 10);
};

template <typename KernSubTableHeader>
struct KernSubTableFormat2
{
  inline int get_kerning (hb_codepoint_t left, hb_codepoint_t right,
			  AAT::hb_aat_apply_context_t *c) const
  {
    /* Disabled until we find a font to test this.  Note that OT vs AAT specify
     * different ClassTable.  OT's has 16bit entries, while AAT has 8bit entries.
     * I've not seen any in the wild. */
    return 0;
    unsigned int l = (this+leftClassTable).get_class (left);
    unsigned int r = (this+rightClassTable).get_class (right);
    unsigned int offset = l + r;
    const FWORD *v = &StructAtOffset<FWORD> (&(this+array), offset);
    if (unlikely (!v->sanitize (&c->sanitizer))) return 0;
    return *v;
  }

  inline bool apply (AAT::hb_aat_apply_context_t *c) const
  {
    TRACE_APPLY (this);

    if (!c->plan->requested_kerning)
      return false;

    if (header.coverage & header.CrossStream)
      return false;

    accelerator_t accel (*this, c);
    hb_kern_machine_t<accelerator_t> machine (accel);
    machine.kern (c->font, c->buffer, c->plan->kern_mask);

    return_trace (true);
  }

  struct accelerator_t
  {
    const KernSubTableFormat2 &table;
    AAT::hb_aat_apply_context_t *c;

    inline accelerator_t (const KernSubTableFormat2 &table_,
			  AAT::hb_aat_apply_context_t *c_) :
			    table (table_), c (c_) {}

    inline int get_kerning (hb_codepoint_t left, hb_codepoint_t right) const
    { return table.get_kerning (left, right, c); }
  };

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (true); /* Disabled.  See above. */
    return_trace (c->check_struct (this) &&
		  leftClassTable.sanitize (c, this) &&
		  rightClassTable.sanitize (c, this) &&
		  array.sanitize (c, this));
  }

  protected:
  KernSubTableHeader		header;
  HBUINT16			rowWidth;	/* The width, in bytes, of a row in the table. */
  OffsetTo<AAT::ClassTable>	leftClassTable;	/* Offset from beginning of this subtable to
						 * left-hand class table. */
  OffsetTo<AAT::ClassTable>	rightClassTable;/* Offset from beginning of this subtable to
						 * right-hand class table. */
  OffsetTo<FWORD>		array;		/* Offset from beginning of this subtable to
						 * the start of the kerning array. */
  public:
  DEFINE_SIZE_MIN (KernSubTableHeader::static_size + 8);
};

template <typename KernSubTableHeader>
struct KernSubTableFormat3
{
  inline int get_kerning (hb_codepoint_t left, hb_codepoint_t right) const
  {
    hb_array_t<const FWORD> kernValue = kernValueZ.as_array (kernValueCount);
    hb_array_t<const HBUINT8> leftClass = StructAfter<const UnsizedArrayOf<HBUINT8> > (kernValue).as_array (glyphCount);
    hb_array_t<const HBUINT8> rightClass = StructAfter<const UnsizedArrayOf<HBUINT8> > (leftClass).as_array (glyphCount);
    hb_array_t<const HBUINT8> kernIndex = StructAfter<const UnsizedArrayOf<HBUINT8> > (rightClass).as_array (leftClassCount * rightClassCount);

    unsigned int leftC = leftClass[left];
    unsigned int rightC = rightClass[right];
    if (unlikely (leftC >= leftClassCount || rightC >= rightClassCount))
      return 0;
    unsigned int i = leftC * rightClassCount + rightC;
    return kernValue[kernIndex[i]];
  }

  inline bool apply (AAT::hb_aat_apply_context_t *c) const
  {
    TRACE_APPLY (this);

    if (!c->plan->requested_kerning)
      return false;

    if (header.coverage & header.CrossStream)
      return false;

    hb_kern_machine_t<KernSubTableFormat3> machine (*this);
    machine.kern (c->font, c->buffer, c->plan->kern_mask);

    return_trace (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  c->check_range (kernValueZ,
				  kernValueCount * sizeof (FWORD) +
				  glyphCount * 2 +
				  leftClassCount * rightClassCount));
  }

  protected:
  KernSubTableHeader	header;
  HBUINT16		glyphCount;	/* The number of glyphs in this font. */
  HBUINT8		kernValueCount;	/* The number of kerning values. */
  HBUINT8		leftClassCount;	/* The number of left-hand classes. */
  HBUINT8		rightClassCount;/* The number of right-hand classes. */
  HBUINT8		flags;		/* Set to zero (reserved for future use). */
  UnsizedArrayOf<FWORD>	kernValueZ;	/* The kerning values.
					 * Length kernValueCount. */
#if 0
  UnsizedArrayOf<HBUINT8>leftClass;	/* The left-hand classes.
					 * Length glyphCount. */
  UnsizedArrayOf<HBUINT8>rightClass;	/* The right-hand classes.
					 * Length glyphCount. */
  UnsizedArrayOf<HBUINT8>kernIndex;	/* The indices into the kernValue array.
					 * Length leftClassCount * rightClassCount */
#endif
  public:
  DEFINE_SIZE_ARRAY (KernSubTableHeader::static_size + 6, kernValueZ);
};

template <typename KernSubTableHeader>
struct KernSubTable
{
  inline unsigned int get_size (void) const { return u.header.length; }
  inline unsigned int get_type (void) const { return u.header.format; }

  inline int get_kerning (hb_codepoint_t left, hb_codepoint_t right) const
  {
    switch (get_type ()) {
    /* This method hooks up to hb_font_t's get_h_kerning.  Only support Format0. */
    case 0: return u.format0.get_kerning (left, right);
    default:return 0;
    }
  }

  template <typename context_t>
  inline typename context_t::return_t dispatch (context_t *c) const
  {
    unsigned int subtable_type = get_type ();
    TRACE_DISPATCH (this, subtable_type);
    switch (subtable_type) {
    case 0:	return_trace (c->dispatch (u.format0));
    case 1:	return_trace (c->dispatch (u.format1));
    case 2:	return_trace (c->dispatch (u.format2));
    case 3:	return_trace (c->dispatch (u.format3));
    default:	return_trace (c->default_return_value ());
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!u.header.sanitize (c) ||
		  u.header.length < u.header.min_size ||
		  !c->check_range (this, u.header.length))) return_trace (false);

    return_trace (dispatch (c));
  }

  public:
  union {
  KernSubTableHeader				header;
  KernSubTableFormat0<KernSubTableHeader>	format0;
  KernSubTableFormat1<KernSubTableHeader>	format1;
  KernSubTableFormat2<KernSubTableHeader>	format2;
  KernSubTableFormat3<KernSubTableHeader>	format3;
  } u;
  public:
  DEFINE_SIZE_MIN (0);
};


template <typename T>
struct KernTable
{
  /* https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern */
  inline const T* thiz (void) const { return static_cast<const T *> (this); }

  inline int get_h_kerning (hb_codepoint_t left, hb_codepoint_t right) const
  {
    typedef KernSubTable<typename T::SubTableHeader> SubTable;

    int v = 0;
    const SubTable *st = CastP<SubTable> (&thiz()->dataZ);
    unsigned int count = thiz()->nTables;
    for (unsigned int i = 0; i < count; i++)
    {
      if ((st->u.header.coverage &
	   (st->u.header.Variation | st->u.header.CrossStream | st->u.header.Direction)) !=
	  st->u.header.DirectionHorizontal)
        continue;
      if (st->u.header.coverage & st->u.header.Override)
        v = 0;
      v += st->get_kerning (left, right);
      st = &StructAfter<SubTable> (*st);
    }
    return v;
  }

  inline void apply (AAT::hb_aat_apply_context_t *c) const
  {
    typedef KernSubTable<typename T::SubTableHeader> SubTable;

    c->set_lookup_index (0);
    const SubTable *st = CastP<SubTable> (&thiz()->dataZ);
    unsigned int count = thiz()->nTables;
    /* If there's an override subtable, skip subtables before that. */
    unsigned int last_override = 0;
    for (unsigned int i = 0; i < count; i++)
    {
      if (!(st->u.header.coverage & (st->u.header.Variation | st->u.header.CrossStream)) &&
	  (st->u.header.coverage & st->u.header.Override))
        last_override = i;
      st = &StructAfter<SubTable> (*st);
    }
    st = CastP<SubTable> (&thiz()->dataZ);
    for (unsigned int i = 0; i < count; i++)
    {
      if (st->u.header.coverage & st->u.header.Variation)
        goto skip;

      if (HB_DIRECTION_IS_HORIZONTAL (c->buffer->props.direction) !=
	  ((st->u.header.coverage & st->u.header.Direction) == st->u.header.DirectionHorizontal))
	goto skip;

      if (i < last_override)
	goto skip;

      if (!c->buffer->message (c->font, "start kern subtable %d", c->lookup_index))
	goto skip;

      c->sanitizer.set_object (*st);

      st->dispatch (c);

      (void) c->buffer->message (c->font, "end kern subtable %d", c->lookup_index);

    skip:
      st = &StructAfter<SubTable> (*st);
      c->set_lookup_index (c->lookup_index + 1);
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!c->check_struct (thiz()) ||
		  thiz()->version != T::VERSION))
      return_trace (false);

    typedef KernSubTable<typename T::SubTableHeader> SubTable;

    const SubTable *st = CastP<SubTable> (&thiz()->dataZ);
    unsigned int count = thiz()->nTables;
    for (unsigned int i = 0; i < count; i++)
    {
      if (unlikely (!st->sanitize (c)))
	return_trace (false);
      st = &StructAfter<SubTable> (*st);
    }

    return_trace (true);
  }
};

struct KernOT : KernTable<KernOT>
{
  friend struct KernTable<KernOT>;

  static const uint16_t VERSION = 0x0000u;

  struct SubTableHeader
  {
    enum Coverage
    {
      Direction		= 0x01u,
      Minimum		= 0x02u,
      CrossStream	= 0x04u,
      Override		= 0x08u,

      Variation		= 0x00u, /* Not supported. */

      DirectionHorizontal= 0x01u
    };

    inline bool sanitize (hb_sanitize_context_t *c) const
    {
      TRACE_SANITIZE (this);
      return_trace (c->check_struct (this));
    }

    public:
    HBUINT16	versionZ;	/* Unused. */
    HBUINT16	length;		/* Length of the subtable (including this header). */
    HBUINT8	format;		/* Subtable format. */
    HBUINT8	coverage;	/* Coverage bits. */
    public:
    DEFINE_SIZE_STATIC (6);
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

  struct SubTableHeader
  {
    enum Coverage
    {
      Direction		= 0x80u,
      CrossStream	= 0x40u,
      Variation		= 0x20u,

      Override		= 0x00u, /* Not supported. */

      DirectionHorizontal= 0x00u
    };

    inline bool sanitize (hb_sanitize_context_t *c) const
    {
      TRACE_SANITIZE (this);
      return_trace (c->check_struct (this));
    }

    public:
    HBUINT32	length;		/* Length of the subtable (including this header). */
    HBUINT8	coverage;	/* Coverage bits. */
    HBUINT8	format;		/* Subtable format. */
    HBUINT16	tupleIndex;	/* The tuple index (used for variations fonts).
				 * This value specifies which tuple this subtable covers. */
    public:
    DEFINE_SIZE_STATIC (8);
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

  inline void apply (AAT::hb_aat_apply_context_t *c) const
  {
    /* TODO Switch to dispatch(). */
    switch (u.major) {
    case 0: u.ot.apply (c);  return;
    case 1: u.aat.apply (c); return;
    default:		     return;
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

} /* namespace OT */


#endif /* HB_OT_KERN_TABLE_HH */
