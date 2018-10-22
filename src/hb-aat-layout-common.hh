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

#ifndef HB_AAT_LAYOUT_COMMON_HH
#define HB_AAT_LAYOUT_COMMON_HH

#include "hb-aat-layout.hh"


namespace AAT {

using namespace OT;


/*
 * Lookup Table
 */

template <typename T> struct Lookup;

template <typename T>
struct LookupFormat0
{
  friend struct Lookup<T>;

  private:
  inline const T* get_value (hb_codepoint_t glyph_id, unsigned int num_glyphs) const
  {
    if (unlikely (glyph_id >= num_glyphs)) return nullptr;
    return &arrayZ[glyph_id];
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (arrayZ.sanitize (c, c->get_num_glyphs ()));
  }

  protected:
  HBUINT16	format;		/* Format identifier--format = 0 */
  UnsizedArrayOf<T>
		arrayZ;		/* Array of lookup values, indexed by glyph index. */
  public:
  DEFINE_SIZE_ARRAY (2, arrayZ);
};


template <typename T>
struct LookupSegmentSingle
{
  inline int cmp (hb_codepoint_t g) const {
    return g < first ? -1 : g <= last ? 0 : +1 ;
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) && value.sanitize (c));
  }

  GlyphID	last;		/* Last GlyphID in this segment */
  GlyphID	first;		/* First GlyphID in this segment */
  T		value;		/* The lookup value (only one) */
  public:
  DEFINE_SIZE_STATIC (4 + T::static_size);
};

template <typename T>
struct LookupFormat2
{
  friend struct Lookup<T>;

  private:
  inline const T* get_value (hb_codepoint_t glyph_id) const
  {
    const LookupSegmentSingle<T> *v = segments.bsearch (glyph_id);
    return v ? &v->value : nullptr;
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (segments.sanitize (c));
  }

  protected:
  HBUINT16	format;		/* Format identifier--format = 2 */
  VarSizedBinSearchArrayOf<LookupSegmentSingle<T> >
		segments;	/* The actual segments. These must already be sorted,
				 * according to the first word in each one (the last
				 * glyph in each segment). */
  public:
  DEFINE_SIZE_ARRAY (8, segments);
};

template <typename T>
struct LookupSegmentArray
{
  inline const T* get_value (hb_codepoint_t glyph_id, const void *base) const
  {
    return first <= glyph_id && glyph_id <= last ? &(base+valuesZ)[glyph_id - first] : nullptr;
  }

  inline int cmp (hb_codepoint_t g) const {
    return g < first ? -1 : g <= last ? 0 : +1 ;
  }

  inline bool sanitize (hb_sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  first <= last &&
		  valuesZ.sanitize (c, base, last - first + 1));
  }

  GlyphID	last;		/* Last GlyphID in this segment */
  GlyphID	first;		/* First GlyphID in this segment */
  OffsetTo<UnsizedArrayOf<T>, HBUINT16, false>
		valuesZ;	/* A 16-bit offset from the start of
				 * the table to the data. */
  public:
  DEFINE_SIZE_STATIC (6);
};

template <typename T>
struct LookupFormat4
{
  friend struct Lookup<T>;

  private:
  inline const T* get_value (hb_codepoint_t glyph_id) const
  {
    const LookupSegmentArray<T> *v = segments.bsearch (glyph_id);
    return v ? v->get_value (glyph_id, this) : nullptr;
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (segments.sanitize (c, this));
  }

  protected:
  HBUINT16	format;		/* Format identifier--format = 4 */
  VarSizedBinSearchArrayOf<LookupSegmentArray<T> >
		segments;	/* The actual segments. These must already be sorted,
				 * according to the first word in each one (the last
				 * glyph in each segment). */
  public:
  DEFINE_SIZE_ARRAY (8, segments);
};

template <typename T>
struct LookupSingle
{
  inline int cmp (hb_codepoint_t g) const { return glyph.cmp (g); }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) && value.sanitize (c));
  }

  GlyphID	glyph;		/* Last GlyphID */
  T		value;		/* The lookup value (only one) */
  public:
  DEFINE_SIZE_STATIC (2 + T::static_size);
};

template <typename T>
struct LookupFormat6
{
  friend struct Lookup<T>;

  private:
  inline const T* get_value (hb_codepoint_t glyph_id) const
  {
    const LookupSingle<T> *v = entries.bsearch (glyph_id);
    return v ? &v->value : nullptr;
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (entries.sanitize (c));
  }

  protected:
  HBUINT16	format;		/* Format identifier--format = 6 */
  VarSizedBinSearchArrayOf<LookupSingle<T> >
		entries;	/* The actual entries, sorted by glyph index. */
  public:
  DEFINE_SIZE_ARRAY (8, entries);
};

template <typename T>
struct LookupFormat8
{
  friend struct Lookup<T>;

  private:
  inline const T* get_value (hb_codepoint_t glyph_id) const
  {
    return firstGlyph <= glyph_id && glyph_id - firstGlyph < glyphCount ?
	   &valueArrayZ[glyph_id - firstGlyph] : nullptr;
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) && valueArrayZ.sanitize (c, glyphCount));
  }

  protected:
  HBUINT16	format;		/* Format identifier--format = 8 */
  GlyphID	firstGlyph;	/* First glyph index included in the trimmed array. */
  HBUINT16	glyphCount;	/* Total number of glyphs (equivalent to the last
				 * glyph minus the value of firstGlyph plus 1). */
  UnsizedArrayOf<T>
		valueArrayZ;	/* The lookup values (indexed by the glyph index
				 * minus the value of firstGlyph). */
  public:
  DEFINE_SIZE_ARRAY (6, valueArrayZ);
};

template <typename T>
struct LookupFormat10
{
  friend struct Lookup<T>;

  private:
  inline const typename T::type get_value_or_null (hb_codepoint_t glyph_id) const
  {
    if (!(firstGlyph <= glyph_id && glyph_id - firstGlyph < glyphCount))
      return Null(T);

    const HBUINT8 *p = &valueArrayZ[(glyph_id - firstGlyph) * valueSize];

    unsigned int v = 0;
    unsigned int count = valueSize;
    for (unsigned int i = 0; i < count; i++)
      v = (v << 8) | *p++;

    return v;
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  valueSize <= 4 &&
		  valueArrayZ.sanitize (c, glyphCount * valueSize));
  }

  protected:
  HBUINT16	format;		/* Format identifier--format = 8 */
  HBUINT16	valueSize;	/* Byte size of each value. */
  GlyphID	firstGlyph;	/* First glyph index included in the trimmed array. */
  HBUINT16	glyphCount;	/* Total number of glyphs (equivalent to the last
				 * glyph minus the value of firstGlyph plus 1). */
  UnsizedArrayOf<HBUINT8>
		valueArrayZ;	/* The lookup values (indexed by the glyph index
				 * minus the value of firstGlyph). */
  public:
  DEFINE_SIZE_ARRAY (8, valueArrayZ);
};

template <typename T>
struct Lookup
{
  inline const T* get_value (hb_codepoint_t glyph_id, unsigned int num_glyphs) const
  {
    switch (u.format) {
    case 0: return u.format0.get_value (glyph_id, num_glyphs);
    case 2: return u.format2.get_value (glyph_id);
    case 4: return u.format4.get_value (glyph_id);
    case 6: return u.format6.get_value (glyph_id);
    case 8: return u.format8.get_value (glyph_id);
    default:return nullptr;
    }
  }

  inline const typename T::type get_value_or_null (hb_codepoint_t glyph_id, unsigned int num_glyphs) const
  {
    switch (u.format) {
      /* Format 10 cannot return a pointer. */
      case 10: return u.format10.get_value_or_null (glyph_id);
      default:
      const T *v = get_value (glyph_id, num_glyphs);
      return v ? *v : Null(T);
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (!u.format.sanitize (c)) return_trace (false);
    switch (u.format) {
    case 0: return_trace (u.format0.sanitize (c));
    case 2: return_trace (u.format2.sanitize (c));
    case 4: return_trace (u.format4.sanitize (c));
    case 6: return_trace (u.format6.sanitize (c));
    case 8: return_trace (u.format8.sanitize (c));
    case 10: return_trace (u.format10.sanitize (c));
    default:return_trace (true);
    }
  }

  protected:
  union {
  HBUINT16		format;		/* Format identifier */
  LookupFormat0<T>	format0;
  LookupFormat2<T>	format2;
  LookupFormat4<T>	format4;
  LookupFormat6<T>	format6;
  LookupFormat8<T>	format8;
  LookupFormat10<T>	format10;
  } u;
  public:
  DEFINE_SIZE_UNION (2, format);
};
/* Lookup 0 has unbounded size (dependant on num_glyphs).  So we need to defined
 * special NULL objects for Lookup<> objects, but since it's template our macros
 * don't work.  So we have to hand-code them here.  UGLY. */
} /* Close namespace. */
/* Ugly hand-coded null objects for template Lookup<> :(. */
extern HB_INTERNAL const unsigned char _hb_Null_AAT_Lookup[2];
template <>
/*static*/ inline const AAT::Lookup<OT::HBUINT16>& Null<AAT::Lookup<OT::HBUINT16> > (void) {
  return *reinterpret_cast<const AAT::Lookup<OT::HBUINT16> *> (_hb_Null_AAT_Lookup);
}
template <>
/*static*/ inline const AAT::Lookup<OT::HBUINT32>& Null<AAT::Lookup<OT::HBUINT32> > (void) {
  return *reinterpret_cast<const AAT::Lookup<OT::HBUINT32> *> (_hb_Null_AAT_Lookup);
}
template <>
/*static*/ inline const AAT::Lookup<OT::Offset<OT::HBUINT16, false> >& Null<AAT::Lookup<OT::Offset<OT::HBUINT16, false> > > (void) {
  return *reinterpret_cast<const AAT::Lookup<OT::Offset<OT::HBUINT16, false> > *> (_hb_Null_AAT_Lookup);
}
namespace AAT {


/*
 * Extended State Table
 */

template <typename T>
struct Entry
{
  inline bool sanitize (hb_sanitize_context_t *c, unsigned int count) const
  {
    TRACE_SANITIZE (this);
    /* Note, we don't recurse-sanitize data because we don't access it.
     * That said, in our DEFINE_SIZE_STATIC we access T::static_size,
     * which ensures that data has a simple sanitize(). To be determined
     * if I need to remove that as well. */
    return_trace (c->check_struct (this));
  }

  public:
  HBUINT16	newState;	/* Byte offset from beginning of state table
				 * to the new state. Really?!?! Or just state
				 * number?  The latter in morx for sure. */
  HBUINT16	flags;		/* Table specific. */
  T		data;		/* Optional offsets to per-glyph tables. */
  public:
  DEFINE_SIZE_STATIC (4 + T::static_size);
};

template <>
struct Entry<void>
{
  inline bool sanitize (hb_sanitize_context_t *c, unsigned int count) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  public:
  HBUINT16	newState;	/* Byte offset from beginning of state table to the new state. */
  HBUINT16	flags;		/* Table specific. */
  public:
  DEFINE_SIZE_STATIC (4);
};

template <typename Extra>
struct StateTable
{
  enum State
  {
    STATE_START_OF_TEXT = 0,
    STATE_START_OF_LINE = 1,
  };
  enum Class
  {
    CLASS_END_OF_TEXT = 0,
    CLASS_OUT_OF_BOUNDS = 1,
    CLASS_DELETED_GLYPH = 2,
    CLASS_END_OF_LINE = 3,
  };

  inline unsigned int get_class (hb_codepoint_t glyph_id, unsigned int num_glyphs) const
  {
    const HBUINT16 *v = (this+classTable).get_value (glyph_id, num_glyphs);
    return v ? (unsigned) *v : (unsigned) CLASS_OUT_OF_BOUNDS;
  }

  inline const Entry<Extra> *get_entries () const
  {
    return (this+entryTable).arrayZ;
  }

  inline const Entry<Extra> *get_entryZ (unsigned int state, unsigned int klass) const
  {
    if (unlikely (klass >= nClasses)) return nullptr;

    const HBUINT16 *states = (this+stateArrayTable).arrayZ;
    const Entry<Extra> *entries = (this+entryTable).arrayZ;

    unsigned int entry = states[state * nClasses + klass];

    return &entries[entry];
  }

  inline bool sanitize (hb_sanitize_context_t *c,
			unsigned int *num_entries_out = nullptr) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!(c->check_struct (this) &&
		    classTable.sanitize (c, this)))) return_trace (false);

    const HBUINT16 *states = (this+stateArrayTable).arrayZ;
    const Entry<Extra> *entries = (this+entryTable).arrayZ;

    unsigned int num_classes = nClasses;

    unsigned int num_states = 1;
    unsigned int num_entries = 0;

    unsigned int state = 0;
    unsigned int entry = 0;
    while (state < num_states)
    {
      if (unlikely (hb_unsigned_mul_overflows (num_classes, states[0].static_size)))
	return_trace (false);

      if (unlikely (!c->check_array (states,
				     num_states,
				     num_classes * states[0].static_size)))
	return_trace (false);
      if ((c->max_ops -= num_states - state) < 0)
	return_trace (false);
      { /* Sweep new states. */
	const HBUINT16 *stop = &states[num_states * num_classes];
	for (const HBUINT16 *p = &states[state * num_classes]; p < stop; p++)
	  num_entries = MAX<unsigned int> (num_entries, *p + 1);
	state = num_states;
      }

      if (unlikely (!c->check_array (entries, num_entries)))
	return_trace (false);
      if ((c->max_ops -= num_entries - entry) < 0)
	return_trace (false);
      { /* Sweep new entries. */
	const Entry<Extra> *stop = &entries[num_entries];
	for (const Entry<Extra> *p = &entries[entry]; p < stop; p++)
	  num_states = MAX<unsigned int> (num_states, p->newState + 1);
	entry = num_entries;
      }
    }

    if (num_entries_out)
      *num_entries_out = num_entries;

    return_trace (true);
  }

  protected:
  HBUINT32	nClasses;	/* Number of classes, which is the number of indices
				 * in a single line in the state array. */
  LOffsetTo<Lookup<HBUINT16>, false>
		classTable;	/* Offset to the class table. */
  LOffsetTo<UnsizedArrayOf<HBUINT16>, false>
		stateArrayTable;/* Offset to the state array. */
  LOffsetTo<UnsizedArrayOf<Entry<Extra> >, false>
		entryTable;	/* Offset to the entry array. */

  public:
  DEFINE_SIZE_STATIC (16);
};

template <typename EntryData>
struct StateTableDriver
{
  inline StateTableDriver (const StateTable<EntryData> &machine_,
			   hb_buffer_t *buffer_,
			   hb_face_t *face_) :
	      machine (machine_),
	      buffer (buffer_),
	      num_glyphs (face_->get_num_glyphs ()) {}

  template <typename context_t>
  inline void drive (context_t *c)
  {
    if (!c->in_place)
      buffer->clear_output ();

    unsigned int state = StateTable<EntryData>::STATE_START_OF_TEXT;
    bool last_was_dont_advance = false;
    for (buffer->idx = 0;;)
    {
      unsigned int klass = buffer->idx < buffer->len ?
			   machine.get_class (buffer->info[buffer->idx].codepoint, num_glyphs) :
			   (unsigned) StateTable<EntryData>::CLASS_END_OF_TEXT;
      const Entry<EntryData> *entry = machine.get_entryZ (state, klass);
      if (unlikely (!entry))
	break;

      /* Unsafe-to-break before this if not in state 0, as things might
       * go differently if we start from state 0 here.
       *
       * Ugh.  The indexing here is ugly... */
      if (state && buffer->backtrack_len () && buffer->idx < buffer->len)
      {
	/* If there's no action and we're just epsilon-transitioning to state 0,
	 * safe to break. */
	if (c->is_actionable (this, entry) ||
	    !(entry->newState == StateTable<EntryData>::STATE_START_OF_TEXT &&
	      entry->flags == context_t::DontAdvance))
	  buffer->unsafe_to_break_from_outbuffer (buffer->backtrack_len () - 1, buffer->idx + 1);
      }

      /* Unsafe-to-break if end-of-text would kick in here. */
      if (buffer->idx + 2 <= buffer->len)
      {
	const Entry<EntryData> *end_entry = machine.get_entryZ (state, 0);
	if (c->is_actionable (this, end_entry))
	  buffer->unsafe_to_break (buffer->idx, buffer->idx + 2);
      }

      if (unlikely (!c->transition (this, entry)))
        break;

      if (unlikely (!buffer->successful)) return;

      last_was_dont_advance = (entry->flags & context_t::DontAdvance) && buffer->max_ops-- > 0;

      state = entry->newState;

      if (buffer->idx == buffer->len)
        break;

      if (!last_was_dont_advance)
        buffer->next_glyph ();
    }

    if (!c->in_place)
    {
      for (; buffer->successful && buffer->idx < buffer->len;)
	buffer->next_glyph ();
      if (likely (buffer->successful))
	buffer->swap_buffers ();
    }
  }

  public:
  const StateTable<EntryData> &machine;
  hb_buffer_t *buffer;
  unsigned int num_glyphs;
};


struct ankr;

struct hb_aat_apply_context_t :
       hb_dispatch_context_t<hb_aat_apply_context_t, bool, HB_DEBUG_APPLY>
{
  inline const char *get_name (void) { return "APPLY"; }
  template <typename T>
  inline return_t dispatch (const T &obj) { return obj.apply (this); }
  static return_t default_return_value (void) { return false; }
  bool stop_sublookup_iteration (return_t r) const { return r; }

  hb_ot_shape_plan_t *plan;
  hb_font_t *font;
  hb_face_t *face;
  hb_buffer_t *buffer;
  hb_sanitize_context_t sanitizer;
  const ankr &ankr_table;
  const char *ankr_end;

  /* Unused. For debug tracing only. */
  unsigned int lookup_index;
  unsigned int debug_depth;

  inline hb_aat_apply_context_t (hb_ot_shape_plan_t *plan_,
				 hb_font_t *font_,
				 hb_buffer_t *buffer_,
				 hb_blob_t *blob = const_cast<hb_blob_t *> (&Null(hb_blob_t)),
				 const ankr &ankr_table_ = Null(ankr),
				 const char *ankr_end_ = nullptr) :
		plan (plan_), font (font_), face (font->face), buffer (buffer_),
		sanitizer (),
		ankr_table (ankr_table_), ankr_end (ankr_end_),
		lookup_index (0), debug_depth (0)
  {
    sanitizer.init (blob);
    sanitizer.set_num_glyphs (face->get_num_glyphs ());
    sanitizer.start_processing ();
    sanitizer.set_max_ops (HB_SANITIZE_MAX_OPS_MAX);
  }

  inline void set_lookup_index (unsigned int i) { lookup_index = i; }

  inline ~hb_aat_apply_context_t (void)
  {
    sanitizer.end_processing ();
  }
};


} /* namespace AAT */


#endif /* HB_AAT_LAYOUT_COMMON_HH */
