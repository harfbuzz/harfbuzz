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

#ifndef HB_AAT_LAYOUT_MORX_TABLE_HH
#define HB_AAT_LAYOUT_MORX_TABLE_HH

#include "hb-open-type-private.hh"
#include "hb-aat-layout-common-private.hh"

#define HB_AAT_TAG_MORT HB_TAG('m','o','r','t')
#define HB_AAT_TAG_MORX HB_TAG('m','o','r','x')


namespace AAT {

using namespace OT;


template <typename Types>
struct RearrangementSubtable
{
  enum {
    MarkFirst	= 0x8000,
    DontAdvance	= 0x4000,
    MarkLast	= 0x2000,
    Verb	= 0x000F,
  };

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY (this);

    bool ret = false;
    unsigned int num_glyphs = c->face->get_num_glyphs ();

    unsigned int state = 0;
    unsigned int start = 0;
    unsigned int end = 0;

    hb_glyph_info_t *info = c->buffer->info;
    unsigned int count = c->buffer->len;

    c->buffer->unsafe_to_break (0, count); /* TODO Improve. */

    for (unsigned int i = 0; i < count; i++)
    {
      unsigned int klass = machine.get_class (info[i].codepoint, num_glyphs);
      const Entry<void> *entry = machine.get_entry (state, klass);
      if (unlikely (!entry))
        break;

      unsigned int flags = entry->flags;

      if (flags & MarkFirst)
	start = i;

      if (flags & MarkLast)
	end = i + 1;

      if ((flags & Verb) && start < end)
      {
	/* The following map has two nibbles, for start-side
	 * and end-side. Values of 0,1,2 mean move that many
	 * to the other side. Value of 3 means move 2 and
	 * flip them. */
	static const unsigned char map[16] =
	{
	  0x00,	/* 0	no change */
	  0x10,	/* 1	Ax => xA */
	  0x01,	/* 2	xD => Dx */
	  0x11,	/* 3	AxD => DxA */
	  0x20,	/* 4	ABx => xAB */
	  0x30,	/* 5	ABx => xBA */
	  0x02,	/* 6	xCD => CDx */
	  0x03,	/* 7	xCD => DCx */
	  0x12,	/* 8	AxCD => CDxA */
	  0x13,	/* 9	AxCD => DCxA */
	  0x21,	/* 10	ABxD => DxAB */
	  0x31,	/* 11	ABxD => DxBA */
	  0x22,	/* 12	ABxCD => CDxAB */
	  0x32,	/* 13	ABxCD => CDxBA */
	  0x23,	/* 14	ABxCD => DCxAB */
	  0x33,	/* 15	ABxCD => DCxBA */
	};

	unsigned int m = map[flags & Verb];
	unsigned int l = MIN<unsigned int> (2, m >> 4);
	unsigned int r = MIN<unsigned int> (2, m & 0x0F);
	bool reverse_l = 3 == (m >> 4);
	bool reverse_r = 3 == (m & 0x0F);

	if (end - start >= l + r)
	{
	  c->buffer->merge_clusters (start, end);

	  hb_glyph_info_t buf[4];
	  memcpy (buf, info + start, l * sizeof (buf[0]));
	  memcpy (buf + 2, info + end - r, r * sizeof (buf[0]));

	  if (l != r)
	    memmove (info + start + r, info + start + l, (end - start - l - r) * sizeof (buf[0]));

	  memcpy (info + start, buf + 2, r * sizeof (buf[0]));
	  memcpy (info + end - l, buf, l * sizeof (buf[0]));
	  if (reverse_l)
	  {
	    buf[0] = info[end - 1];
	    info[end - 1] = info[end - 2];
	    info[end - 2] = buf[0];
	  }
	  if (reverse_r)
	  {
	    buf[0] = info[start];
	    info[start] = info[start + 1];
	    info[start + 1] = buf[0];
	  }
	}
      }

      if (false/* XXX*/ && flags & DontAdvance)
        i--; /* XXX Detect infinite loop. */

      state = entry->newState;
    }

    return_trace (ret);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (machine.sanitize (c, 0/*XXX*/));
  }

  protected:
  StateTable<Types, void>	machine;
  public:
  DEFINE_SIZE_MIN (2);
};

struct ContextualSubtable
{
  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY (this);
    /* TODO */
    return_trace (false);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    /* TODO */
    return_trace (false);
  }
};

struct LigatureSubtable
{
  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY (this);
    /* TODO */
    return_trace (false);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    /* TODO */
    return_trace (false);
  }
};

struct NoncontextualSubtable
{
  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY (this);

    bool ret = false;
    unsigned int num_glyphs = c->face->get_num_glyphs ();

    hb_glyph_info_t *info = c->buffer->info;
    unsigned int count = c->buffer->len;
    for (unsigned int i = 0; i < count; i++)
    {
      const GlyphID *replacement = substitute.get_value (info[i].codepoint, num_glyphs);
      if (replacement)
      {
	info[i].codepoint = *replacement;
	ret = true;
      }
    }

    return_trace (ret);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (substitute.sanitize (c));
  }

  protected:
  Lookup<GlyphID>	substitute;
  public:
  DEFINE_SIZE_MIN (2);
};

struct InsertionSubtable
{
  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY (this);
    /* TODO */
    return_trace (false);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    /* TODO */
    return_trace (false);
  }
};


struct Feature
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  public:
  HBUINT16	featureType;	/* The type of feature. */
  HBUINT16	featureSetting;	/* The feature's setting (aka selector). */
  HBUINT32	enableFlags;	/* Flags for the settings that this feature
				 * and setting enables. */
  HBUINT32	disableFlags;	/* Complement of flags for the settings that this
				 * feature and setting disable. */

  public:
  DEFINE_SIZE_STATIC (12);
};


template <typename Types>
struct ChainSubtable
{
  template <typename> struct Chain;
  friend struct Chain<Types>;

  typedef typename Types::HBUINT HBUINT;

  inline unsigned int get_size (void) const { return length; }
  inline unsigned int get_type (void) const { return coverage & 0xFF; }

  enum Type {
    Rearrangement	= 0,
    Contextual		= 1,
    Ligature		= 2,
    Noncontextual	= 4,
    Insertion		= 5
  };

  inline void apply (hb_apply_context_t *c) const
  {
    dispatch (c);
  }

  template <typename context_t>
  inline typename context_t::return_t dispatch (context_t *c) const
  {
    unsigned int subtable_type = get_type ();
    TRACE_DISPATCH (this, subtable_type);
    switch (subtable_type) {
    case Rearrangement:		return_trace (c->dispatch (u.rearrangement));
    case Contextual:		return_trace (c->dispatch (u.contextual));
    case Ligature:		return_trace (c->dispatch (u.ligature));
    case Noncontextual:		return_trace (c->dispatch (u.noncontextual));
    case Insertion:		return_trace (c->dispatch (u.insertion));
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
  HBUINT		length;		/* Total subtable length, including this header. */
  HBUINT		coverage;	/* Coverage flags and subtable type. */
  HBUINT32		subFeatureFlags;/* The 32-bit mask identifying which subtable this is. */
  union {
  RearrangementSubtable<Types>
			rearrangement;
  ContextualSubtable	contextual;
  LigatureSubtable	ligature;
  NoncontextualSubtable	noncontextual;
  InsertionSubtable	insertion;
  } u;
  public:
  DEFINE_SIZE_MIN (2 * sizeof (HBUINT) + 4);
};

template <typename Types>
struct Chain
{
  typedef typename Types::HBUINT HBUINT;

  inline void apply (hb_apply_context_t *c) const
  {
    const ChainSubtable<Types> *subtable = &StructAtOffset<ChainSubtable<Types> > (featureZ, featureZ[0].static_size * featureCount);
    unsigned int count = subtableCount;
    for (unsigned int i = 0; i < count; i++)
    {
      subtable->apply (c);
      subtable = &StructAfter<ChainSubtable<Types> > (*subtable);
    }
  }

  inline unsigned int get_size (void) const { return length; }

  inline bool sanitize (hb_sanitize_context_t *c, unsigned int major) const
  {
    TRACE_SANITIZE (this);
    if (!length.sanitize (c) ||
	length < min_size ||
	!c->check_range (this, length))
      return_trace (false);

    if (!c->check_array (featureZ, featureZ[0].static_size, featureCount))
      return_trace (false);

    const ChainSubtable<Types> *subtable = &StructAtOffset<ChainSubtable<Types> > (featureZ, featureZ[0].static_size * featureCount);
    unsigned int count = subtableCount;
    for (unsigned int i = 0; i < count; i++)
    {
      if (!subtable->sanitize (c))
	return_trace (false);
      subtable = &StructAfter<ChainSubtable<Types> > (*subtable);
    }

    return_trace (true);
  }

  protected:
  HBUINT32	defaultFlags;	/* The default specification for subtables. */
  HBUINT32	length;		/* Total byte count, including this header. */
  HBUINT		featureCount;	/* Number of feature subtable entries. */
  HBUINT		subtableCount;	/* The number of subtables in the chain. */

  Feature		featureZ[VAR];	/* Features. */
  ChainSubtable<Types>	subtableX[VAR];	/* Subtables. */
  // subtableGlyphCoverageArray if major == 3

  public:
  DEFINE_SIZE_MIN (8 + 2 * sizeof (HBUINT));
};


/*
 * The 'mort'/'morx' Tables
 */

template <typename Types>
struct mortmorx
{
  static const hb_tag_t mortTag	= HB_AAT_TAG_MORT;
  static const hb_tag_t morxTag	= HB_AAT_TAG_MORX;

  typedef typename Types::HBUINT HBUINT;

  inline void apply (hb_apply_context_t *c) const
  {
    const Chain<Types> *chain = chains;
    unsigned int count = chainCount;
    for (unsigned int i = 0; i < count; i++)
    {
      chain->apply (c);
      chain = &StructAfter<Chain<Types> > (*chain);
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (!version.sanitize (c) ||
	(version.major >> (sizeof (HBUINT) == 4 ? 1 : 0)) != 1 ||
	!chainCount.sanitize (c))
      return_trace (false);

    const Chain<Types> *chain = chains;
    unsigned int count = chainCount;
    for (unsigned int i = 0; i < count; i++)
    {
      if (!chain->sanitize (c, version.major))
	return_trace (false);
      chain = &StructAfter<Chain<Types> > (*chain);
    }

    return_trace (true);
  }

  protected:
  FixedVersion<>version;	/* Version number of the glyph metamorphosis table.
				 * 1 for mort, 2 or 3 for morx. */
  HBUINT32	chainCount;	/* Number of metamorphosis chains contained in this
				 * table. */
  Chain<Types>	chains[VAR];	/* Chains. */

  public:
  DEFINE_SIZE_MIN (8);
};

struct MorxTypes
{
  typedef HBUINT32 HBUINT;
  typedef HBUINT16 HBUSHORT;
  struct ClassType : Lookup<HBUINT16>
  {
    inline unsigned int get_class (hb_codepoint_t glyph_id, unsigned int num_glyphs) const
    {
      const HBUINT16 *v = get_value (glyph_id, num_glyphs);
      return v ? *v : 1;
    }
  };
};
struct MortTypes
{
  typedef HBUINT16 HBUINT;
  typedef HBUINT8 HBUSHORT;
  struct ClassType : ClassTable
  {
    inline unsigned int get_class (hb_codepoint_t glyph_id, unsigned int num_glyphs HB_UNUSED) const
    {
      return ClassTable::get_class (glyph_id);
    }
  };
};

struct mort : mortmorx<MortTypes>
{
  static const hb_tag_t tableTag	= HB_AAT_TAG_MORT;
};

struct morx : mortmorx<MorxTypes>
{
  static const hb_tag_t tableTag	= HB_AAT_TAG_MORX;
};


} /* namespace AAT */


#endif /* HB_AAT_LAYOUT_MORX_TABLE_HH */
