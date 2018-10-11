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
    return_trace (likely (pairs.sanitize (c)));
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
    HBUINT16	ligActionIndex;	/* Index to the first ligActionTable entry
				 * for processing this group, if indicated
				 * by the flags. */
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

    inline driver_context_t (const KerxSubTableFormat1 *table)
	{}

    inline bool is_actionable (StateTableDriver<EntryData> *driver,
			       const Entry<EntryData> *entry)
    {
      return false; // XXX return (entry->flags & Verb) && start < end;
    }
    inline bool transition (StateTableDriver<EntryData> *driver,
			    const Entry<EntryData> *entry)
    {
      //hb_buffer_t *buffer = driver->buffer;
      //unsigned int flags = entry->flags;

      return true;
    }

    public:
    private:
  };

  inline bool apply (hb_aat_apply_context_t *c) const
  {
    TRACE_APPLY (this);

    if (!c->plan->requested_kerning)
      return false;

    driver_context_t dc (this);

    StateTableDriver<EntryData> driver (machine, c->buffer, c->font->face);
    driver.drive (&dc);

    return_trace (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (machine.sanitize (c)));
  }

  protected:
  KerxSubTableHeader				header;
  StateTable<EntryData>				machine;
  LOffsetTo<UnsizedArrayOf<FWORD>, false>	values;
  public:
  DEFINE_SIZE_STATIC (32);
};

struct KerxSubTableFormat2
{
  inline int get_kerning (hb_codepoint_t left, hb_codepoint_t right,
			  unsigned int num_glyphs) const
  {
    unsigned int l = (this+leftClassTable).get_value_or_null (left, num_glyphs);
    unsigned int r = (this+rightClassTable).get_value_or_null (right, num_glyphs);
    unsigned int offset = l + r;
    const FWORD *v = &StructAtOffset<FWORD> (&(this+array), offset);
    if (unlikely ((const char *) v < (const char *) &array ||
		  (const char *) v + v->static_size - (const char *) this > header.length))
      return 0;
    return *v;
  }

  inline bool apply (hb_aat_apply_context_t *c) const
  {
    TRACE_APPLY (this);

    if (!c->plan->requested_kerning)
      return false;

    accelerator_t accel (*this,
			 c->face->get_num_glyphs ());
    hb_kern_machine_t<accelerator_t> machine (accel);
    machine.kern (c->font, c->buffer, c->plan->kern_mask);

    return_trace (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (rowWidth.sanitize (c) &&
			  leftClassTable.sanitize (c, this) &&
			  rightClassTable.sanitize (c, this) &&
			  array.sanitize (c, this)));
  }

  struct accelerator_t
  {
    const KerxSubTableFormat2 &table;
    unsigned int num_glyphs;

    inline accelerator_t (const KerxSubTableFormat2 &table_,
			  unsigned int num_glyphs_)
			  : table (table_), num_glyphs (num_glyphs_) {}

    inline int get_kerning (hb_codepoint_t left, hb_codepoint_t right) const
    {
      return table.get_kerning (left, right, num_glyphs);
    }
  };

  protected:
  KerxSubTableHeader	header;
  HBUINT32		rowWidth;	/* The width, in bytes, of a row in the table. */
  LOffsetTo<Lookup<HBUINT16> >
			leftClassTable;	/* Offset from beginning of this subtable to
					 * left-hand class table. */
  LOffsetTo<Lookup<HBUINT16> >
			rightClassTable;/* Offset from beginning of this subtable to
					 * right-hand class table. */
  LOffsetTo<FWORD>	array;		/* Offset from beginning of this subtable to
					 * the start of the kerning array. */
  public:
  DEFINE_SIZE_STATIC (28);
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
  KerxSubTableHeader	header;
  public:
  DEFINE_SIZE_STATIC (12);
};

struct KerxSubTableFormat6
{
  enum Flags
  {
    ValuesAreLong	= 0x00000001,
  };

  inline bool is_long (void) const { return flags & ValuesAreLong; }

  inline int get_kerning (hb_codepoint_t left, hb_codepoint_t right,
			  unsigned int num_glyphs) const
  {
    if (is_long ())
    {
      const U::Long &t = u.l;
      unsigned int l = (this+t.rowIndexTable).get_value_or_null (left, num_glyphs);
      unsigned int r = (this+t.columnIndexTable).get_value_or_null (right, num_glyphs);
      unsigned int offset = l + r;
      const FWORD32 *v = &StructAtOffset<FWORD32> (&(this+t.array), offset * sizeof (FWORD32));
      if (unlikely ((const char *) v < (const char *) &t.array ||
		    (const char *) v + v->static_size - (const char *) this > header.length))
	return 0;
      return *v;
    }
    else
    {
      const U::Short &t = u.s;
      unsigned int l = (this+t.rowIndexTable).get_value_or_null (left, num_glyphs);
      unsigned int r = (this+t.columnIndexTable).get_value_or_null (right, num_glyphs);
      unsigned int offset = l + r;
      const FWORD *v = &StructAtOffset<FWORD> (&(this+t.array), offset * sizeof (FWORD));
      if (unlikely ((const char *) v < (const char *) &t.array ||
		    (const char *) v + v->static_size - (const char *) this > header.length))
	return 0;
      return *v;
    }
  }

  inline bool apply (hb_aat_apply_context_t *c) const
  {
    TRACE_APPLY (this);

    if (!c->plan->requested_kerning)
      return false;

    accelerator_t accel (*this,
			 c->face->get_num_glyphs ());
    hb_kern_machine_t<accelerator_t> machine (accel);
    machine.kern (c->font, c->buffer, c->plan->kern_mask);

    return_trace (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) &&
			  is_long () ?
			  (
			    u.l.rowIndexTable.sanitize (c, this) &&
			    u.l.columnIndexTable.sanitize (c, this) &&
			    u.l.array.sanitize (c, this)
			  ) : (
			    u.s.rowIndexTable.sanitize (c, this) &&
			    u.s.columnIndexTable.sanitize (c, this) &&
			    u.s.array.sanitize (c, this)
			  )));
  }

  struct accelerator_t
  {
    const KerxSubTableFormat6 &table;
    unsigned int num_glyphs;

    inline accelerator_t (const KerxSubTableFormat6 &table_,
			  unsigned int num_glyphs_)
			  : table (table_), num_glyphs (num_glyphs_) {}

    inline int get_kerning (hb_codepoint_t left, hb_codepoint_t right) const
    {
      return table.get_kerning (left, right, num_glyphs);
    }
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
      LOffsetTo<Lookup<HBUINT32> >	rowIndexTable;
      LOffsetTo<Lookup<HBUINT32> >	columnIndexTable;
      LOffsetTo<FWORD32>		array;
    } l;
    struct Short
    {
      LOffsetTo<Lookup<HBUINT16> >	rowIndexTable;
      LOffsetTo<Lookup<HBUINT16> >	columnIndexTable;
      LOffsetTo<FWORD>			array;
    } s;
  } u;
  public:
  DEFINE_SIZE_STATIC (32);
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

      if (table->u.header.coverage & (KerxTable::CrossStream | KerxTable::Variation) ||
	  table->u.header.tupleCount)
	goto skip; /* We do NOT handle cross-stream or variation kerning. */

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
