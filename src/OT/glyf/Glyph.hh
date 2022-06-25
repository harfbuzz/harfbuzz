#ifndef OT_GLYF_GLYPH_HH
#define OT_GLYF_GLYPH_HH


#include "../../hb-open-type.hh"


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

struct GlyphHeader
{
  bool has_data () const { return numberOfContours; }

  template <typename accelerator_t>
  bool get_extents (hb_font_t *font, const accelerator_t &glyf_accelerator,
		    hb_codepoint_t gid, hb_glyph_extents_t *extents) const
  {
    /* Undocumented rasterizer behavior: shift glyph to the left by (lsb - xMin), i.e., xMin = lsb */
    /* extents->x_bearing = hb_min (glyph_header.xMin, glyph_header.xMax); */
    extents->x_bearing = font->em_scale_x (glyf_accelerator.hmtx->get_side_bearing (gid));
    extents->y_bearing = font->em_scale_y (hb_max (yMin, yMax));
    extents->width     = font->em_scale_x (hb_max (xMin, xMax) - hb_min (xMin, xMax));
    extents->height    = font->em_scale_y (hb_min (yMin, yMax) - hb_max (yMin, yMax));

    return true;
  }

  HBINT16	numberOfContours;
		    /* If the number of contours is
		     * greater than or equal to zero,
		     * this is a simple glyph; if negative,
		     * this is a composite glyph. */
  FWORD	xMin;	/* Minimum x for coordinate data. */
  FWORD	yMin;	/* Minimum y for coordinate data. */
  FWORD	xMax;	/* Maximum x for coordinate data. */
  FWORD	yMax;	/* Maximum y for coordinate data. */
  public:
  DEFINE_SIZE_STATIC (10);
};

struct SimpleGlyph
{
  enum simple_glyph_flag_t
  {
    FLAG_ON_CURVE       = 0x01,
    FLAG_X_SHORT        = 0x02,
    FLAG_Y_SHORT        = 0x04,
    FLAG_REPEAT         = 0x08,
    FLAG_X_SAME         = 0x10,
    FLAG_Y_SAME         = 0x20,
    FLAG_OVERLAP_SIMPLE = 0x40,
    FLAG_RESERVED2      = 0x80
  };

  const GlyphHeader &header;
  hb_bytes_t bytes;
  SimpleGlyph (const GlyphHeader &header_, hb_bytes_t bytes_) :
    header (header_), bytes (bytes_) {}

  unsigned int instruction_len_offset () const
  { return GlyphHeader::static_size + 2 * header.numberOfContours; }

  unsigned int length (unsigned int instruction_len) const
  { return instruction_len_offset () + 2 + instruction_len; }

  unsigned int instructions_length () const
  {
    unsigned int instruction_length_offset = instruction_len_offset ();
    if (unlikely (instruction_length_offset + 2 > bytes.length)) return 0;

    const HBUINT16 &instructionLength = StructAtOffset<HBUINT16> (&bytes, instruction_length_offset);
    /* Out of bounds of the current glyph */
    if (unlikely (length (instructionLength) > bytes.length)) return 0;
    return instructionLength;
  }

  const hb_bytes_t trim_padding () const
  {
    /* based on FontTools _g_l_y_f.py::trim */
    const uint8_t *glyph = (uint8_t*) bytes.arrayZ;
    const uint8_t *glyph_end = glyph + bytes.length;
    /* simple glyph w/contours, possibly trimmable */
    glyph += instruction_len_offset ();

    if (unlikely (glyph + 2 >= glyph_end)) return hb_bytes_t ();
    unsigned int num_coordinates = StructAtOffset<HBUINT16> (glyph - 2, 0) + 1;
    unsigned int num_instructions = StructAtOffset<HBUINT16> (glyph, 0);

    glyph += 2 + num_instructions;

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
    return bytes.sub_array (0, bytes.length + coord_bytes - (glyph_end - glyph));
  }

  /* zero instruction length */
  void drop_hints ()
  {
    GlyphHeader &glyph_header = const_cast<GlyphHeader &> (header);
    (HBUINT16 &) StructAtOffset<HBUINT16> (&glyph_header, instruction_len_offset ()) = 0;
  }

  void drop_hints_bytes (hb_bytes_t &dest_start, hb_bytes_t &dest_end) const
  {
    unsigned int instructions_len = instructions_length ();
    unsigned int glyph_length = length (instructions_len);
    dest_start = bytes.sub_array (0, glyph_length - instructions_len);
    dest_end = bytes.sub_array (glyph_length, bytes.length - glyph_length);
  }

  void set_overlaps_flag ()
  {
    if (unlikely (!header.numberOfContours)) return;

    unsigned flags_offset = length (instructions_length ());
    if (unlikely (flags_offset + 1 > bytes.length)) return;

    HBUINT8 &first_flag = (HBUINT8 &) StructAtOffset<HBUINT16> (&bytes, flags_offset);
    first_flag = (uint8_t) first_flag | FLAG_OVERLAP_SIMPLE;
  }

  static bool read_points (const HBUINT8 *&p /* IN/OUT */,
			   contour_point_vector_t &points_ /* IN/OUT */,
			   const hb_bytes_t &bytes,
			   void (* setter) (contour_point_t &_, float v),
			   const simple_glyph_flag_t short_flag,
			   const simple_glyph_flag_t same_flag)
  {
    float v = 0;
    for (unsigned i = 0; i < points_.length; i++)
    {
      uint8_t flag = points_[i].flag;
      if (flag & short_flag)
      {
	if (unlikely (!bytes.check_range (p))) return false;
	if (flag & same_flag)
	  v += *p++;
	else
	  v -= *p++;
      }
      else
      {
	if (!(flag & same_flag))
	{
	  if (unlikely (!bytes.check_range ((const HBUINT16 *) p))) return false;
	  v += *(const HBINT16 *) p;
	  p += HBINT16::static_size;
	}
      }
      setter (points_[i], v);
    }
    return true;
  }

  bool get_contour_points (contour_point_vector_t &points_ /* OUT */,
			   bool phantom_only = false) const
  {
    const HBUINT16 *endPtsOfContours = &StructAfter<HBUINT16> (header);
    int num_contours = header.numberOfContours;
    if (unlikely (!bytes.check_range (&endPtsOfContours[num_contours + 1]))) return false;
    unsigned int num_points = endPtsOfContours[num_contours - 1] + 1;

    points_.resize (num_points);
    for (unsigned int i = 0; i < points_.length; i++) points_[i].init ();
    if (phantom_only) return true;

    for (int i = 0; i < num_contours; i++)
      points_[endPtsOfContours[i]].is_end_point = true;

    /* Skip instructions */
    const HBUINT8 *p = &StructAtOffset<HBUINT8> (&endPtsOfContours[num_contours + 1],
						 endPtsOfContours[num_contours]);

    /* Read flags */
    for (unsigned int i = 0; i < num_points; i++)
    {
      if (unlikely (!bytes.check_range (p))) return false;
      uint8_t flag = *p++;
      points_[i].flag = flag;
      if (flag & FLAG_REPEAT)
      {
	if (unlikely (!bytes.check_range (p))) return false;
	unsigned int repeat_count = *p++;
	while ((repeat_count-- > 0) && (++i < num_points))
	  points_[i].flag = flag;
      }
    }

    /* Read x & y coordinates */
    return read_points (p, points_, bytes, [] (contour_point_t &p, float v) { p.x = v; },
			FLAG_X_SHORT, FLAG_X_SAME)
	&& read_points (p, points_, bytes, [] (contour_point_t &p, float v) { p.y = v; },
			FLAG_Y_SHORT, FLAG_Y_SAME);
  }
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
