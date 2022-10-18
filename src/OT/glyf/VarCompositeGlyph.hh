#ifndef OT_GLYF_VARCOMPOSITEGLYPH_HH
#define OT_GLYF_VARCOMPOSITEGLYPH_HH


#include "../../hb-open-type.hh"
#include "coord-setter.hh"


namespace OT {
namespace glyf_impl {


struct VarCompositeGlyphRecord
{
  protected:
  enum var_composite_glyph_flag_t
  {
    USE_MY_METRICS		= 0x0001,
    AXIS_INDICES_ARE_SHORT	= 0x0002,
    UNIFORM_SCALE		= 0x0004,
    HAVE_TRANSLATE_X		= 0x0008,
    HAVE_TRANSLATE_Y		= 0x0010,
    HAVE_ROTATION		= 0x0020,
    HAVE_SCALE_X		= 0x0040,
    HAVE_SCALE_Y		= 0x0080,
    HAVE_SKEW_X			= 0x0100,
    HAVE_SKEW_Y			= 0x0200,
    HAVE_TCENTER_X		= 0x0400,
    HAVE_TCENTER_Y		= 0x0800,
  };

  public:

  static constexpr unsigned NUM_TRANSFORM_POINTS = 5;

  unsigned int get_size () const
  {
    unsigned int size = min_size;

    unsigned axis_width = (flags & AXIS_INDICES_ARE_SHORT) ? 4 : 3;
    size += num_axes * axis_width;

    if (flags & HAVE_TRANSLATE_X)	size += 2;
    if (flags & HAVE_TRANSLATE_Y)	size += 2;
    if (flags & HAVE_ROTATION)		size += 2;
    if (flags & HAVE_SCALE_X)		size += 2;
    if (flags & HAVE_SCALE_Y)		size += 2;
    if (flags & HAVE_SKEW_X)		size += 2;
    if (flags & HAVE_SKEW_Y)		size += 2;
    if (flags & HAVE_TCENTER_X)		size += 2;
    if (flags & HAVE_TCENTER_Y)		size += 2;

    return size;
  }

  bool has_more () const { return true; }

  bool is_use_my_metrics () const { return flags & USE_MY_METRICS; }

  hb_codepoint_t get_gid () const
  {
    return gid;
  }

  unsigned get_num_axes () const
  {
    return num_axes;
  }

  unsigned get_num_points () const
  {
    return num_axes + NUM_TRANSFORM_POINTS;
  }

  void transform_points (hb_array_t<contour_point_t> transformation_points,
			 contour_point_vector_t &points) const
  {
    float matrix[4];
    contour_point_t trans;

    get_transformation_from_points (transformation_points, matrix, trans);

    points.transform (matrix);
    points.translate (trans);
  }

  static inline void transform (float (&matrix)[4], contour_point_t &trans,
				float (other)[6])
  {
    // https://github.com/fonttools/fonttools/blob/f66ee05f71c8b57b5f519ee975e95edcd1466e14/Lib/fontTools/misc/transform.py#L268
    float xx1 = other[0];
    float xy1 = other[1];
    float yx1 = other[2];
    float yy1 = other[3];
    float dx1 = other[4];
    float dy1 = other[5];
    float xx2 = matrix[0];
    float xy2 = matrix[1];
    float yx2 = matrix[2];
    float yy2 = matrix[3];
    float dx2 = trans.x;
    float dy2 = trans.y;

    matrix[0] = xx1*xx2 + xy1*yx2;
    matrix[1] = xx1*xy2 + xy1*yy2;
    matrix[2] = yx1*xx2 + yy1*yx2;
    matrix[3] = yx1*xy2 + yy1*yy2;
    trans.x = xx2*dx1 + yx2*dy1 + dx2;
    trans.y = xy2*dx1 + yy2*dy1 + dy2;
  }

  static void translate (float (&matrix)[4], contour_point_t &trans,
			 float translateX, float translateY)
  {
    // https://github.com/fonttools/fonttools/blob/f66ee05f71c8b57b5f519ee975e95edcd1466e14/Lib/fontTools/misc/transform.py#L213
    float other[6] = {1.f, 0.f, 0.f, 1.f, translateX, translateY};
    transform (matrix, trans, other);
  }

  static void scale (float (&matrix)[4], contour_point_t &trans,
		     float scaleX, float scaleY)
  {
    // https://github.com/fonttools/fonttools/blob/f66ee05f71c8b57b5f519ee975e95edcd1466e14/Lib/fontTools/misc/transform.py#L224
    float other[6] = {scaleX, 0.f, 0.f, scaleY, 0.f, 0.f};
    transform (matrix, trans, other);
  }

  static void rotate (float (&matrix)[4], contour_point_t &trans,
		      float rotation)
  {
    // https://github.com/fonttools/fonttools/blob/f66ee05f71c8b57b5f519ee975e95edcd1466e14/Lib/fontTools/misc/transform.py#L240
    rotation = rotation * float (M_PI);
    float c = cosf (rotation);
    float s = sinf (rotation);
    float other[6] = {c, s, -s, c, 0.f, 0.f};
    transform (matrix, trans, other);
  }

  static void skew (float (&matrix)[4], contour_point_t &trans,
		    float skewX, float skewY)
  {
    // https://github.com/fonttools/fonttools/blob/f66ee05f71c8b57b5f519ee975e95edcd1466e14/Lib/fontTools/misc/transform.py#L255
    skewX = skewX * float (M_PI);
    skewY = skewY * float (M_PI);
    float other[6] = {1.f, tanf (skewY), tanf (skewX), 1.f, 0.f, 0.f};
    transform (matrix, trans, other);
  }

  bool get_points (contour_point_vector_t &points) const
  {
    float translateX = 0.f;
    float translateY = 0.f;
    float rotation = 0.f;
    float scaleX = 1.f * (1 << 12);
    float scaleY = 1.f * (1 << 12);
    float skewX = 0.f;
    float skewY = 0.f;
    float tCenterX = 0.f;
    float tCenterY = 0.f;

    if (unlikely (!points.resize (points.length + get_num_points ()))) return false;

    unsigned axis_width = (flags & AXIS_INDICES_ARE_SHORT) ? 2 : 1;
    unsigned axes_size = num_axes * axis_width;

    const F2DOT14 *q = (const F2DOT14 *) (axes_size +
					  &StructAfter<const HBUINT8> (num_axes));

    hb_array_t<contour_point_t> axis_points = points.sub_array (points.length - get_num_points ());
    unsigned count = num_axes;
    for (unsigned i = 0; i < count; i++)
      axis_points[i].x = *q++;

    const HBUINT16 *p = (const HBUINT16 *) q;

    if (flags & HAVE_TRANSLATE_X)	translateX = * (const FWORD *) p++;
    if (flags & HAVE_TRANSLATE_Y)	translateY = * (const FWORD *) p++;
    if (flags & HAVE_ROTATION)		rotation = * (const F2DOT14 *) p++;
    if (flags & HAVE_SCALE_X)		scaleX = * (const F4DOT12 *) p++;
    if (flags & HAVE_SCALE_Y)		scaleY = * (const F4DOT12 *) p++;
    if (flags & HAVE_SKEW_X)		skewX = * (const F2DOT14 *) p++;
    if (flags & HAVE_SKEW_Y)		skewY = * (const F2DOT14 *) p++;
    if (flags & HAVE_TCENTER_X)		tCenterX = * (const FWORD *) p++;
    if (flags & HAVE_TCENTER_Y)		tCenterY = * (const FWORD *) p++;

    if ((flags & UNIFORM_SCALE) && !(flags & HAVE_SCALE_Y))
      scaleY = scaleX;

    hb_array_t<contour_point_t> t = points.sub_array (points.length - NUM_TRANSFORM_POINTS);
    t[0].x = translateX;
    t[0].y = translateY;
    t[1].x = rotation;
    t[2].x = scaleX;
    t[2].y = scaleY;
    t[3].x = skewX;
    t[3].y = skewY;
    t[4].x = tCenterX;
    t[4].y = tCenterY;

    return true;
  }

  void get_transformation_from_points (hb_array_t<contour_point_t> points,
				       float (&matrix)[4], contour_point_t &trans) const
  {
    matrix[0] = matrix[3] = 1.f;
    matrix[1] = matrix[2] = 0.f;
    trans.init (0.f, 0.f);

    hb_array_t<contour_point_t> t = points.sub_array (points.length - NUM_TRANSFORM_POINTS);

    float translateX = t[0].x;
    float translateY = t[0].y;
    float rotation = t[1].x / (1 << 14);
    float scaleX = t[2].x / (1 << 12);
    float scaleY = t[2].y / (1 << 12);
    float skewX = t[3].x / (1 << 14);
    float skewY = t[3].y / (1 << 14);
    float tCenterX = t[4].x;
    float tCenterY = t[4].y;

    translate (matrix, trans, translateX + tCenterX, translateY + tCenterY);
    rotate (matrix, trans, rotation);
    scale (matrix, trans, scaleX, scaleY);
    skew (matrix, trans, -skewX, skewY);
    translate (matrix, trans, -tCenterX, -tCenterY);
  }

  void set_variations (coord_setter_t &setter,
		       hb_array_t<contour_point_t> axis_points) const
  {
    unsigned axis_width = (flags & AXIS_INDICES_ARE_SHORT) ? 2 : 1;

    const HBUINT8  *p = &StructAfter<const HBUINT8>  (num_axes);
    const HBUINT16 *q = &StructAfter<const HBUINT16> (num_axes);

    unsigned count = num_axes;
    for (unsigned i = 0; i < count; i++)
    {
      unsigned axis_index = axis_width == 1 ? *p++ : *q++;
      setter[axis_index] = axis_points[i].x;
    }
  }

  protected:
  HBUINT16	flags;
  HBGlyphID16	gid;
  HBUINT8	num_axes;
  public:
  DEFINE_SIZE_MIN (5);
};

using var_composite_iter_t = composite_iter_tmpl<VarCompositeGlyphRecord>;

struct VarCompositeGlyph
{
  const GlyphHeader &header;
  hb_bytes_t bytes;
  VarCompositeGlyph (const GlyphHeader &header_, hb_bytes_t bytes_) :
    header (header_), bytes (bytes_) {}

  var_composite_iter_t iter () const
  { return var_composite_iter_t (bytes, &StructAfter<VarCompositeGlyphRecord, GlyphHeader> (header)); }

};


} /* namespace glyf_impl */
} /* namespace OT */


#endif /* OT_GLYF_VARCOMPOSITEGLYPH_HH */
