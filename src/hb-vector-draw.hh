#ifndef HB_VECTOR_DRAW_HH
#define HB_VECTOR_DRAW_HH

#include "hb.hh"

#include "hb-vector.h"
#include "hb-geometry.hh"
#include "hb-machinery.hh"
#include "hb-vector-utils.hh"
#include "hb-vector-svg.hh"

struct hb_vector_draw_t
{
  hb_object_header_t header;

  hb_vector_format_t format = HB_VECTOR_FORMAT_INVALID;
  hb_transform_t<> transform = {1, 0, 0, 1, 0, 0};
  float x_scale_factor = 1.f;
  float y_scale_factor = 1.f;
  hb_vector_extents_t extents = {0, 0, 0, 0};
  bool has_extents = false;
  hb_color_t foreground = HB_COLOR (0, 0, 0, 255);
  hb_color_t background = HB_COLOR (0, 0, 0, 0);
  hb_buf_t id_prefix;

  hb_buf_t defs;
  hb_buf_t body;
  hb_buf_t path;

  void set_precision (unsigned p)
  {
    p = hb_min (p, 12u);
    defs.precision = p;
    body.precision = p;
    path.precision = p;
  }

  unsigned get_precision () const { return path.precision; }
  hb_set_t *defined_glyphs = nullptr;
  hb_blob_t *recycled_blob = nullptr;

  hb_font_t *cached_font = nullptr;
  unsigned cached_serial = (unsigned) -1;

  void changed ()
  {
    if (defined_glyphs)
      hb_set_clear (defined_glyphs);
    defs.shrink (0);
    body.shrink (0);
    path.shrink (0);
  }

  void check_font (hb_font_t *font)
  {
    unsigned serial = hb_font_get_serial (font);
    if (font != cached_font || serial != cached_serial)
    {
      changed ();
      hb_font_destroy (cached_font);
      cached_font = hb_font_reference (font);
      cached_serial = serial;
    }
  }

  void flush_path ()
  {
    if (!path.length) return;
    switch (format)
    {
      case HB_VECTOR_FORMAT_PDF: flush_path_pdf (); break;
      case HB_VECTOR_FORMAT_SVG: flush_path_svg (); break;
      case HB_VECTOR_FORMAT_INVALID: default: break;
    }
    path.shrink (0);
  }

  void flush_path_pdf ()
  {
    body.append_num (hb_color_get_red (foreground) / 255.f, 4);
    body.append_c (' ');
    body.append_num (hb_color_get_green (foreground) / 255.f, 4);
    body.append_c (' ');
    body.append_num (hb_color_get_blue (foreground) / 255.f, 4);
    body.append_str (" rg\n");
    body.append_len (path.arrayZ, path.length);
    body.append_str ("f\n");
  }

  void flush_path_svg ()
  {
    unsigned r = hb_color_get_red (foreground);
    unsigned g = hb_color_get_green (foreground);
    unsigned b = hb_color_get_blue (foreground);
    unsigned a = hb_color_get_alpha (foreground);
    body.append_str ("<g transform=\"scale(1,-1)\">"
			      "<path d=\"");
    body.append_len (path.arrayZ, path.length);
    body.append_str ("\"");
    if (r || g || b || a != 255)
    {
      body.append_str (" fill=\"rgb(");
      body.append_unsigned (r);
      body.append_c (',');
      body.append_unsigned (g);
      body.append_c (',');
      body.append_unsigned (b);
      body.append_str (")\"");
      if (a < 255)
      {
	body.append_str (" fill-opacity=\"");
	body.append_num (a / 255.f, 4);
	body.append_c ('"');
      }
    }
    body.append_str ("/></g>\n");
  }

  void transform_xy (float x, float y, float *tx, float *ty)
  {
    hb_vector_transform_point (transform, x_scale_factor, y_scale_factor, x, y, tx, ty);
  }

  void append_xy_svg (float x, float y)
  {
    float tx, ty;
    transform_xy (x, y, &tx, &ty);
    path.append_num (tx, path.precision);
    path.append_c (',');
    path.append_num (ty, path.precision);
  }

  void append_xy_pdf (float x, float y)
  {
    float tx, ty;
    transform_xy (x, y, &tx, &ty);
    path.append_num (tx, path.precision);
    path.append_c (' ');
    path.append_num (ty, path.precision);
  }
};

#endif /* HB_VECTOR_DRAW_HH */
