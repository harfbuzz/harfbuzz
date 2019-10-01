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
  UnsizedArrayOf<HBUINT8>	dataZ;		/* Location data. */
  public:
  DEFINE_SIZE_MIN (0); /* In reality, this is UNBOUNDED() type; but since we always
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

		subset_glyph.source_glyph = glyf.bytes_for_glyph ((const char *) this,
								  subset_glyph.old_gid);
		if (plan->drop_hints) subset_glyph.drop_hints (glyf);
		else subset_glyph.dest_start = subset_glyph.source_glyph;

		return subset_glyph;
	      })
    | hb_sink (glyphs)
    ;

    glyf.fini ();
  }

  static void
  _fix_component_gids (const hb_subset_plan_t *plan,
		       hb_bytes_t glyph)
  {
    OT::glyf::CompositeGlyphHeader::Iterator iterator;
    if (OT::glyf::CompositeGlyphHeader::get_iterator (&glyph,
						      glyph.length,
						      &iterator))
    {
      do
      {
	hb_codepoint_t new_gid;
	if (!plan->new_gid_for_old_gid (iterator.current->glyphIndex,
					&new_gid))
	  continue;
	((OT::glyf::CompositeGlyphHeader *) iterator.current)->glyphIndex = new_gid;
      } while (iterator.move_to_next ());
    }
  }

  static void
  _zero_instruction_length (hb_bytes_t glyph)
  {
    const GlyphHeader &glyph_header = *glyph.as<GlyphHeader> ();
    if (!glyph_header.is_simple_glyph ()) return;  // only for simple glyphs

    unsigned int instruction_len_offset = glyph_header.simple_instruction_len_offset ();
    const HBUINT16 &instruction_len = StructAtOffset<HBUINT16> (&glyph, instruction_len_offset);
    (HBUINT16 &) instruction_len = 0;
  }

  static bool _remove_composite_instruction_flag (hb_bytes_t glyph)
  {
    const GlyphHeader &glyph_header = *glyph.as<GlyphHeader> ();
    if (!glyph_header.is_composite_glyph ()) return true;  // only for composites

    /* remove WE_HAVE_INSTRUCTIONS from flags in dest */
    OT::glyf::CompositeGlyphHeader::Iterator composite_it;
    if (unlikely (!OT::glyf::CompositeGlyphHeader::get_iterator (&glyph, glyph.length,
								 &composite_it)))
      return false;
    const OT::glyf::CompositeGlyphHeader *composite_header;
    do
    {
      composite_header = composite_it.current;
      OT::HBUINT16 *flags = const_cast<OT::HBUINT16 *> (&composite_header->flags);
      *flags = (uint16_t) *flags & ~OT::glyf::CompositeGlyphHeader::WE_HAVE_INSTRUCTIONS;
    } while (composite_it.move_to_next ());
    return true;
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

  struct GlyphHeader
  {
    unsigned int simple_instruction_len_offset () const
    { return static_size + 2 * numberOfContours; }

    unsigned int simple_length (unsigned int instruction_len) const
    { return simple_instruction_len_offset () + 2 + instruction_len; }

    bool is_composite_glyph () const { return numberOfContours < 0; }
    bool is_simple_glyph () const    { return numberOfContours > 0; }

    bool has_data () const { return numberOfContours; }

    HBINT16		numberOfContours;	/* If the number of contours is
						 * greater than or equal to zero,
						 * this is a simple glyph; if negative,
						 * this is a composite glyph. */
    FWORD		xMin;			/* Minimum x for coordinate data. */
    FWORD		yMin;			/* Minimum y for coordinate data. */
    FWORD		xMax;			/* Maximum x for coordinate data. */
    FWORD		yMax;			/* Maximum y for coordinate data. */

    DEFINE_SIZE_STATIC (10);
  };

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
	point1 = ((const HBUINT16 *)p)[0];
	point2 = ((const HBUINT16 *)p)[1];
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
    { return (flags & (SCALED_COMPONENT_OFFSET|UNSCALED_COMPONENT_OFFSET)) == SCALED_COMPONENT_OFFSET; }

    bool get_transformation (float (&matrix)[4], contour_point_t &trans) const
    {
      matrix[0] = matrix[3] = 1.f;
      matrix[1] = matrix[2] = 0.f;

      int tx, ty;
      const HBINT8 *p = &StructAfter<const HBINT8> (glyphIndex);
      if (flags & ARG_1_AND_2_ARE_WORDS)
      {
	tx = *(const HBINT16 *)p;
	p += HBINT16::static_size;
	ty = *(const HBINT16 *)p;
	p += HBINT16::static_size;
      }
      else
      {
	tx = *p++;
	ty = *p++;
      }
      if (is_anchored ()) tx = ty = 0;

      trans.init ((float)tx, (float)ty);

      if (flags & WE_HAVE_A_SCALE)
      {
	matrix[0] = matrix[3] = ((const F2DOT14*)p)->to_float ();
	return true;
      }
      else if (flags & WE_HAVE_AN_X_AND_Y_SCALE)
      {
	matrix[0] = ((const F2DOT14*)p)[0].to_float ();
	matrix[3] = ((const F2DOT14*)p)[1].to_float ();
	return true;
      }
      else if (flags & WE_HAVE_A_TWO_BY_TWO)
      {
	matrix[0] = ((const F2DOT14*)p)[0].to_float ();
	matrix[1] = ((const F2DOT14*)p)[1].to_float ();
	matrix[2] = ((const F2DOT14*)p)[2].to_float ();
	matrix[3] = ((const F2DOT14*)p)[3].to_float ();
	return true;
      }
      return tx || ty;
    }

    public:
    // TODO rewrite using new iterator framework if possible
    struct Iterator
    {
      const char *glyph_start;
      const char *glyph_end;
      const CompositeGlyphHeader *current;

      bool move_to_next ()
      {
	if (current->flags & CompositeGlyphHeader::MORE_COMPONENTS)
	{
	  const CompositeGlyphHeader *possible =
	    &StructAfter<CompositeGlyphHeader, CompositeGlyphHeader> (*current);
	  if (unlikely (!in_range (possible))) return false;
	  current = possible;
	  return true;
	}
	return false;
      }

      bool in_range (const CompositeGlyphHeader *composite) const
      {
	return (const char *) composite >= glyph_start
	    && ((const char *) composite + CompositeGlyphHeader::min_size) <= glyph_end
	    && ((const char *) composite + composite->get_size ()) <= glyph_end;
      }
    };

    static bool get_iterator (const char * glyph_data,
			      unsigned int length,
			      CompositeGlyphHeader::Iterator *iterator /* OUT */)
    {
      const GlyphHeader &glyph_header = *hb_bytes_t (glyph_data, length).as<GlyphHeader> ();
      if (!glyph_header.has_data ()) return false; /* Empty glyph; zero extents. */

      if (glyph_header.is_composite_glyph ())
      {
	const CompositeGlyphHeader *possible =
	  &StructAfter<CompositeGlyphHeader, GlyphHeader> (glyph_header);

	iterator->glyph_start = glyph_data;
	iterator->glyph_end = (const char *) glyph_data + length;
	if (!iterator->in_range (possible))
	  return false;
	iterator->current = possible;
	return true;
      }

      return false;
    }

    DEFINE_SIZE_MIN (4);
  };

  struct accelerator_t
  {
    void init (hb_face_t *face)
    {
      memset (this, 0, sizeof (accelerator_t));

      const OT::head &head = *face->table.head;
      if (head.indexToLocFormat > 1 || head.glyphDataFormat != 0)
	/* Unknown format.  Leave num_glyphs=0, that takes care of disabling us. */
	return;
      short_offset = 0 == head.indexToLocFormat;

      loca_table = hb_sanitize_context_t ().reference_table<loca> (face);
      glyf_table = hb_sanitize_context_t ().reference_table<glyf> (face);

      num_glyphs = hb_max (1u, loca_table.get_length () / (short_offset ? 2 : 4)) - 1;

      gvar_accel.init (face);
      hmtx_accel.init (face);
      vmtx_accel.init (face);
    }

    void fini ()
    {
      loca_table.destroy ();
      glyf_table.destroy ();
      gvar_accel.fini ();
      hmtx_accel.fini ();
      vmtx_accel.fini ();
    }

    /*
     * Returns true if the referenced glyph is a valid glyph and a composite glyph.
     * If true is returned a pointer to the composite glyph will be written into
     * composite.
     */
    bool get_composite (hb_codepoint_t glyph,
			CompositeGlyphHeader::Iterator *composite /* OUT */) const
    {
      if (unlikely (!num_glyphs))
	return false;

      unsigned int start_offset, end_offset;
      if (!get_offsets (glyph, &start_offset, &end_offset))
	return false; /* glyph not found */

      return CompositeGlyphHeader::get_iterator ((const char *) this->glyf_table + start_offset,
						 end_offset - start_offset,
						 composite);
    }

    enum simple_glyph_flag_t
    {
      FLAG_ON_CURVE = 0x01,
      FLAG_X_SHORT = 0x02,
      FLAG_Y_SHORT = 0x04,
      FLAG_REPEAT = 0x08,
      FLAG_X_SAME = 0x10,
      FLAG_Y_SAME = 0x20,
      FLAG_RESERVED1 = 0x40,
      FLAG_RESERVED2 = 0x80
    };

    enum phantom_point_index_t {
      PHANTOM_LEFT = 0,
      PHANTOM_RIGHT = 1,
      PHANTOM_TOP = 2,
      PHANTOM_BOTTOM = 3,
      PHANTOM_COUNT = 4
    };

    protected:
    const GlyphHeader &get_header (hb_codepoint_t glyph) const
    {
      unsigned int start_offset, end_offset;
      if (!get_offsets (glyph, &start_offset, &end_offset) || end_offset - start_offset < GlyphHeader::static_size)
	return Null(GlyphHeader);

      return StructAtOffset<GlyphHeader> (glyf_table, start_offset);
    }

    struct x_setter_t
    {
      void set (contour_point_t &point, float v) const { point.x = v; }
      bool is_short (uint8_t flag) const { return (flag & FLAG_X_SHORT) != 0; }
      bool is_same (uint8_t flag) const { return (flag & FLAG_X_SAME) != 0; }
    };

    struct y_setter_t
    {
      void set (contour_point_t &point, float v) const { point.y = v; }
      bool is_short (uint8_t flag) const { return (flag & FLAG_Y_SHORT) != 0; }
      bool is_same (uint8_t flag) const { return (flag & FLAG_Y_SAME) != 0; }
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
	    v += *(const HBINT16 *)p;
	    p += HBINT16::static_size;
	  }
	}
	coord_setter.set (points_[i], v);
      }
      return true;
    }

    void init_phantom_points (hb_codepoint_t glyph, hb_array_t<contour_point_t> &phantoms /* IN/OUT */) const
    {
      const GlyphHeader &header = get_header (glyph);
      int h_delta = (int)header.xMin - hmtx_accel.get_side_bearing (glyph);
      int v_orig  = (int)header.yMax + vmtx_accel.get_side_bearing (glyph);
      unsigned int h_adv = hmtx_accel.get_advance (glyph);
      unsigned int v_adv = vmtx_accel.get_advance (glyph);

      phantoms[PHANTOM_LEFT].x = h_delta;
      phantoms[PHANTOM_RIGHT].x = h_adv + h_delta;
      phantoms[PHANTOM_TOP].y = v_orig;
      phantoms[PHANTOM_BOTTOM].y = -(int)v_adv + v_orig;
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
      unsigned int start_offset, end_offset;
      if (unlikely (!get_offsets (glyph, &start_offset, &end_offset))) return false;
      if (unlikely (end_offset - start_offset < GlyphHeader::static_size))
      {
      	/* empty glyph */
	points_.resize (PHANTOM_COUNT);
	for (unsigned int i = 0; i < points_.length; i++) points_[i].init ();
      	return true;
      }

      CompositeGlyphHeader::Iterator composite;
      if (get_composite (glyph, &composite))
      {
      	/* For a composite glyph, add one pseudo point for each component */
	do { num_points++; } while (composite.move_to_next());
	points_.resize (num_points + PHANTOM_COUNT);
	for (unsigned int i = 0; i < points_.length; i++) points_[i].init ();
	return true;
      }

      const GlyphHeader &glyph_header = StructAtOffset<GlyphHeader> (glyf_table, start_offset);
      int16_t num_contours = (int16_t) glyph_header.numberOfContours;
      const HBUINT16 *end_pts = &StructAfter<HBUINT16, GlyphHeader> (glyph_header);

      range_checker_t checker (glyf_table, start_offset, end_offset);
      num_points = 0;
      if (num_contours > 0)
      {
	if (unlikely (!checker.in_range (&end_pts[num_contours + 1]))) return false;
	num_points = end_pts[num_contours - 1] + 1;
      }
      else if (num_contours < 0)
      {
	CompositeGlyphHeader::Iterator composite;
	if (unlikely (!get_composite (glyph, &composite))) return false;
	do
	{
	  num_points++;
	} while (composite.move_to_next());
      }

      points_.resize (num_points + PHANTOM_COUNT);
      for (unsigned int i = 0; i < points_.length; i++) points_[i].init ();
      if ((num_contours <= 0) || phantom_only) return true;

      /* Read simple glyph points if !phantom_only */
      end_points_.resize (num_contours);

      for (int16_t i = 0; i < num_contours; i++)
      	end_points_[i] = end_pts[i];

      /* Skip instructions */
      const HBUINT8 *p = &StructAtOffset<HBUINT8> (&end_pts[num_contours+1], end_pts[num_contours]);

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

    /* Note: Recursively calls itself.
     * all_points includes phantom points
     */
    bool get_points_var (hb_codepoint_t glyph,
			 const int *coords, unsigned int coord_count,
			 contour_point_vector_t &all_points /* OUT */,
			 unsigned int depth=0) const
    {
      if (unlikely (depth++ > HB_MAX_NESTING_LEVEL)) return false;
      contour_point_vector_t	points;
      hb_vector_t<unsigned int>	end_points;
      if (unlikely (!get_contour_points (glyph, points, end_points))) return false;
      hb_array_t<contour_point_t>	phantoms = points.sub_array (points.length-PHANTOM_COUNT, PHANTOM_COUNT);
      init_phantom_points (glyph, phantoms);
      if (unlikely (!gvar_accel.apply_deltas_to_points (glyph, coords, coord_count, points.as_array (), end_points.as_array ()))) return false;

      unsigned int comp_index = 0;
      CompositeGlyphHeader::Iterator composite;
      if (!get_composite (glyph, &composite))
      {
      	/* simple glyph */
	all_points.extend (points.as_array ());
      }
      else
      {
	/* composite glyph */
	do
	{
	  contour_point_vector_t comp_points;
	  if (unlikely (!get_points_var (composite.current->glyphIndex, coords, coord_count,
	  				 comp_points, depth))
	      		|| comp_points.length < PHANTOM_COUNT) return false;

	  /* Copy phantom points from component if USE_MY_METRICS flag set */
	  if (composite.current->flags & CompositeGlyphHeader::USE_MY_METRICS)
	    for (unsigned int i = 0; i < PHANTOM_COUNT; i++)
	      phantoms[i] = comp_points[comp_points.length - PHANTOM_COUNT + i];

	  /* Apply component transformation & translation */
	  composite.current->transform_points (comp_points);

	  /* Apply translatation from gvar */
	  comp_points.translate (points[comp_index]);

	  if (composite.current->is_anchored ())
	  {
	    unsigned int p1, p2;
	    composite.current->get_anchor_points (p1, p2);
	    if (likely (p1 < all_points.length && p2 < comp_points.length))
	    {
	      contour_point_t	delta;
	      delta.init (all_points[p1].x - comp_points[p2].x,
	      		  all_points[p1].y - comp_points[p2].y);

	      comp_points.translate (delta);
	    }
	  }

	  all_points.extend (comp_points.sub_array (0, comp_points.length - PHANTOM_COUNT));

	  comp_index++;
	} while (composite.move_to_next());

	all_points.extend (phantoms);
      }

      return true;
    }

    bool get_var_extents_and_phantoms (hb_codepoint_t glyph,
				       const int *coords, unsigned int coord_count,
				       hb_glyph_extents_t *extents=nullptr /* OUt */,
				       contour_point_vector_t *phantoms=nullptr /* OUT */) const
    {
      contour_point_vector_t all_points;
      if (unlikely (!get_points_var (glyph, coords, coord_count, all_points) ||
      		    all_points.length < PHANTOM_COUNT)) return false;

      /* Undocumented rasterizer behavior:
       * Shift points horizontally by the updated left side bearing
       */
      contour_point_t delta;
      delta.init (-all_points[all_points.length - PHANTOM_COUNT + PHANTOM_LEFT].x, 0.f);
      if (delta.x != 0.f) all_points.translate (delta);

      if (extents != nullptr)
      {
	contour_bounds_t	bounds;
	for (unsigned int i = 0; i + PHANTOM_COUNT < all_points.length; i++)
	  bounds.add (all_points[i]);

	if (bounds.min.x > bounds.max.x)
	{
	  extents->width = 0;
	  extents->x_bearing = 0;
	}
	else
	{
	  extents->x_bearing = (int32_t)floorf (bounds.min.x);
	  extents->width = (int32_t)ceilf (bounds.max.x) - extents->x_bearing;
	}
	if (bounds.min.y > bounds.max.y)
	{
	  extents->height = 0;
	  extents->y_bearing = 0;
	}
	else
	{
	  extents->y_bearing = (int32_t)ceilf (bounds.max.y);
	  extents->height = (int32_t)floorf (bounds.min.y) - extents->y_bearing;
	}
      }
      if (phantoms != nullptr)
      {
      	for (unsigned int i = 0; i < PHANTOM_COUNT; i++)
	  (*phantoms)[i] = all_points[all_points.length - PHANTOM_COUNT + i];
      }
      return true;
    }

    bool get_var_metrics (hb_codepoint_t glyph,
			  const int *coords, unsigned int coord_count,
			  contour_point_vector_t &phantoms) const
    { return get_var_extents_and_phantoms (glyph, coords, coord_count, nullptr, &phantoms); }

    bool get_extents_var (hb_codepoint_t glyph,
			  const int *coords, unsigned int coord_count,
			  hb_glyph_extents_t *extents) const
    { return get_var_extents_and_phantoms (glyph, coords, coord_count, extents); }

    public:
    /* based on FontTools _g_l_y_f.py::trim */
    bool remove_padding (unsigned int start_offset,
			 unsigned int *end_offset) const
    {
      unsigned int glyph_length = *end_offset - start_offset;
      const char *glyph = ((const char *) glyf_table) + start_offset;
      const GlyphHeader &glyph_header = *hb_bytes_t (glyph, glyph_length).as<GlyphHeader> ();
      if (!glyph_header.has_data ()) return true;

      const char *glyph_end = glyph + glyph_length;
      if (glyph_header.is_composite_glyph ())
	/* Trimming for composites not implemented.
	 * If removing hints it falls out of that. */
	return true;
      else
      {
	/* simple glyph w/contours, possibly trimmable */
	glyph += glyph_header.simple_instruction_len_offset ();

	if (unlikely (glyph + 2 >= glyph_end)) return false;
	uint16_t nCoordinates = (uint16_t) StructAtOffset<HBUINT16> (glyph - 2, 0) + 1;
	uint16_t nInstructions = (uint16_t) StructAtOffset<HBUINT16> (glyph, 0);

	glyph += 2 + nInstructions;
	if (unlikely (glyph + 2 >= glyph_end)) return false;

	unsigned int coordBytes = 0;
	unsigned int coordsWithFlags = 0;
	while (glyph < glyph_end)
	{
	  uint8_t flag = (uint8_t) *glyph;
	  glyph++;

	  unsigned int repeat = 1;
	  if (flag & FLAG_REPEAT)
	  {
	    if (glyph >= glyph_end)
	    {
	      DEBUG_MSG (SUBSET, nullptr, "Bad flag");
	      return false;
	    }
	    repeat = ((uint8_t) *glyph) + 1;
	    glyph++;
	  }

	  unsigned int xBytes, yBytes;
	  xBytes = yBytes = 0;
	  if (flag & FLAG_X_SHORT) xBytes = 1;
	  else if ((flag & FLAG_X_SAME) == 0) xBytes = 2;

	  if (flag & FLAG_Y_SHORT) yBytes = 1;
	  else if ((flag & FLAG_Y_SAME) == 0) yBytes = 2;

	  coordBytes += (xBytes + yBytes) * repeat;
	  coordsWithFlags += repeat;
	  if (coordsWithFlags >= nCoordinates)
	    break;
	}

	if (coordsWithFlags != nCoordinates)
	{
	  DEBUG_MSG (SUBSET, nullptr, "Expect %d coords to have flags, got flags for %d",
		     nCoordinates, coordsWithFlags);
	  return false;
	}
	glyph += coordBytes;

	if (glyph < glyph_end)
	  *end_offset -= glyph_end - glyph;
      }
      return true;
    }

    bool get_offsets (hb_codepoint_t  glyph,
		      unsigned int   *start_offset /* OUT */,
		      unsigned int   *end_offset   /* OUT */) const
    {
      if (unlikely (glyph >= num_glyphs))
	return false;

      if (short_offset)
      {
	const HBUINT16 *offsets = (const HBUINT16 *) loca_table->dataZ.arrayZ;
	*start_offset = 2 * offsets[glyph];
	*end_offset   = 2 * offsets[glyph + 1];
      }
      else
      {
	const HBUINT32 *offsets = (const HBUINT32 *) loca_table->dataZ.arrayZ;

	*start_offset = offsets[glyph];
	*end_offset   = offsets[glyph + 1];
      }

      if (*start_offset > *end_offset || *end_offset > glyf_table.get_length ())
	return false;

      return true;
    }

    bool get_instruction_length (hb_bytes_t glyph,
				 unsigned int * length /* OUT */) const
    {
      const GlyphHeader &glyph_header = *glyph.as<GlyphHeader> ();
      /* Empty glyph; no instructions. */
      if (!glyph_header.has_data ())
      {
	*length = 0;
	// only 0 byte glyphs are healthy when missing GlyphHeader
	return glyph.length == 0;
      }
      if (glyph_header.is_composite_glyph ())
      {
	unsigned int start = glyph.length;
	unsigned int end = glyph.length;
	unsigned int glyph_offset = &glyph - glyf_table;
	CompositeGlyphHeader::Iterator composite_it;
	if (unlikely (!CompositeGlyphHeader::get_iterator (&glyph, glyph.length,
							   &composite_it)))
	  return false;
	const CompositeGlyphHeader *last;
	do
	{
	  last = composite_it.current;
	} while (composite_it.move_to_next ());

	if ((uint16_t) last->flags & CompositeGlyphHeader::WE_HAVE_INSTRUCTIONS)
	  start = ((char *) last - (char *) glyf_table->dataZ.arrayZ)
		+ last->get_size () - glyph_offset;
	if (unlikely (start > end))
	{
	  DEBUG_MSG (SUBSET, nullptr, "Invalid instruction offset, %d is outside "
				      "%d byte buffer", start, glyph.length);
	  return false;
	}
	*length = end - start;
      }
      else
      {
	unsigned int instruction_len_offset = glyph_header.simple_instruction_len_offset ();
	if (unlikely (instruction_len_offset + 2 > glyph.length))
	{
	  DEBUG_MSG(SUBSET, nullptr, "Glyph size is too short, missing field instructionLength.");
	  return false;
	}

	const HBUINT16 &instruction_len = StructAtOffset<HBUINT16> (&glyph, instruction_len_offset);
	/* Out of bounds of the current glyph */
	if (unlikely (glyph_header.simple_length (instruction_len) > glyph.length))
	{
	  DEBUG_MSG(SUBSET, nullptr, "The instructions array overruns the glyph's boundaries.");
	  return false;
	}
	*length = (uint16_t) instruction_len;
      }
      return true;
    }

    unsigned int get_advance_var (hb_codepoint_t glyph,
				  const int *coords, unsigned int coord_count,
				  bool vertical) const
    {
      bool success = false;
      contour_point_vector_t	phantoms;
      phantoms.resize (PHANTOM_COUNT);

      if (likely (coord_count == gvar_accel.get_axis_count ()))
	success = get_var_metrics (glyph, coords, coord_count, phantoms);

      if (unlikely (!success))
	return vertical? vmtx_accel.get_advance (glyph): hmtx_accel.get_advance (glyph);

      if (vertical)
	return roundf (phantoms[PHANTOM_TOP].y - phantoms[PHANTOM_BOTTOM].y);
      else
	return roundf (phantoms[PHANTOM_RIGHT].x - phantoms[PHANTOM_LEFT].x);
    }

    int get_side_bearing_var (hb_codepoint_t glyph, const int *coords, unsigned int coord_count, bool vertical) const
    {
      hb_glyph_extents_t extents;
      contour_point_vector_t	phantoms;
      phantoms.resize (PHANTOM_COUNT);

      if (unlikely (!get_var_extents_and_phantoms (glyph, coords, coord_count, &extents, &phantoms)))
	return vertical? vmtx_accel.get_side_bearing (glyph): hmtx_accel.get_side_bearing (glyph);

      return vertical? (int)ceilf (phantoms[PHANTOM_TOP].y) - extents.y_bearing: (int)floorf (phantoms[PHANTOM_LEFT].x);
    }

    bool get_extents (hb_font_t *font, hb_codepoint_t glyph, hb_glyph_extents_t *extents) const
    {
      unsigned int coord_count;
      const int *coords = hb_font_get_var_coords_normalized (font, &coord_count);
      if (coords && coord_count > 0 && coord_count == gvar_accel.get_axis_count ())
	return get_extents_var (glyph, coords, coord_count, extents);

      unsigned int start_offset, end_offset;
      if (!get_offsets (glyph, &start_offset, &end_offset))
	return false;

      if (end_offset - start_offset < GlyphHeader::static_size)
	return true; /* Empty glyph; zero extents. */

      const GlyphHeader &glyph_header = StructAtOffset<GlyphHeader> (glyf_table, start_offset);

      /* Undocumented rasterizer behavior: shift glyph to the left by (lsb - xMin), i.e., xMin = lsb */
      /* extents->x_bearing = hb_min (glyph_header.xMin, glyph_header.xMax); */
      extents->x_bearing = hmtx_accel.get_side_bearing (glyph);
      extents->y_bearing = hb_max (glyph_header.yMin, glyph_header.yMax);
      extents->width     = hb_max (glyph_header.xMin, glyph_header.xMax) - hb_min (glyph_header.xMin, glyph_header.xMax);
      extents->height    = hb_min (glyph_header.yMin, glyph_header.yMax) - extents->y_bearing;

      return true;
    }

  hb_bytes_t bytes_for_glyph (const char * glyf, hb_codepoint_t gid)
  {
    unsigned int start_offset, end_offset;
    if (unlikely (!(get_offsets (gid, &start_offset, &end_offset) &&
      remove_padding (start_offset, &end_offset))))
    {
      DEBUG_MSG(SUBSET, nullptr, "Unable to get offset or remove padding for %d", gid);
      return hb_bytes_t ();
    }
    hb_bytes_t glyph_bytes = hb_bytes_t (glyf + start_offset, end_offset - start_offset);
    if (!glyph_bytes.as<GlyphHeader> ()->has_data ())
    {
      DEBUG_MSG(SUBSET, nullptr, "Glyph size smaller than minimum header %d", gid);
      return hb_bytes_t ();
    }
    return glyph_bytes;
  }

    private:
    bool short_offset;
    unsigned int num_glyphs;
    hb_blob_ptr_t<loca> loca_table;
    hb_blob_ptr_t<glyf> glyf_table;

    /* variable font support */
    gvar::accelerator_t		gvar_accel;
    hmtx::accelerator_t		hmtx_accel;
    vmtx::accelerator_t		vmtx_accel;
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
    dest_glyph = hb_bytes_t (&dest_glyph, dest_glyph.length + dest_end.copy(c).length);
    unsigned int pad_length = padding ();
    DEBUG_MSG(SUBSET, nullptr, "serialize %d byte glyph, width %d pad %d", dest_glyph.length, dest_glyph.length  + pad_length, pad_length);

    HBUINT8 pad;
    pad = 0;
    while (pad_length > 0)
    {
      c->embed(pad);
      pad_length--;
    }

    if (dest_glyph.length)
    {
      _fix_component_gids (plan, dest_glyph);
      if (plan->drop_hints)
      {
	_zero_instruction_length (dest_glyph);
	c->check_success (_remove_composite_instruction_flag (dest_glyph));
      }
    }

    return_trace (true);
  }

    void drop_hints (const OT::glyf::accelerator_t& glyf)
    {
      if (source_glyph.length == 0) return;

      unsigned int instruction_len = 0;
      if (!glyf.get_instruction_length (source_glyph, &instruction_len))
      {
	DEBUG_MSG (SUBSET, nullptr, "Unable to read instruction length for new_gid %d",
		   new_gid);
	return;
      }

      const GlyphHeader& header = *source_glyph.as<GlyphHeader> ();
      DEBUG_MSG (SUBSET, nullptr, "Unable to read instruction length for new_gid %d",
		 new_gid);
      if (header.is_composite_glyph ())
      {
	/* just chop instructions off the end for composite glyphs */
	dest_start = hb_bytes_t (&source_glyph, source_glyph.length - instruction_len);
      }
      else
      {
	unsigned int glyph_length = header.simple_length (instruction_len);
	dest_start = hb_bytes_t (&source_glyph, glyph_length - instruction_len);
	dest_end = hb_bytes_t (&source_glyph + glyph_length,
			       source_glyph.length - glyph_length);
	DEBUG_MSG (SUBSET, nullptr, "source_len %d start len %d glyph_len %d "
				    "instruction_len %d end len %d",
		   source_glyph.length, dest_start.length, glyph_length,
		   instruction_len, dest_end.length);
      }
    }

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
