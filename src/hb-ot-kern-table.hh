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

#include "hb-open-type-private.hh"

namespace OT {


/*
 * kern -- Kerning
 */

#define HB_OT_TAG_kern HB_TAG('k','e','r','n')

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

  inline unsigned int get_size (void) const
  { return pairs.get_size (); }

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

struct KernSubTableFormat2
{
  inline unsigned int get_size (void) const
  {
    /* XXX */
    return 0;
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);

    /* XXX */

    return_trace (true);
  }
};

struct KernSubTable
{
  inline unsigned int get_size (unsigned int format) const
  {
    switch (format) {
    case 0: return u.format0.get_size ();
    case 2: return u.format2.get_size ();
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

  inline unsigned int get_size (void) const { return thiz()->length; }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (thiz()) &&
		  thiz()->length >= thiz()->min_size &&
		  c->check_array (thiz(), 1, thiz()->length) &&
		  thiz()->subtable.sanitize (c, thiz()->format) &&
		  thiz()->subtable.get_size (thiz()-> format) <= thiz()->length - thiz()->min_size);
  }
};

template <typename T>
struct KernTable
{
  /* https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern */
  inline const T* thiz (void) const { return static_cast<const T *> (this); }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!c->check_struct (thiz()) ||
		  thiz()->version != T::VERSION))
      return_trace (false);

    const typename T::SubTableWrapper *st = CastP<typename T::SubTableWrapper> (thiz()->data);
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
  friend struct KernTable;

  static const uint16_t VERSION = 0x0000u;

  struct SubTableWrapper : KernSubTableWrapper<SubTableWrapper>
  {
    friend struct KernSubTableWrapper;

    protected:
    USHORT	versionZ;	/* Unused. */
    USHORT	length;		/* Length of the subtable (including this header). */
    BYTE	format;		/* Subtable format. */
    BYTE	coverage;	/* Coverage bits. */
    KernSubTable subtable;	/* Subtable data. */
    public:
    DEFINE_SIZE_MIN (6);
  };

  protected:
  USHORT	version;	/* Version--0x0000u */
  USHORT	nTables;	/* Number of subtables in the kerning table. */
  BYTE		data[VAR];
  public:
  DEFINE_SIZE_ARRAY (4, data);
};

struct KernAAT : KernTable<KernAAT>
{
  friend struct KernTable;

  static const uint32_t VERSION = 0x00010000u;

  struct SubTableWrapper : KernSubTableWrapper<SubTableWrapper>
  {
    friend struct KernSubTableWrapper;

    protected:
    ULONG	length;		/* Length of the subtable (including this header). */
    BYTE	coverage;	/* Coverage bits. */
    BYTE	format;		/* Subtable format. */
    USHORT	tupleIndex;	/* The tuple index (used for variations fonts).
				 * This value specifies which tuple this subtable covers. */
    KernSubTable subtable;	/* Subtable data. */
    public:
    DEFINE_SIZE_MIN (8);
  };

  protected:
  ULONG		version;	/* Version--0x00010000u */
  ULONG		nTables;	/* Number of subtables in the kerning table. */
  BYTE		data[VAR];
  public:
  DEFINE_SIZE_ARRAY (8, data);
};

struct kern
{
  static const hb_tag_t tableTag = HB_OT_TAG_kern;

  private:
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (!u.major.sanitize (c)) return_trace (false);
    switch (u.major) {
    case 0: return_trace (u.ot.sanitize (c));
    case 1: return_trace (u.aat.sanitize (c));
    default:return_trace (true);
    }
  }

  protected:
  union {
  USHORT		major;
  KernOT		ot;
  KernAAT		aat;
  } u;
  public:
  DEFINE_SIZE_UNION (2, major);
};

} /* namespace OT */


#endif /* HB_OT_KERN_TABLE_HH */
