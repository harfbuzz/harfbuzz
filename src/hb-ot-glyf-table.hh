/*
 * Copyright © 2015  Google, Inc.
 * Copyright © 2019  Adobe Inc.
 * Copyright © 2019  Ebrahim Byagowi
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
 * Google Author(s): Behdad Esfahbod, Garret Rieger, Roderick Sheeter
 * Adobe Author(s): Michiharu Ariza
 */

#ifndef HB_OT_GLYF_TABLE_HH
#define HB_OT_GLYF_TABLE_HH

#include "hb-open-type.hh"
#include "hb-ot-head-table.hh"
#include "hb-ot-hmtx-table.hh"
#include "hb-ot-var-gvar-table.hh"

#include <float.h>

namespace OT {


/*
 * loca -- Index to Location
 * https://docs.microsoft.com/en-us/typography/opentype/spec/loca
 */
#define HB_OT_TAG_loca HB_TAG('l','o','c','a')


struct loca
{
  friend struct glyf;

  static constexpr hb_tag_t tableTag = HB_OT_TAG_loca;

  bool sanitize (hb_sanitize_context_t *c HB_UNUSED) const
  {
    TRACE_SANITIZE (this);
    return_trace (true);
  }

  protected:
  UnsizedArrayOf<HBUINT8>
		dataZ;	/* Location data. */
  public:
  DEFINE_SIZE_MIN (0);	/* In reality, this is UNBOUNDED() type; but since we always
			 * check the size externally, allow Null() object of it by
			 * defining it _MIN instead. */
};


/*
 * glyf -- TrueType Glyph Data
 * https://docs.microsoft.com/en-us/typography/opentype/spec/glyf
 */
#define HB_OT_TAG_glyf HB_TAG('g','l','y','f')


struct glyf
{
  static constexpr hb_tag_t tableTag = HB_OT_TAG_glyf;

  bool sanitize (hb_sanitize_context_t *c HB_UNUSED) const
  {
    TRACE_SANITIZE (this);
    /* We don't check for anything specific here.  The users of the
     * struct do all the hard work... */
    return_trace (true);
  }

  template<typename Iterator,
	   hb_requires (hb_is_source_of (Iterator, unsigned int))>
  static bool
  _add_loca_and_head (hb_subset_plan_t * plan, Iterator padded_offsets)
  {
    unsigned max_offset = + padded_offsets | hb_reduce(hb_add, 0);
    unsigned num_offsets = padded_offsets.len () + 1;
    bool use_short_loca = max_offset < 0x1FFFF;
    unsigned entry_size = use_short_loca ? 2 : 4;
    char *loca_prime_data = (char *) calloc (entry_size, num_offsets);

    if (unlikely (!loca_prime_data)) return false;

    DEBUG_MSG (SUBSET, nullptr, "loca entry_size %d num_offsets %d "
				"max_offset %d size %d",
	       entry_size, num_offsets, max_offset, entry_size * num_offsets);

    if (use_short_loca)
      _write_loca (padded_offsets, 1, hb_array ((HBUINT16*) loca_prime_data, num_offsets));
    else
      _write_loca (padded_offsets, 0, hb_array ((HBUINT32*) loca_prime_data, num_offsets));

    hb_blob_t * loca_blob = hb_blob_create (loca_prime_data,
					    entry_size * num_offsets,
					    HB_MEMORY_MODE_WRITABLE,
					    loca_prime_data,
					    free);

    bool result = plan->add_table (HB_OT_TAG_loca, loca_blob)
		  && _add_head_and_set_loca_version (plan, use_short_loca);

    hb_blob_destroy (loca_blob);
    return result;
  }

  template<typename IteratorIn, typename IteratorOut,
	   hb_requires (hb_is_source_of (IteratorIn, unsigned int)),
	   hb_requires (hb_is_sink_of (IteratorOut, unsigned))>
  static void
  _write_loca (IteratorIn it, unsigned right_shift, IteratorOut dest)
  {
    unsigned int offset = 0;
    dest << 0;
    + it
    | hb_map ([=, &offset] (unsigned int padded_size)
	      {
		offset += padded_size;
		DEBUG_MSG (SUBSET, nullptr, "loca entry offset %d", offset);
		return offset >> right_shift;
	      })
    | hb_sink (dest)
    ;
  }

  // requires source of SubsetGlyph complains the identifier isn't declared
  template <typename Iterator>
  bool serialize (hb_serialize_context_t *c,
		  Iterator it,
		  const hb_subset_plan_t *plan)
  {
    TRACE_SERIALIZE (this);
    for (const auto &_ : it) _.serialize (c, plan);
    return_trace (true);
  }

  bool subset (hb_subset_context_t *c) const
  {
    TRACE_SUBSET (this);

    glyf *glyf_prime = c->serializer->start_embed <glyf> ();
    if (unlikely (!c->serializer->check_success (glyf_prime))) return_trace (false);

    // Byte region(s) per glyph to output
    // unpadded, hints removed if so requested
    // If we fail to process a glyph we produce an empty (0-length) glyph
    hb_vector_t<SubsetGlyph> glyphs;
    _populate_subset_glyphs (c->plan, &glyphs);

    glyf_prime->serialize (c->serializer, hb_iter (glyphs), c->plan);

    auto padded_offsets =
    + hb_iter (glyphs)
    | hb_map (&SubsetGlyph::padded_size)
    ;

    if (c->serializer->in_error ()) return_trace (false);
    return_trace (c->serializer->check_success (_add_loca_and_head (c->plan,
								    padded_offsets)));
  }

  template <typename SubsetGlyph>
  void
  _populate_subset_glyphs (const hb_subset_plan_t * plan,
			   hb_vector_t<SubsetGlyph> * glyphs /* OUT */) const
  {
    OT::glyf::accelerator_t glyf;
    glyf.init (plan->source);

    + hb_range (plan->num_output_glyphs ())
    | hb_map ([&] (hb_codepoint_t new_gid)
	      {
		SubsetGlyph subset_glyph = {0};
		subset_glyph.new_gid = new_gid;

		// should never fail: all old gids should be mapped
		if (!plan->old_gid_for_new_gid (new_gid, &subset_glyph.old_gid))
		  return subset_glyph;

		subset_glyph.source_glyph = glyf.bytes_for_glyph (subset_glyph.old_gid, true);
		if (plan->drop_hints) subset_glyph.drop_hints_bytes ();
		else subset_glyph.dest_start = subset_glyph.source_glyph;

		return subset_glyph;
	      })
    | hb_sink (glyphs)
    ;

    glyf.fini ();
  }

  static bool
  _add_head_and_set_loca_version (hb_subset_plan_t *plan, bool use_short_loca)
  {
    hb_blob_t *head_blob = hb_sanitize_context_t ().reference_table<head> (plan->source);
    hb_blob_t *head_prime_blob = hb_blob_copy_writable_or_fail (head_blob);
    hb_blob_destroy (head_blob);

    if (unlikely (!head_prime_blob))
      return false;

    head *head_prime = (head *) hb_blob_get_data_writable (head_prime_blob, nullptr);
    head_prime->indexToLocFormat = use_short_loca ? 0 : 1;
    bool success = plan->add_table (HB_OT_TAG_head, head_prime_blob);

    hb_blob_destroy (head_prime_blob);
    return success;
  }

  struct CompositeGlyphHeader
  {
    enum composite_glyph_flag_t
    {
      ARG_1_AND_2_ARE_WORDS =      0x0001,
      ARGS_ARE_XY_VALUES =         0x0002,
      ROUND_XY_TO_GRID =           0x0004,
      WE_HAVE_A_SCALE =            0x0008,
      MORE_COMPONENTS =            0x0020,
      WE_HAVE_AN_X_AND_Y_SCALE =   0x0040,
      WE_HAVE_A_TWO_BY_TWO =       0x0080,
      WE_HAVE_INSTRUCTIONS =       0x0100,
      USE_MY_METRICS =             0x0200,
      OVERLAP_COMPOUND =           0x0400,
      SCALED_COMPONENT_OFFSET =    0x0800,
      UNSCALED_COMPONENT_OFFSET =  0x1000
    };

    HBUINT16	flags;
    HBGlyphID	glyphIndex;

    unsigned int get_size () const
    {
      unsigned int size = min_size;
      // arg1 and 2 are int16
      if (flags & ARG_1_AND_2_ARE_WORDS) size += 4;
      // arg1 and 2 are int8
      else size += 2;

      // One x 16 bit (scale)
      if (flags & WE_HAVE_A_SCALE) size += 2;
      // Two x 16 bit (xscale, yscale)
      else if (flags & WE_HAVE_AN_X_AND_Y_SCALE) size += 4;
      // Four x 16 bit (xscale, scale01, scale10, yscale)
      else if (flags & WE_HAVE_A_TWO_BY_TWO) size += 8;

      return size;
    }

    bool is_anchored () const { return (flags & ARGS_ARE_XY_VALUES) == 0; }
    void get_anchor_points (unsigned int &point1, unsigned int &point2) const
    {
      const HBUINT8 *p = &StructAfter<const HBUINT8> (glyphIndex);
      if (flags & ARG_1_AND_2_ARE_WORDS)
      {
	point1 = ((const HBUINT16 *) p)[0];
	point2 = ((const HBUINT16 *) p)[1];
      }
      else
      {
	point1 = p[0];
	point2 = p[1];
      }
    }

    void transform_points (contour_point_vector_t &points) const
    {
      float matrix[4];
      contour_point_t trans;
      if (get_transformation (matrix, trans))
      {
	if (scaled_offsets ())
	{
	  points.translate (trans);
	  points.transform (matrix);
	}
	else
	{
	  points.transform (matrix);
	  points.translate (trans);
	}
      }
    }

    protected:
    bool scaled_offsets () const
    { return (flags & (SCALED_COMPONENT_OFFSET | UNSCALED_COMPONENT_OFFSET)) == SCALED_COMPONENT_OFFSET; }

    bool get_transformation (float (&matrix)[4], contour_point_t &trans) const
    {
      matrix[0] = matrix[3] = 1.f;
      matrix[1] = matrix[2] = 0.f;

      int tx, ty;
      const HBINT8 *p = &StructAfter<const HBINT8> (glyphIndex);
      if (flags & ARG_1_AND_2_ARE_WORDS)
      {
	tx = *(const HBINT16 *) p;
	p += HBINT16::static_size;
	ty = *(const HBINT16 *) p;
	p += HBINT16::static_size;
      }
      else
      {
	tx = *p++;
	ty = *p++;
      }
      if (is_anchored ()) tx = ty = 0;

      trans.init ((float) tx, (float) ty);

      {
	const F2DOT14 *points = (const F2DOT14 *) p;
	if (flags & WE_HAVE_A_SCALE)
	{
	  matrix[0] = matrix[3] = points[0].to_float ();
	  return true;
	}
	else if (flags & WE_HAVE_AN_X_AND_Y_SCALE)
	{
	  matrix[0] = points[0].to_float ();
	  matrix[3] = points[1].to_float ();
	  return true;
	}
	else if (flags & WE_HAVE_A_TWO_BY_TWO)
	{
	  matrix[0] = points[0].to_float ();
	  matrix[1] = points[1].to_float ();
	  matrix[2] = points[2].to_float ();
	  matrix[3] = points[3].to_float ();
	  return true;
	}
      }
      return tx || ty;
    }

    public:
    DEFINE_SIZE_MIN (4);
  };

  struct composite_iter_t : hb_iter_with_fallback_t<composite_iter_t, const CompositeGlyphHeader &>
  {
    typedef const CompositeGlyphHeader *__item_t__;
    composite_iter_t (hb_bytes_t glyph_, __item_t__ current_) : glyph (glyph_), current (current_)
    { if (!in_range (current)) current = nullptr; }
    composite_iter_t () : glyph (hb_bytes_t ()), current (nullptr) {}

    const CompositeGlyphHeader &__item__ () const { return *current; }
    bool __more__ () const { return current; }
    void __next__ ()
    {
      if (!(current->flags & CompositeGlyphHeader::MORE_COMPONENTS)) { current = nullptr; return; }

      const CompositeGlyphHeader *possible = &StructAfter<CompositeGlyphHeader,
							  CompositeGlyphHeader> (*current);
      if (!in_range (possible)) { current = nullptr; return; }
      current = possible;
    }
    bool operator != (const composite_iter_t& o) const
    { return glyph != o.glyph || current != o.current; }

    bool in_range (const CompositeGlyphHeader *composite) const
    {
      return glyph.sub_array ((const char *) composite - (const char *) &glyph,
			       CompositeGlyphHeader::min_size).as<CompositeGlyphHeader> () != &Null (CompositeGlyphHeader);
    }

    private:
    hb_bytes_t glyph;
    __item_t__ current;
  };

  struct GlyphHeader
  {
    struct SimpleHeader
    {
      const GlyphHeader &header;
      SimpleHeader (const GlyphHeader &header_) : header (header_) {}

      unsigned int instruction_len_offset () const
      { return static_size + 2 * header.numberOfContours; }

      unsigned int length (unsigned int instruction_len) const
      { return instruction_len_offset () + 2 + instruction_len; }

      unsigned int instructions_length (hb_bytes_t glyph) const
      {
	unsigned int instruction_length_offset = instruction_len_offset ();
	if (unlikely (instruction_length_offset + 2 > glyph.length)) return 0;

	const HBUINT16 &instructionLength = StructAtOffset<HBUINT16> (&glyph, instruction_length_offset);
	/* Out of bounds of the current glyph */
	if (unlikely (length (instructionLength) > glyph.length)) return 0;
	return instructionLength;
      }

      enum simple_glyph_flag_t
      {
	FLAG_ON_CURVE  = 0x01,
	FLAG_X_SHORT   = 0x02,
	FLAG_Y_SHORT   = 0x04,
	FLAG_REPEAT    = 0x08,
	FLAG_X_SAME    = 0x10,
	FLAG_Y_SAME    = 0x20,
	FLAG_RESERVED1 = 0x40,
	FLAG_RESERVED2 = 0x80
      };

      hb_bytes_t bytes_without_padding (hb_bytes_t glyph_bytes) const
      {
	/* based on FontTools _g_l_y_f.py::trim */
	const char *glyph = glyph_bytes.arrayZ;
	const char *glyph_end = glyph + glyph_bytes.length;
	/* simple glyph w/contours, possibly trimmable */
	glyph += instruction_len_offset ();

	if (unlikely (glyph + 2 >= glyph_end)) return hb_bytes_t ();
	unsigned int num_coordinates = StructAtOffset<HBUINT16> (glyph - 2, 0) + 1;
	unsigned int num_instructions = StructAtOffset<HBUINT16> (glyph, 0);

	glyph += 2 + num_instructions;
	if (unlikely (glyph + 2 >= glyph_end)) return hb_bytes_t ();

	unsigned int coord_bytes = 0;
	unsigned int coords_with_flags = 0;
	while (glyph < glyph_end)
	{
	  uint8_t flag = *glyph;
	  glyph++;

	  unsigned int repeat = 1;
	  if (flag & FLAG_REPEAT)
	  {
	    if (unlikely (glyph >= glyph_end)) return hb_bytes_t ();
	    repeat = *glyph + 1;
	    glyph++;
	  }

	  unsigned int xBytes, yBytes;
	  xBytes = yBytes = 0;
	  if (flag & FLAG_X_SHORT) xBytes = 1;
	  else if ((flag & FLAG_X_SAME) == 0) xBytes = 2;

	  if (flag & FLAG_Y_SHORT) yBytes = 1;
	  else if ((flag & FLAG_Y_SAME) == 0) yBytes = 2;

	  coord_bytes += (xBytes + yBytes) * repeat;
	  coords_with_flags += repeat;
	  if (coords_with_flags >= num_coordinates) break;
	}

	if (unlikely (coords_with_flags != num_coordinates)) return hb_bytes_t ();
	return glyph_bytes.sub_array (0, glyph_bytes.length + coord_bytes - (glyph_end - glyph));
      }

      /* zero instruction length */
      void drop_hints ()
      {
	GlyphHeader &glyph_header = const_cast<GlyphHeader &> (header);
	(HBUINT16 &) StructAtOffset<HBUINT16> (&glyph_header, instruction_len_offset ()) = 0;
      }

      void drop_hints_bytes (hb_bytes_t source_glyph, hb_bytes_t &dest_start, hb_bytes_t &dest_end) const
      {
	unsigned int instructions_len = instructions_length (source_glyph);
	unsigned int glyph_length = length (instructions_len);
	dest_start = source_glyph.sub_array (0, glyph_length - instructions_len);
	dest_end = source_glyph.sub_array (glyph_length, source_glyph.length - glyph_length);
      }
    };

    struct CompositeHeader
    {
      const GlyphHeader &header;
      CompositeHeader (const GlyphHeader &header_) : header (header_) {}

      composite_iter_t get_iterator (hb_bytes_t glyph) const
      { return composite_iter_t (glyph, &StructAfter<CompositeGlyphHeader, GlyphHeader> (header)); }

      unsigned int instructions_length (hb_bytes_t glyph) const
      {
	unsigned int start = glyph.length;
	unsigned int end = glyph.length;
	const CompositeGlyphHeader *last = nullptr;
	for (auto &item : get_iterator (glyph))
	  last = &item;
	if (unlikely (!last)) return 0;

	if ((uint16_t) last->flags & CompositeGlyphHeader::WE_HAVE_INSTRUCTIONS)
	  start = (char *) last - &glyph + last->get_size ();
	if (unlikely (start > end)) return 0;
	return end - start;
      }

      /* Trimming for composites not implemented.
       * If removing hints it falls out of that. */
      hb_bytes_t bytes_without_padding (hb_bytes_t glyph_bytes) const
      { return glyph_bytes; }

      /* remove WE_HAVE_INSTRUCTIONS flag from composite glyph */
      void drop_hints (hb_bytes_t glyph_bytes)
      {
	for (const auto &_ : get_iterator (glyph_bytes))
	{
	  HBUINT16 &flags = *const_cast<OT::HBUINT16 *> (&_.flags);
	  flags = (uint16_t) flags & ~OT::glyf::CompositeGlyphHeader::WE_HAVE_INSTRUCTIONS;
	}
      }

      /* Chop instructions off the end */
      void drop_hints_bytes (hb_bytes_t source_glyph, hb_bytes_t &dest_start) const
      { dest_start = source_glyph.sub_array (0, source_glyph.length - instructions_length (source_glyph)); }
    };

    enum glyph_type_t { EMPTY, SIMPLE, COMPOSITE };

    unsigned get_type () const
    {
      if (is_simple_glyph ()) return SIMPLE;
      else if (is_composite_glyph ()) return COMPOSITE;
      else return EMPTY;
    }

    composite_iter_t get_composite_iterator (hb_bytes_t glyph) const
    {
      if (!is_composite_glyph ()) return composite_iter_t ();
      return CompositeHeader (*this).get_iterator (glyph);
    }

    hb_bytes_t bytes_without_padding (hb_bytes_t glyph_bytes) const
    {
      switch (get_type ())
      {
      case COMPOSITE: return CompositeHeader (*this).bytes_without_padding (glyph_bytes);
      case SIMPLE:    return SimpleHeader (*this).bytes_without_padding (glyph_bytes);
      default:        return glyph_bytes;
      }
    }

    void drop_hints (hb_bytes_t glyph_bytes)
    {
      switch (get_type ())
      {
      case COMPOSITE: CompositeHeader (*this).drop_hints (glyph_bytes); return;
      case SIMPLE:    SimpleHeader (*this).drop_hints (); return;
      default:        return;
      }
    }

    void drop_hints_bytes (hb_bytes_t source_glyph, hb_bytes_t &dest_start, hb_bytes_t &dest_end) const
    {
      switch (get_type ())
      {
      case COMPOSITE: CompositeHeader (*this).drop_hints_bytes (source_glyph, dest_start); return;
      case SIMPLE:    SimpleHeader (*this).drop_hints_bytes (source_glyph, dest_start, dest_end); return;
      default:        return;
      }
    }

    bool has_data ()           const { return numberOfContours; }
    bool is_simple_glyph ()    const { return numberOfContours > 0; }
    bool is_composite_glyph () const { return numberOfContours < 0; }

    HBINT16	numberOfContours;
			/* If the number of contours is
			 * greater than or equal to zero,
			 * this is a simple glyph; if negative,
			 * this is a composite glyph. */
    FWORD	xMin;	/* Minimum x for coordinate data. */
    FWORD	yMin;	/* Minimum y for coordinate data. */
    FWORD	xMax;	/* Maximum x for coordinate data. */
    FWORD	yMax;	/* Maximum y for coordinate data. */

    DEFINE_SIZE_STATIC (10);
  };

  struct accelerator_t
  {
    void init (hb_face_t *face_)
    {
      short_offset = false;
      num_glyphs = 0;
      loca_table = nullptr;
      glyf_table = nullptr;
      face = face_;
      const OT::head &head = *face->table.head;
      if (head.indexToLocFormat > 1 || head.glyphDataFormat != 0)
	/* Unknown format.  Leave num_glyphs=0, that takes care of disabling us. */
	return;
      short_offset = 0 == head.indexToLocFormat;

      loca_table = hb_sanitize_context_t ().reference_table<loca> (face);
      glyf_table = hb_sanitize_context_t ().reference_table<glyf> (face);

      num_glyphs = hb_max (1u, loca_table.get_length () / (short_offset ? 2 : 4)) - 1;
    }

    void fini ()
    {
      loca_table.destroy ();
      glyf_table.destroy ();
    }

    enum simple_glyph_flag_t
    {
      FLAG_ON_CURVE  = 0x01,
      FLAG_X_SHORT   = 0x02,
      FLAG_Y_SHORT   = 0x04,
      FLAG_REPEAT    = 0x08,
      FLAG_X_SAME    = 0x10,
      FLAG_Y_SAME    = 0x20,
      FLAG_RESERVED1 = 0x40,
      FLAG_RESERVED2 = 0x80
    };

    enum phantom_point_index_t
    {
      PHANTOM_LEFT   = 0,
      PHANTOM_RIGHT  = 1,
      PHANTOM_TOP    = 2,
      PHANTOM_BOTTOM = 3,
      PHANTOM_COUNT  = 4
    };

    protected:
    struct x_setter_t
    {
      void set (contour_point_t &point, float v) const { point.x = v; }
      bool is_short (uint8_t flag) const { return flag & FLAG_X_SHORT; }
      bool is_same  (uint8_t flag) const { return flag & FLAG_X_SAME; }
    };

    struct y_setter_t
    {
      void set (contour_point_t &point, float v) const { point.y = v; }
      bool is_short (uint8_t flag) const { return flag & FLAG_Y_SHORT; }
      bool is_same  (uint8_t flag) const { return flag & FLAG_Y_SAME; }
    };

    template <typename T>
    static bool read_points (const HBUINT8 *&p /* IN/OUT */,
			     contour_point_vector_t &points_ /* IN/OUT */,
			     const range_checker_t &checker)
    {
      T coord_setter;
      float v = 0;
      for (unsigned int i = 0; i < points_.length - PHANTOM_COUNT; i++)
      {
	uint8_t flag = points_[i].flag;
	if (coord_setter.is_short (flag))
	{
	  if (unlikely (!checker.in_range (p))) return false;
	  if (coord_setter.is_same (flag))
	    v += *p++;
	  else
	    v -= *p++;
	}
	else
	{
	  if (!coord_setter.is_same (flag))
	  {
	    if (unlikely (!checker.in_range ((const HBUINT16 *)p))) return false;
	    v += *(const HBINT16 *) p;
	    p += HBINT16::static_size;
	  }
	}
	coord_setter.set (points_[i], v);
      }
      return true;
    }

    void init_phantom_points (hb_codepoint_t glyph, hb_array_t<contour_point_t> &phantoms /* IN/OUT */) const
    {
      const GlyphHeader &header = *bytes_for_glyph (glyph).as<GlyphHeader> ();
      int h_delta = (int) header.xMin - face->table.hmtx->get_side_bearing (glyph);
      int v_orig  = (int) header.yMax + face->table.vmtx->get_side_bearing (glyph);
      unsigned int h_adv = face->table.hmtx->get_advance (glyph);
      unsigned int v_adv = face->table.vmtx->get_advance (glyph);

      phantoms[PHANTOM_LEFT].x = h_delta;
      phantoms[PHANTOM_RIGHT].x = h_adv + h_delta;
      phantoms[PHANTOM_TOP].y = v_orig;
      phantoms[PHANTOM_BOTTOM].y = v_orig - (int) v_adv;
    }

    /* for a simple glyph, return contour end points, flags, along with coordinate points
     * for a composite glyph, return pseudo component points
     * in both cases points trailed with four phantom points
     */
    bool get_contour_points (hb_codepoint_t glyph,
			     contour_point_vector_t &points_ /* OUT */,
			     hb_vector_t<unsigned int> &end_points_ /* OUT */,
			     const bool phantom_only=false) const
    {
      unsigned int num_points = 0;
      hb_bytes_t bytes = bytes_for_glyph (glyph);
      const GlyphHeader &glyph_header = *bytes.as<GlyphHeader> ();
      if (glyph_header.is_composite_glyph ())
      {
	/* add one pseudo point for each component in composite glyph */
	num_points += hb_len (glyph_header.get_composite_iterator (bytes));
	points_.resize (num_points + PHANTOM_COUNT);
	for (unsigned int i = 0; i < points_.length; i++) points_[i].init ();
	return true;
      }
      else if (glyph_header.is_simple_glyph ())
      {
	const HBUINT16 *end_pts = &StructAfter<HBUINT16, GlyphHeader> (glyph_header);
	range_checker_t checker (bytes.arrayZ, 0, bytes.length);
	num_points = 0;
	int num_contours = glyph_header.numberOfContours;
	if (unlikely (!checker.in_range (&end_pts[num_contours + 1]))) return false;
	num_points = end_pts[glyph_header.numberOfContours - 1] + 1;

	points_.resize (num_points + PHANTOM_COUNT);
	for (unsigned int i = 0; i < points_.length; i++) points_[i].init ();
	if (phantom_only) return true;

	/* Read simple glyph points if !phantom_only */
	end_points_.resize (num_contours);

	for (int i = 0; i < num_contours; i++)
	  end_points_[i] = end_pts[i];

	/* Skip instructions */
	const HBUINT8 *p = &StructAtOffset<HBUINT8> (&end_pts[num_contours + 1],
						     end_pts[num_contours]);

	/* Read flags */
	for (unsigned int i = 0; i < num_points; i++)
	{
	  if (unlikely (!checker.in_range (p))) return false;
	  uint8_t flag = *p++;
	  points_[i].flag = flag;
	  if ((flag & FLAG_REPEAT) != 0)
	  {
	    if (unlikely (!checker.in_range (p))) return false;
	    unsigned int repeat_count = *p++;
	    while ((repeat_count-- > 0) && (++i < num_points))
	      points_[i].flag = flag;
	  }
	}

	/* Read x & y coordinates */
	return (read_points<x_setter_t> (p, points_, checker) &&
		read_points<y_setter_t> (p, points_, checker));
      }
      else
      {
	/* empty glyph */
	points_.resize (PHANTOM_COUNT);
	for (unsigned int i = 0; i < points_.length; i++) points_[i].init ();
	return true;
      }
    }

    struct contour_bounds_t
    {
      contour_bounds_t () { min.x = min.y = FLT_MAX; max.x = max.y = -FLT_MAX; }

      void add (const contour_point_t &p)
      {
	min.x = hb_min (min.x, p.x);
	min.y = hb_min (min.y, p.y);
	max.x = hb_max (max.x, p.x);
	max.y = hb_max (max.y, p.y);
      }

      bool empty () const { return (min.x >= max.x) || (min.y >= max.y); }

      contour_point_t	min;
      contour_point_t	max;
    };

#ifndef HB_NO_VAR
    /* Note: Recursively calls itself.
     * all_points includes phantom points
     */
    bool get_points_var (hb_codepoint_t glyph,
			 const int *coords, unsigned int coord_count,
			 contour_point_vector_t &all_points /* OUT */,
			 unsigned int depth = 0) const
    {
      if (unlikely (depth++ > HB_MAX_NESTING_LEVEL)) return false;
      contour_point_vector_t points;
      hb_vector_t<unsigned int> end_points;
      if (unlikely (!get_contour_points (glyph, points, end_points))) return false;
      hb_array_t<contour_point_t> phantoms = points.sub_array (points.length - PHANTOM_COUNT, PHANTOM_COUNT);
      init_phantom_points (glyph, phantoms);
      if (unlikely (!face->table.gvar->apply_deltas_to_points (glyph, coords, coord_count, points.as_array (), end_points.as_array ()))) return false;

      unsigned int comp_index = 0;
      hb_bytes_t bytes = bytes_for_glyph (glyph);
      const GlyphHeader &glyph_header = *bytes.as<GlyphHeader> ();
      if (glyph_header.is_simple_glyph ())
	all_points.extend (points.as_array ());
      else if (glyph_header.is_composite_glyph ())
      {
	for (auto &item : glyph_header.get_composite_iterator (bytes))
	{
	  contour_point_vector_t comp_points;
	  if (unlikely (!get_points_var (item.glyphIndex, coords, coord_count,
					 comp_points, depth))
			|| comp_points.length < PHANTOM_COUNT)
	    return false;

	  /* Copy phantom points from component if USE_MY_METRICS flag set */
	  if (item.flags & CompositeGlyphHeader::USE_MY_METRICS)
	    for (unsigned int i = 0; i < PHANTOM_COUNT; i++)
	      phantoms[i] = comp_points[comp_points.length - PHANTOM_COUNT + i];

	  /* Apply component transformation & translation */
	  item.transform_points (comp_points);

	  /* Apply translatation from gvar */
	  comp_points.translate (points[comp_index]);

	  if (item.is_anchored ())
	  {
	    unsigned int p1, p2;
	    item.get_anchor_points (p1, p2);
	    if (likely (p1 < all_points.length && p2 < comp_points.length))
	    {
	      contour_point_t delta;
	      delta.init (all_points[p1].x - comp_points[p2].x,
			  all_points[p1].y - comp_points[p2].y);

	      comp_points.translate (delta);
	    }
	  }

	  all_points.extend (comp_points.sub_array (0, comp_points.length - PHANTOM_COUNT));

	  comp_index++;
	}

	all_points.extend (phantoms);
      }
      else return false;

      return true;
    }

    bool get_var_extents_and_phantoms (hb_font_t *font, hb_codepoint_t glyph,
				       hb_glyph_extents_t *extents=nullptr /* OUT */,
				       contour_point_vector_t *phantoms=nullptr /* OUT */) const
    {
      contour_point_vector_t all_points;
      if (unlikely (!get_points_var (glyph, font->coords, font->num_coords, all_points) ||
		    all_points.length < PHANTOM_COUNT)) return false;

      /* Undocumented rasterizer behavior:
       * Shift points horizontally by the updated left side bearing
       */
      contour_point_t delta;
      delta.init (-all_points[all_points.length - PHANTOM_COUNT + PHANTOM_LEFT].x, 0.f);
      if (delta.x != 0.f) all_points.translate (delta);

      if (extents != nullptr)
      {
	contour_bounds_t bounds;
	for (unsigned int i = 0; i + PHANTOM_COUNT < all_points.length; i++)
	  bounds.add (all_points[i]);

	if (bounds.min.x > bounds.max.x)
	{
	  extents->width = 0;
	  extents->x_bearing = 0;
	}
	else
	{
	  extents->x_bearing = font->em_scalef_x (bounds.min.x);
	  extents->width = font->em_scalef_x (bounds.max.x - bounds.min.x);
	}
	if (bounds.min.y > bounds.max.y)
	{
	  extents->height = 0;
	  extents->y_bearing = 0;
	}
	else
	{
	  extents->y_bearing = font->em_scalef_y (bounds.max.y);
	  extents->height = font->em_scalef_y (bounds.min.y - bounds.max.y);
	}
      }
      if (phantoms)
	for (unsigned int i = 0; i < PHANTOM_COUNT; i++)
	  (*phantoms)[i] = all_points[all_points.length - PHANTOM_COUNT + i];
      return true;
    }

    bool get_var_metrics (hb_font_t *font, hb_codepoint_t glyph,
			  contour_point_vector_t &phantoms) const
    { return get_var_extents_and_phantoms (font, glyph, nullptr, &phantoms); }

    bool get_extents_var (hb_font_t *font, hb_codepoint_t glyph,
			  hb_glyph_extents_t *extents) const
    { return get_var_extents_and_phantoms (font, glyph,  extents); }
#endif

    public:
#ifndef HB_NO_VAR
    unsigned int get_advance_var (hb_font_t *font, hb_codepoint_t glyph,
				  bool is_vertical) const
    {
      bool success = false;
      contour_point_vector_t phantoms;
      phantoms.resize (PHANTOM_COUNT);

      if (likely (font->num_coords == face->table.gvar->get_axis_count ()))
	success = get_var_metrics (font, glyph, phantoms);

      if (unlikely (!success))
	return is_vertical ? face->table.vmtx->get_advance (glyph) : face->table.hmtx->get_advance (glyph);

      if (is_vertical)
	return roundf (phantoms[PHANTOM_TOP].y - phantoms[PHANTOM_BOTTOM].y);
      else
	return roundf (phantoms[PHANTOM_RIGHT].x - phantoms[PHANTOM_LEFT].x);
    }

    int get_side_bearing_var (hb_font_t *font, hb_codepoint_t glyph, bool is_vertical) const
    {
      hb_glyph_extents_t extents;
      contour_point_vector_t phantoms;
      phantoms.resize (PHANTOM_COUNT);

      if (unlikely (!get_var_extents_and_phantoms (font, glyph, &extents, &phantoms)))
	return is_vertical ? face->table.vmtx->get_side_bearing (glyph) : face->table.hmtx->get_side_bearing (glyph);

      return is_vertical ? ceil (phantoms[PHANTOM_TOP].y) - extents.y_bearing : floor (phantoms[PHANTOM_LEFT].x);
    }
#endif

    bool get_extents (hb_font_t *font, hb_codepoint_t glyph, hb_glyph_extents_t *extents) const
    {
#ifndef HB_NO_VAR
      unsigned int coord_count;
      const int *coords = hb_font_get_var_coords_normalized (font, &coord_count);
      if (coords && coord_count > 0 && coord_count == face->table.gvar->get_axis_count ())
	return get_extents_var (font, glyph, extents);
#endif

      if (unlikely (glyph >= num_glyphs)) return false;

      const GlyphHeader &glyph_header = *bytes_for_glyph (glyph).as<GlyphHeader> ();
      if (unlikely (!glyph_header.has_data ()))
	return true; /* Empty glyph; zero extents. */

      /* Undocumented rasterizer behavior: shift glyph to the left by (lsb - xMin), i.e., xMin = lsb */
      /* extents->x_bearing = hb_min (glyph_header.xMin, glyph_header.xMax); */
      extents->x_bearing = font->em_scale_x (face->table.hmtx->get_side_bearing (glyph));
      extents->y_bearing = font->em_scale_y (hb_max (glyph_header.yMin, glyph_header.yMax));
      extents->width     = font->em_scale_x (hb_max (glyph_header.xMin, glyph_header.xMax) - hb_min (glyph_header.xMin, glyph_header.xMax));
      extents->height    = font->em_scale_y (hb_min (glyph_header.yMin, glyph_header.yMax) - hb_max (glyph_header.yMin, glyph_header.yMax));

      return true;
    }

    hb_bytes_t bytes_for_glyph (hb_codepoint_t gid,
				bool needs_padding_removal = false) const
    {
      unsigned int start_offset, end_offset;
      if (unlikely (gid >= num_glyphs)) return hb_bytes_t ();

      if (short_offset)
      {
	const HBUINT16 *offsets = (const HBUINT16 *) loca_table->dataZ.arrayZ;
	start_offset = 2 * offsets[gid];
	end_offset   = 2 * offsets[gid + 1];
      }
      else
      {
	const HBUINT32 *offsets = (const HBUINT32 *) loca_table->dataZ.arrayZ;
	start_offset = offsets[gid];
	end_offset   = offsets[gid + 1];
      }

      if (unlikely (start_offset > end_offset || end_offset > glyf_table.get_length ()))
	return hb_bytes_t ();

      hb_bytes_t glyph_bytes ((const char *) this->glyf_table + start_offset,
			      end_offset - start_offset);

      const GlyphHeader &glyph_header = *glyph_bytes.as<GlyphHeader> ();

      /* Empty glyph or its size is smaller than minimum header */
      if (!glyph_header.has_data ()) return hb_bytes_t ();

      if (!needs_padding_removal) return glyph_bytes;

      return glyph_header.bytes_without_padding (glyph_bytes);
    }

    void
    add_gid_and_children (hb_codepoint_t gid, hb_set_t *gids_to_retain,
			  unsigned int depth = 0) const
    {
      if (unlikely (depth++ > HB_MAX_NESTING_LEVEL)) return;
      /* Check if is already visited */
      if (gids_to_retain->has (gid)) return;

      gids_to_retain->add (gid);

      hb_bytes_t glyph_bytes = bytes_for_glyph (gid);
      const GlyphHeader &glyph_header = *glyph_bytes.as<GlyphHeader> ();
      for (auto &item : glyph_header.get_composite_iterator (glyph_bytes))
        add_gid_and_children (item.glyphIndex, gids_to_retain, depth);
    }

    private:
    bool short_offset;
    unsigned int num_glyphs;
    hb_blob_ptr_t<loca> loca_table;
    hb_blob_ptr_t<glyf> glyf_table;
    hb_face_t *face;
  };

  struct SubsetGlyph
  {
    hb_codepoint_t new_gid;
    hb_codepoint_t old_gid;
    hb_bytes_t source_glyph;
    hb_bytes_t dest_start;  /* region of source_glyph to copy first */
    hb_bytes_t dest_end;    /* region of source_glyph to copy second */

    bool serialize (hb_serialize_context_t *c,
		    const hb_subset_plan_t *plan) const
    {
      TRACE_SERIALIZE (this);

      hb_bytes_t dest_glyph = dest_start.copy (c);
      dest_glyph = hb_bytes_t (&dest_glyph, dest_glyph.length + dest_end.copy (c).length);
      unsigned int pad_length = padding ();
      DEBUG_MSG (SUBSET, nullptr, "serialize %d byte glyph, width %d pad %d", dest_glyph.length, dest_glyph.length  + pad_length, pad_length);

      HBUINT8 pad;
      pad = 0;
      while (pad_length > 0)
      {
	c->embed (pad);
	pad_length--;
      }

      if (!unlikely (dest_glyph.length)) return_trace (true);

      /* update components gids */
      GlyphHeader &glyph_header = *const_cast<GlyphHeader *> (dest_glyph.as<GlyphHeader> ());
      for (auto &_ : glyph_header.get_composite_iterator (dest_glyph))
      {
	hb_codepoint_t new_gid;
	if (plan->new_gid_for_old_gid (_.glyphIndex, &new_gid))
	  ((OT::glyf::CompositeGlyphHeader *) &_)->glyphIndex = new_gid;
      }

      if (plan->drop_hints) glyph_header.drop_hints (dest_glyph);

      return_trace (true);
    }

    void drop_hints_bytes ()
    { source_glyph.as<GlyphHeader> ()->drop_hints_bytes (source_glyph, dest_start, dest_end); }

    unsigned int      length () const { return dest_start.length + dest_end.length; }
    /* pad to 2 to ensure 2-byte loca will be ok */
    unsigned int     padding () const { return length () % 2; }
    unsigned int padded_size () const { return length () + padding (); }
  };

  protected:
  UnsizedArrayOf<HBUINT8>
		dataZ;	/* Glyphs data. */
  public:
  DEFINE_SIZE_MIN (0);	/* In reality, this is UNBOUNDED() type; but since we always
			 * check the size externally, allow Null() object of it by
			 * defining it _MIN instead. */
};

struct glyf_accelerator_t : glyf::accelerator_t {};

} /* namespace OT */


#endif /* HB_OT_GLYF_TABLE_HH */
