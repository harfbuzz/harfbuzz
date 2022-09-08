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

  static bool read_flags (const HBUINT8 *&p /* IN/OUT */,
			  contour_point_vector_t &points_ /* IN/OUT */,
			  const HBUINT8 *end)
  {
    unsigned count = points_.length;
    for (unsigned int i = 0; i < count;)
    {
      if (unlikely (p + 1 > end)) return false;
      uint8_t flag = *p++;
      points_.arrayZ[i++].flag = flag;
      if (flag & FLAG_REPEAT)
      {
	if (unlikely (p + 1 > end)) return false;
	unsigned int repeat_count = *p++;
	unsigned stop = hb_min (i + repeat_count, count);
	for (; i < stop;)
	  points_.arrayZ[i++].flag = flag;
      }
    }
    return true;
  }

  static bool read_points (const HBUINT8 *&p /* IN/OUT */,
			   contour_point_vector_t &points_ /* IN/OUT */,
			   const HBUINT8 *end,
			   float contour_point_t::*m,
			   const simple_glyph_flag_t short_flag,
			   const simple_glyph_flag_t same_flag)
  {
    int v = 0;

    unsigned count = points_.length;
    for (unsigned i = 0; i < count; i++)
    {
      unsigned flag = points_[i].flag;
      if (flag & short_flag)
      {
	if (unlikely (p + 1 > end)) return false;
	if (flag & same_flag)
	  v += *p++;
	else
	  v -= *p++;
      }
      else
      {
	if (!(flag & same_flag))
	{
	  if (unlikely (p + HBINT16::static_size > end)) return false;
	  v += *(const HBINT16 *) p;
	  p += HBINT16::static_size;
	}
      }
      points_.arrayZ[i].*m = v;
    }
    return true;
  }

  bool get_contour_points (contour_point_vector_t &points_ /* OUT */,
			   bool phantom_only = false) const
  {
    const HBUINT16 *endPtsOfContours = &StructAfter<HBUINT16> (header);
    int num_contours = header.numberOfContours;
    assert (num_contours);
    /* One extra item at the end, for the instruction-count below. */
    if (unlikely (!bytes.check_range (&endPtsOfContours[num_contours]))) return false;
    unsigned int num_points = endPtsOfContours[num_contours - 1] + 1;

    points_.alloc (num_points + 4); // Allocate for phantom points, to avoid a possible copy
    if (!points_.resize (num_points)) return false;
    if (phantom_only) return true;

    for (int i = 0; i < num_contours; i++)
      points_[endPtsOfContours[i]].is_end_point = true;

    /* Skip instructions */
    const HBUINT8 *p = &StructAtOffset<HBUINT8> (&endPtsOfContours[num_contours + 1],
						 endPtsOfContours[num_contours]);

    if (unlikely ((const char *) p < bytes.arrayZ)) return false; /* Unlikely overflow */
    const HBUINT8 *end = (const HBUINT8 *) (bytes.arrayZ + bytes.length);
    if (unlikely (p >= end)) return false;

    /* Read x & y coordinates */
    return read_flags (p, points_, end)
        && read_points (p, points_, end, &contour_point_t::x,
			FLAG_X_SHORT, FLAG_X_SAME)
	&& read_points (p, points_, end, &contour_point_t::y,
			FLAG_Y_SHORT, FLAG_Y_SAME);
  }

  static void encode_coord (int value,
                            uint8_t &flag,
                            const simple_glyph_flag_t short_flag,
                            const simple_glyph_flag_t same_flag,
                            hb_vector_t<uint8_t> &coords /* OUT */)
  {
    if (value == 0)
    {
      flag |= same_flag;
    }
    else if (value >= -255 && value <= 255)
    {
      flag |= short_flag;
      if (value > 0) flag |= same_flag;
      else value = -value;

      coords.push ((uint8_t)value);
    }
    else
    {
      int16_t val = value;
      coords.push (val >> 8);
      coords.push (val & 0xff);
    }
  }

  static void encode_flag (uint8_t &flag,
                           uint8_t &repeat,
                           uint8_t &lastflag,
                           hb_vector_t<uint8_t> &flags /* OUT */)
  {
    if (flag == lastflag && repeat != 255)
    {
      repeat = repeat + 1;
      if (repeat == 1)
      {
        flags.push(flag);
      }
      else
      {
        unsigned len = flags.length;
        flags[len-2] = flag | FLAG_REPEAT;
        flags[len-1] = repeat;
      }
    }
    else
    {
      repeat = 0;
      flags.push (flag);
    }
    lastflag = flag;
  }

  bool compile_bytes_with_deltas (const contour_point_vector_t &all_points,
                                  bool no_hinting,
                                  hb_bytes_t &dest_bytes /* OUT */)
  {
    if (header.numberOfContours == 0 || all_points.length <= 4)
    {
      dest_bytes = hb_bytes_t ();
      return true;
    }
    //convert absolute values to relative values
    unsigned num_points = all_points.length - 4;
    hb_vector_t<hb_pair_t<int, int>> deltas;
    deltas.resize (num_points);

    for (unsigned i = 0; i < num_points; i++)
    {
      deltas[i].first = i == 0 ? roundf (all_points[i].x) : roundf (all_points[i].x) - roundf (all_points[i-1].x);
      deltas[i].second = i == 0 ? roundf (all_points[i].y) : roundf (all_points[i].y) - roundf (all_points[i-1].y);
    }

    hb_vector_t<uint8_t> flags, x_coords, y_coords;
    flags.alloc (num_points);
    x_coords.alloc (2*num_points);
    y_coords.alloc (2*num_points);

    uint8_t lastflag = 0, repeat = 0;

    for (unsigned i = 0; i < num_points; i++)
    {
      uint8_t flag = all_points[i].flag;
      flag &= FLAG_ON_CURVE + FLAG_OVERLAP_SIMPLE;

      encode_coord (deltas[i].first, flag, FLAG_X_SHORT, FLAG_X_SAME, x_coords);
      encode_coord (deltas[i].second, flag, FLAG_Y_SHORT, FLAG_Y_SAME, y_coords);
      if (i == 0) lastflag = flag + 1; //make lastflag != flag for the first point
      encode_flag (flag, repeat, lastflag, flags);
    }

    unsigned len_before_instrs = 2 * header.numberOfContours + 2;
    unsigned len_instrs = instructions_length ();
    unsigned total_len = len_before_instrs + flags.length + x_coords.length + y_coords.length;

    if (!no_hinting)
      total_len += len_instrs;

    char *p = (char *) hb_calloc (total_len, sizeof (char));
    if (unlikely (!p)) return false;

    const char *src = bytes.arrayZ + GlyphHeader::static_size;
    char *cur = p;
    memcpy (p, src, len_before_instrs);

    cur += len_before_instrs;
    src += len_before_instrs;

    if (!no_hinting)
    {
      memcpy (cur, src, len_instrs);
      cur += len_instrs;
    }

    memcpy (cur, flags.arrayZ, flags.length);
    cur += flags.length;

    memcpy (cur, x_coords.arrayZ, x_coords.length);
    cur += x_coords.length;

    memcpy (cur, y_coords.arrayZ, y_coords.length);

    dest_bytes = hb_bytes_t (p, total_len);
    return true;
  }
};


} /* namespace glyf_impl */
} /* namespace OT */


#endif /* OT_GLYF_SIMPLEGLYPH_HH */
