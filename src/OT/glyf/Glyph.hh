#ifndef OT_GLYF_GLYPH_HH
#define OT_GLYF_GLYPH_HH


#include "../../hb-open-type.hh"

#include "GlyphHeader.hh"
#include "SimpleGlyph.hh"
#include "CompositeGlyph.hh"


namespace OT {

struct glyf_accelerator_t;

namespace glyf_impl {


enum phantom_point_index_t
{
  PHANTOM_LEFT   = 0,
  PHANTOM_RIGHT  = 1,
  PHANTOM_TOP    = 2,
  PHANTOM_BOTTOM = 3,
  PHANTOM_COUNT  = 4
};

struct Glyph
{
  enum glyph_type_t { EMPTY, SIMPLE, COMPOSITE };

  public:
  composite_iter_t get_composite_iterator () const
  {
    if (type != COMPOSITE) return composite_iter_t ();
    return CompositeGlyph (*header, bytes).iter ();
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

  void update_mtx (const hb_subset_plan_t *plan,
                   int xMin, int yMax,
                   const contour_point_vector_t &all_points) const
  {
    hb_codepoint_t new_gid = 0;
    if (!plan->new_gid_for_old_gid (gid, &new_gid))
      return;

    unsigned len = all_points.length;
    float leftSideX = all_points[len - 4].x;
    float rightSideX = all_points[len - 3].x;
    float topSideY = all_points[len - 2].y;
    float bottomSideY = all_points[len - 1].y;

    int hori_aw = roundf (rightSideX - leftSideX);
    if (hori_aw < 0) hori_aw = 0;
    int lsb = roundf (xMin - leftSideX);
    plan->hmtx_map->set (new_gid, hb_pair (hori_aw, lsb));

    int vert_aw = roundf (topSideY - bottomSideY);
    if (vert_aw < 0) vert_aw = 0;
    int tsb = roundf (topSideY - yMax);
    plan->vmtx_map->set (new_gid, hb_pair (vert_aw, tsb));
  }

  bool compile_header_bytes (const hb_subset_plan_t *plan,
                             const contour_point_vector_t &all_points,
                             hb_bytes_t &dest_bytes /* OUT */) const
  {
    GlyphHeader *glyph_header = nullptr;
    if (all_points.length > 4)
    {
      glyph_header = (GlyphHeader *) hb_calloc (1, GlyphHeader::static_size);
      if (unlikely (!glyph_header)) return false;
    }

    int xMin, xMax;
    xMin = xMax = roundf (all_points[0].x);

    int yMin, yMax;
    yMin = yMax = roundf (all_points[0].y);

    for (unsigned i = 1; i < all_points.length - 4; i++)
    {
      float rounded_x = roundf (all_points[i].x);
      float rounded_y = roundf (all_points[i].y);
      xMin = hb_min (xMin, rounded_x);
      xMax = hb_max (xMax, rounded_x);
      yMin = hb_min (yMin, rounded_y);
      yMax = hb_max (yMax, rounded_y);
    }

    update_mtx (plan, xMin, yMax, all_points);

    /*for empty glyphs: all_points only include phantom points.
     *just update metrics and then return */
    if (all_points.length == 4)
      return true;

    glyph_header->numberOfContours = header->numberOfContours;
    glyph_header->xMin = xMin;
    glyph_header->yMin = yMin;
    glyph_header->xMax = xMax;
    glyph_header->yMax = yMax;

    dest_bytes = hb_bytes_t ((const char *)glyph_header, GlyphHeader::static_size);
    return true;
  }

  bool compile_bytes_with_deltas (const hb_subset_plan_t *plan,
                                  hb_font_t *font,
                                  const glyf_accelerator_t &glyf,
                                  hb_bytes_t &dest_start,  /* IN/OUT */
                                  hb_bytes_t &dest_end /* OUT */) const
  {
    contour_point_vector_t all_points, deltas;
    get_points (font, glyf, all_points, &deltas, false);

    switch (type) {
    case COMPOSITE:
      if (!CompositeGlyph (*header, bytes).compile_bytes_with_deltas (dest_start,
                                                                      deltas,
                                                                      dest_end))
        return false;
      break;
    case SIMPLE:
      if (!SimpleGlyph (*header, bytes).compile_bytes_with_deltas (all_points,
                                                                   plan->flags & HB_SUBSET_FLAGS_NO_HINTING,
                                                                   dest_end))
        return false;
      break;
    default:
      /* set empty bytes for empty glyph
       * do not use source glyph's pointers */
      dest_start = hb_bytes_t ();
      dest_end = hb_bytes_t ();
      break;
    }

    return compile_header_bytes (plan, all_points, dest_start);
  }


  /* Note: Recursively calls itself.
   * all_points includes phantom points
   */
  template <typename accelerator_t>
  bool get_points (hb_font_t *font, const accelerator_t &glyf_accelerator,
		   contour_point_vector_t &all_points /* OUT */,
		   contour_point_vector_t *deltas = nullptr, /* OUT */
		   bool use_my_metrics = true,
		   bool phantom_only = false,
		   unsigned int depth = 0) const
  {
    if (unlikely (depth > HB_MAX_NESTING_LEVEL)) return false;
    contour_point_vector_t stack_points;
    bool inplace = type == SIMPLE && all_points.length == 0;
    /* Load into all_points if it's empty, as an optimization. */
    contour_point_vector_t &points = inplace ? all_points : stack_points;

    switch (type) {
    case COMPOSITE:
    {
      /* pseudo component points for each component in composite glyph */
      unsigned num_points = hb_len (CompositeGlyph (*header, bytes).iter ());
      if (unlikely (!points.resize (num_points))) return false;
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
      int lsb = 0;
      int h_delta = glyf_accelerator.hmtx->get_leading_bearing_without_var_unscaled (gid, &lsb) ?
		    (int) header->xMin - lsb : 0;
      int tsb = 0;
      int v_orig  = (int) header->yMax +
#ifndef HB_NO_VERTICAL
		    ((void) glyf_accelerator.vmtx->get_leading_bearing_without_var_unscaled (gid, &tsb), tsb)
#else
		    0
#endif
		    ;
      unsigned h_adv = glyf_accelerator.hmtx->get_advance_without_var_unscaled (gid);
      unsigned v_adv =
#ifndef HB_NO_VERTICAL
		       glyf_accelerator.vmtx->get_advance_without_var_unscaled (gid)
#else
		       - font->face->get_upem ()
#endif
		       ;
      phantoms[PHANTOM_LEFT].x = h_delta;
      phantoms[PHANTOM_RIGHT].x = h_adv + h_delta;
      phantoms[PHANTOM_TOP].y = v_orig;
      phantoms[PHANTOM_BOTTOM].y = v_orig - (int) v_adv;
    }

    if (deltas != nullptr && depth == 0 && type == COMPOSITE)
    {
      if (unlikely (!deltas->resize (points.length))) return false;
      for (unsigned i = 0 ; i < points.length; i++)
        deltas->arrayZ[i] = points.arrayZ[i];
    }

#ifndef HB_NO_VAR
    glyf_accelerator.gvar->apply_deltas_to_points (gid, font, points.as_array ());
#endif

    // mainly used by CompositeGlyph calculating new X/Y offset value so no need to extend it
    // with child glyphs' points
    if (deltas != nullptr && depth == 0 && type == COMPOSITE)
    {
      for (unsigned i = 0 ; i < points.length; i++)
      {
        deltas->arrayZ[i].x = points.arrayZ[i].x - deltas->arrayZ[i].x;
        deltas->arrayZ[i].y = points.arrayZ[i].y - deltas->arrayZ[i].y;
      }
    }

    switch (type) {
    case SIMPLE:
      if (!inplace)
	all_points.extend (points.as_array ());
      break;
    case COMPOSITE:
    {
      contour_point_vector_t comp_points;
      unsigned int comp_index = 0;
      for (auto &item : get_composite_iterator ())
      {
        comp_points.reset ();
	if (unlikely (!glyf_accelerator.glyph_for_gid (item.get_gid ())
				       .get_points (font, glyf_accelerator, comp_points,
						    deltas, use_my_metrics, phantom_only, depth + 1)))
	  return false;

	/* Copy phantom points from component if USE_MY_METRICS flag set */
	if (use_my_metrics && item.is_use_my_metrics ())
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

    return !all_points.in_error ();
  }

  bool get_extents_without_var_scaled (hb_font_t *font, const glyf_accelerator_t &glyf_accelerator,
				       hb_glyph_extents_t *extents) const
  {
    if (type == EMPTY) return true; /* Empty glyph; zero extents. */
    return header->get_extents_without_var_scaled (font, glyf_accelerator, gid, extents);
  }

  hb_bytes_t get_bytes () const { return bytes; }

  Glyph (hb_bytes_t bytes_ = hb_bytes_t (),
	 hb_codepoint_t gid_ = (hb_codepoint_t) -1) : bytes (bytes_),
						      header (bytes.as<GlyphHeader> ()),
						      gid (gid_)
  {
    int num_contours = header->numberOfContours;
    if (unlikely (num_contours == 0)) type = EMPTY;
    else if (num_contours > 0) type = SIMPLE;
    else type = COMPOSITE; /* negative numbers */
  }

  protected:
  hb_bytes_t bytes;
  const GlyphHeader *header;
  hb_codepoint_t gid;
  unsigned type;
};


} /* namespace glyf_impl */
} /* namespace OT */


#endif /* OT_GLYF_GLYPH_HH */
