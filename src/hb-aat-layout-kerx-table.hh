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
#include "hb-ot-layout-gpos-table.hh"
#include "hb-ot-kern-table.hh"

/*
 * kerx -- Extended Kerning
 * https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6kerx.html
 */
#define HB_AAT_TAG_kerx HB_TAG('k','e','r','x')


namespace AAT {

using namespace OT;


static inline int
kerxTupleKern (int value,
	       unsigned int tupleCount,
	       const void *base,
	       hb_aat_apply_context_t *c)
{
  if (likely (!tupleCount)) return value;

  unsigned int offset = value;
  const FWORD *pv = &StructAtOffset<FWORD> (base, offset);
  if (unlikely (!pv->sanitize (&c->sanitizer))) return 0;
  return *pv;
}


struct KerxSubTableHeader
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this)));
  }

  public:
  HBUINT32	length;
  HBUINT32	coverage;
  HBUINT32	tupleCount;
  public:
  DEFINE_SIZE_STATIC (12);
};

struct KerxSubTableFormat0
{
  inline int get_kerning (hb_codepoint_t left, hb_codepoint_t right) const
  {
    if (header.tupleCount) return 0; /* TODO kerxTupleKern */
    hb_glyph_pair_t pair = {left, right};
    int i = pairs.bsearch (pair);
    return i == -1 ? 0 : pairs[i].get_kerning ();
  }

  inline bool apply (hb_aat_apply_context_t *c) const
  {
    TRACE_APPLY (this);

    if (!c->plan->requested_kerning)
      return false;

    hb_kern_machine_t<KerxSubTableFormat0> machine (*this);

    machine.kern (c->font, c->buffer, c->plan->kern_mask);

    return_trace (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) &&
			  pairs.sanitize (c)));
  }

  protected:
  KerxSubTableHeader	header;
  BinSearchArrayOf<KernPair, HBUINT32>
			pairs;	/* Sorted kern records. */
  public:
  DEFINE_SIZE_ARRAY (28, pairs);
};

struct KerxSubTableFormat1
{
  struct EntryData
  {
    HBUINT16	kernActionIndex;/* Index into the kerning value array. If
				 * this index is 0xFFFF, then no kerning
				 * is to be performed. */
    public:
    DEFINE_SIZE_STATIC (2);
  };

  struct driver_context_t
  {
    static const bool in_place = true;
    enum Flags
    {
      Push		= 0x8000,	/* If set, push this glyph on the kerning stack. */
      DontAdvance	= 0x4000,	/* If set, don't advance to the next glyph
					 * before going to the new state. */
      Reset		= 0x2000,	/* If set, reset the kerning data (clear the stack) */
      Reserved		= 0x1FFF,	/* Not used; set to 0. */
    };

    inline driver_context_t (const KerxSubTableFormat1 *table,
			     hb_aat_apply_context_t *c_) :
	c (c_),
	/* Apparently the offset kernAction is from the beginning of the state-machine,
	 * similar to offsets in morx table, NOT from beginning of this table, like
	 * other subtables in kerx.  Discovered via testing. */
	kernAction (&table->machine + table->kernAction),
	depth (0) {}

    inline bool is_actionable (StateTableDriver<EntryData> *driver,
			       const Entry<EntryData> *entry)
    {
      return entry->data.kernActionIndex != 0xFFFF;
    }
    inline bool transition (StateTableDriver<EntryData> *driver,
			    const Entry<EntryData> *entry)
    {
      hb_buffer_t *buffer = driver->buffer;
      unsigned int flags = entry->flags;

      if (flags & Reset)
      {
	depth = 0;
      }

      if (flags & Push)
      {
	if (likely (depth < ARRAY_LENGTH (stack)))
	  stack[depth++] = buffer->idx;
	else
	  depth = 0; /* Probably not what CoreText does, but better? */
      }

      if (entry->data.kernActionIndex != 0xFFFF)
      {
	const FWORD *actions = &kernAction[entry->data.kernActionIndex];
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
	  if (idx < buffer->len && buffer->info[idx].mask & kern_mask)
	  {
	    /* XXX Non-forward direction... */
	    if (HB_DIRECTION_IS_HORIZONTAL (buffer->props.direction))
	      buffer->pos[idx].x_advance += c->font->em_scale_x (v);
	    else
	      buffer->pos[idx].y_advance += c->font->em_scale_y (v);
	  }
	}
	depth = 0;
      }

      return true;
    }

    private:
    hb_aat_apply_context_t *c;
    const UnsizedArrayOf<FWORD> &kernAction;
    unsigned int stack[8];
    unsigned int depth;
  };

  inline bool apply (hb_aat_apply_context_t *c) const
  {
    TRACE_APPLY (this);

    if (!c->plan->requested_kerning)
      return false;

    if (header.tupleCount)
      return_trace (false); /* TODO kerxTupleKern */

    driver_context_t dc (this, c);

    StateTableDriver<EntryData> driver (machine, c->buffer, c->font->face);
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
  KerxSubTableHeader				header;
  StateTable<EntryData>				machine;
  LOffsetTo<UnsizedArrayOf<FWORD>, false>	kernAction;
  public:
  DEFINE_SIZE_STATIC (32);
};

struct KerxSubTableFormat2
{
  inline int get_kerning (hb_codepoint_t left, hb_codepoint_t right,
			  hb_aat_apply_context_t *c) const
  {
    unsigned int num_glyphs = c->sanitizer.get_num_glyphs ();
    unsigned int l = (this+leftClassTable).get_value_or_null (left, num_glyphs);
    unsigned int r = (this+rightClassTable).get_value_or_null (right, num_glyphs);
    unsigned int offset = l + r;
    const FWORD *v = &StructAtOffset<FWORD> (&(this+array), offset);
    if (unlikely (!v->sanitize (&c->sanitizer))) return 0;
    return kerxTupleKern (*v, header.tupleCount, this, c);
  }

  inline bool apply (hb_aat_apply_context_t *c) const
  {
    TRACE_APPLY (this);

    if (!c->plan->requested_kerning)
      return false;

    accelerator_t accel (*this, c);
    hb_kern_machine_t<accelerator_t> machine (accel);
    machine.kern (c->font, c->buffer, c->plan->kern_mask);

    return_trace (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) &&
			  leftClassTable.sanitize (c, this) &&
			  rightClassTable.sanitize (c, this) &&
			  c->check_range (this, array)));
  }

  struct accelerator_t
  {
    const KerxSubTableFormat2 &table;
    hb_aat_apply_context_t *c;

    inline accelerator_t (const KerxSubTableFormat2 &table_,
			  hb_aat_apply_context_t *c_) :
			    table (table_), c (c_) {}

    inline int get_kerning (hb_codepoint_t left, hb_codepoint_t right) const
    { return table.get_kerning (left, right, c); }
  };

  protected:
  KerxSubTableHeader	header;
  HBUINT32		rowWidth;	/* The width, in bytes, of a row in the table. */
  LOffsetTo<Lookup<HBUINT16>, false>
			leftClassTable;	/* Offset from beginning of this subtable to
					 * left-hand class table. */
  LOffsetTo<Lookup<HBUINT16>, false>
			rightClassTable;/* Offset from beginning of this subtable to
					 * right-hand class table. */
  LOffsetTo<UnsizedArrayOf<FWORD>, false>
			 array;		/* Offset from beginning of this subtable to
					 * the start of the kerning array. */
  public:
  DEFINE_SIZE_STATIC (28);
};

struct KerxSubTableFormat4
{
  struct EntryData
  {
    HBUINT16	ankrActionIndex;/* Either 0xFFFF (for no action) or the index of
				 * the action to perform. */
    public:
    DEFINE_SIZE_STATIC (2);
  };

  struct driver_context_t
  {
    static const bool in_place = true;
    enum Flags
    {
      Mark		= 0x8000,	/* If set, remember this glyph as the marked glyph. */
      DontAdvance	= 0x4000,	/* If set, don't advance to the next glyph before
					 * going to the new state. */
      Reserved		= 0x3FFF,	/* Not used; set to 0. */
    };

    enum SubTableFlags
    {
      ActionType	= 0xC0000000,	/* A two-bit field containing the action type. */
      Unused		= 0x3F000000,	/* Unused - must be zero. */
      Offset		= 0x00FFFFFF,	/* Masks the offset in bytes from the beginning
					 * of the subtable to the beginning of the control
					 * point table. */
    };

    inline driver_context_t (const KerxSubTableFormat4 *table,
			     hb_aat_apply_context_t *c_) :
	c (c_),
	action_type ((table->flags & ActionType) >> 30),
	ankrData ((HBUINT16 *) ((const char *) &table->machine + (table->flags & Offset))),
	mark_set (false),
	mark (0) {}

    inline bool is_actionable (StateTableDriver<EntryData> *driver,
			       const Entry<EntryData> *entry)
    {
      return entry->data.ankrActionIndex != 0xFFFF;
    }
    inline bool transition (StateTableDriver<EntryData> *driver,
			    const Entry<EntryData> *entry)
    {
      hb_buffer_t *buffer = driver->buffer;
      unsigned int flags = entry->flags;

      if (mark_set && entry->data.ankrActionIndex != 0xFFFF && buffer->idx < buffer->len)
      {
	hb_glyph_position_t &o = buffer->cur_pos();
	switch (action_type)
	{
	  case 0: /* Control Point Actions.*/
	  {
	    /* indexed into glyph outline. */
	    const HBUINT16 *data = &ankrData[entry->data.ankrActionIndex];
	    if (!c->sanitizer.check_array (data, 2))
	      return false;
	    HB_UNUSED unsigned int markControlPoint = *data++;
	    HB_UNUSED unsigned int currControlPoint = *data++;
	    hb_position_t markX = 0;
	    hb_position_t markY = 0;
	    hb_position_t currX = 0;
	    hb_position_t currY = 0;
	    if (!c->font->get_glyph_contour_point_for_origin (c->buffer->info[mark].codepoint,
							      markControlPoint,
							      HB_DIRECTION_LTR /*XXX*/,
							      &markX, &markY) ||
		!c->font->get_glyph_contour_point_for_origin (c->buffer->cur ().codepoint,
							      currControlPoint,
							      HB_DIRECTION_LTR /*XXX*/,
							      &currX, &currY))
	      return true; /* True, such that the machine continues. */

	    o.x_offset = markX - currX;
	    o.y_offset = markY - currY;
	  }
	  break;

	  case 1: /* Anchor Point Actions. */
	  {
	   /* Indexed into 'ankr' table. */
	    const HBUINT16 *data = &ankrData[entry->data.ankrActionIndex];
	    if (!c->sanitizer.check_array (data, 2))
	      return false;
	    unsigned int markAnchorPoint = *data++;
	    unsigned int currAnchorPoint = *data++;
	    const Anchor markAnchor = c->ankr_table.get_anchor (c->buffer->info[mark].codepoint,
								markAnchorPoint,
								c->sanitizer.get_num_glyphs (),
								c->ankr_end);
	    const Anchor currAnchor = c->ankr_table.get_anchor (c->buffer->cur ().codepoint,
								currAnchorPoint,
								c->sanitizer.get_num_glyphs (),
								c->ankr_end);

	    o.x_offset = c->font->em_scale_x (markAnchor.xCoordinate) - c->font->em_scale_x (currAnchor.xCoordinate);
	    o.y_offset = c->font->em_scale_y (markAnchor.yCoordinate) - c->font->em_scale_y (currAnchor.yCoordinate);
	  }
	  break;

	  case 2: /* Control Point Coordinate Actions. */
	  {
	    const FWORD *data = (const FWORD *) &ankrData[entry->data.ankrActionIndex];
	    if (!c->sanitizer.check_array (data, 4))
	      return false;
	    int markX = *data++;
	    int markY = *data++;
	    int currX = *data++;
	    int currY = *data++;

	    o.x_offset = c->font->em_scale_x (markX) - c->font->em_scale_x (currX);
	    o.y_offset = c->font->em_scale_y (markY) - c->font->em_scale_y (currY);
	  }
	  break;
	}
	o.attach_type() = ATTACH_TYPE_MARK;
	o.attach_chain() = (int) mark - (int) buffer->idx;
	buffer->scratch_flags |= HB_BUFFER_SCRATCH_FLAG_HAS_GPOS_ATTACHMENT;
      }

      if (flags & Mark)
      {
	mark_set = true;
	mark = buffer->idx;
      }

      return true;
    }

    private:
    hb_aat_apply_context_t *c;
    unsigned int action_type;
    const HBUINT16 *ankrData;
    bool mark_set;
    unsigned int mark;
  };

  inline bool apply (hb_aat_apply_context_t *c) const
  {
    TRACE_APPLY (this);

    driver_context_t dc (this, c);

    StateTableDriver<EntryData> driver (machine, c->buffer, c->font->face);
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
  KerxSubTableHeader	header;
  StateTable<EntryData>	machine;
  HBUINT32		flags;
  public:
  DEFINE_SIZE_STATIC (32);
};

struct KerxSubTableFormat6
{
  enum Flags
  {
    ValuesAreLong	= 0x00000001,
  };

  inline bool is_long (void) const { return flags & ValuesAreLong; }

  inline int get_kerning (hb_codepoint_t left, hb_codepoint_t right,
			  hb_aat_apply_context_t *c) const
  {
    unsigned int num_glyphs = c->sanitizer.get_num_glyphs ();
    if (is_long ())
    {
      const U::Long &t = u.l;
      unsigned int l = (this+t.rowIndexTable).get_value_or_null (left, num_glyphs);
      unsigned int r = (this+t.columnIndexTable).get_value_or_null (right, num_glyphs);
      unsigned int offset = l + r;
      if (unlikely (offset < l)) return 0; /* Addition overflow. */
      if (unlikely (hb_unsigned_mul_overflows (offset, sizeof (FWORD32)))) return 0;
      const FWORD32 *v = &StructAtOffset<FWORD32> (&(this+t.array), offset * sizeof (FWORD32));
      if (unlikely (!v->sanitize (&c->sanitizer))) return 0;
      return kerxTupleKern (*v, header.tupleCount, &(this+vector), c);
    }
    else
    {
      const U::Short &t = u.s;
      unsigned int l = (this+t.rowIndexTable).get_value_or_null (left, num_glyphs);
      unsigned int r = (this+t.columnIndexTable).get_value_or_null (right, num_glyphs);
      unsigned int offset = l + r;
      const FWORD *v = &StructAtOffset<FWORD> (&(this+t.array), offset * sizeof (FWORD));
      if (unlikely (!v->sanitize (&c->sanitizer))) return 0;
      return kerxTupleKern (*v, header.tupleCount, &(this+vector), c);
    }
  }

  inline bool apply (hb_aat_apply_context_t *c) const
  {
    TRACE_APPLY (this);

    if (!c->plan->requested_kerning)
      return false;

    accelerator_t accel (*this, c);
    hb_kern_machine_t<accelerator_t> machine (accel);
    machine.kern (c->font, c->buffer, c->plan->kern_mask);

    return_trace (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) &&
			  (is_long () ?
			   (
			     u.l.rowIndexTable.sanitize (c, this) &&
			     u.l.columnIndexTable.sanitize (c, this) &&
			     c->check_range (this, u.l.array)
			   ) : (
			     u.s.rowIndexTable.sanitize (c, this) &&
			     u.s.columnIndexTable.sanitize (c, this) &&
			     c->check_range (this, u.s.array)
			   )) &&
			  (header.tupleCount == 0 ||
			   c->check_range (this, vector))));
  }

  struct accelerator_t
  {
    const KerxSubTableFormat6 &table;
    hb_aat_apply_context_t *c;

    inline accelerator_t (const KerxSubTableFormat6 &table_,
			  hb_aat_apply_context_t *c_) :
			    table (table_), c (c_) {}

    inline int get_kerning (hb_codepoint_t left, hb_codepoint_t right) const
    { return table.get_kerning (left, right, c); }
  };

  protected:
  KerxSubTableHeader		header;
  HBUINT32			flags;
  HBUINT16			rowCount;
  HBUINT16			columnCount;
  union U
  {
    struct Long
    {
      LOffsetTo<Lookup<HBUINT32>, false>	rowIndexTable;
      LOffsetTo<Lookup<HBUINT32>, false>	columnIndexTable;
      LOffsetTo<UnsizedArrayOf<FWORD32>, false>	array;
    } l;
    struct Short
    {
      LOffsetTo<Lookup<HBUINT16>, false>	rowIndexTable;
      LOffsetTo<Lookup<HBUINT16>, false>	columnIndexTable;
      LOffsetTo<UnsizedArrayOf<FWORD>, false>	array;
    } s;
  } u;
  LOffsetTo<UnsizedArrayOf<FWORD>, false>	vector;
  public:
  DEFINE_SIZE_STATIC (36);
};

struct KerxTable
{
  friend struct kerx;

  inline unsigned int get_size (void) const { return u.header.length; }
  inline unsigned int get_type (void) const { return u.header.coverage & SubtableType; }

  enum Coverage
  {
    Vertical		= 0x80000000,	/* Set if table has vertical kerning values. */
    CrossStream		= 0x40000000,	/* Set if table has cross-stream kerning values. */
    Variation		= 0x20000000,	/* Set if table has variation kerning values. */
    Backwards		= 0x10000000,	/* If clear, process the glyphs forwards, that
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
    if (!u.header.sanitize (c) ||
	!c->check_range (this, u.header.length))
      return_trace (false);

    return_trace (dispatch (c));
  }

protected:
  union {
  KerxSubTableHeader	header;
  KerxSubTableFormat0	format0;
  KerxSubTableFormat1	format1;
  KerxSubTableFormat2	format2;
  KerxSubTableFormat4	format4;
  KerxSubTableFormat6	format6;
  } u;
public:
  DEFINE_SIZE_MIN (12);
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

      if (table->u.header.coverage & (KerxTable::CrossStream))
	goto skip; /* We do NOT handle cross-stream. */

      if (HB_DIRECTION_IS_VERTICAL (c->buffer->props.direction) !=
	  bool (table->u.header.coverage & KerxTable::Vertical))
	goto skip;

      reverse = bool (table->u.header.coverage & KerxTable::Backwards) !=
		HB_DIRECTION_IS_BACKWARD (c->buffer->props.direction);

      if (!c->buffer->message (c->font, "start kerx subtable %d", c->lookup_index))
	goto skip;

      if (reverse)
	c->buffer->reverse ();

      c->sanitizer.set_object (*table);

      /* XXX Reverse-kern is not working yet...
       * hb_kern_machine_t would need to know that it's reverse-kerning.
       * Or better yet, make it work in reverse as well, so we don't have
       * to reverse and reverse back? */
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
