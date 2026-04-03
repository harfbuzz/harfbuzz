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
#include <stdio.h>

/* PDF paint backend for COLRv0/v1 color font rendering.
 *
 * Supports:
 *   - Solid colors with alpha (via ExtGState)
 *   - Glyph and rectangle clipping
 *   - Affine transforms
 *   - Groups (save/restore)
 *   - Linear gradients (PDF Type 2 axial shading)
 *   - Radial gradients (PDF Type 3 radial shading)
 *   - Sweep gradients (approximated via solid fallback)
 */


/* ---- PDF object collector ---- */

struct hb_pdf_obj_t
{
  hb_vector_t<char> data;
};

/* Collects extra PDF objects (shadings, functions, ExtGState)
 * during painting.  Referenced from the content stream by name
 * (e.g. /SH0, /GS0) and emitted at render time. */
struct hb_pdf_resources_t
{
  hb_vector_t<hb_pdf_obj_t> objects;   /* extra objects, starting at id 5 */
  hb_vector_t<char> extgstate_dict;    /* /GS0 5 0 R /GS1 6 0 R ... */
  hb_vector_t<char> shading_dict;      /* /SH0 7 0 R ... */
  unsigned extgstate_count = 0;
  unsigned shading_count = 0;

  unsigned add_object (hb_vector_t<char> &&obj_data)
  {
    unsigned id = 5 + objects.length; /* objects 1-4 are fixed */
    hb_pdf_obj_t obj;
    obj.data = std::move (obj_data);
    objects.push (std::move (obj));
    return id;
  }

  /* Add ExtGState for fill opacity, return resource name index. */
  unsigned add_extgstate_alpha (float alpha, unsigned precision)
  {
    unsigned idx = extgstate_count++;
    hb_vector_t<char> obj;
    hb_buf_append_str (&obj, "<< /Type /ExtGState /ca ");
    hb_buf_append_num (&obj, alpha, 4);
    hb_buf_append_str (&obj, " >>");
    unsigned obj_id = add_object (std::move (obj));

    /* Add to resource dict. */
    hb_buf_append_str (&extgstate_dict, "/GS");
    hb_buf_append_unsigned (&extgstate_dict, idx);
    hb_buf_append_c (&extgstate_dict, ' ');
    hb_buf_append_unsigned (&extgstate_dict, obj_id);
    hb_buf_append_str (&extgstate_dict, " 0 R ");
    (void) precision;
    return idx;
  }

  /* Add a shading + function for a gradient, return resource name index. */
  unsigned add_shading (hb_vector_t<char> &&shading_data)
  {
    unsigned idx = shading_count++;
    unsigned obj_id = add_object (std::move (shading_data));

    hb_buf_append_str (&shading_dict, "/SH");
    hb_buf_append_unsigned (&shading_dict, idx);
    hb_buf_append_c (&shading_dict, ' ');
    hb_buf_append_unsigned (&shading_dict, obj_id);
    hb_buf_append_str (&shading_dict, " 0 R ");
    return idx;
  }
};

/* Store resources pointer in the paint struct's defs buffer
 * (repurposed — defs is unused for PDF). */
static hb_pdf_resources_t *
hb_pdf_get_resources (hb_vector_paint_t *paint)
{
  if (!paint->defs.length)
  {
    /* First call: allocate and store. */
    auto *res = (hb_pdf_resources_t *) hb_calloc (1, sizeof (hb_pdf_resources_t));
    if (unlikely (!res)) return nullptr;
    new (res) hb_pdf_resources_t ();
    paint->defs.resize (sizeof (void *));
    memcpy (paint->defs.arrayZ, &res, sizeof (void *));
  }
  hb_pdf_resources_t *res;
  memcpy (&res, paint->defs.arrayZ, sizeof (void *));
  return res;
}

static void
hb_pdf_free_resources (hb_vector_paint_t *paint)
{
  if (paint->defs.length >= sizeof (void *))
  {
    hb_pdf_resources_t *res;
    memcpy (&res, paint->defs.arrayZ, sizeof (void *));
    if (res)
    {
      res->~hb_pdf_resources_t ();
      hb_free (res);
    }
  }
  paint->defs.clear ();
}


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

/* Emit a glyph outline as PDF path operators into buf. */
static void
hb_pdf_emit_glyph_path (hb_vector_paint_t *paint,
			 hb_font_t *font,
			 hb_codepoint_t glyph,
			 hb_vector_t<char> *buf)
{
  hb_vector_draw_t tmp;
  tmp.format = HB_VECTOR_FORMAT_PDF;
  tmp.precision = paint->precision;
  tmp.transform = {1, 0, 0, 1, 0, 0};
  tmp.path.alloc (1024);

  hb_font_draw_glyph (font, glyph,
		       hb_vector_draw_get_funcs (),
		       &tmp);

  hb_buf_append_len (buf, tmp.path.arrayZ, tmp.path.length);
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
  hb_buf_append_str (&body, "q\n");
  hb_buf_append_num (&body, xx, paint->precision);
  hb_buf_append_c (&body, ' ');
  hb_buf_append_num (&body, yx, paint->precision);
  hb_buf_append_c (&body, ' ');
  hb_buf_append_num (&body, xy, paint->precision);
  hb_buf_append_c (&body, ' ');
  hb_buf_append_num (&body, yy, paint->precision);
  hb_buf_append_c (&body, ' ');
  hb_buf_append_num (&body, dx, paint->precision);
  hb_buf_append_c (&body, ' ');
  hb_buf_append_num (&body, dy, paint->precision);
  hb_buf_append_str (&body, " cm\n");
}

static void
hb_pdf_paint_pop_transform (hb_paint_funcs_t *,
			    void *paint_data,
			    void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!hb_pdf_paint_ensure_initialized (paint)))
    return;
  hb_buf_append_str (&paint->current_body (), "Q\n");
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
  hb_buf_append_str (&body, "q\n");
  hb_pdf_emit_glyph_path (paint, font, glyph, &body);
  hb_buf_append_str (&body, "W n\n");
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
  hb_buf_append_str (&body, "q\n");
  hb_buf_append_num (&body, xmin, paint->precision);
  hb_buf_append_c (&body, ' ');
  hb_buf_append_num (&body, ymin, paint->precision);
  hb_buf_append_c (&body, ' ');
  hb_buf_append_num (&body, xmax - xmin, paint->precision);
  hb_buf_append_c (&body, ' ');
  hb_buf_append_num (&body, ymax - ymin, paint->precision);
  hb_buf_append_str (&body, " re W n\n");
}

static void
hb_pdf_paint_pop_clip (hb_paint_funcs_t *,
		       void *paint_data,
		       void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!hb_pdf_paint_ensure_initialized (paint)))
    return;
  hb_buf_append_str (&paint->current_body (), "Q\n");
}

/* Paint a solid color, including alpha via ExtGState. */
static void
hb_pdf_paint_solid_color (hb_vector_paint_t *paint, hb_color_t c)
{
  auto &body = paint->current_body ();

  float r = hb_color_get_red (c) / 255.f;
  float g = hb_color_get_green (c) / 255.f;
  float b = hb_color_get_blue (c) / 255.f;
  float a = hb_color_get_alpha (c) / 255.f;

  if (a < 1.f / 255.f)
    return;

  /* Set alpha via ExtGState if needed. */
  if (a < 1.f - 1.f / 512.f)
  {
    auto *res = hb_pdf_get_resources (paint);
    if (res)
    {
      unsigned gs_idx = res->add_extgstate_alpha (a, paint->precision);
      hb_buf_append_str (&body, "/GS");
      hb_buf_append_unsigned (&body, gs_idx);
      hb_buf_append_str (&body, " gs\n");
    }
  }

  /* Set fill color. */
  hb_buf_append_num (&body, r, 4);
  hb_buf_append_c (&body, ' ');
  hb_buf_append_num (&body, g, 4);
  hb_buf_append_c (&body, ' ');
  hb_buf_append_num (&body, b, 4);
  hb_buf_append_str (&body, " rg\n");

  /* Paint a huge rect (will be clipped). */
  hb_buf_append_str (&body, "-32767 -32767 65534 65534 re f\n");
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
  return false;
}

/* ---- Gradient helpers ---- */

/* Build a PDF Type 2 (exponential interpolation) function for
 * a single color stop pair. */
static void
hb_pdf_build_interpolation_function (hb_vector_t<char> *obj,
				     hb_color_t c0, hb_color_t c1)
{
  hb_buf_append_str (obj, "<< /FunctionType 2 /Domain [0 1] /N 1\n");
  hb_buf_append_str (obj, "/C0 [");
  hb_buf_append_num (obj, hb_color_get_red (c0) / 255.f, 4);
  hb_buf_append_c (obj, ' ');
  hb_buf_append_num (obj, hb_color_get_green (c0) / 255.f, 4);
  hb_buf_append_c (obj, ' ');
  hb_buf_append_num (obj, hb_color_get_blue (c0) / 255.f, 4);
  hb_buf_append_str (obj, "]\n/C1 [");
  hb_buf_append_num (obj, hb_color_get_red (c1) / 255.f, 4);
  hb_buf_append_c (obj, ' ');
  hb_buf_append_num (obj, hb_color_get_green (c1) / 255.f, 4);
  hb_buf_append_c (obj, ' ');
  hb_buf_append_num (obj, hb_color_get_blue (c1) / 255.f, 4);
  hb_buf_append_str (obj, "] >>");
}

/* Build a stitching function (Type 3) for multiple color stops. */
static unsigned
hb_pdf_build_gradient_function (hb_pdf_resources_t *res,
				hb_vector_paint_t *paint,
				hb_color_line_t *color_line)
{
  unsigned count = hb_color_line_get_color_stops (color_line, 0, nullptr, nullptr);
  paint->color_stops_scratch.resize (count);
  hb_color_line_get_color_stops (color_line, 0, &count, paint->color_stops_scratch.arrayZ);

  if (count < 2)
  {
    /* Single stop: constant function. */
    hb_color_t c = count ? paint->color_stops_scratch.arrayZ[0].color
			 : HB_COLOR (0, 0, 0, 255);
    hb_vector_t<char> obj;
    hb_pdf_build_interpolation_function (&obj, c, c);
    return res->add_object (std::move (obj));
  }

  /* Sort by offset. */
  paint->color_stops_scratch.as_array ().qsort (
    [] (const hb_color_stop_t &a, const hb_color_stop_t &b)
    { return a.offset < b.offset; });

  if (count == 2)
  {
    /* Two stops: single interpolation function. */
    hb_vector_t<char> obj;
    hb_pdf_build_interpolation_function (&obj,
					  paint->color_stops_scratch.arrayZ[0].color,
					  paint->color_stops_scratch.arrayZ[1].color);
    return res->add_object (std::move (obj));
  }

  /* Multiple stops: create sub-functions and stitch. */
  hb_vector_t<unsigned> sub_func_ids;
  for (unsigned i = 0; i + 1 < count; i++)
  {
    hb_vector_t<char> sub;
    hb_pdf_build_interpolation_function (&sub,
					  paint->color_stops_scratch.arrayZ[i].color,
					  paint->color_stops_scratch.arrayZ[i + 1].color);
    sub_func_ids.push (res->add_object (std::move (sub)));
  }

  /* Stitching function (Type 3). */
  hb_vector_t<char> obj;
  hb_buf_append_str (&obj, "<< /FunctionType 3 /Domain [0 1]\n");

  /* Functions array. */
  hb_buf_append_str (&obj, "/Functions [");
  for (unsigned i = 0; i < sub_func_ids.length; i++)
  {
    if (i) hb_buf_append_c (&obj, ' ');
    hb_buf_append_unsigned (&obj, sub_func_ids.arrayZ[i]);
    hb_buf_append_str (&obj, " 0 R");
  }
  hb_buf_append_str (&obj, "]\n");

  /* Bounds. */
  hb_buf_append_str (&obj, "/Bounds [");
  for (unsigned i = 1; i + 1 < count; i++)
  {
    if (i > 1) hb_buf_append_c (&obj, ' ');
    hb_buf_append_num (&obj, paint->color_stops_scratch.arrayZ[i].offset, 4);
  }
  hb_buf_append_str (&obj, "]\n");

  /* Encode array. */
  hb_buf_append_str (&obj, "/Encode [");
  for (unsigned i = 0; i + 1 < count; i++)
  {
    if (i) hb_buf_append_c (&obj, ' ');
    hb_buf_append_str (&obj, "0 1");
  }
  hb_buf_append_str (&obj, "] >>");

  return res->add_object (std::move (obj));
}

static void
hb_pdf_paint_linear_gradient (hb_paint_funcs_t *,
			      void *paint_data,
			      hb_color_line_t *color_line,
			      float x0, float y0,
			      float x1, float y1,
			      float x2 HB_UNUSED, float y2 HB_UNUSED,
			      void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!hb_pdf_paint_ensure_initialized (paint)))
    return;
  auto *res = hb_pdf_get_resources (paint);
  if (unlikely (!res))
    return;

  unsigned func_id = hb_pdf_build_gradient_function (res, paint, color_line);

  /* Build Type 2 (axial) shading. */
  hb_vector_t<char> sh;
  hb_buf_append_str (&sh, "<< /ShadingType 2 /ColorSpace /DeviceRGB\n");
  hb_buf_append_str (&sh, "/Coords [");
  hb_buf_append_num (&sh, x0, paint->precision);
  hb_buf_append_c (&sh, ' ');
  hb_buf_append_num (&sh, y0, paint->precision);
  hb_buf_append_c (&sh, ' ');
  hb_buf_append_num (&sh, x1, paint->precision);
  hb_buf_append_c (&sh, ' ');
  hb_buf_append_num (&sh, y1, paint->precision);
  hb_buf_append_str (&sh, "]\n/Function ");
  hb_buf_append_unsigned (&sh, func_id);
  hb_buf_append_str (&sh, " 0 R\n");

  hb_paint_extend_t extend = hb_color_line_get_extend (color_line);
  if (extend == HB_PAINT_EXTEND_PAD)
    hb_buf_append_str (&sh, "/Extend [true true]\n");

  hb_buf_append_str (&sh, ">>");

  unsigned sh_idx = res->add_shading (std::move (sh));

  auto &body = paint->current_body ();
  hb_buf_append_str (&body, "/SH");
  hb_buf_append_unsigned (&body, sh_idx);
  hb_buf_append_str (&body, " sh\n");
}

static void
hb_pdf_paint_radial_gradient (hb_paint_funcs_t *,
			      void *paint_data,
			      hb_color_line_t *color_line,
			      float x0, float y0, float r0,
			      float x1, float y1, float r1,
			      void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!hb_pdf_paint_ensure_initialized (paint)))
    return;
  auto *res = hb_pdf_get_resources (paint);
  if (unlikely (!res))
    return;

  unsigned func_id = hb_pdf_build_gradient_function (res, paint, color_line);

  /* Build Type 3 (radial) shading. */
  hb_vector_t<char> sh;
  hb_buf_append_str (&sh, "<< /ShadingType 3 /ColorSpace /DeviceRGB\n");
  hb_buf_append_str (&sh, "/Coords [");
  hb_buf_append_num (&sh, x0, paint->precision);
  hb_buf_append_c (&sh, ' ');
  hb_buf_append_num (&sh, y0, paint->precision);
  hb_buf_append_c (&sh, ' ');
  hb_buf_append_num (&sh, r0, paint->precision);
  hb_buf_append_c (&sh, ' ');
  hb_buf_append_num (&sh, x1, paint->precision);
  hb_buf_append_c (&sh, ' ');
  hb_buf_append_num (&sh, y1, paint->precision);
  hb_buf_append_c (&sh, ' ');
  hb_buf_append_num (&sh, r1, paint->precision);
  hb_buf_append_str (&sh, "]\n/Function ");
  hb_buf_append_unsigned (&sh, func_id);
  hb_buf_append_str (&sh, " 0 R\n");

  hb_paint_extend_t extend = hb_color_line_get_extend (color_line);
  if (extend == HB_PAINT_EXTEND_PAD)
    hb_buf_append_str (&sh, "/Extend [true true]\n");

  hb_buf_append_str (&sh, ">>");

  unsigned sh_idx = res->add_shading (std::move (sh));

  auto &body = paint->current_body ();
  hb_buf_append_str (&body, "/SH");
  hb_buf_append_unsigned (&body, sh_idx);
  hb_buf_append_str (&body, " sh\n");
}

static hb_color_t
hb_pdf_lerp_color (hb_color_t c0, hb_color_t c1, float t)
{
  auto lerp = [&] (unsigned shift) -> unsigned {
    unsigned v0 = (c0 >> shift) & 0xFF;
    unsigned v1 = (c1 >> shift) & 0xFF;
    return (unsigned) (v0 + t * ((float) v1 - (float) v0) + 0.5f);
  };
  return HB_COLOR (lerp (16), lerp (8), lerp (0), lerp (24));
}

/* Encode a 16-bit big-endian unsigned value into buf. */
static void
hb_pdf_encode_u16 (hb_vector_t<char> *buf, uint16_t v)
{
  char bytes[2] = {(char) (v >> 8), (char) (v & 0xFF)};
  hb_buf_append_len (buf, bytes, 2);
}

/* Encode a coordinate as 16-bit value relative to Decode range. */
static void
hb_pdf_encode_coord (hb_vector_t<char> *buf,
		     float val, float lo, float hi)
{
  float t = (val - lo) / (hi - lo);
  t = hb_clamp (t, 0.f, 1.f);
  hb_pdf_encode_u16 (buf, (uint16_t) (t * 65535.f + 0.5f));
}

/* Encode RGB from hb_color_t as 3 bytes. */
static void
hb_pdf_encode_color_rgb (hb_vector_t<char> *buf, hb_color_t c)
{
  char rgb[3] = {(char) hb_color_get_red (c),
		 (char) hb_color_get_green (c),
		 (char) hb_color_get_blue (c)};
  hb_buf_append_len (buf, rgb, 3);
}

/* Encode one Coons patch control point. */
static void
hb_pdf_encode_point (hb_vector_t<char> *buf,
		     float x, float y,
		     float xlo, float xhi,
		     float ylo, float yhi)
{
  hb_pdf_encode_coord (buf, x, xlo, xhi);
  hb_pdf_encode_coord (buf, y, ylo, yhi);
}

/* Emit one Coons patch sector into the mesh stream.
 * Splits large arcs into sub-patches of max 90°. */
static void
hb_pdf_add_sweep_patch (hb_vector_t<char> *mesh,
			float cx, float cy,
			float xlo, float xhi, float ylo, float yhi,
			float a0, hb_color_t c0_in,
			float a1, hb_color_t c1_in)
{
  const float R = 32767.f;
  const float eps = 0.5f;
  const float MAX_SECTOR = (float) M_PI / 2.f;

  int num_splits = (int) ceilf (fabsf (a1 - a0) / MAX_SECTOR);
  if (num_splits < 1) num_splits = 1;

  for (int s = 0; s < num_splits; s++)
  {
    float k0 = (float) s / num_splits;
    float k1 = (float) (s + 1) / num_splits;
    float sa0 = a0 + k0 * (a1 - a0);
    float sa1 = a0 + k1 * (a1 - a0);
    hb_color_t sc0 = hb_pdf_lerp_color (c0_in, c1_in, k0);
    hb_color_t sc1 = hb_pdf_lerp_color (c0_in, c1_in, k1);

    float da = sa1 - sa0;
    float kappa = (4.f / 3.f) * tanf (da / 4.f);

    float cos0 = cosf (sa0), sin0 = sinf (sa0);
    float cos1 = cosf (sa1), sin1 = sinf (sa1);

    float p0x = cx + eps * cos0, p0y = cy + eps * sin0;
    float p3x = cx + R * cos0,   p3y = cy + R * sin0;
    float p6x = cx + R * cos1,   p6y = cy + R * sin1;
    float p9x = cx + eps * cos1, p9y = cy + eps * sin1;

    /* Edge 1: p0→p3, radial straight line. */
    float e1_1x = p0x + (p3x - p0x) / 3.f;
    float e1_1y = p0y + (p3y - p0y) / 3.f;
    float e1_2x = p0x + 2.f * (p3x - p0x) / 3.f;
    float e1_2y = p0y + 2.f * (p3y - p0y) / 3.f;

    /* Edge 2: p3→p6, outer arc. */
    float e2_1x = p3x + kappa * R * (-sin0);
    float e2_1y = p3y + kappa * R * ( cos0);
    float e2_2x = p6x - kappa * R * (-sin1);
    float e2_2y = p6y - kappa * R * ( cos1);

    /* Edge 3: p6→p9, radial straight line. */
    float e3_1x = p6x + (p9x - p6x) / 3.f;
    float e3_1y = p6y + (p9y - p6y) / 3.f;
    float e3_2x = p6x + 2.f * (p9x - p6x) / 3.f;
    float e3_2y = p6y + 2.f * (p9y - p6y) / 3.f;

    /* Edge 4: p9→p0, inner arc. */
    float e4_1x = p9x + kappa * eps * (-sin1);
    float e4_1y = p9y + kappa * eps * ( cos1);
    float e4_2x = p0x - kappa * eps * (-sin0);
    float e4_2y = p0y - kappa * eps * ( cos0);

    hb_buf_append_c (mesh, '\0'); /* flag = 0, new patch */

    hb_pdf_encode_point (mesh, p0x, p0y, xlo, xhi, ylo, yhi);
    hb_pdf_encode_point (mesh, e1_1x, e1_1y, xlo, xhi, ylo, yhi);
    hb_pdf_encode_point (mesh, e1_2x, e1_2y, xlo, xhi, ylo, yhi);
    hb_pdf_encode_point (mesh, p3x, p3y, xlo, xhi, ylo, yhi);
    hb_pdf_encode_point (mesh, e2_1x, e2_1y, xlo, xhi, ylo, yhi);
    hb_pdf_encode_point (mesh, e2_2x, e2_2y, xlo, xhi, ylo, yhi);
    hb_pdf_encode_point (mesh, p6x, p6y, xlo, xhi, ylo, yhi);
    hb_pdf_encode_point (mesh, e3_1x, e3_1y, xlo, xhi, ylo, yhi);
    hb_pdf_encode_point (mesh, e3_2x, e3_2y, xlo, xhi, ylo, yhi);
    hb_pdf_encode_point (mesh, p9x, p9y, xlo, xhi, ylo, yhi);
    hb_pdf_encode_point (mesh, e4_1x, e4_1y, xlo, xhi, ylo, yhi);
    hb_pdf_encode_point (mesh, e4_2x, e4_2y, xlo, xhi, ylo, yhi);

    hb_pdf_encode_color_rgb (mesh, sc0); /* inner start */
    hb_pdf_encode_color_rgb (mesh, sc0); /* outer start */
    hb_pdf_encode_color_rgb (mesh, sc1); /* outer end */
    hb_pdf_encode_color_rgb (mesh, sc1); /* inner end */
  }
}

static void
hb_pdf_paint_sweep_gradient (hb_paint_funcs_t *,
			     void *paint_data,
			     hb_color_line_t *color_line,
			     float cx, float cy,
			     float start_angle,
			     float end_angle,
			     void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!hb_pdf_paint_ensure_initialized (paint)))
    return;
  auto *res = hb_pdf_get_resources (paint);
  if (unlikely (!res))
    return;

  /* Get and sort color stops. */
  unsigned n_stops = hb_color_line_get_color_stops (color_line, 0, nullptr, nullptr);
  paint->color_stops_scratch.resize (n_stops);
  hb_color_line_get_color_stops (color_line, 0, &n_stops, paint->color_stops_scratch.arrayZ);
  paint->color_stops_scratch.as_array ().qsort (
    [] (const hb_color_stop_t &a, const hb_color_stop_t &b)
    { return a.offset < b.offset; });
  hb_color_stop_t *stops = paint->color_stops_scratch.arrayZ;

  if (!n_stops)
    return;

  hb_paint_extend_t extend = hb_color_line_get_extend (color_line);

  const float R = 32767.f;
  float xlo = cx - R - 1, xhi = cx + R + 1;
  float ylo = cy - R - 1, yhi = cy + R + 1;

  hb_vector_t<char> mesh;
  mesh.alloc (256);

  /* Same tiling logic as hb_svg_add_sweep_gradient_patches. */

  if (start_angle == end_angle)
  {
    if (extend == HB_PAINT_EXTEND_PAD)
    {
      if (start_angle > 0.f)
	hb_pdf_add_sweep_patch (&mesh, cx, cy, xlo, xhi, ylo, yhi,
				0.f, stops[0].color, start_angle, stops[0].color);
      if (end_angle < HB_2_PI)
	hb_pdf_add_sweep_patch (&mesh, cx, cy, xlo, xhi, ylo, yhi,
				end_angle, stops[n_stops - 1].color, HB_2_PI, stops[n_stops - 1].color);
    }
    goto emit;
  }

  if (end_angle < start_angle)
  {
    float tmp = start_angle; start_angle = end_angle; end_angle = tmp;
    for (unsigned i = 0; i < n_stops - 1 - i; i++)
    {
      hb_color_stop_t t = stops[i];
      stops[i] = stops[n_stops - 1 - i];
      stops[n_stops - 1 - i] = t;
    }
    for (unsigned i = 0; i < n_stops; i++)
      stops[i].offset = 1.f - stops[i].offset;
  }

  {
    /* Map stop offsets to angles. */
    float angles_buf[16];
    hb_color_t colors_buf[16];
    float *angles = angles_buf;
    hb_color_t *colors = colors_buf;
    bool dynamic = false;

    if (n_stops > 16)
    {
      angles = (float *) hb_malloc (sizeof (float) * n_stops);
      colors = (hb_color_t *) hb_malloc (sizeof (hb_color_t) * n_stops);
      if (!angles || !colors)
      {
	hb_free (angles);
	hb_free (colors);
	goto emit;
      }
      dynamic = true;
    }

    for (unsigned i = 0; i < n_stops; i++)
    {
      angles[i] = start_angle + stops[i].offset * (end_angle - start_angle);
      colors[i] = stops[i].color;
    }

    if (extend == HB_PAINT_EXTEND_PAD)
    {
      unsigned pos;
      hb_color_t color0 = colors[0];
      for (pos = 0; pos < n_stops; pos++)
      {
	if (angles[pos] >= 0)
	{
	  if (pos > 0)
	  {
	    float f = (0.f - angles[pos - 1]) / (angles[pos] - angles[pos - 1]);
	    color0 = hb_pdf_lerp_color (colors[pos - 1], colors[pos], f);
	  }
	  break;
	}
      }
      if (pos == n_stops)
      {
	color0 = colors[n_stops - 1];
	hb_pdf_add_sweep_patch (&mesh, cx, cy, xlo, xhi, ylo, yhi,
				0.f, color0, HB_2_PI, color0);
	goto done;
      }
      hb_pdf_add_sweep_patch (&mesh, cx, cy, xlo, xhi, ylo, yhi,
			      0.f, color0, angles[pos], colors[pos]);
      for (pos++; pos < n_stops; pos++)
      {
	if (angles[pos] <= HB_2_PI)
	  hb_pdf_add_sweep_patch (&mesh, cx, cy, xlo, xhi, ylo, yhi,
				  angles[pos - 1], colors[pos - 1], angles[pos], colors[pos]);
	else
	{
	  float f = (HB_2_PI - angles[pos - 1]) / (angles[pos] - angles[pos - 1]);
	  hb_color_t color1 = hb_pdf_lerp_color (colors[pos - 1], colors[pos], f);
	  hb_pdf_add_sweep_patch (&mesh, cx, cy, xlo, xhi, ylo, yhi,
				  angles[pos - 1], colors[pos - 1], HB_2_PI, color1);
	  break;
	}
      }
      if (pos == n_stops)
      {
	color0 = colors[n_stops - 1];
	hb_pdf_add_sweep_patch (&mesh, cx, cy, xlo, xhi, ylo, yhi,
				angles[n_stops - 1], color0, HB_2_PI, color0);
	goto done;
      }
    }
    else
    {
      float span = angles[n_stops - 1] - angles[0];
      if (fabsf (span) < 1e-6f)
	goto done;

      int k = 0;
      if (angles[0] >= 0)
      {
	float ss = angles[0];
	while (ss > 0)
	{
	  if (span > 0) { ss -= span; k--; }
	  else          { ss += span; k++; }
	}
      }
      else
      {
	float ee = angles[n_stops - 1];
	while (ee < 0)
	{
	  if (span > 0) { ee += span; k++; }
	  else          { ee -= span; k--; }
	}
      }

      span = fabsf (span);
      for (int l = k; l < 1000; l++)
      {
	for (unsigned i = 1; i < n_stops; i++)
	{
	  float a0_l, a1_l;
	  hb_color_t col0, col1;
	  if ((l % 2 != 0) && (extend == HB_PAINT_EXTEND_REFLECT))
	  {
	    a0_l = angles[0] + angles[n_stops - 1] - angles[n_stops - i] + l * span;
	    a1_l = angles[0] + angles[n_stops - 1] - angles[n_stops - 1 - i] + l * span;
	    col0 = colors[n_stops - i];
	    col1 = colors[n_stops - 1 - i];
	  }
	  else
	  {
	    a0_l = angles[i - 1] + l * span;
	    a1_l = angles[i] + l * span;
	    col0 = colors[i - 1];
	    col1 = colors[i];
	  }

	  if (a1_l < 0.f) continue;
	  if (a0_l < 0.f)
	  {
	    float f = (0.f - a0_l) / (a1_l - a0_l);
	    hb_color_t c = hb_pdf_lerp_color (col0, col1, f);
	    hb_pdf_add_sweep_patch (&mesh, cx, cy, xlo, xhi, ylo, yhi,
				    0.f, c, a1_l, col1);
	  }
	  else if (a1_l >= HB_2_PI)
	  {
	    float f = (HB_2_PI - a0_l) / (a1_l - a0_l);
	    hb_color_t c = hb_pdf_lerp_color (col0, col1, f);
	    hb_pdf_add_sweep_patch (&mesh, cx, cy, xlo, xhi, ylo, yhi,
				    a0_l, col0, HB_2_PI, c);
	    goto done;
	  }
	  else
	    hb_pdf_add_sweep_patch (&mesh, cx, cy, xlo, xhi, ylo, yhi,
				    a0_l, col0, a1_l, col1);
	}
      }
    }

done:
    if (dynamic)
    {
      hb_free (angles);
      hb_free (colors);
    }
  }

emit:
  if (!mesh.length)
    return;

  /* Build the shading stream object. */
  hb_vector_t<char> sh;
  hb_buf_append_str (&sh, "<< /ShadingType 6 /ColorSpace /DeviceRGB\n");
  hb_buf_append_str (&sh, "/BitsPerCoordinate 16 /BitsPerComponent 8 /BitsPerFlag 8\n");
  hb_buf_append_str (&sh, "/Decode [");
  hb_buf_append_num (&sh, xlo, 2);
  hb_buf_append_c (&sh, ' ');
  hb_buf_append_num (&sh, xhi, 2);
  hb_buf_append_c (&sh, ' ');
  hb_buf_append_num (&sh, ylo, 2);
  hb_buf_append_c (&sh, ' ');
  hb_buf_append_num (&sh, yhi, 2);
  hb_buf_append_str (&sh, " 0 1 0 1 0 1]\n");
  hb_buf_append_str (&sh, "/Length ");
  hb_buf_append_unsigned (&sh, mesh.length);
  hb_buf_append_str (&sh, " >>\nstream\n");
  hb_buf_append_len (&sh, mesh.arrayZ, mesh.length);
  hb_buf_append_str (&sh, "\nendstream");

  unsigned sh_idx = res->add_shading (std::move (sh));

  auto &body = paint->current_body ();
  hb_buf_append_str (&body, "/SH");
  hb_buf_append_unsigned (&body, sh_idx);
  hb_buf_append_str (&body, " sh\n");
}

static void
hb_pdf_paint_push_group (hb_paint_funcs_t *,
			 void *paint_data,
			 void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!hb_pdf_paint_ensure_initialized (paint)))
    return;
  hb_buf_append_str (&paint->current_body (), "q\n");
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
  hb_buf_append_str (&paint->current_body (), "Q\n");
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

  hb_vector_t<char> &content = paint->group_stack.arrayZ[0];
  hb_pdf_resources_t *res = hb_pdf_get_resources (paint);

  float ex = paint->extents.x;
  float ey = paint->extents.y;
  float ew = paint->extents.width;
  float eh = paint->extents.height;

  unsigned num_extra = res ? res->objects.length : 0;
  unsigned total_objects = 4 + num_extra; /* 1-based: 1..total_objects */

  /* Build PDF. */
  hb_vector_t<char> out;
  hb_buf_recover_recycled (paint->recycled_blob, &out);
  out.alloc (content.length + num_extra * 128 + 1024);

  hb_vector_t<unsigned> offsets;
  offsets.resize (total_objects);

  hb_buf_append_str (&out, "%PDF-1.4\n%\xC0\xC1\xC2\xC3\n");

  /* Object 1: Catalog */
  offsets.arrayZ[0] = out.length;
  hb_buf_append_str (&out, "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n");

  /* Object 2: Pages */
  offsets.arrayZ[1] = out.length;
  hb_buf_append_str (&out, "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n");

  /* Object 3: Page */
  offsets.arrayZ[2] = out.length;
  hb_buf_append_str (&out, "3 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [");
  hb_buf_append_num (&out, ex, paint->precision);
  hb_buf_append_c (&out, ' ');
  hb_buf_append_num (&out, -(ey + eh), paint->precision);
  hb_buf_append_c (&out, ' ');
  hb_buf_append_num (&out, ex + ew, paint->precision);
  hb_buf_append_c (&out, ' ');
  hb_buf_append_num (&out, -ey, paint->precision);
  hb_buf_append_str (&out, "]\n/Contents 4 0 R");

  /* Resources. */
  bool has_resources = res &&
    (res->extgstate_dict.length || res->shading_dict.length);
  if (has_resources)
  {
    hb_buf_append_str (&out, "\n/Resources <<");
    if (res->extgstate_dict.length)
    {
      hb_buf_append_str (&out, " /ExtGState << ");
      hb_buf_append_len (&out, res->extgstate_dict.arrayZ, res->extgstate_dict.length);
      hb_buf_append_str (&out, ">>");
    }
    if (res->shading_dict.length)
    {
      hb_buf_append_str (&out, " /Shading << ");
      hb_buf_append_len (&out, res->shading_dict.arrayZ, res->shading_dict.length);
      hb_buf_append_str (&out, ">>");
    }
    hb_buf_append_str (&out, " >>");
  }

  hb_buf_append_str (&out, " >>\nendobj\n");

  /* Object 4: Content stream */
  offsets.arrayZ[3] = out.length;
  hb_buf_append_str (&out, "4 0 obj\n<< /Length ");
  hb_buf_append_unsigned (&out, content.length);
  hb_buf_append_str (&out, " >>\nstream\n");
  hb_buf_append_len (&out, content.arrayZ, content.length);
  hb_buf_append_str (&out, "endstream\nendobj\n");

  /* Extra objects (functions, shadings, ExtGState). */
  for (unsigned i = 0; i < num_extra; i++)
  {
    offsets.arrayZ[4 + i] = out.length;
    hb_buf_append_unsigned (&out, 5 + i);
    hb_buf_append_str (&out, " 0 obj\n");
    auto &obj = res->objects.arrayZ[i];
    hb_buf_append_len (&out, obj.data.arrayZ, obj.data.length);
    hb_buf_append_str (&out, "\nendobj\n");
  }

  /* Cross-reference table */
  unsigned xref_offset = out.length;
  hb_buf_append_str (&out, "xref\n0 ");
  hb_buf_append_unsigned (&out, total_objects + 1);
  hb_buf_append_str (&out, "\n0000000000 65535 f \n");
  for (unsigned i = 0; i < total_objects; i++)
  {
    char tmp[21];
    snprintf (tmp, sizeof (tmp), "%010u 00000 n \n", offsets.arrayZ[i]);
    hb_buf_append_len (&out, tmp, 20);
  }

  /* Trailer */
  hb_buf_append_str (&out, "trailer\n<< /Size ");
  hb_buf_append_unsigned (&out, total_objects + 1);
  hb_buf_append_str (&out, " /Root 1 0 R >>\nstartxref\n");
  hb_buf_append_unsigned (&out, xref_offset);
  hb_buf_append_str (&out, "\n%%EOF\n");

  hb_blob_t *blob = hb_buf_blob_from (&paint->recycled_blob, &out);

  /* Reset state. */
  hb_pdf_free_resources (paint);
  paint->group_stack.clear ();
  paint->path.clear ();
  paint->has_extents = false;
  paint->extents = {0, 0, 0, 0};

  return blob;
}
