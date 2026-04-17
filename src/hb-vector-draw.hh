#ifndef HB_VECTOR_DRAW_HH
#define HB_VECTOR_DRAW_HH

#include "hb.hh"

#include "hb-vector.h"
#include "hb-geometry.hh"
#include "hb-machinery.hh"
#include "hb-vector-svg-utils.hh"
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
  unsigned precision = 2;
  hb_vector_t<char> id_prefix;

  hb_vector_t<char> defs;
  hb_vector_t<char> body;
  hb_vector_t<char> path;
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
      case HB_VECTOR_FORMAT_PDF:
	/* Per-glyph fill color. */
	hb_buf_append_num (&body, hb_color_get_red (foreground) / 255.f, 4);
	hb_buf_append_c (&body, ' ');
	hb_buf_append_num (&body, hb_color_get_green (foreground) / 255.f, 4);
	hb_buf_append_c (&body, ' ');
	hb_buf_append_num (&body, hb_color_get_blue (foreground) / 255.f, 4);
	hb_buf_append_str (&body, " rg\n");
	hb_buf_append_len (&body, path.arrayZ, path.length);
	hb_buf_append_str (&body, "f\n");
	break;
      case HB_VECTOR_FORMAT_SVG:
      {
	unsigned r = hb_color_get_red (foreground);
	unsigned g = hb_color_get_green (foreground);
	unsigned b = hb_color_get_blue (foreground);
	unsigned a = hb_color_get_alpha (foreground);
	hb_buf_append_str (&body, "<g transform=\"scale(1,-1)\">"
				  "<path d=\"");
	hb_buf_append_len (&body, path.arrayZ, path.length);
	hb_buf_append_str (&body, "\"");
	if (r || g || b || a != 255)
	{
	  hb_buf_append_str (&body, " fill=\"rgb(");
	  hb_buf_append_unsigned (&body, r);
	  hb_buf_append_c (&body, ',');
	  hb_buf_append_unsigned (&body, g);
	  hb_buf_append_c (&body, ',');
	  hb_buf_append_unsigned (&body, b);
	  hb_buf_append_str (&body, ")\"");
	  if (a < 255)
	  {
	    hb_buf_append_str (&body, " fill-opacity=\"");
	    hb_buf_append_num (&body, a / 255.f, 4);
	    hb_buf_append_c (&body, '"');
	  }
	}
	hb_buf_append_str (&body, "/></g>\n");
	break;
      }
      case HB_VECTOR_FORMAT_INVALID: default: break;
    }
    path.shrink (0);
  }

  void append_xy (float x, float y)
  {
    float tx, ty;
    hb_vector_transform_point (transform, x_scale_factor, y_scale_factor, x, y, &tx, &ty);
    hb_buf_append_num (&path, tx, precision);
    switch (format)
    {
      case HB_VECTOR_FORMAT_PDF: hb_buf_append_c (&path, ' '); break;
      case HB_VECTOR_FORMAT_SVG: hb_buf_append_c (&path, ','); break;
      case HB_VECTOR_FORMAT_INVALID: default: break;
    }
    hb_buf_append_num (&path, ty, precision);
  }
};

#endif /* HB_VECTOR_DRAW_HH */
