#ifndef OT_GLYF_SIMPLEGLYPH_HH
#define OT_GLYF_SIMPLEGLYPH_HH


#include "../../hb-open-type.hh"


namespace OT {
namespace glyf_impl {


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
    if (unlikely (!bytes.check_range (&endPtsOfContours[num_contours - 1]))) return false;
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


} /* namespace glyf_impl */
} /* namespace OT */


#endif /* OT_GLYF_SIMPLEGLYPH_HH */
