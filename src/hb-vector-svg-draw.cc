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

#include "hb-vector-draw.hh"
#include "hb-blob.hh"
#include "hb-map.hh"
#include "hb-vector-svg-path.hh"
#include "hb-vector-svg-subset.hh"

#include <algorithm>
#include <math.h>
#include <string.h>

HB_UNUSED static inline bool
hb_svg_buffer_contains (const hb_vector_t<char> &buf, const char *needle)
{
  unsigned nlen = (unsigned) strlen (needle);
  if (!nlen || buf.length < nlen)
    return false;

  for (unsigned i = 0; i + nlen <= buf.length; i++)
    if (buf.arrayZ[i] == needle[0] &&
        !memcmp (buf.arrayZ + i, needle, nlen))
      return true;
  return false;
}

static void
hb_vector_draw_move_to (hb_draw_funcs_t *,
                        void *draw_data,
                        hb_draw_state_t *,
                        float to_x, float to_y,
                        void *)
{
  auto *d = (hb_vector_draw_t *) draw_data;
  if (d->format == HB_VECTOR_FORMAT_PDF)
  {
    d->append_xy (to_x, to_y);
    hb_buf_append_str (&d->path, " m\n");
  }
  else
  {
    hb_buf_append_c (&d->path, 'M');
    d->append_xy (to_x, to_y);
  }
}

static void
hb_vector_draw_line_to (hb_draw_funcs_t *,
                        void *draw_data,
                        hb_draw_state_t *,
                        float to_x, float to_y,
                        void *)
{
  auto *d = (hb_vector_draw_t *) draw_data;
  if (d->format == HB_VECTOR_FORMAT_PDF)
  {
    d->append_xy (to_x, to_y);
    hb_buf_append_str (&d->path, " l\n");
  }
  else
  {
    hb_buf_append_c (&d->path, 'L');
    d->append_xy (to_x, to_y);
  }
}

static void
hb_vector_draw_quadratic_to (hb_draw_funcs_t *,
                             void *draw_data,
                             hb_draw_state_t *st,
                             float cx, float cy,
                             float to_x, float to_y,
                             void *)
{
  auto *d = (hb_vector_draw_t *) draw_data;
  if (d->format == HB_VECTOR_FORMAT_PDF)
  {
    /* PDF has no quadratic operator; promote to cubic. */
    float sx = st->current_x, sy = st->current_y;
    float c1x = sx + 2.f / 3.f * (cx - sx);
    float c1y = sy + 2.f / 3.f * (cy - sy);
    float c2x = to_x + 2.f / 3.f * (cx - to_x);
    float c2y = to_y + 2.f / 3.f * (cy - to_y);
    d->append_xy (c1x, c1y);
    hb_buf_append_c (&d->path, ' ');
    d->append_xy (c2x, c2y);
    hb_buf_append_c (&d->path, ' ');
    d->append_xy (to_x, to_y);
    hb_buf_append_str (&d->path, " c\n");
  }
  else
  {
    hb_buf_append_c (&d->path, 'Q');
    d->append_xy (cx, cy);
    hb_buf_append_c (&d->path, ' ');
    d->append_xy (to_x, to_y);
  }
}

static void
hb_vector_draw_cubic_to (hb_draw_funcs_t *,
                         void *draw_data,
                         hb_draw_state_t *,
                         float c1x, float c1y,
                         float c2x, float c2y,
                         float to_x, float to_y,
                         void *)
{
  auto *d = (hb_vector_draw_t *) draw_data;
  if (d->format == HB_VECTOR_FORMAT_PDF)
  {
    d->append_xy (c1x, c1y);
    hb_buf_append_c (&d->path, ' ');
    d->append_xy (c2x, c2y);
    hb_buf_append_c (&d->path, ' ');
    d->append_xy (to_x, to_y);
    hb_buf_append_str (&d->path, " c\n");
  }
  else
  {
    hb_buf_append_c (&d->path, 'C');
    d->append_xy (c1x, c1y);
    hb_buf_append_c (&d->path, ' ');
    d->append_xy (c2x, c2y);
    hb_buf_append_c (&d->path, ' ');
    d->append_xy (to_x, to_y);
  }
}

static void
hb_vector_draw_close_path (hb_draw_funcs_t *,
                           void *draw_data,
                           hb_draw_state_t *,
                           void *)
{
  auto *d = (hb_vector_draw_t *) draw_data;
  if (d->format == HB_VECTOR_FORMAT_PDF)
    hb_buf_append_str (&d->path, "h\n");
  else
    hb_buf_append_c (&d->path, 'Z');
}

static inline void free_static_vector_draw_funcs ();

static struct hb_vector_draw_funcs_lazy_loader_t
  : hb_draw_funcs_lazy_loader_t<hb_vector_draw_funcs_lazy_loader_t>
{
  static hb_draw_funcs_t *create ()
  {
    hb_draw_funcs_t *funcs = hb_draw_funcs_create ();
    hb_draw_funcs_set_move_to_func (funcs, (hb_draw_move_to_func_t) hb_vector_draw_move_to, nullptr, nullptr);
    hb_draw_funcs_set_line_to_func (funcs, (hb_draw_line_to_func_t) hb_vector_draw_line_to, nullptr, nullptr);
    hb_draw_funcs_set_quadratic_to_func (funcs, (hb_draw_quadratic_to_func_t) hb_vector_draw_quadratic_to, nullptr, nullptr);
    hb_draw_funcs_set_cubic_to_func (funcs, (hb_draw_cubic_to_func_t) hb_vector_draw_cubic_to, nullptr, nullptr);
    hb_draw_funcs_set_close_path_func (funcs, (hb_draw_close_path_func_t) hb_vector_draw_close_path, nullptr, nullptr);
    hb_draw_funcs_make_immutable (funcs);
    hb_atexit (free_static_vector_draw_funcs);
    return funcs;
  }
} static_vector_draw_funcs;

static inline void
free_static_vector_draw_funcs ()
{
  static_vector_draw_funcs.free_instance ();
}

static hb_draw_funcs_t *
hb_vector_draw_funcs_get ()
{
  return static_vector_draw_funcs.get_unconst ();
}

/**
 * hb_vector_draw_create_or_fail:
 * @format: output format.
 *
 * Creates a new draw context for vector output.
 *
 * Return value: (nullable): a newly allocated #hb_vector_draw_t, or `NULL` on failure.
 *
 * Since: 13.0.0
 */
hb_vector_draw_t *
hb_vector_draw_create_or_fail (hb_vector_format_t format)
{
  if (format != HB_VECTOR_FORMAT_SVG && format != HB_VECTOR_FORMAT_PDF)
    return nullptr;

  hb_vector_draw_t *draw = hb_object_create<hb_vector_draw_t> ();
  if (unlikely (!draw))
    return nullptr;
  draw->format = format;
  draw->defined_glyphs = hb_set_create ();
  draw->defs.alloc (2048);
  draw->body.alloc (8192);
  draw->path.alloc (2048);
  return draw;
}

/**
 * hb_vector_draw_reference:
 * @draw: a draw context.
 *
 * Increases the reference count of @draw.
 *
 * Return value: (transfer full): referenced @draw.
 *
 * Since: 13.0.0
 */
hb_vector_draw_t *
hb_vector_draw_reference (hb_vector_draw_t *draw)
{
  return hb_object_reference (draw);
}

/**
 * hb_vector_draw_destroy:
 * @draw: a draw context.
 *
 * Decreases the reference count of @draw and destroys it when it reaches zero.
 *
 * Since: 13.0.0
 */
void
hb_vector_draw_destroy (hb_vector_draw_t *draw)
{
  if (!hb_object_should_destroy (draw))
    return;

  hb_blob_destroy (draw->recycled_blob);
  hb_set_destroy (draw->defined_glyphs);
  hb_object_actually_destroy (draw);
  hb_free (draw);
}

/**
 * hb_vector_draw_set_user_data:
 * @draw: a draw context.
 * @key: user-data key.
 * @data: user-data value.
 * @destroy: (nullable): destroy callback for @data.
 * @replace: whether to replace an existing value for @key.
 *
 * Attaches user data to @draw.
 *
 * Return value: `true` on success, `false` otherwise.
 *
 * Since: 13.0.0
 */
hb_bool_t
hb_vector_draw_set_user_data (hb_vector_draw_t   *draw,
                              hb_user_data_key_t *key,
                              void               *data,
                              hb_destroy_func_t   destroy,
                              hb_bool_t           replace)
{
  return hb_object_set_user_data (draw, key, data, destroy, replace);
}

/**
 * hb_vector_draw_get_user_data:
 * @draw: a draw context.
 * @key: user-data key.
 *
 * Gets previously attached user data from @draw.
 *
 * Return value: (nullable): user-data value associated with @key.
 *
 * Since: 13.0.0
 */
void *
hb_vector_draw_get_user_data (hb_vector_draw_t   *draw,
                              hb_user_data_key_t *key)
{
  return hb_object_get_user_data (draw, key);
}

/**
 * hb_vector_draw_set_transform:
 * @draw: a draw context.
 * @xx: transform xx component.
 * @yx: transform yx component.
 * @xy: transform xy component.
 * @yy: transform yy component.
 * @dx: transform x translation.
 * @dy: transform y translation.
 *
 * Sets the affine transform used when drawing glyphs.
 *
 * Since: 13.0.0
 */
void
hb_vector_draw_set_transform (hb_vector_draw_t *draw,
                              float xx, float yx,
                              float xy, float yy,
                              float dx, float dy)
{
  draw->transform = {xx, yx, xy, yy, dx, dy};
}

/**
 * hb_vector_draw_get_transform:
 * @draw: a draw context.
 * @xx: (out) (nullable): transform xx component.
 * @yx: (out) (nullable): transform yx component.
 * @xy: (out) (nullable): transform xy component.
 * @yy: (out) (nullable): transform yy component.
 * @dx: (out) (nullable): transform x translation.
 * @dy: (out) (nullable): transform y translation.
 *
 * Gets the affine transform used when drawing glyphs.
 *
 * Since: 13.0.0
 */
void
hb_vector_draw_get_transform (hb_vector_draw_t *draw,
                              float *xx, float *yx,
                              float *xy, float *yy,
                              float *dx, float *dy)
{
  if (xx) *xx = draw->transform.xx;
  if (yx) *yx = draw->transform.yx;
  if (xy) *xy = draw->transform.xy;
  if (yy) *yy = draw->transform.yy;
  if (dx) *dx = draw->transform.x0;
  if (dy) *dy = draw->transform.y0;
}

/**
 * hb_vector_draw_set_scale_factor:
 * @draw: a draw context.
 * @x_scale_factor: x scale factor.
 * @y_scale_factor: y scale factor.
 *
 * Sets additional output scaling factors.
 *
 * Since: 13.0.0
 */
void
hb_vector_draw_set_scale_factor (hb_vector_draw_t *draw,
                                 float x_scale_factor,
                                 float y_scale_factor)
{
  draw->x_scale_factor = x_scale_factor > 0.f ? x_scale_factor : 1.f;
  draw->y_scale_factor = y_scale_factor > 0.f ? y_scale_factor : 1.f;
}

/**
 * hb_vector_draw_get_scale_factor:
 * @draw: a draw context.
 * @x_scale_factor: (out) (nullable): x scale factor.
 * @y_scale_factor: (out) (nullable): y scale factor.
 *
 * Gets additional output scaling factors.
 *
 * Since: 13.0.0
 */
void
hb_vector_draw_get_scale_factor (hb_vector_draw_t *draw,
                                 float *x_scale_factor,
                                 float *y_scale_factor)
{
  if (x_scale_factor) *x_scale_factor = draw->x_scale_factor;
  if (y_scale_factor) *y_scale_factor = draw->y_scale_factor;
}

/**
 * hb_vector_draw_set_extents:
 * @draw: a draw context.
 * @extents: (nullable): output extents to set or expand.
 *
 * Sets or expands output extents on @draw. Passing `NULL` clears extents.
 *
 * Since: 13.0.0
 */
void
hb_vector_draw_set_extents (hb_vector_draw_t *draw,
                            const hb_vector_extents_t *extents)
{
  if (!extents)
  {
    draw->extents = {0, 0, 0, 0};
    draw->has_extents = false;
    return;
  }

  if (!(extents->width > 0.f && extents->height > 0.f))
    return;

  if (draw->has_extents)
  {
    float x0 = hb_min (draw->extents.x, extents->x);
    float y0 = hb_min (draw->extents.y, extents->y);
    float x1 = hb_max (draw->extents.x + draw->extents.width,
                       extents->x + extents->width);
    float y1 = hb_max (draw->extents.y + draw->extents.height,
                       extents->y + extents->height);
    draw->extents = {x0, y0, x1 - x0, y1 - y0};
  }
  else
  {
    draw->extents = *extents;
    draw->has_extents = true;
  }
}

/**
 * hb_vector_draw_get_extents:
 * @draw: a draw context.
 * @extents: (out) (nullable): where to store current output extents.
 *
 * Gets current output extents from @draw.
 *
 * Return value: `true` if extents are set, `false` otherwise.
 *
 * Since: 13.0.0
 */
hb_bool_t
hb_vector_draw_get_extents (hb_vector_draw_t *draw,
                            hb_vector_extents_t *extents)
{
  if (!draw->has_extents)
    return false;

  if (extents)
    *extents = draw->extents;
  return true;
}

/**
 * hb_vector_draw_set_glyph_extents:
 * @draw: a draw context.
 * @glyph_extents: glyph extents in font units.
 *
 * Expands @draw extents using @glyph_extents under the current transform.
 *
 * Return value: `true` on success, `false` otherwise.
 *
 * Since: 13.0.0
 */
hb_bool_t
hb_vector_draw_set_glyph_extents (hb_vector_draw_t *draw,
                                  const hb_glyph_extents_t *glyph_extents)
{
  hb_bool_t has_extents = draw->has_extents;
  hb_bool_t ret = hb_svg_set_glyph_extents_common (draw->transform,
						   draw->x_scale_factor,
						   draw->y_scale_factor,
						   glyph_extents,
						   &draw->extents,
						   &has_extents);
  draw->has_extents = has_extents;
  return ret;
}

/**
 * hb_vector_draw_get_funcs:
 *
 * Gets draw callbacks implemented by the vector draw backend.
 *
 * Return value: (transfer none): immutable #hb_draw_funcs_t singleton.
 *
 * Since: 13.0.0
 */
hb_draw_funcs_t *
hb_vector_draw_get_funcs (void)
{
  return hb_vector_draw_funcs_get ();
}

/**
 * hb_vector_draw_glyph:
 * @draw: a draw context.
 * @font: font object.
 * @glyph: glyph ID.
 * @pen_x: glyph x origin before context transform.
 * @pen_y: glyph y origin before context transform.
 * @extents_mode: extents update mode.
 *
 * Draws one glyph into @draw.
 *
 * Return value: `true` if glyph data was emitted, `false` otherwise.
 *
 * Since: 13.0.0
 */
hb_bool_t
hb_vector_draw_glyph (hb_vector_draw_t *draw,
                      hb_font_t *font,
                      hb_codepoint_t glyph,
                      float pen_x,
                      float pen_y,
                      hb_vector_extents_mode_t extents_mode)
{
  if (draw->format != HB_VECTOR_FORMAT_SVG &&
      draw->format != HB_VECTOR_FORMAT_PDF)
    return false;

  if (extents_mode == HB_VECTOR_EXTENTS_MODE_EXPAND)
  {
    hb_glyph_extents_t ge;
    if (hb_font_get_glyph_extents (font, glyph, &ge))
    {
      float xx = draw->transform.xx;
      float yx = draw->transform.yx;
      float xy = draw->transform.xy;
      float yy = draw->transform.yy;
      float tx = draw->transform.x0 + xx * pen_x + xy * pen_y;
      float ty = draw->transform.y0 + yx * pen_x + yy * pen_y;
      hb_transform_t<> extents_transform = {xx, yx, -xy, -yy, tx, ty};

      hb_bool_t has_extents = draw->has_extents;
      hb_svg_set_glyph_extents_common (extents_transform,
                                       draw->x_scale_factor,
                                       draw->y_scale_factor,
                                       &ge,
                                       &draw->extents,
                                       &has_extents);
      draw->has_extents = has_extents;
    }
  }

  if (draw->format == HB_VECTOR_FORMAT_PDF)
  {
    /* PDF: always inline.  Pen and font coords are both Y-up. */
    hb_transform_t<> saved = draw->transform;
    draw->transform = {1, 0, 0, 1, pen_x, pen_y};

    draw->path.clear ();
    hb_font_draw_glyph (font, glyph, hb_vector_draw_funcs_get (), draw);
    draw->transform = saved;

    if (!draw->path.length)
      return false;

    hb_buf_append_len (&draw->body, draw->path.arrayZ, draw->path.length);
    hb_buf_append_str (&draw->body, "f\n");
    return true;
  }

  bool needs_def = !draw->flat && !hb_set_has (draw->defined_glyphs, glyph);
  if (needs_def)
  {
    draw->path.clear ();
    hb_svg_path_sink_t sink = {&draw->path, draw->precision};
    hb_font_draw_glyph (font, glyph, hb_svg_path_draw_funcs_get (), &sink);
    if (!draw->path.length)
      return false;
    hb_buf_append_str (&draw->defs, "<path id=\"p");
    hb_buf_append_unsigned (&draw->defs, glyph);
    hb_buf_append_str (&draw->defs, "\" d=\"");
    hb_buf_append_len (&draw->defs, draw->path.arrayZ, draw->path.length);
    hb_buf_append_str (&draw->defs, "\"/>\n");
    hb_set_add (draw->defined_glyphs, glyph);
  }

  if (draw->flat)
  {
    draw->path.clear ();
    hb_svg_path_sink_t sink = {&draw->path, draw->precision};
    hb_font_draw_glyph (font, glyph, hb_svg_path_draw_funcs_get (), &sink);

    if (!draw->path.length)
      return false;

    float xx = draw->transform.xx;
    float yx = draw->transform.yx;
    float xy = draw->transform.xy;
    float yy = draw->transform.yy;
    float tx = draw->transform.x0 + xx * pen_x + xy * pen_y;
    float ty = draw->transform.y0 + yx * pen_x + yy * pen_y;

    hb_buf_append_str (&draw->body, "<path d=\"");
    hb_buf_append_len (&draw->body, draw->path.arrayZ, draw->path.length);
    hb_buf_append_str (&draw->body, "\" transform=\"");
    hb_svg_append_instance_transform (&draw->body,
                                      draw->precision,
                                      draw->x_scale_factor,
                                      draw->y_scale_factor,
                                      xx, yx, xy, yy, tx, ty);
    hb_buf_append_str (&draw->body, "\"/>\n");
    return true;
  }

  float xx = draw->transform.xx;
  float yx = draw->transform.yx;
  float xy = draw->transform.xy;
  float yy = draw->transform.yy;
  float tx = draw->transform.x0 + xx * pen_x + xy * pen_y;
  float ty = draw->transform.y0 + yx * pen_x + yy * pen_y;

  hb_buf_append_str (&draw->body, "<use href=\"#p");
  hb_buf_append_unsigned (&draw->body, glyph);
  hb_buf_append_str (&draw->body, "\" transform=\"");
  hb_svg_append_instance_transform (&draw->body,
                                    draw->precision,
                                    draw->x_scale_factor,
                                    draw->y_scale_factor,
                                    xx, yx, xy, yy, tx, ty);
  hb_buf_append_str (&draw->body, "\"/>\n");
  return true;
}

/**
 * hb_vector_draw_set_flat:
 * @draw: a draw context.
 * @flat: whether to flatten geometry and disable reuse.
 *
 * Enables or disables draw flattening.
 *
 * Since: 13.0.0
 */
void
hb_vector_draw_set_flat (hb_vector_draw_t *draw,
                        hb_bool_t flat)
{
  draw->flat = !!flat;
}

/**
 * hb_vector_draw_set_precision:
 * @draw: a draw context.
 * @precision: decimal precision.
 *
 * Sets numeric output precision for draw output.
 *
 * Since: 13.0.0
 */
void
hb_vector_draw_set_precision (hb_vector_draw_t *draw,
                             unsigned precision)
{
  draw->precision = hb_min (precision, 12u);
}

/**
 * hb_vector_draw_render:
 * @draw: a draw context.
 *
 * Renders accumulated draw content to an SVG blob.
 *
 * Return value: (transfer full) (nullable): output blob, or `NULL` if rendering cannot proceed.
 *
 * Since: 13.0.0
 */
static hb_blob_t *
hb_vector_draw_render_pdf (hb_vector_draw_t *draw)
{
  if (!draw->has_extents)
    return nullptr;

  /* Collect the content stream.  The path coordinates are in
   * SVG space (Y-down).  Prepend a CTM that flips Y so the
   * PDF page (Y-up) renders correctly. */
  float ex = draw->extents.x;
  float ey = draw->extents.y;
  float ew = draw->extents.width;
  float eh = draw->extents.height;

  hb_vector_t<char> stream;
  stream.alloc (draw->body.length + draw->path.length + 128);

  /* Path coords are in font space (Y-up); no CTM needed. */

  if (draw->body.length)
    hb_buf_append_len (&stream, draw->body.arrayZ, draw->body.length);
  else if (draw->path.length)
  {
    hb_buf_append_len (&stream, draw->path.arrayZ, draw->path.length);
    hb_buf_append_str (&stream, "f\n");
  }

  /* Build PDF objects, tracking byte offsets for xref. */
  hb_vector_t<char> out;
  hb_buf_recover_recycled (draw->recycled_blob, &out);
  out.alloc (stream.length + 512);

  unsigned offsets[5]; /* objects 1-4, plus end */

  hb_buf_append_str (&out, "%PDF-1.4\n%\xC0\xC1\xC2\xC3\n");

  /* Object 1: Catalog */
  offsets[0] = out.length;
  hb_buf_append_str (&out, "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n");

  /* Object 2: Pages */
  offsets[1] = out.length;
  hb_buf_append_str (&out, "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n");

  /* Object 3: Page.  Extents are in SVG space (y = -font_y).
   * Convert back: font Y range = [-(ey+eh) .. -ey]. */
  offsets[2] = out.length;
  hb_buf_append_str (&out, "3 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [");
  hb_buf_append_num (&out, ex, draw->precision);
  hb_buf_append_c (&out, ' ');
  hb_buf_append_num (&out, -(ey + eh), draw->precision);
  hb_buf_append_c (&out, ' ');
  hb_buf_append_num (&out, ex + ew, draw->precision);
  hb_buf_append_c (&out, ' ');
  hb_buf_append_num (&out, -ey, draw->precision);
  hb_buf_append_str (&out, "] /Contents 4 0 R >>\nendobj\n");

  /* Object 4: Content stream */
  offsets[3] = out.length;
  hb_buf_append_str (&out, "4 0 obj\n<< /Length ");
  hb_buf_append_unsigned (&out, stream.length);
  hb_buf_append_str (&out, " >>\nstream\n");
  hb_buf_append_len (&out, stream.arrayZ, stream.length);
  hb_buf_append_str (&out, "endstream\nendobj\n");

  /* Cross-reference table */
  unsigned xref_offset = out.length;
  hb_buf_append_str (&out, "xref\n0 5\n");
  hb_buf_append_str (&out, "0000000000 65535 f \n");
  for (unsigned i = 0; i < 4; i++)
  {
    char tmp[21];
    snprintf (tmp, sizeof (tmp), "%010u 00000 n \n", offsets[i]);
    hb_buf_append_len (&out, tmp, 20);
  }

  /* Trailer */
  hb_buf_append_str (&out, "trailer\n<< /Size 5 /Root 1 0 R >>\nstartxref\n");
  hb_buf_append_unsigned (&out, xref_offset);
  hb_buf_append_str (&out, "\n%%EOF\n");

  hb_blob_t *blob = hb_buf_blob_from (&draw->recycled_blob, &out);

  draw->path.clear ();
  draw->defs.clear ();
  draw->body.clear ();
  hb_set_clear (draw->defined_glyphs);
  draw->has_extents = false;
  draw->extents = {0, 0, 0, 0};

  return blob;
}

hb_blob_t *
hb_vector_draw_render (hb_vector_draw_t *draw)
{
  if (draw->format == HB_VECTOR_FORMAT_PDF)
    return hb_vector_draw_render_pdf (draw);

  if (draw->format != HB_VECTOR_FORMAT_SVG)
    return nullptr;
  if (!draw->has_extents)
    return nullptr;

  hb_vector_t<char> out;
  hb_buf_recover_recycled (draw->recycled_blob, &out);
  unsigned estimated = draw->defs.length +
                       (draw->body.length ? draw->body.length : draw->path.length) +
                       256;
  out.alloc (estimated);
  hb_buf_append_str (&out, "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"");
  hb_buf_append_num (&out, draw->extents.x, draw->precision);
  hb_buf_append_c (&out, ' ');
  hb_buf_append_num (&out, draw->extents.y, draw->precision);
  hb_buf_append_c (&out, ' ');
  hb_buf_append_num (&out, draw->extents.width, draw->precision);
  hb_buf_append_c (&out, ' ');
  hb_buf_append_num (&out, draw->extents.height, draw->precision);
  hb_buf_append_str (&out, "\" width=\"");
  hb_buf_append_num (&out, draw->extents.width, draw->precision);
  hb_buf_append_str (&out, "\" height=\"");
  hb_buf_append_num (&out, draw->extents.height, draw->precision);
  hb_buf_append_str (&out, "\">\n");

  if (draw->defs.length)
  {
    hb_buf_append_str (&out, "<defs>\n");
    hb_buf_append_len (&out, draw->defs.arrayZ, draw->defs.length);
    hb_buf_append_str (&out, "</defs>\n");
  }

  if (draw->body.length)
  {
    hb_buf_append_len (&out, draw->body.arrayZ, draw->body.length);
  }
  else if (draw->path.length)
  {
    hb_buf_append_str (&out, "<path d=\"");
    hb_buf_append_len (&out, draw->path.arrayZ, draw->path.length);
    hb_buf_append_str (&out, "\"/>\n");
  }

  hb_buf_append_str (&out, "</svg>\n");

  hb_blob_t *blob = hb_buf_blob_from (&draw->recycled_blob, &out);

  draw->path.clear ();
  draw->defs.clear ();
  draw->body.clear ();
  hb_set_clear (draw->defined_glyphs);
  draw->has_extents = false;
  draw->extents = {0, 0, 0, 0};

  return blob;
}

/**
 * hb_vector_draw_reset:
 * @draw: a draw context.
 *
 * Resets @draw state and clears accumulated content.
 *
 * Since: 13.0.0
 */
void
hb_vector_draw_reset (hb_vector_draw_t *draw)
{
  draw->transform = {1, 0, 0, 1, 0, 0};
  draw->x_scale_factor = 1.f;
  draw->y_scale_factor = 1.f;
  draw->extents = {0, 0, 0, 0};
  draw->has_extents = false;
  draw->precision = 2;
  draw->flat = false;
  draw->defs.clear ();
  draw->body.clear ();
  draw->path.clear ();
  hb_set_clear (draw->defined_glyphs);
}

/**
 * hb_vector_draw_recycle_blob:
 * @draw: a draw context.
 * @blob: (nullable): previously rendered blob to recycle.
 *
 * Provides a blob for internal buffer reuse by later render calls.
 *
 * Since: 13.0.0
 */
void
hb_vector_draw_recycle_blob (hb_vector_draw_t *draw,
                             hb_blob_t *blob)
{
  hb_blob_destroy (draw->recycled_blob);
  draw->recycled_blob = nullptr;
  if (!blob || blob == hb_blob_get_empty ())
    return;
  draw->recycled_blob = blob;
}
