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

#include <hb-open-type-private.hh>
#include <hb-aat-layout-common-private.hh>

#define HB_AAT_TAG_MORT HB_TAG('m','o','r','t')
#define HB_AAT_TAG_MORX HB_TAG('m','o','r','x')


namespace AAT {

using namespace OT;


struct RearrangementSubtable
{
  /* TODO */
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    /* TODO */
    return_trace (false);
  }
};

struct ContextualSubtable
{
  /* TODO */
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    /* TODO */
    return_trace (false);
  }
};

struct LigatureSubtable
{
  /* TODO */
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    /* TODO */
    return_trace (false);
  }
};

struct NoncontextualSubtable
{
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
  /* TODO */
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
  UINT16	featureType;	/* The type of feature. */
  UINT16	featureSetting;	/* The feature's setting (aka selector). */
  UINT32	enableFlags;	/* Flags for the settings that this feature
				 * and setting enables. */
  UINT32	disableFlags;	/* Complement of flags for the settings that this
				 * feature and setting disable. */

  public:
  DEFINE_SIZE_STATIC (12);
};


template <typename UINT>
struct ChainSubtable
{
  template <typename> struct Chain;
  friend struct Chain<UINT>;

  inline unsigned int get_size (void) const { return length; }
  inline unsigned int get_type (void) const { return coverage & 0xFF; }

  enum Type {
    Rearrangement	= 0,
    Contextual		= 1,
    Ligature		= 2,
    Noncontextual	= 4,
    Insertion		= 5
  };

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
  UINT		length;		/* Total subtable length, including this header. */
  UINT		coverage;	/* Coverage flags and subtable type. */
  UINT32	subFeatureFlags;/* The 32-bit mask identifying which subtable this is. */
  union {
  RearrangementSubtable	rearrangement;
  ContextualSubtable	contextual;
  LigatureSubtable	ligature;
  NoncontextualSubtable	noncontextual;
  InsertionSubtable	insertion;
  } u;
  public:
  DEFINE_SIZE_MIN (2 * sizeof (UINT) + 4);
};

template <typename UINT>
struct Chain
{

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

    const ChainSubtable<UINT> *subtable = &StructAtOffset<ChainSubtable<UINT> > (featureZ, featureZ[0].static_size * featureCount);
    unsigned int count = subtableCount;
    for (unsigned int i = 0; i < count; i++)
    {
      if (!subtable->sanitize (c))
	return_trace (false);
      subtable = &StructAfter<ChainSubtable<UINT> > (*subtable);
    }

    return_trace (true);
  }

  protected:
  UINT32	defaultFlags;	/* The default specification for subtables. */
  UINT32	length;		/* Total byte count, including this header. */
  UINT		featureCount;	/* Number of feature subtable entries. */
  UINT		subtableCount;	/* The number of subtables in the chain. */

  Feature		featureZ[VAR];	/* Features. */
  ChainSubtable<UINT>	subtableX[VAR];	/* Subtables. */
  // subtableGlyphCoverageArray if major == 3

  public:
  DEFINE_SIZE_MIN (8 + 2 * sizeof (UINT));
};


/*
 * The 'mort'/'morx' Tables
 */

template <typename UINT>
struct mortmorx
{
  static const hb_tag_t mortTag	= HB_AAT_TAG_MORT;
  static const hb_tag_t morxTag	= HB_AAT_TAG_MORX;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (!version.sanitize (c) ||
	version.major >> (sizeof (UINT) == 4) != 1 ||
	!chainCount.sanitize (c))
      return_trace (false);

    const Chain<UINT> *chain = chains;
    unsigned int count = chainCount;
    for (unsigned int i = 0; i < count; i++)
    {
      if (!chain->sanitize (c, version.major))
	return_trace (false);
      chain = &StructAfter<Chain<UINT> > (*chain);
    }

    return_trace (true);
  }

  protected:
  FixedVersion<>version;	/* Version number of the glyph metamorphosis table.
				 * 1 for mort, 2 or 3 for morx. */
  UINT32	chainCount;	/* Number of metamorphosis chains contained in this
				 * table. */
  Chain<UINT>	chains[VAR];	/* Chains. */

  public:
  DEFINE_SIZE_MIN (8);
};

struct mort : mortmorx<UINT16>
{
  static const hb_tag_t tableTag	= HB_AAT_TAG_MORT;
};

struct morx : mortmorx<UINT32>
{
  static const hb_tag_t tableTag	= HB_AAT_TAG_MORX;
};


} /* namespace AAT */


#endif /* HB_AAT_LAYOUT_MORX_TABLE_HH */
