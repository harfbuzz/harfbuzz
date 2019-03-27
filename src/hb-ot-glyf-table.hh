/*
 * Copyright © 2015  Google, Inc.
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
 * Adobe Author(s): Michiharu Ariza
 */

#ifndef HB_OT_GLYF_TABLE_HH
#define HB_OT_GLYF_TABLE_HH

#include "hb-open-type.hh"
#include "hb-ot-head-table.hh"
#include "hb-ot-hmtx-table.hh"
#include "hb-ot-var-gvar-table.hh"
#include "hb-subset-glyf.hh"

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
			* defining it MIN() instead. */
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

  bool subset (hb_subset_plan_t *plan) const
  {
    hb_blob_t *glyf_prime = nullptr;
    hb_blob_t *loca_prime = nullptr;

    bool success = true;
    bool use_short_loca = false;
    if (hb_subset_glyf_and_loca (plan, &use_short_loca, &glyf_prime, &loca_prime)) {
      success = success && plan->add_table (HB_OT_TAG_glyf, glyf_prime);
      success = success && plan->add_table (HB_OT_TAG_loca, loca_prime);
      success = success && _add_head_and_set_loca_version (plan, use_short_loca);
    } else {
      success = false;
    }
    hb_blob_destroy (loca_prime);
    hb_blob_destroy (glyf_prime);

    return success;
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
    head_prime->indexToLocFormat.set (use_short_loca ? 0 : 1);
    bool success = plan->add_table (HB_OT_TAG_head, head_prime_blob);

    hb_blob_destroy (head_prime_blob);
    return success;
  }

  struct GlyphHeader
  {
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
    enum composite_glyph_flag_t {
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

    HBUINT16 flags;
    GlyphID  glyphIndex;

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
      if (length < GlyphHeader::static_size)
	return false; /* Empty glyph; zero extents. */

      const GlyphHeader &glyph_header = StructAtOffset<GlyphHeader> (glyph_data, 0);
      if (glyph_header.numberOfContours < 0)
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

      num_glyphs = MAX (1u, loca_table.get_length () / (short_offset ? 2 : 4)) - 1;

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

    enum simple_glyph_flag_t {
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
      int v_delta = (int)header.yMax - vmtx_accel.get_side_bearing (glyph);
      unsigned int h_adv = hmtx_accel.get_advance (glyph);
      unsigned int v_adv = vmtx_accel.get_advance (glyph);

      phantoms[PHANTOM_LEFT].x = h_delta;
      phantoms[PHANTOM_RIGHT].x = h_adv + h_delta;
      phantoms[PHANTOM_TOP].y = v_delta;
      phantoms[PHANTOM_BOTTOM].y = -v_adv + v_delta;
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

    /* Note: Recursively calls itself. */
    bool get_var_metrics (hb_codepoint_t glyph,
			  const int *coords, unsigned int coord_count,
			  contour_point_vector_t &phantoms /* OUT */,
			  unsigned int depth) const
    {
      contour_point_vector_t	points;
      hb_vector_t<unsigned int>	end_points;
      if (unlikely (!get_contour_points (glyph, points, end_points, true/*phantom_only*/))) return false;
      hb_array_t<contour_point_t>	phantoms_array = points.sub_array (points.length-PHANTOM_COUNT, PHANTOM_COUNT);
      init_phantom_points (glyph, phantoms_array);
      if (unlikely (!gvar_accel.apply_deltas_to_points (glyph, coords, coord_count,
      							points.as_array (), end_points.as_array ()))) return false;

      for (unsigned int i = 0; i < PHANTOM_COUNT; i++)
      	phantoms[i] = points[points.length - PHANTOM_COUNT + i];

      CompositeGlyphHeader::Iterator composite;
      if (!get_composite (glyph, &composite)) return true;	/* simple glyph */
      do
      {
	if (composite.current->flags & CompositeGlyphHeader::USE_MY_METRICS)
	{
	  if (unlikely (depth >= HB_MAX_NESTING_LEVEL ||
	  		!get_var_metrics (composite.current->glyphIndex, coords, coord_count,
					  phantoms, depth+1))) return false;

	  composite.current->transform_points (phantoms);
	}
      } while (composite.move_to_next());
      return true;
    }

    struct contour_bounds_t
    {
      contour_bounds_t () { min.x = min.y = FLT_MAX; max.x = max.y = FLT_MIN; }

      void add (const contour_point_t &p)
      {
      	min.x = MIN (min.x, p.x);
      	min.y = MIN (min.y, p.y);
      	max.x = MAX (max.x, p.x);
      	max.y = MAX (max.y, p.y);
      }

      bool empty () const { return (min.x >= max.x) || (min.y >= max.y); }

      contour_point_t	min;
      contour_point_t	max;
    };

    /* Note: Recursively calls itself.
     * all_points does not include phantom points
     */
    bool get_points_var (hb_codepoint_t glyph,
			 const int *coords, unsigned int coord_count,
			 contour_point_vector_t &all_points /* OUT */,
			 unsigned int depth) const
    {
      contour_point_vector_t	points;
      hb_vector_t<unsigned int>	end_points;
      if (unlikely (!get_contour_points (glyph, points, end_points))) return false;
      hb_array_t<contour_point_t>	phantoms_array = points.sub_array (points.length-PHANTOM_COUNT, PHANTOM_COUNT);
      init_phantom_points (glyph, phantoms_array);
      if (unlikely (!gvar_accel.apply_deltas_to_points (glyph, coords, coord_count, points.as_array (), end_points.as_array ()))) return false;

      unsigned int comp_index = 0;
      CompositeGlyphHeader::Iterator composite;
      if (!get_composite (glyph, &composite))
      {
      	/* simple glyph */
      	if (likely (points.length >= PHANTOM_COUNT))
	  all_points.extend (points.sub_array (0, points.length - PHANTOM_COUNT));
      }
      else
      {
	/* composite glyph */
	do
	{
	  contour_point_vector_t comp_points;
	  if (unlikely (depth >= HB_MAX_NESTING_LEVEL ||
	  		!get_points_var (composite.current->glyphIndex, coords, coord_count,
	  				 comp_points, depth+1))) return false;

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
	  all_points.extend (comp_points.as_array ());

	  comp_index++;
	} while (composite.move_to_next());
      }

      {
	/* Undocumented rasterizer behavior:
	 * Shift points horizontally by the updated left side bearing
	 */
	contour_point_t delta;
	delta.init (points[points.length - PHANTOM_COUNT + PHANTOM_LEFT].x, 0.f);
	if (delta.x != 0.f) all_points.translate (delta);
      }

      return true;
    }

    bool get_extents_var (hb_codepoint_t glyph,
			  const int *coords, unsigned int coord_count,
			  hb_glyph_extents_t *extents) const
    {
      contour_point_vector_t all_points;
      if (unlikely (!get_points_var (glyph, coords, coord_count, all_points, 0))) return false;

      contour_bounds_t	bounds;
      for (unsigned int i = 0; i < all_points.length; i++)
      	bounds.add (all_points[i]);

      if (bounds.min.x >= bounds.max.x)
      {
	extents->width = 0;
	extents->x_bearing = 0;
      }
      else
      {
	extents->x_bearing = (int32_t)floorf (bounds.min.x);
	extents->width = (int32_t)ceilf (bounds.max.x) - extents->x_bearing;
      }
      if (bounds.min.y >= bounds.max.y)
      {
	extents->height = 0;
	extents->y_bearing = 0;
      }
      else
      {
	extents->y_bearing = (int32_t)ceilf (bounds.max.y);
	extents->height = (int32_t)floorf (bounds.min.y) - extents->y_bearing;
      }
      return true;
    }

    public:
    /* based on FontTools _g_l_y_f.py::trim */
    bool remove_padding (unsigned int start_offset,
				unsigned int *end_offset) const
    {
      if (*end_offset - start_offset < GlyphHeader::static_size) return true;

      const char *glyph = ((const char *) glyf_table) + start_offset;
      const char * const glyph_end = glyph + (*end_offset - start_offset);
      const GlyphHeader &glyph_header = StructAtOffset<GlyphHeader> (glyph, 0);
      int16_t num_contours = (int16_t) glyph_header.numberOfContours;

      if (num_contours < 0)
	/* Trimming for composites not implemented.
	 * If removing hints it falls out of that. */
	return true;
      else if (num_contours > 0)
      {
	/* simple glyph w/contours, possibly trimmable */
	glyph += GlyphHeader::static_size + 2 * num_contours;

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
	      DEBUG_MSG(SUBSET, nullptr, "Bad flag");
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
	  DEBUG_MSG(SUBSET, nullptr, "Expect %d coords to have flags, got flags for %d", nCoordinates, coordsWithFlags);
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

    bool get_instruction_offsets (unsigned int start_offset,
				  unsigned int end_offset,
				  unsigned int *instruction_start /* OUT */,
				  unsigned int *instruction_end /* OUT */) const
    {
      if (end_offset - start_offset < GlyphHeader::static_size)
      {
	*instruction_start = 0;
	*instruction_end = 0;
	return true; /* Empty glyph; no instructions. */
      }
      const GlyphHeader &glyph_header = StructAtOffset<GlyphHeader> (glyf_table, start_offset);
      int16_t num_contours = (int16_t) glyph_header.numberOfContours;
      if (num_contours < 0)
      {
	CompositeGlyphHeader::Iterator composite_it;
	if (unlikely (!CompositeGlyphHeader::get_iterator (
	    (const char*) this->glyf_table + start_offset,
	     end_offset - start_offset, &composite_it))) return false;
	const CompositeGlyphHeader *last;
	do {
	  last = composite_it.current;
	} while (composite_it.move_to_next ());

	if ((uint16_t) last->flags & CompositeGlyphHeader::WE_HAVE_INSTRUCTIONS)
	  *instruction_start = ((char *) last - (char *) glyf_table->dataZ.arrayZ) + last->get_size ();
	else
	  *instruction_start = end_offset;
	*instruction_end = end_offset;
	if (unlikely (*instruction_start > *instruction_end))
	{
	  DEBUG_MSG(SUBSET, nullptr, "Invalid instruction offset, %d is outside [%d, %d]", *instruction_start, start_offset, end_offset);
	  return false;
	}
      }
      else
      {
	unsigned int instruction_length_offset = start_offset + GlyphHeader::static_size + 2 * num_contours;
	if (unlikely (instruction_length_offset + 2 > end_offset))
	{
	  DEBUG_MSG(SUBSET, nullptr, "Glyph size is too short, missing field instructionLength.");
	  return false;
	}

	const HBUINT16 &instruction_length = StructAtOffset<HBUINT16> (glyf_table, instruction_length_offset);
	unsigned int start = instruction_length_offset + 2;
	unsigned int end = start + (uint16_t) instruction_length;
	if (unlikely (end > end_offset)) // Out of bounds of the current glyph
	{
	  DEBUG_MSG(SUBSET, nullptr, "The instructions array overruns the glyph's boundaries.");
	  return false;
	}

	*instruction_start = start;
	*instruction_end = end;
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
	success = get_var_metrics (glyph, coords, coord_count, phantoms, 0);

      if (unlikely (!success))
	return vertical? vmtx_accel.get_advance (glyph): hmtx_accel.get_advance (glyph);

      if (vertical)
      	return (unsigned int)roundf (phantoms[PHANTOM_TOP].y - phantoms[PHANTOM_BOTTOM].y);
      else
      	return (unsigned int)roundf (phantoms[PHANTOM_RIGHT].x - phantoms[PHANTOM_LEFT].x);
    }

    int get_side_bearing_var (hb_codepoint_t glyph, const int *coords, unsigned int coord_count, bool vertical) const
    {
      contour_point_vector_t	phantoms;
      phantoms.resize (PHANTOM_COUNT);

      if (unlikely (!get_var_metrics (glyph, coords, coord_count, phantoms, 0)))
	return vertical? vmtx_accel.get_side_bearing (glyph): hmtx_accel.get_side_bearing (glyph);

      return (int)(vertical? -ceilf (phantoms[PHANTOM_TOP].y): floorf (phantoms[PHANTOM_LEFT].x));
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

      extents->x_bearing = MIN (glyph_header.xMin, glyph_header.xMax);
      extents->y_bearing = MAX (glyph_header.yMin, glyph_header.yMax);
      extents->width     = MAX (glyph_header.xMin, glyph_header.xMax) - extents->x_bearing;
      extents->height    = MIN (glyph_header.yMin, glyph_header.yMax) - extents->y_bearing;

      return true;
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

  protected:
  UnsizedArrayOf<HBUINT8>	dataZ;		/* Glyphs data. */
  public:
  DEFINE_SIZE_MIN (0); /* In reality, this is UNBOUNDED() type; but since we always
			* check the size externally, allow Null() object of it by
			* defining it MIN() instead. */
};

struct glyf_accelerator_t : glyf::accelerator_t {};

} /* namespace OT */


#endif /* HB_OT_GLYF_TABLE_HH */
