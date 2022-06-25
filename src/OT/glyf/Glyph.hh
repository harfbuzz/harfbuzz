#ifndef OT_GLYF_GLYPH_HH
#define OT_GLYF_GLYPH_HH


#include "../../hb-open-type.hh"

#include "GlyphHeader.hh"
#include "SimpleGlyph.hh"


namespace OT {

struct glyf_accelerator_t;

namespace glyf_impl {


struct CompositeGlyphChain
{
  protected:
  enum composite_glyph_flag_t
  {
    ARG_1_AND_2_ARE_WORDS	= 0x0001,
    ARGS_ARE_XY_VALUES		= 0x0002,
    ROUND_XY_TO_GRID		= 0x0004,
    WE_HAVE_A_SCALE		= 0x0008,
    MORE_COMPONENTS		= 0x0020,
    WE_HAVE_AN_X_AND_Y_SCALE	= 0x0040,
    WE_HAVE_A_TWO_BY_TWO	= 0x0080,
    WE_HAVE_INSTRUCTIONS	= 0x0100,
    USE_MY_METRICS		= 0x0200,
    OVERLAP_COMPOUND		= 0x0400,
    SCALED_COMPONENT_OFFSET	= 0x0800,
    UNSCALED_COMPONENT_OFFSET = 0x1000
  };

  public:
  unsigned int get_size () const
  {
    unsigned int size = min_size;
    /* arg1 and 2 are int16 */
    if (flags & ARG_1_AND_2_ARE_WORDS) size += 4;
    /* arg1 and 2 are int8 */
    else size += 2;

    /* One x 16 bit (scale) */
    if (flags & WE_HAVE_A_SCALE) size += 2;
    /* Two x 16 bit (xscale, yscale) */
    else if (flags & WE_HAVE_AN_X_AND_Y_SCALE) size += 4;
    /* Four x 16 bit (xscale, scale01, scale10, yscale) */
    else if (flags & WE_HAVE_A_TWO_BY_TWO) size += 8;

    return size;
  }

  void set_glyph_index (hb_codepoint_t new_gid) { glyphIndex = new_gid; }
  hb_codepoint_t get_glyph_index ()       const { return glyphIndex; }

  void drop_instructions_flag ()  { flags = (uint16_t) flags & ~WE_HAVE_INSTRUCTIONS; }
  void set_overlaps_flag ()
  {
    flags = (uint16_t) flags | OVERLAP_COMPOUND;
  }

  bool has_instructions ()  const { return   flags & WE_HAVE_INSTRUCTIONS; }

  bool has_more ()          const { return   flags & MORE_COMPONENTS; }
  bool is_use_my_metrics () const { return   flags & USE_MY_METRICS; }
  bool is_anchored ()       const { return !(flags & ARGS_ARE_XY_VALUES); }
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

  protected:
  HBUINT16	flags;
  HBGlyphID16	glyphIndex;
  public:
  DEFINE_SIZE_MIN (4);
};

struct composite_iter_t : hb_iter_with_fallback_t<composite_iter_t, const CompositeGlyphChain &>
{
  typedef const CompositeGlyphChain *__item_t__;
  composite_iter_t (hb_bytes_t glyph_, __item_t__ current_) :
      glyph (glyph_), current (nullptr), current_size (0)
  {
    set_next (current_);
  }

  composite_iter_t () : glyph (hb_bytes_t ()), current (nullptr), current_size (0) {}

  const CompositeGlyphChain &__item__ () const { return *current; }
  bool __more__ () const { return current; }
  void __next__ ()
  {
    if (!current->has_more ()) { current = nullptr; return; }

    set_next (&StructAtOffset<CompositeGlyphChain> (current, current_size));
  }
  composite_iter_t __end__ () const { return composite_iter_t (); }
  bool operator != (const composite_iter_t& o) const
  { return current != o.current; }


  void set_next (const CompositeGlyphChain *composite)
  {
    if (!glyph.check_range (composite, CompositeGlyphChain::min_size))
    {
      current = nullptr;
      current_size = 0;
      return;
    }
    unsigned size = composite->get_size ();
    if (!glyph.check_range (composite, size))
    {
      current = nullptr;
      current_size = 0;
      return;
    }

    current = composite;
    current_size = size;
  }

  private:
  hb_bytes_t glyph;
  __item_t__ current;
  unsigned current_size;
};

enum phantom_point_index_t
{
  PHANTOM_LEFT   = 0,
  PHANTOM_RIGHT  = 1,
  PHANTOM_TOP    = 2,
  PHANTOM_BOTTOM = 3,
  PHANTOM_COUNT  = 4
};

struct CompositeGlyph
{
  const GlyphHeader &header;
  hb_bytes_t bytes;
  CompositeGlyph (const GlyphHeader &header_, hb_bytes_t bytes_) :
    header (header_), bytes (bytes_) {}

  composite_iter_t get_iterator () const
  { return composite_iter_t (bytes, &StructAfter<CompositeGlyphChain, GlyphHeader> (header)); }

  unsigned int instructions_length (hb_bytes_t bytes) const
  {
    unsigned int start = bytes.length;
    unsigned int end = bytes.length;
    const CompositeGlyphChain *last = nullptr;
    for (auto &item : get_iterator ())
      last = &item;
    if (unlikely (!last)) return 0;

    if (last->has_instructions ())
      start = (char *) last - &bytes + last->get_size ();
    if (unlikely (start > end)) return 0;
    return end - start;
  }

  /* Trimming for composites not implemented.
   * If removing hints it falls out of that. */
  const hb_bytes_t trim_padding () const { return bytes; }

  void drop_hints ()
  {
    for (const auto &_ : get_iterator ())
      const_cast<CompositeGlyphChain &> (_).drop_instructions_flag ();
  }

  /* Chop instructions off the end */
  void drop_hints_bytes (hb_bytes_t &dest_start) const
  { dest_start = bytes.sub_array (0, bytes.length - instructions_length (bytes)); }

  void set_overlaps_flag ()
  {
    CompositeGlyphChain& glyph_chain = const_cast<CompositeGlyphChain &> (
	StructAfter<CompositeGlyphChain, GlyphHeader> (header));
    if (!bytes.check_range(&glyph_chain, CompositeGlyphChain::min_size))
      return;
    glyph_chain.set_overlaps_flag ();
  }
};


struct Glyph
{
  enum glyph_type_t { EMPTY, SIMPLE, COMPOSITE };

  public:
  composite_iter_t get_composite_iterator () const
  {
    if (type != COMPOSITE) return composite_iter_t ();
    return CompositeGlyph (*header, bytes).get_iterator ();
  }

  const hb_bytes_t trim_padding () const
  {
    switch (type) {
    case COMPOSITE: return CompositeGlyph (*header, bytes).trim_padding ();
    case SIMPLE:    return SimpleGlyph (*header, bytes).trim_padding ();
    default:        return bytes;
    }
  }

  void drop_hints ()
  {
    switch (type) {
    case COMPOSITE: CompositeGlyph (*header, bytes).drop_hints (); return;
    case SIMPLE:    SimpleGlyph (*header, bytes).drop_hints (); return;
    default:        return;
    }
  }

  void set_overlaps_flag ()
  {
    switch (type) {
    case COMPOSITE: CompositeGlyph (*header, bytes).set_overlaps_flag (); return;
    case SIMPLE:    SimpleGlyph (*header, bytes).set_overlaps_flag (); return;
    default:        return;
    }
  }

  void drop_hints_bytes (hb_bytes_t &dest_start, hb_bytes_t &dest_end) const
  {
    switch (type) {
    case COMPOSITE: CompositeGlyph (*header, bytes).drop_hints_bytes (dest_start); return;
    case SIMPLE:    SimpleGlyph (*header, bytes).drop_hints_bytes (dest_start, dest_end); return;
    default:        return;
    }
  }

  /* Note: Recursively calls itself.
   * all_points includes phantom points
   */
  template <typename accelerator_t>
  bool get_points (hb_font_t *font, const accelerator_t &glyf_accelerator,
		   contour_point_vector_t &all_points /* OUT */,
		   bool phantom_only = false,
		   unsigned int depth = 0) const
  {
    if (unlikely (depth > HB_MAX_NESTING_LEVEL)) return false;
    contour_point_vector_t points;

    switch (type) {
    case COMPOSITE:
    {
      /* pseudo component points for each component in composite glyph */
      unsigned num_points = hb_len (CompositeGlyph (*header, bytes).get_iterator ());
      if (unlikely (!points.resize (num_points))) return false;
      for (unsigned i = 0; i < points.length; i++)
	points[i].init ();
      break;
    }
    case SIMPLE:
      if (unlikely (!SimpleGlyph (*header, bytes).get_contour_points (points, phantom_only)))
	return false;
      break;
    }

    /* Init phantom points */
    if (unlikely (!points.resize (points.length + PHANTOM_COUNT))) return false;
    hb_array_t<contour_point_t> phantoms = points.sub_array (points.length - PHANTOM_COUNT, PHANTOM_COUNT);
    {
      for (unsigned i = 0; i < PHANTOM_COUNT; ++i) phantoms[i].init ();
      int h_delta = (int) header->xMin -
		    glyf_accelerator.hmtx->get_side_bearing (gid);
      int v_orig  = (int) header->yMax +
#ifndef HB_NO_VERTICAL
		    glyf_accelerator.vmtx->get_side_bearing (gid)
#else
		    0
#endif
		    ;
      unsigned h_adv = glyf_accelerator.hmtx->get_advance (gid);
      unsigned v_adv =
#ifndef HB_NO_VERTICAL
		       glyf_accelerator.vmtx->get_advance (gid)
#else
		       - font->face->get_upem ()
#endif
		       ;
      phantoms[PHANTOM_LEFT].x = h_delta;
      phantoms[PHANTOM_RIGHT].x = h_adv + h_delta;
      phantoms[PHANTOM_TOP].y = v_orig;
      phantoms[PHANTOM_BOTTOM].y = v_orig - (int) v_adv;
    }

#ifndef HB_NO_VAR
    glyf_accelerator.gvar->apply_deltas_to_points (gid, font, points.as_array ());
#endif

    switch (type) {
    case SIMPLE:
      all_points.extend (points.as_array ());
      break;
    case COMPOSITE:
    {
      unsigned int comp_index = 0;
      for (auto &item : get_composite_iterator ())
      {
	contour_point_vector_t comp_points;
	if (unlikely (!glyf_accelerator.glyph_for_gid (item.get_glyph_index ())
				       .get_points (font, glyf_accelerator, comp_points,
						    phantom_only, depth + 1)
		      || comp_points.length < PHANTOM_COUNT))
	  return false;

	/* Copy phantom points from component if USE_MY_METRICS flag set */
	if (item.is_use_my_metrics ())
	  for (unsigned int i = 0; i < PHANTOM_COUNT; i++)
	    phantoms[i] = comp_points[comp_points.length - PHANTOM_COUNT + i];

	/* Apply component transformation & translation */
	item.transform_points (comp_points);

	/* Apply translation from gvar */
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
    } break;
    default:
      all_points.extend (phantoms);
    }

    if (depth == 0) /* Apply at top level */
    {
      /* Undocumented rasterizer behavior:
       * Shift points horizontally by the updated left side bearing
       */
      contour_point_t delta;
      delta.init (-phantoms[PHANTOM_LEFT].x, 0.f);
      if (delta.x) all_points.translate (delta);
    }

    return true;
  }

  bool get_extents (hb_font_t *font, const glyf_accelerator_t &glyf_accelerator,
		    hb_glyph_extents_t *extents) const
  {
    if (type == EMPTY) return true; /* Empty glyph; zero extents. */
    return header->get_extents (font, glyf_accelerator, gid, extents);
  }

  hb_bytes_t get_bytes () const { return bytes; }

  Glyph (hb_bytes_t bytes_ = hb_bytes_t (),
	 hb_codepoint_t gid_ = (hb_codepoint_t) -1) : bytes (bytes_), gid (gid_),
						      header (bytes.as<GlyphHeader> ())
  {
    int num_contours = header->numberOfContours;
    if (unlikely (num_contours == 0)) type = EMPTY;
    else if (num_contours > 0) type = SIMPLE;
    else type = COMPOSITE; /* negative numbers */
  }

  protected:
  hb_bytes_t bytes;
  hb_codepoint_t gid;
  const GlyphHeader *header;
  unsigned type;
};


} /* namespace glyf_impl */
} /* namespace OT */


#endif /* OT_GLYF_GLYPH_HH */
