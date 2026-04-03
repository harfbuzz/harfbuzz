/*
 * Copyright © 2026  Behdad Esfahbod
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
 * Author(s): Behdad Esfahbod
 */

#include "hb.hh"

#include "hb-vector-paint.hh"
#include "hb-vector-draw.hh"

#include <math.h>

/* PDF paint callbacks for COLRv0/v1 color glyphs.
 *
 * Maps the HarfBuzz paint API to PDF content stream operators:
 *   push_transform / pop_transform  →  q + cm / Q
 *   push_clip_glyph                 →  q + path + W n
 *   push_clip_rectangle             →  q + re + W n
 *   pop_clip                        →  Q
 *   color                           →  rg + re + f
 *   push_group / pop_group          →  q / Q
 *   linear/radial/sweep gradient    →  solid fallback (first stop color)
 */

/* ---- helpers ---- */

static hb_bool_t
hb_pdf_paint_ensure_initialized (hb_vector_paint_t *paint)
{
  if (paint->group_stack.length)
    return !paint->group_stack.in_error () &&
           !paint->group_stack.tail ().in_error ();
  if (unlikely (!paint->group_stack.push_or_fail ()))
    return false;
  paint->group_stack.tail ().alloc (4096);
  if (unlikely (paint->group_stack.tail ().in_error ()))
  {
    paint->group_stack.pop ();
    return false;
  }
  return !paint->group_stack.in_error ();
}

static void
hb_pdf_append_num (hb_vector_t<char> *buf, float v, unsigned precision)
{
  hb_svg_append_num (buf, v, precision);
}

/* Emit a glyph outline as PDF path operators into buf. */
static void
hb_pdf_emit_glyph_path (hb_vector_paint_t *paint,
			 hb_font_t *font,
			 hb_codepoint_t glyph,
			 hb_vector_t<char> *buf)
{
  /* Reuse the draw struct's path emission.  Build a temporary
   * draw context with PDF format and identity transform. */
  hb_vector_draw_t tmp;
  tmp.format = HB_VECTOR_FORMAT_PDF;
  tmp.precision = paint->precision;
  tmp.transform = {1, 0, 0, 1, 0, 0};
  tmp.path.alloc (1024);

  hb_font_draw_glyph (font, glyph,
		       hb_vector_draw_get_funcs (),
		       &tmp);

  hb_svg_append_len (buf, tmp.path.arrayZ, tmp.path.length);
}

/* ---- paint callbacks ---- */

static void
hb_pdf_paint_push_transform (hb_paint_funcs_t *,
			     void *paint_data,
			     float xx, float yx,
			     float xy, float yy,
			     float dx, float dy,
			     void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!hb_pdf_paint_ensure_initialized (paint)))
    return;

  auto &body = paint->current_body ();
  hb_svg_append_str (&body, "q\n");
  hb_pdf_append_num (&body, xx, paint->precision);
  hb_svg_append_c (&body, ' ');
  hb_pdf_append_num (&body, yx, paint->precision);
  hb_svg_append_c (&body, ' ');
  hb_pdf_append_num (&body, xy, paint->precision);
  hb_svg_append_c (&body, ' ');
  hb_pdf_append_num (&body, yy, paint->precision);
  hb_svg_append_c (&body, ' ');
  hb_pdf_append_num (&body, dx, paint->precision);
  hb_svg_append_c (&body, ' ');
  hb_pdf_append_num (&body, dy, paint->precision);
  hb_svg_append_str (&body, " cm\n");
}

static void
hb_pdf_paint_pop_transform (hb_paint_funcs_t *,
			    void *paint_data,
			    void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!hb_pdf_paint_ensure_initialized (paint)))
    return;
  hb_svg_append_str (&paint->current_body (), "Q\n");
}

static void
hb_pdf_paint_push_clip_glyph (hb_paint_funcs_t *,
			      void *paint_data,
			      hb_codepoint_t glyph,
			      hb_font_t *font,
			      void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!hb_pdf_paint_ensure_initialized (paint)))
    return;

  auto &body = paint->current_body ();
  hb_svg_append_str (&body, "q\n");
  hb_pdf_emit_glyph_path (paint, font, glyph, &body);
  hb_svg_append_str (&body, "W n\n");
}

static void
hb_pdf_paint_push_clip_rectangle (hb_paint_funcs_t *,
				  void *paint_data,
				  float xmin, float ymin,
				  float xmax, float ymax,
				  void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!hb_pdf_paint_ensure_initialized (paint)))
    return;

  auto &body = paint->current_body ();
  hb_svg_append_str (&body, "q\n");
  hb_pdf_append_num (&body, xmin, paint->precision);
  hb_svg_append_c (&body, ' ');
  hb_pdf_append_num (&body, ymin, paint->precision);
  hb_svg_append_c (&body, ' ');
  hb_pdf_append_num (&body, xmax - xmin, paint->precision);
  hb_svg_append_c (&body, ' ');
  hb_pdf_append_num (&body, ymax - ymin, paint->precision);
  hb_svg_append_str (&body, " re W n\n");
}

static void
hb_pdf_paint_pop_clip (hb_paint_funcs_t *,
		       void *paint_data,
		       void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!hb_pdf_paint_ensure_initialized (paint)))
    return;
  hb_svg_append_str (&paint->current_body (), "Q\n");
}

static void
hb_pdf_paint_solid_color (hb_vector_paint_t *paint, hb_color_t c)
{
  auto &body = paint->current_body ();

  float r = hb_color_get_red (c) / 255.f;
  float g = hb_color_get_green (c) / 255.f;
  float b = hb_color_get_blue (c) / 255.f;
  float a = hb_color_get_alpha (c) / 255.f;

  /* Set fill color. */
  hb_pdf_append_num (&body, r, 4);
  hb_svg_append_c (&body, ' ');
  hb_pdf_append_num (&body, g, 4);
  hb_svg_append_c (&body, ' ');
  hb_pdf_append_num (&body, b, 4);
  hb_svg_append_str (&body, " rg\n");

  /* Paint a huge rect (will be clipped). */
  if (a >= 1.f / 255.f)
    hb_svg_append_str (&body, "-32767 -32767 65534 65534 re f\n");
}

static void
hb_pdf_paint_color (hb_paint_funcs_t *,
		    void *paint_data,
		    hb_bool_t is_foreground,
		    hb_color_t color,
		    void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!hb_pdf_paint_ensure_initialized (paint)))
    return;

  hb_color_t c = color;
  if (is_foreground)
    c = HB_COLOR (hb_color_get_blue (paint->foreground),
		  hb_color_get_green (paint->foreground),
		  hb_color_get_red (paint->foreground),
		  (unsigned) hb_color_get_alpha (paint->foreground) *
		  hb_color_get_alpha (color) / 255);

  hb_pdf_paint_solid_color (paint, c);
}

static hb_bool_t
hb_pdf_paint_image (hb_paint_funcs_t *,
		    void *,
		    hb_blob_t *,
		    unsigned, unsigned,
		    hb_tag_t,
		    float,
		    hb_glyph_extents_t *,
		    void *)
{
  /* Images not supported in PDF output. */
  return false;
}

/* Gradient fallback: use the first color stop. */
static hb_color_t
hb_pdf_gradient_fallback_color (hb_vector_paint_t *paint,
				hb_color_line_t *color_line)
{
  unsigned count = 1;
  hb_color_stop_t stop;
  hb_color_line_get_color_stops (color_line, 0, &count, &stop);
  if (count)
    return hb_color_line_get_extend (color_line) == HB_PAINT_EXTEND_PAD
	   ? stop.color : HB_COLOR (0, 0, 0, 255);
  (void) paint;
  return HB_COLOR (0, 0, 0, 255);
}

static void
hb_pdf_paint_linear_gradient (hb_paint_funcs_t *,
			      void *paint_data,
			      hb_color_line_t *color_line,
			      float, float, float, float, float, float,
			      void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!hb_pdf_paint_ensure_initialized (paint)))
    return;
  hb_pdf_paint_solid_color (paint, hb_pdf_gradient_fallback_color (paint, color_line));
}

static void
hb_pdf_paint_radial_gradient (hb_paint_funcs_t *,
			      void *paint_data,
			      hb_color_line_t *color_line,
			      float, float, float, float, float, float,
			      void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!hb_pdf_paint_ensure_initialized (paint)))
    return;
  hb_pdf_paint_solid_color (paint, hb_pdf_gradient_fallback_color (paint, color_line));
}

static void
hb_pdf_paint_sweep_gradient (hb_paint_funcs_t *,
			     void *paint_data,
			     hb_color_line_t *color_line,
			     float, float, float, float,
			     void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!hb_pdf_paint_ensure_initialized (paint)))
    return;
  hb_pdf_paint_solid_color (paint, hb_pdf_gradient_fallback_color (paint, color_line));
}

static void
hb_pdf_paint_push_group (hb_paint_funcs_t *,
			 void *paint_data,
			 void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!hb_pdf_paint_ensure_initialized (paint)))
    return;
  hb_svg_append_str (&paint->current_body (), "q\n");
}

static void
hb_pdf_paint_pop_group (hb_paint_funcs_t *,
			void *paint_data,
			hb_paint_composite_mode_t mode HB_UNUSED,
			void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!hb_pdf_paint_ensure_initialized (paint)))
    return;
  /* PDF 1.4 doesn't support compositing modes; just restore. */
  hb_svg_append_str (&paint->current_body (), "Q\n");
}

/* ---- lazy loader for paint funcs ---- */

static inline void free_static_pdf_paint_funcs ();

static struct hb_pdf_paint_funcs_lazy_loader_t
  : hb_paint_funcs_lazy_loader_t<hb_pdf_paint_funcs_lazy_loader_t>
{
  static hb_paint_funcs_t *create ()
  {
    hb_paint_funcs_t *funcs = hb_paint_funcs_create ();
    hb_paint_funcs_set_push_transform_func (funcs, (hb_paint_push_transform_func_t) hb_pdf_paint_push_transform, nullptr, nullptr);
    hb_paint_funcs_set_pop_transform_func (funcs, (hb_paint_pop_transform_func_t) hb_pdf_paint_pop_transform, nullptr, nullptr);
    hb_paint_funcs_set_push_clip_glyph_func (funcs, (hb_paint_push_clip_glyph_func_t) hb_pdf_paint_push_clip_glyph, nullptr, nullptr);
    hb_paint_funcs_set_push_clip_rectangle_func (funcs, (hb_paint_push_clip_rectangle_func_t) hb_pdf_paint_push_clip_rectangle, nullptr, nullptr);
    hb_paint_funcs_set_pop_clip_func (funcs, (hb_paint_pop_clip_func_t) hb_pdf_paint_pop_clip, nullptr, nullptr);
    hb_paint_funcs_set_color_func (funcs, (hb_paint_color_func_t) hb_pdf_paint_color, nullptr, nullptr);
    hb_paint_funcs_set_image_func (funcs, (hb_paint_image_func_t) hb_pdf_paint_image, nullptr, nullptr);
    hb_paint_funcs_set_linear_gradient_func (funcs, (hb_paint_linear_gradient_func_t) hb_pdf_paint_linear_gradient, nullptr, nullptr);
    hb_paint_funcs_set_radial_gradient_func (funcs, (hb_paint_radial_gradient_func_t) hb_pdf_paint_radial_gradient, nullptr, nullptr);
    hb_paint_funcs_set_sweep_gradient_func (funcs, (hb_paint_sweep_gradient_func_t) hb_pdf_paint_sweep_gradient, nullptr, nullptr);
    hb_paint_funcs_set_push_group_func (funcs, (hb_paint_push_group_func_t) hb_pdf_paint_push_group, nullptr, nullptr);
    hb_paint_funcs_set_pop_group_func (funcs, (hb_paint_pop_group_func_t) hb_pdf_paint_pop_group, nullptr, nullptr);
    hb_paint_funcs_make_immutable (funcs);
    hb_atexit (free_static_pdf_paint_funcs);
    return funcs;
  }
} static_pdf_paint_funcs;

static inline void
free_static_pdf_paint_funcs ()
{
  static_pdf_paint_funcs.free_instance ();
}

hb_paint_funcs_t *
hb_vector_pdf_paint_funcs_get ()
{
  return static_pdf_paint_funcs.get_unconst ();
}

/* ---- render ---- */

hb_blob_t *
hb_vector_pdf_paint_render (hb_vector_paint_t *paint)
{
  if (!paint->has_extents)
    return nullptr;
  if (!paint->group_stack.length ||
      !paint->group_stack.arrayZ[0].length)
    return nullptr;

  /* The content stream is in group_stack[0]. */
  hb_vector_t<char> &content = paint->group_stack.arrayZ[0];

  float ex = paint->extents.x;
  float ey = paint->extents.y;
  float ew = paint->extents.width;
  float eh = paint->extents.height;

  /* Build PDF. */
  hb_vector_t<char> out;
  hb_svg_recover_recycled_buffer (paint->recycled_blob, &out);
  out.alloc (content.length + 512);

  unsigned offsets[5];

  hb_svg_append_str (&out, "%PDF-1.4\n%\xC0\xC1\xC2\xC3\n");

  offsets[0] = out.length;
  hb_svg_append_str (&out, "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n");

  offsets[1] = out.length;
  hb_svg_append_str (&out, "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n");

  offsets[2] = out.length;
  hb_svg_append_str (&out, "3 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [");
  hb_pdf_append_num (&out, ex, paint->precision);
  hb_svg_append_c (&out, ' ');
  hb_pdf_append_num (&out, -(ey + eh), paint->precision);
  hb_svg_append_c (&out, ' ');
  hb_pdf_append_num (&out, ex + ew, paint->precision);
  hb_svg_append_c (&out, ' ');
  hb_pdf_append_num (&out, -ey, paint->precision);
  hb_svg_append_str (&out, "] /Contents 4 0 R >>\nendobj\n");

  offsets[3] = out.length;
  hb_svg_append_str (&out, "4 0 obj\n<< /Length ");
  hb_svg_append_unsigned (&out, content.length);
  hb_svg_append_str (&out, " >>\nstream\n");
  hb_svg_append_len (&out, content.arrayZ, content.length);
  hb_svg_append_str (&out, "endstream\nendobj\n");

  unsigned xref_offset = out.length;
  hb_svg_append_str (&out, "xref\n0 5\n");
  hb_svg_append_str (&out, "0000000000 65535 f \n");
  for (unsigned i = 0; i < 4; i++)
  {
    char tmp[21];
    snprintf (tmp, sizeof (tmp), "%010u 00000 n \n", offsets[i]);
    hb_svg_append_len (&out, tmp, 20);
  }

  hb_svg_append_str (&out, "trailer\n<< /Size 5 /Root 1 0 R >>\nstartxref\n");
  hb_svg_append_unsigned (&out, xref_offset);
  hb_svg_append_str (&out, "\n%%EOF\n");

  hb_blob_t *blob = hb_svg_blob_from_buffer (&paint->recycled_blob, &out);

  /* Reset state. */
  paint->group_stack.clear ();
  paint->defs.clear ();
  paint->path.clear ();
  paint->has_extents = false;
  paint->extents = {0, 0, 0, 0};

  return blob;
}
