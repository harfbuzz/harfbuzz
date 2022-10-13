#ifndef OT_GLYF_VARCOMPOSITEGLYPH_HH
#define OT_GLYF_VARCOMPOSITEGLYPH_HH


#include "../../hb-open-type.hh"


namespace OT {
namespace glyf_impl {


struct VarCompositeGlyphRecord
{
  protected:
  enum var_composite_glyph_flag_t
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
    UNSCALED_COMPONENT_OFFSET	= 0x1000,
#ifndef HB_NO_BEYOND_64K
    GID_IS_24BIT		= 0x2000
#endif
  };

  public:
  unsigned int get_size () const
  {
    unsigned int size = min_size;
    /* glyphIndex is 24bit instead of 16bit */
#ifndef HB_NO_BEYOND_64K
    if (flags & GID_IS_24BIT) size += HBGlyphID24::static_size - HBGlyphID16::static_size;
#endif
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

  void transform_points (contour_point_vector_t &points) const
  {
    float matrix[4];
    contour_point_t trans;
    if (get_transformation (matrix, trans))
    {
      points.transform (matrix);
      points.translate (trans);
    }
  }

  bool get_transformation (float (&matrix)[4], contour_point_t &trans) const
  {
    matrix[0] = matrix[3] = 1.f;
    matrix[1] = matrix[2] = 0.f;

    const auto *p = &StructAfter<const HBINT8> (flags);
#ifndef HB_NO_BEYOND_64K
    if (flags & GID_IS_24BIT)
      p += HBGlyphID24::static_size;
    else
#endif
      p += HBGlyphID16::static_size;
    int tx, ty;
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
  hb_codepoint_t get_gid () const
  {
#ifndef HB_NO_BEYOND_64K
    if (flags & GID_IS_24BIT)
      return StructAfter<const HBGlyphID24> (flags);
    else
#endif
      return StructAfter<const HBGlyphID16> (flags);
  }

  protected:
  HBUINT16	flags;
  HBUINT24	pad;
  public:
  DEFINE_SIZE_MIN (4);
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
