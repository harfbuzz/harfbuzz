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

#include "hb-vector.h"
#include "hb-blob.hh"
#include "hb-geometry.hh"
#include "hb-map.hh"
#include "hb-vector-svg-subset.hh"

#include <algorithm>
#include <math.h>
#include <string.h>


static bool
hb_svg_append_len (hb_vector_t<char> *buf,
                   const char *s,
                   unsigned len)
{
  unsigned old_len = buf->length;
  if (unlikely (!buf->resize_dirty ((int) (old_len + len))))
    return false;
  hb_memcpy (buf->arrayZ + old_len, s, len);
  return true;
}

static bool
hb_svg_append_c (hb_vector_t<char> *buf, char c)
{
  return !!buf->push (c);
}

static bool
hb_svg_append_str (hb_vector_t<char> *buf, const char *s)
{
  return hb_svg_append_len (buf, s, (unsigned) strlen (s));
}

static bool
hb_svg_append_unsigned (hb_vector_t<char> *buf, unsigned v)
{
  char tmp[10];
  unsigned n = 0;
  do {
    tmp[n++] = (char) ('0' + (v % 10));
    v /= 10;
  } while (v);

  unsigned old_len = buf->length;
  if (unlikely (!buf->resize_dirty ((int) (old_len + n))))
    return false;

  for (unsigned i = 0; i < n; i++)
    buf->arrayZ[old_len + i] = tmp[n - 1 - i];
  return true;
}

static bool
hb_svg_append_hex_byte (hb_vector_t<char> *buf, unsigned v)
{
  static const char hex[] = "0123456789ABCDEF";
  char tmp[2] = {hex[(v >> 4) & 15], hex[v & 15]};
  return hb_svg_append_len (buf, tmp, 2);
}

static void
hb_svg_append_num (hb_vector_t<char> *buf,
                   float v,
                   unsigned precision,
                   bool keep_nonzero = false)
{
  unsigned effective_precision = precision;
  if (keep_nonzero && v != 0.f)
    while (effective_precision < 12)
    {
      float rounded_zero_threshold = 0.5f;
      for (unsigned i = 0; i < effective_precision; i++)
        rounded_zero_threshold *= 0.1f;
      if (fabsf (v) >= rounded_zero_threshold)
        break;
      effective_precision++;
    }

  float rounded_zero_threshold = 0.5f;
  for (unsigned i = 0; i < effective_precision; i++)
    rounded_zero_threshold *= 0.1f;
  if (fabsf (v) < rounded_zero_threshold)
    v = 0.f;

  if (!(v == v) || !isfinite (v))
  {
    hb_svg_append_c (buf, '0');
    return;
  }

  char fmt[20];
  snprintf (fmt, sizeof (fmt), "%%.%uf", effective_precision);
  char out[128];
  snprintf (out, sizeof (out), fmt, (double) v);

  const char *decimal_point = ".";
#ifndef HB_NO_SETLOCALE
#if defined(HAVE_XLOCALE_H)
  lconv *lc = nullptr;
  hb_locale_t current_locale = hb_uselocale ((hb_locale_t) 0);
  if (current_locale)
    lc = localeconv_l (current_locale);
  if (lc && lc->decimal_point && lc->decimal_point[0])
    decimal_point = lc->decimal_point;
#endif
#endif

  if (decimal_point[0] != '.' || decimal_point[1] != '\0')
  {
    char *p = strstr (out, decimal_point);
    if (p)
    {
      unsigned dp_len = (unsigned) strlen (decimal_point);
      unsigned tail_len = (unsigned) strlen (p + dp_len);
      memmove (p + 1, p + dp_len, tail_len + 1);
      *p = '.';
    }
  }

  char *dot = strchr (out, '.');
  if (dot)
  {
    char *end = out + strlen (out) - 1;
    while (end > dot && *end == '0')
      *end-- = '\0';
    if (end == dot)
      *end = '\0';
  }

  hb_svg_append_str (buf, out);
}

static inline unsigned
hb_svg_scale_precision (unsigned precision)
{
  return precision < 7 ? 7 : precision;
}

static inline bool
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

struct hb_svg_blob_meta_t
{
  char *data;
  int allocated;
  bool transferred;
  bool in_replace;
};

static hb_user_data_key_t hb_svg_blob_meta_user_data_key;

static void
hb_svg_blob_meta_set_buffer (hb_svg_blob_meta_t *meta,
			     char *data,
			     int allocated)
{
  meta->data = data;
  meta->allocated = allocated;
  meta->transferred = false;
}

static void
hb_svg_blob_meta_release_buffer (hb_svg_blob_meta_t *meta)
{
  if (!meta)
    return;
  if (!meta->transferred && meta->data)
    hb_free (meta->data);
  meta->data = nullptr;
  meta->allocated = 0;
  meta->transferred = true;
}

static void
hb_svg_blob_meta_destroy (void *data)
{
  auto *meta = (hb_svg_blob_meta_t *) data;
  hb_svg_blob_meta_release_buffer (meta);
  if (meta->in_replace)
  {
    meta->in_replace = false;
    return;
  }
  hb_free (meta);
}

static hb_blob_t *
hb_svg_blob_from_buffer (hb_blob_t **recycled_blob,
			 hb_vector_t<char> *buf)
{
  unsigned len = 0;
  int allocated = 0;
  char *data = buf->steal (&len, &allocated);
  if (!data)
    return nullptr;

  hb_blob_t *blob = nullptr;
  if (*recycled_blob)
    blob = *recycled_blob;
  bool reused_blob = blob && blob != hb_blob_get_empty ();
  bool new_meta = false;
  auto *meta = reused_blob
             ? (hb_svg_blob_meta_t *) hb_blob_get_user_data (blob, &hb_svg_blob_meta_user_data_key)
             : nullptr;
  if (!meta)
  {
    meta = (hb_svg_blob_meta_t *) hb_malloc (sizeof (hb_svg_blob_meta_t));
    if (!meta)
    {
      hb_free (data);
      return nullptr;
    }
    meta->data = nullptr;
    meta->allocated = 0;
    meta->transferred = true;
    meta->in_replace = false;
    new_meta = true;
  }

  if (reused_blob)
  {
    /* replace_buffer() first destroys old buffer user_data; keep meta alive. */
    meta->in_replace = true;
    blob->replace_buffer (data, len, HB_MEMORY_MODE_WRITABLE, meta, hb_svg_blob_meta_destroy);
    hb_svg_blob_meta_set_buffer (meta, data, allocated);
  }
  else
  {
    hb_svg_blob_meta_set_buffer (meta, data, allocated);
    blob = hb_blob_create (data, len, HB_MEMORY_MODE_WRITABLE, meta, hb_svg_blob_meta_destroy);
  }

  if (unlikely (blob == hb_blob_get_empty ()))
  {
    if (new_meta)
      hb_free (meta);
    hb_free (data);
    return nullptr;
  }

  if (new_meta &&
      !hb_blob_set_user_data (blob,
			      &hb_svg_blob_meta_user_data_key,
			      meta,
			      nullptr,
			      true))
  {
    if (!reused_blob)
      hb_blob_destroy (blob);
    return nullptr;
  }

  if (*recycled_blob)
    *recycled_blob = nullptr;

  return blob;
}

static void
hb_svg_recover_recycled_buffer (hb_blob_t *blob,
				hb_vector_t<char> *buf)
{
  if (!blob)
    return;

  auto *meta = (hb_svg_blob_meta_t *) hb_blob_get_user_data (blob, &hb_svg_blob_meta_user_data_key);
  if (!meta || meta->transferred || !meta->data)
    return;

  buf->recycle_buffer (meta->data, 0, meta->allocated);
  meta->data = nullptr;
  meta->allocated = 0;
  meta->transferred = true;
}

static void
hb_svg_append_color (hb_vector_t<char> *buf,
                     hb_color_t color,
                     bool with_alpha)
{
  unsigned r = hb_color_get_red (color);
  unsigned g = hb_color_get_green (color);
  unsigned b = hb_color_get_blue (color);
  unsigned a = hb_color_get_alpha (color);
  hb_svg_append_c (buf, '#');
  hb_svg_append_hex_byte (buf, r);
  hb_svg_append_hex_byte (buf, g);
  hb_svg_append_hex_byte (buf, b);
  if (with_alpha && a != 255)
  {
    hb_svg_append_str (buf, "\" fill-opacity=\"");
    hb_svg_append_num (buf, a / 255.f, 4);
  }
}

static void
hb_svg_transform_point (const hb_transform_t<> &t,
                        float x_scale_factor,
                        float y_scale_factor,
                        float x, float y,
                        float *tx, float *ty)
{
  float xx = x, yy = y;
  t.transform_point (xx, yy);
  *tx = xx / (x_scale_factor > 0 ? x_scale_factor : 1.f);
  *ty = yy / (y_scale_factor > 0 ? y_scale_factor : 1.f);
}

static hb_bool_t
hb_svg_set_glyph_extents_common (const hb_transform_t<> &transform,
                                 float x_scale_factor,
                                 float y_scale_factor,
                                 const hb_glyph_extents_t *glyph_extents,
                                 hb_vector_extents_t *extents,
                                 hb_bool_t *has_extents)
{
  float x0 = (float) glyph_extents->x_bearing;
  float y0 = (float) glyph_extents->y_bearing;
  float x1 = x0 + glyph_extents->width;
  float y1 = y0 + glyph_extents->height;

  float px[4] = {x0, x0, x1, x1};
  float py[4] = {y0, y1, y0, y1};

  float tx, ty;
  hb_svg_transform_point (transform, x_scale_factor, y_scale_factor, px[0], py[0], &tx, &ty);
  float tx_min = tx, tx_max = tx;
  float ty_min = ty, ty_max = ty;

  for (unsigned i = 1; i < 4; i++)
  {
    hb_svg_transform_point (transform, x_scale_factor, y_scale_factor, px[i], py[i], &tx, &ty);
    tx_min = hb_min (tx_min, tx);
    tx_max = hb_max (tx_max, tx);
    ty_min = hb_min (ty_min, ty);
    ty_max = hb_max (ty_max, ty);
  }

  if (tx_max <= tx_min || ty_max <= ty_min)
  {
    return false;
  }

  if (*has_extents)
  {
    float x0 = hb_min (extents->x, tx_min);
    float y0 = hb_min (extents->y, ty_min);
    float x1 = hb_max (extents->x + extents->width, tx_max);
    float y1 = hb_max (extents->y + extents->height, ty_max);
    *extents = {x0, y0, x1 - x0, y1 - y0};
  }
  else
  {
    *extents = {tx_min, ty_min, tx_max - tx_min, ty_max - ty_min};
    *has_extents = true;
  }
  return true;
}


struct hb_vector_draw_t
{
  hb_object_header_t header;

  hb_vector_format_t format = HB_VECTOR_FORMAT_SVG;
  hb_transform_t<> transform = {1, 0, 0, 1, 0, 0};
  float x_scale_factor = 1.f;
  float y_scale_factor = 1.f;
  hb_vector_extents_t extents = {0, 0, 0, 0};
  bool has_extents = false;
  unsigned precision = 2;
  bool flat = false;

  hb_vector_t<char> defs;
  hb_vector_t<char> body;
  hb_vector_t<char> path;
  hb_set_t *defined_glyphs = nullptr;
  hb_blob_t *recycled_blob = nullptr;

  void append_xy (float x, float y)
  {
    float tx, ty;
    hb_svg_transform_point (transform, x_scale_factor, y_scale_factor, x, y, &tx, &ty);
    hb_svg_append_num (&path, tx, precision);
    hb_svg_append_c (&path, ',');
    hb_svg_append_num (&path, ty, precision);
  }
};

struct hb_vector_paint_t
{
  hb_object_header_t header;

  hb_vector_format_t format = HB_VECTOR_FORMAT_SVG;
  hb_transform_t<> transform = {1, 0, 0, 1, 0, 0};
  float x_scale_factor = 1.f;
  float y_scale_factor = 1.f;
  hb_vector_extents_t extents = {0, 0, 0, 0};
  bool has_extents = false;

  hb_color_t foreground = HB_COLOR (0, 0, 0, 255);
  int palette = 0;
  unsigned precision = 2;
  bool flat = false;

  hb_vector_t<char> defs;
  hb_vector_t<char> body;
  hb_vector_t<char> path;
  hb_vector_t<hb_vector_t<char>> group_stack;
  uint64_t transform_group_open_mask = 0;
  unsigned transform_group_depth = 0;
  unsigned transform_group_overflow_depth = 0;

  unsigned clip_rect_counter = 0;
  unsigned gradient_counter = 0;
  unsigned color_glyph_counter = 0;
  hb_set_t *defined_outlines = nullptr;
  hb_set_t *defined_clips = nullptr;
  hb_hashmap_t<uint64_t, uint64_t> defined_color_glyphs;
  hb_vector_t<hb_color_stop_t> color_stops_scratch;
  hb_blob_t *recycled_blob = nullptr;
  bool current_color_glyph_has_svg_image = false;
  hb_codepoint_t current_svg_image_glyph = HB_CODEPOINT_INVALID;
  unsigned svg_image_counter = 0;

  hb_vector_t<char> &current_body () { return group_stack.tail (); }

  void append_global_transform_prefix (hb_vector_t<char> *buf)
  {
    if (transform.xx == 1.f && transform.yx == 0.f &&
        transform.xy == 0.f && transform.yy == 1.f &&
        transform.x0 == 0.f && transform.y0 == 0.f &&
        x_scale_factor == 1.f && y_scale_factor == 1.f)
      return;

    unsigned sprec = hb_svg_scale_precision (precision);
    hb_svg_append_str (buf, "<g transform=\"matrix(");
    hb_svg_append_num (buf, transform.xx / x_scale_factor, sprec, true);
    hb_svg_append_c (buf, ',');
    hb_svg_append_num (buf, transform.yx / y_scale_factor, sprec, true);
    hb_svg_append_c (buf, ',');
    hb_svg_append_num (buf, transform.xy / x_scale_factor, sprec, true);
    hb_svg_append_c (buf, ',');
    hb_svg_append_num (buf, transform.yy / y_scale_factor, sprec, true);
    hb_svg_append_c (buf, ',');
    hb_svg_append_num (buf, transform.x0 / x_scale_factor, precision);
    hb_svg_append_c (buf, ',');
    hb_svg_append_num (buf, transform.y0 / y_scale_factor, precision);
    hb_svg_append_str (buf, ")\">\n");
  }

  void append_global_transform_suffix (hb_vector_t<char> *buf)
  {
    if (transform.xx == 1.f && transform.yx == 0.f &&
        transform.xy == 0.f && transform.yy == 1.f &&
        transform.x0 == 0.f && transform.y0 == 0.f &&
        x_scale_factor == 1.f && y_scale_factor == 1.f)
      return;
    hb_svg_append_str (buf, "</g>\n");
  }
};

static inline uint64_t
hb_svg_pack_color_glyph_cache_entry (unsigned def_id,
                                     bool image_like)
{
  return ((uint64_t) def_id << 1) | (image_like ? 1ull : 0ull);
}

static inline unsigned
hb_svg_cache_entry_def_id (uint64_t v)
{
  return (unsigned) (v >> 1);
}

static inline bool
hb_svg_cache_entry_image_like (uint64_t v)
{
  return !!(v & 1ull);
}

static inline uint64_t
hb_svg_color_glyph_cache_key (hb_codepoint_t glyph,
                              unsigned palette,
                              hb_color_t foreground)
{
  uint64_t lo = ((uint64_t) palette << 32) | (uint64_t) foreground;
  return ((uint64_t) glyph << 32) | (uint64_t) hb_hash (lo);
}

static inline void
hb_svg_append_instance_transform (hb_vector_t<char> *out,
                                  unsigned precision,
                                  float x_scale_factor,
                                  float y_scale_factor,
                                  float xx, float yx,
                                  float xy, float yy,
                                  float tx, float ty)
{
  unsigned sprec = hb_svg_scale_precision (precision);
  if (xx == 1.f && yx == 0.f && xy == 0.f && yy == 1.f)
  {
    float sx = 1.f / x_scale_factor;
    float sy = 1.f / y_scale_factor;
    hb_svg_append_str (out, "translate(");
    hb_svg_append_num (out, tx / x_scale_factor, precision);
    hb_svg_append_c (out, ',');
    hb_svg_append_num (out, ty / y_scale_factor, precision);
    hb_svg_append_str (out, ") scale(");
    hb_svg_append_num (out, sx, sprec, true);
    hb_svg_append_c (out, ',');
    hb_svg_append_num (out, -sy, sprec, true);
    hb_svg_append_c (out, ')');
  }
  else
  {
    hb_svg_append_str (out, "matrix(");
    hb_svg_append_num (out, xx / x_scale_factor, sprec, true);
    hb_svg_append_c (out, ',');
    hb_svg_append_num (out, yx / y_scale_factor, sprec, true);
    hb_svg_append_c (out, ',');
    hb_svg_append_num (out, -xy / x_scale_factor, sprec, true);
    hb_svg_append_c (out, ',');
    hb_svg_append_num (out, -yy / y_scale_factor, sprec, true);
    hb_svg_append_c (out, ',');
    hb_svg_append_num (out, tx / x_scale_factor, precision);
    hb_svg_append_c (out, ',');
    hb_svg_append_num (out, ty / y_scale_factor, precision);
    hb_svg_append_c (out, ')');
  }
}

static inline void
hb_svg_append_image_instance_translate (hb_vector_t<char> *out,
                                        unsigned precision,
                                        float x_scale_factor,
                                        float y_scale_factor,
                                        float tx, float ty)
{
  hb_svg_append_str (out, "translate(");
  hb_svg_append_num (out, tx / x_scale_factor, precision);
  hb_svg_append_c (out, ',');
  hb_svg_append_num (out, ty / y_scale_factor, precision);
  hb_svg_append_c (out, ')');
}


static void
hb_vector_draw_move_to (hb_draw_funcs_t *,
                        void *draw_data,
                        hb_draw_state_t *,
                        float to_x, float to_y,
                        void *)
{
  auto *d = (hb_vector_draw_t *) draw_data;
  hb_svg_append_c (&d->path, 'M');
  d->append_xy (to_x, to_y);
}

static void
hb_vector_draw_line_to (hb_draw_funcs_t *,
                        void *draw_data,
                        hb_draw_state_t *,
                        float to_x, float to_y,
                        void *)
{
  auto *d = (hb_vector_draw_t *) draw_data;
  hb_svg_append_c (&d->path, 'L');
  d->append_xy (to_x, to_y);
}

static void
hb_vector_draw_quadratic_to (hb_draw_funcs_t *,
                             void *draw_data,
                             hb_draw_state_t *,
                             float cx, float cy,
                             float to_x, float to_y,
                             void *)
{
  auto *d = (hb_vector_draw_t *) draw_data;
  hb_svg_append_c (&d->path, 'Q');
  d->append_xy (cx, cy);
  hb_svg_append_c (&d->path, ' ');
  d->append_xy (to_x, to_y);
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
  hb_svg_append_c (&d->path, 'C');
  d->append_xy (c1x, c1y);
  hb_svg_append_c (&d->path, ' ');
  d->append_xy (c2x, c2y);
  hb_svg_append_c (&d->path, ' ');
  d->append_xy (to_x, to_y);
}

static void
hb_vector_draw_close_path (hb_draw_funcs_t *,
                           void *draw_data,
                           hb_draw_state_t *,
                           void *)
{
  auto *d = (hb_vector_draw_t *) draw_data;
  hb_svg_append_c (&d->path, 'Z');
}


struct hb_svg_path_sink_t
{
  hb_vector_t<char> *path;
  unsigned precision;
};

static void
hb_svg_path_move_to (hb_draw_funcs_t *,
                     void *draw_data,
                     hb_draw_state_t *,
                     float to_x, float to_y,
                     void *)
{
  auto *s = (hb_svg_path_sink_t *) draw_data;
  hb_svg_append_c (s->path, 'M');
  hb_svg_append_num (s->path, to_x, s->precision);
  hb_svg_append_c (s->path, ',');
  hb_svg_append_num (s->path, to_y, s->precision);
}

static void
hb_svg_path_line_to (hb_draw_funcs_t *,
                     void *draw_data,
                     hb_draw_state_t *,
                     float to_x, float to_y,
                     void *)
{
  auto *s = (hb_svg_path_sink_t *) draw_data;
  hb_svg_append_c (s->path, 'L');
  hb_svg_append_num (s->path, to_x, s->precision);
  hb_svg_append_c (s->path, ',');
  hb_svg_append_num (s->path, to_y, s->precision);
}

static void
hb_svg_path_quadratic_to (hb_draw_funcs_t *,
                          void *draw_data,
                          hb_draw_state_t *,
                          float cx, float cy,
                          float to_x, float to_y,
                          void *)
{
  auto *s = (hb_svg_path_sink_t *) draw_data;
  hb_svg_append_c (s->path, 'Q');
  hb_svg_append_num (s->path, cx, s->precision);
  hb_svg_append_c (s->path, ',');
  hb_svg_append_num (s->path, cy, s->precision);
  hb_svg_append_c (s->path, ' ');
  hb_svg_append_num (s->path, to_x, s->precision);
  hb_svg_append_c (s->path, ',');
  hb_svg_append_num (s->path, to_y, s->precision);
}

static void
hb_svg_path_cubic_to (hb_draw_funcs_t *,
                      void *draw_data,
                      hb_draw_state_t *,
                      float c1x, float c1y,
                      float c2x, float c2y,
                      float to_x, float to_y,
                      void *)
{
  auto *s = (hb_svg_path_sink_t *) draw_data;
  hb_svg_append_c (s->path, 'C');
  hb_svg_append_num (s->path, c1x, s->precision);
  hb_svg_append_c (s->path, ',');
  hb_svg_append_num (s->path, c1y, s->precision);
  hb_svg_append_c (s->path, ' ');
  hb_svg_append_num (s->path, c2x, s->precision);
  hb_svg_append_c (s->path, ',');
  hb_svg_append_num (s->path, c2y, s->precision);
  hb_svg_append_c (s->path, ' ');
  hb_svg_append_num (s->path, to_x, s->precision);
  hb_svg_append_c (s->path, ',');
  hb_svg_append_num (s->path, to_y, s->precision);
}

static void
hb_svg_path_close_path (hb_draw_funcs_t *,
                        void *draw_data,
                        hb_draw_state_t *,
                        void *)
{
  auto *s = (hb_svg_path_sink_t *) draw_data;
  hb_svg_append_c (s->path, 'Z');
}

static hb_draw_funcs_t *
hb_vector_draw_funcs_singleton ()
{
  static hb_draw_funcs_t *funcs = nullptr;
  if (likely (funcs))
    return funcs;

  funcs = hb_draw_funcs_create ();
  hb_draw_funcs_set_move_to_func (funcs, (hb_draw_move_to_func_t) hb_vector_draw_move_to, nullptr, nullptr);
  hb_draw_funcs_set_line_to_func (funcs, (hb_draw_line_to_func_t) hb_vector_draw_line_to, nullptr, nullptr);
  hb_draw_funcs_set_quadratic_to_func (funcs, (hb_draw_quadratic_to_func_t) hb_vector_draw_quadratic_to, nullptr, nullptr);
  hb_draw_funcs_set_cubic_to_func (funcs, (hb_draw_cubic_to_func_t) hb_vector_draw_cubic_to, nullptr, nullptr);
  hb_draw_funcs_set_close_path_func (funcs, (hb_draw_close_path_func_t) hb_vector_draw_close_path, nullptr, nullptr);
  hb_draw_funcs_make_immutable (funcs);
  return funcs;
}

static hb_draw_funcs_t *
hb_svg_path_draw_funcs_singleton ()
{
  static hb_draw_funcs_t *funcs = nullptr;
  if (likely (funcs))
    return funcs;

  funcs = hb_draw_funcs_create ();
  hb_draw_funcs_set_move_to_func (funcs, (hb_draw_move_to_func_t) hb_svg_path_move_to, nullptr, nullptr);
  hb_draw_funcs_set_line_to_func (funcs, (hb_draw_line_to_func_t) hb_svg_path_line_to, nullptr, nullptr);
  hb_draw_funcs_set_quadratic_to_func (funcs, (hb_draw_quadratic_to_func_t) hb_svg_path_quadratic_to, nullptr, nullptr);
  hb_draw_funcs_set_cubic_to_func (funcs, (hb_draw_cubic_to_func_t) hb_svg_path_cubic_to, nullptr, nullptr);
  hb_draw_funcs_set_close_path_func (funcs, (hb_draw_close_path_func_t) hb_svg_path_close_path, nullptr, nullptr);
  hb_draw_funcs_make_immutable (funcs);
  return funcs;
}


static hb_bool_t
hb_svg_get_color_stops (hb_vector_paint_t *paint,
                        hb_color_line_t *color_line,
                        hb_vector_t<hb_color_stop_t> *stops)
{
  unsigned len = hb_color_line_get_color_stops (color_line, 0, nullptr, nullptr);
  if (unlikely (!stops->resize (len)))
    return false;
  hb_color_line_get_color_stops (color_line, 0, &len, stops->arrayZ);

  for (unsigned i = 0; i < len; i++)
    if (stops->arrayZ[i].is_foreground)
      stops->arrayZ[i].color = HB_COLOR (hb_color_get_blue (paint->foreground),
                                         hb_color_get_green (paint->foreground),
                                         hb_color_get_red (paint->foreground),
                                         (unsigned) hb_color_get_alpha (stops->arrayZ[i].color) *
                                         hb_color_get_alpha (paint->foreground) / 255);
  return true;
}

static const char *
hb_svg_extend_mode_str (hb_paint_extend_t ext)
{
  switch (ext)
  {
    case HB_PAINT_EXTEND_PAD: return "pad";
    case HB_PAINT_EXTEND_REPEAT: return "repeat";
    case HB_PAINT_EXTEND_REFLECT: return "reflect";
    default: return "pad";
  }
}

static int
hb_svg_color_stop_cmp (const void *a, const void *b)
{
  const hb_color_stop_t *x = (const hb_color_stop_t *) a;
  const hb_color_stop_t *y = (const hb_color_stop_t *) b;
  if (x->offset < y->offset) return -1;
  if (x->offset > y->offset) return 1;
  return 0;
}

static void
hb_svg_emit_color_stops (hb_vector_paint_t *paint,
                         hb_vector_t<char> *buf,
                         hb_vector_t<hb_color_stop_t> *stops)
{
  for (unsigned i = 0; i < stops->length; i++)
  {
    hb_color_t c = stops->arrayZ[i].color;
    hb_svg_append_str (buf, "<stop offset=\"");
    hb_svg_append_num (buf, stops->arrayZ[i].offset, 4);
    hb_svg_append_str (buf, "\" stop-color=\"rgb(");
    hb_svg_append_unsigned (buf, hb_color_get_red (c));
    hb_svg_append_c (buf, ',');
    hb_svg_append_unsigned (buf, hb_color_get_green (c));
    hb_svg_append_c (buf, ',');
    hb_svg_append_unsigned (buf, hb_color_get_blue (c));
    hb_svg_append_str (buf, ")\"");
    if (hb_color_get_alpha (c) != 255)
    {
      hb_svg_append_str (buf, " stop-opacity=\"");
      hb_svg_append_num (buf, hb_color_get_alpha (c) / 255.f, 4);
      hb_svg_append_c (buf, '"');
    }
    hb_svg_append_str (buf, "/>\n");
  }
}

static const char *
hb_svg_composite_mode_str (hb_paint_composite_mode_t mode)
{
  switch (mode)
  {
    case HB_PAINT_COMPOSITE_MODE_CLEAR:
    case HB_PAINT_COMPOSITE_MODE_SRC:
    case HB_PAINT_COMPOSITE_MODE_DEST:
    case HB_PAINT_COMPOSITE_MODE_DEST_OVER:
    case HB_PAINT_COMPOSITE_MODE_SRC_IN:
    case HB_PAINT_COMPOSITE_MODE_DEST_IN:
    case HB_PAINT_COMPOSITE_MODE_SRC_OUT:
    case HB_PAINT_COMPOSITE_MODE_DEST_OUT:
    case HB_PAINT_COMPOSITE_MODE_SRC_ATOP:
    case HB_PAINT_COMPOSITE_MODE_DEST_ATOP:
    case HB_PAINT_COMPOSITE_MODE_XOR:
    case HB_PAINT_COMPOSITE_MODE_PLUS:
      return nullptr;
    case HB_PAINT_COMPOSITE_MODE_SRC_OVER: return "normal";
    case HB_PAINT_COMPOSITE_MODE_SCREEN: return "screen";
    case HB_PAINT_COMPOSITE_MODE_OVERLAY: return "overlay";
    case HB_PAINT_COMPOSITE_MODE_DARKEN: return "darken";
    case HB_PAINT_COMPOSITE_MODE_LIGHTEN: return "lighten";
    case HB_PAINT_COMPOSITE_MODE_COLOR_DODGE: return "color-dodge";
    case HB_PAINT_COMPOSITE_MODE_COLOR_BURN: return "color-burn";
    case HB_PAINT_COMPOSITE_MODE_HARD_LIGHT: return "hard-light";
    case HB_PAINT_COMPOSITE_MODE_SOFT_LIGHT: return "soft-light";
    case HB_PAINT_COMPOSITE_MODE_DIFFERENCE: return "difference";
    case HB_PAINT_COMPOSITE_MODE_EXCLUSION: return "exclusion";
    case HB_PAINT_COMPOSITE_MODE_MULTIPLY: return "multiply";
    case HB_PAINT_COMPOSITE_MODE_HSL_HUE: return "hue";
    case HB_PAINT_COMPOSITE_MODE_HSL_SATURATION: return "saturation";
    case HB_PAINT_COMPOSITE_MODE_HSL_COLOR: return "color";
    case HB_PAINT_COMPOSITE_MODE_HSL_LUMINOSITY: return "luminosity";
    default: return nullptr;
  }
}


static void hb_vector_paint_push_transform (hb_paint_funcs_t *, void *,
                                            float, float, float, float, float, float,
                                            void *);
static void hb_vector_paint_pop_transform (hb_paint_funcs_t *, void *, void *);
static void hb_vector_paint_push_clip_glyph (hb_paint_funcs_t *, void *, hb_codepoint_t, hb_font_t *, void *);
static void hb_vector_paint_push_clip_rectangle (hb_paint_funcs_t *, void *, float, float, float, float, void *);
static void hb_vector_paint_pop_clip (hb_paint_funcs_t *, void *, void *);
static void hb_vector_paint_color (hb_paint_funcs_t *, void *, hb_bool_t, hb_color_t, void *);
static hb_bool_t hb_vector_paint_image (hb_paint_funcs_t *, void *, hb_blob_t *, unsigned, unsigned, hb_tag_t, float, hb_glyph_extents_t *, void *);
static void hb_vector_paint_linear_gradient (hb_paint_funcs_t *, void *, hb_color_line_t *, float, float, float, float, float, float, void *);
static void hb_vector_paint_radial_gradient (hb_paint_funcs_t *, void *, hb_color_line_t *, float, float, float, float, float, float, void *);
static void hb_vector_paint_sweep_gradient (hb_paint_funcs_t *, void *, hb_color_line_t *, float, float, float, float, void *);
static void hb_vector_paint_push_group (hb_paint_funcs_t *, void *, void *);
static void hb_vector_paint_pop_group (hb_paint_funcs_t *, void *, hb_paint_composite_mode_t, void *);
static hb_bool_t hb_vector_paint_color_glyph (hb_paint_funcs_t *, void *, hb_codepoint_t, hb_font_t *, void *);

static hb_paint_funcs_t *
hb_vector_paint_funcs_singleton ()
{
  static hb_paint_funcs_t *funcs = nullptr;
  if (likely (funcs))
    return funcs;

  funcs = hb_paint_funcs_create ();
  hb_paint_funcs_set_push_transform_func (funcs, (hb_paint_push_transform_func_t) hb_vector_paint_push_transform, nullptr, nullptr);
  hb_paint_funcs_set_pop_transform_func (funcs, (hb_paint_pop_transform_func_t) hb_vector_paint_pop_transform, nullptr, nullptr);
  hb_paint_funcs_set_push_clip_glyph_func (funcs, (hb_paint_push_clip_glyph_func_t) hb_vector_paint_push_clip_glyph, nullptr, nullptr);
  hb_paint_funcs_set_push_clip_rectangle_func (funcs, (hb_paint_push_clip_rectangle_func_t) hb_vector_paint_push_clip_rectangle, nullptr, nullptr);
  hb_paint_funcs_set_pop_clip_func (funcs, (hb_paint_pop_clip_func_t) hb_vector_paint_pop_clip, nullptr, nullptr);
  hb_paint_funcs_set_color_func (funcs, (hb_paint_color_func_t) hb_vector_paint_color, nullptr, nullptr);
  hb_paint_funcs_set_image_func (funcs, (hb_paint_image_func_t) hb_vector_paint_image, nullptr, nullptr);
  hb_paint_funcs_set_linear_gradient_func (funcs, (hb_paint_linear_gradient_func_t) hb_vector_paint_linear_gradient, nullptr, nullptr);
  hb_paint_funcs_set_radial_gradient_func (funcs, (hb_paint_radial_gradient_func_t) hb_vector_paint_radial_gradient, nullptr, nullptr);
  hb_paint_funcs_set_sweep_gradient_func (funcs, (hb_paint_sweep_gradient_func_t) hb_vector_paint_sweep_gradient, nullptr, nullptr);
  hb_paint_funcs_set_push_group_func (funcs, (hb_paint_push_group_func_t) hb_vector_paint_push_group, nullptr, nullptr);
  hb_paint_funcs_set_pop_group_func (funcs, (hb_paint_pop_group_func_t) hb_vector_paint_pop_group, nullptr, nullptr);
  hb_paint_funcs_set_color_glyph_func (funcs, (hb_paint_color_glyph_func_t) hb_vector_paint_color_glyph, nullptr, nullptr);
  hb_paint_funcs_make_immutable (funcs);
  return funcs;
}

static void
hb_vector_paint_ensure_initialized (hb_vector_paint_t *paint)
{
  if (paint->group_stack.length)
    return;
  paint->group_stack.push (hb_vector_t<char> {});
}

static void
hb_vector_paint_push_transform (hb_paint_funcs_t *,
                                void *paint_data,
                                float xx, float yx,
                                float xy, float yy,
                                float dx, float dy,
                                void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  hb_vector_paint_ensure_initialized (paint);

  if (unlikely (paint->transform_group_overflow_depth))
  {
    paint->transform_group_overflow_depth++;
    return;
  }
  if (unlikely (paint->transform_group_depth >= 64))
  {
    paint->transform_group_overflow_depth = 1;
    return;
  }

  hb_bool_t opened =
    !(fabsf (xx - 1.f) < 1e-6f && fabsf (yx) < 1e-6f &&
      fabsf (xy) < 1e-6f && fabsf (yy - 1.f) < 1e-6f &&
      fabsf (dx) < 1e-6f && fabsf (dy) < 1e-6f);
  paint->transform_group_open_mask = (paint->transform_group_open_mask << 1) | (opened ? 1ull : 0ull);
  paint->transform_group_depth++;

  if (!opened)
    return;

  auto &body = paint->current_body ();
  unsigned sprec = hb_svg_scale_precision (paint->precision);
  hb_svg_append_str (&body, "<g transform=\"matrix(");
  hb_svg_append_num (&body, xx, sprec, true);
  hb_svg_append_c (&body, ',');
  hb_svg_append_num (&body, yx, sprec, true);
  hb_svg_append_c (&body, ',');
  hb_svg_append_num (&body, xy, sprec, true);
  hb_svg_append_c (&body, ',');
  hb_svg_append_num (&body, yy, sprec, true);
  hb_svg_append_c (&body, ',');
  hb_svg_append_num (&body, dx, paint->precision);
  hb_svg_append_c (&body, ',');
  hb_svg_append_num (&body, dy, paint->precision);
  hb_svg_append_str (&body, ")\">\n");
}

static void
hb_vector_paint_pop_transform (hb_paint_funcs_t *,
                               void *paint_data,
                               void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  hb_vector_paint_ensure_initialized (paint);
  if (unlikely (paint->transform_group_overflow_depth))
  {
    paint->transform_group_overflow_depth--;
    return;
  }
  if (!paint->transform_group_depth)
    return;
  paint->transform_group_depth--;
  hb_bool_t opened = !!(paint->transform_group_open_mask & 1ull);
  paint->transform_group_open_mask >>= 1;
  if (opened)
    hb_svg_append_str (&paint->current_body (), "</g>\n");
}

static void
hb_vector_paint_push_clip_glyph (hb_paint_funcs_t *,
                                 void *paint_data,
                                 hb_codepoint_t glyph,
                                 hb_font_t *font,
                                 void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  hb_vector_paint_ensure_initialized (paint);

  if (!hb_set_has (paint->defined_outlines, glyph))
  {
    hb_set_add (paint->defined_outlines, glyph);
    paint->path.clear ();
    hb_svg_path_sink_t sink = {&paint->path, paint->precision};
    hb_font_draw_glyph (font, glyph, hb_svg_path_draw_funcs_singleton (), &sink);
    hb_svg_append_str (&paint->defs, "<path id=\"p");
    hb_svg_append_unsigned (&paint->defs, glyph);
    hb_svg_append_str (&paint->defs, "\" d=\"");
    hb_svg_append_len (&paint->defs, paint->path.arrayZ, paint->path.length);
    hb_svg_append_str (&paint->defs, "\"/>\n");
  }

  if (!hb_set_has (paint->defined_clips, glyph))
  {
    hb_set_add (paint->defined_clips, glyph);
    hb_svg_append_str (&paint->defs, "<clipPath id=\"clip-g");
    hb_svg_append_unsigned (&paint->defs, glyph);
    hb_svg_append_str (&paint->defs, "\"><use href=\"#p");
    hb_svg_append_unsigned (&paint->defs, glyph);
    hb_svg_append_str (&paint->defs, "\"/></clipPath>\n");
  }

  hb_svg_append_str (&paint->current_body (), "<g clip-path=\"url(#clip-g");
  hb_svg_append_unsigned (&paint->current_body (), glyph);
  hb_svg_append_str (&paint->current_body (), ")\">\n");
}

static void
hb_vector_paint_push_clip_rectangle (hb_paint_funcs_t *,
                                     void *paint_data,
                                     float xmin, float ymin,
                                     float xmax, float ymax,
                                     void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  hb_vector_paint_ensure_initialized (paint);

  unsigned clip_id = paint->clip_rect_counter++;
  hb_svg_append_str (&paint->defs, "<clipPath id=\"c");
  hb_svg_append_unsigned (&paint->defs, clip_id);
  hb_svg_append_str (&paint->defs, "\"><rect x=\"");
  hb_svg_append_num (&paint->defs, xmin, paint->precision);
  hb_svg_append_str (&paint->defs, "\" y=\"");
  hb_svg_append_num (&paint->defs, ymin, paint->precision);
  hb_svg_append_str (&paint->defs, "\" width=\"");
  hb_svg_append_num (&paint->defs, xmax - xmin, paint->precision);
  hb_svg_append_str (&paint->defs, "\" height=\"");
  hb_svg_append_num (&paint->defs, ymax - ymin, paint->precision);
  hb_svg_append_str (&paint->defs, "\"/></clipPath>\n");

  hb_svg_append_str (&paint->current_body (), "<g clip-path=\"url(#c");
  hb_svg_append_unsigned (&paint->current_body (), clip_id);
  hb_svg_append_str (&paint->current_body (), ")\">\n");
}

static void
hb_vector_paint_pop_clip (hb_paint_funcs_t *,
                          void *paint_data,
                          void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  hb_vector_paint_ensure_initialized (paint);
  hb_svg_append_str (&paint->current_body (), "</g>\n");
}

static void
hb_vector_paint_color (hb_paint_funcs_t *,
                       void *paint_data,
                       hb_bool_t is_foreground,
                       hb_color_t color,
                       void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  hb_vector_paint_ensure_initialized (paint);

  hb_color_t c = color;
  if (is_foreground)
    c = HB_COLOR (hb_color_get_blue (paint->foreground),
                  hb_color_get_green (paint->foreground),
                  hb_color_get_red (paint->foreground),
                  (unsigned) hb_color_get_alpha (paint->foreground) * hb_color_get_alpha (color) / 255);

  auto &body = paint->current_body ();
  hb_svg_append_str (&body, "<rect x=\"-32767\" y=\"-32767\" width=\"65534\" height=\"65534\" fill=\"");
  hb_svg_append_color (&body, c, true);
  hb_svg_append_str (&body, "\"/>\n");
}

static hb_bool_t
hb_vector_paint_image (hb_paint_funcs_t *,
                       void *paint_data,
                       hb_blob_t *image,
                       unsigned width,
                       unsigned height,
                       hb_tag_t format,
                       float slant HB_UNUSED,
                       hb_glyph_extents_t *extents,
                       void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  hb_vector_paint_ensure_initialized (paint);

  if (format != HB_TAG ('s','v','g',' '))
    return false;

  auto &body = paint->current_body ();
  paint->current_color_glyph_has_svg_image = true;

  hb_vector_t<char> subset_body;
  bool subset_ok = hb_svg_subset_glyph_image (image,
                                              paint->current_svg_image_glyph,
                                              &paint->svg_image_counter,
                                              &paint->defs,
                                              &subset_body);
  if (unlikely (!subset_ok))
    return false;

  if (extents)
  {
    hb_svg_append_str (&body, "<g transform=\"translate(");
    hb_svg_append_num (&body, (float) extents->x_bearing, paint->precision);
    hb_svg_append_c (&body, ',');
    hb_svg_append_num (&body, (float) extents->y_bearing, paint->precision);
    hb_svg_append_str (&body, ") scale(");
    hb_svg_append_num (&body, (float) extents->width / width, paint->precision);
    hb_svg_append_c (&body, ',');
    hb_svg_append_num (&body, (float) extents->height / height, paint->precision);
    hb_svg_append_str (&body, ")\">\n");
  }

  hb_svg_append_len (&body, subset_body.arrayZ, subset_body.length);
  hb_svg_append_c (&body, '\n');

  if (extents)
    hb_svg_append_str (&body, "</g>\n");

  return true;
}

static void
hb_vector_paint_linear_gradient (hb_paint_funcs_t *,
                                 void *paint_data,
                                 hb_color_line_t *color_line,
                                 float x0, float y0,
                                 float x1, float y1,
                                 float x2, float y2,
                                 void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  hb_vector_paint_ensure_initialized (paint);

  hb_vector_t<hb_color_stop_t> &stops = paint->color_stops_scratch;
  if (!hb_svg_get_color_stops (paint, color_line, &stops) || !stops.length)
    return;

  qsort (stops.arrayZ, stops.length, sizeof (hb_color_stop_t), hb_svg_color_stop_cmp);

  unsigned grad_id = paint->gradient_counter++;

  hb_svg_append_str (&paint->defs, "<linearGradient id=\"gr");
  hb_svg_append_unsigned (&paint->defs, grad_id);
  hb_svg_append_str (&paint->defs, "\" gradientUnits=\"userSpaceOnUse\" x1=\"");
  hb_svg_append_num (&paint->defs, x0, paint->precision);
  hb_svg_append_str (&paint->defs, "\" y1=\"");
  hb_svg_append_num (&paint->defs, y0, paint->precision);
  hb_svg_append_str (&paint->defs, "\" x2=\"");
  hb_svg_append_num (&paint->defs, x1 + (x1 - x2), paint->precision);
  hb_svg_append_str (&paint->defs, "\" y2=\"");
  hb_svg_append_num (&paint->defs, y1 + (y1 - y2), paint->precision);
  hb_svg_append_str (&paint->defs, "\" spreadMethod=\"");
  hb_svg_append_str (&paint->defs, hb_svg_extend_mode_str (hb_color_line_get_extend (color_line)));
  hb_svg_append_str (&paint->defs, "\">\n");
  hb_svg_emit_color_stops (paint, &paint->defs, &stops);
  hb_svg_append_str (&paint->defs, "</linearGradient>\n");

  hb_svg_append_str (&paint->current_body (),
                     "<rect x=\"-32767\" y=\"-32767\" width=\"65534\" height=\"65534\" fill=\"url(#gr");
  hb_svg_append_unsigned (&paint->current_body (), grad_id);
  hb_svg_append_str (&paint->current_body (), ")\"/>\n");
}

static void
hb_vector_paint_radial_gradient (hb_paint_funcs_t *,
                                 void *paint_data,
                                 hb_color_line_t *color_line,
                                 float x0, float y0, float r0,
                                 float x1, float y1, float r1,
                                 void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  hb_vector_paint_ensure_initialized (paint);

  hb_vector_t<hb_color_stop_t> &stops = paint->color_stops_scratch;
  if (!hb_svg_get_color_stops (paint, color_line, &stops) || !stops.length)
    return;

  qsort (stops.arrayZ, stops.length, sizeof (hb_color_stop_t), hb_svg_color_stop_cmp);

  unsigned grad_id = paint->gradient_counter++;

  hb_svg_append_str (&paint->defs, "<radialGradient id=\"gr");
  hb_svg_append_unsigned (&paint->defs, grad_id);
  hb_svg_append_str (&paint->defs, "\" gradientUnits=\"userSpaceOnUse\" cx=\"");
  hb_svg_append_num (&paint->defs, x1, paint->precision);
  hb_svg_append_str (&paint->defs, "\" cy=\"");
  hb_svg_append_num (&paint->defs, y1, paint->precision);
  hb_svg_append_str (&paint->defs, "\" r=\"");
  hb_svg_append_num (&paint->defs, r1, paint->precision);
  hb_svg_append_str (&paint->defs, "\" fx=\"");
  hb_svg_append_num (&paint->defs, x0, paint->precision);
  hb_svg_append_str (&paint->defs, "\" fy=\"");
  hb_svg_append_num (&paint->defs, y0, paint->precision);
  if (r0 > 0)
  {
    hb_svg_append_str (&paint->defs, "\" fr=\"");
    hb_svg_append_num (&paint->defs, r0, paint->precision);
  }
  hb_svg_append_str (&paint->defs, "\" spreadMethod=\"");
  hb_svg_append_str (&paint->defs, hb_svg_extend_mode_str (hb_color_line_get_extend (color_line)));
  hb_svg_append_str (&paint->defs, "\">\n");
  hb_svg_emit_color_stops (paint, &paint->defs, &stops);
  hb_svg_append_str (&paint->defs, "</radialGradient>\n");

  hb_svg_append_str (&paint->current_body (),
                     "<rect x=\"-32767\" y=\"-32767\" width=\"65534\" height=\"65534\" fill=\"url(#gr");
  hb_svg_append_unsigned (&paint->current_body (), grad_id);
  hb_svg_append_str (&paint->current_body (), ")\"/>\n");
}

static void
hb_vector_paint_sweep_gradient (hb_paint_funcs_t *,
                                void *paint_data,
                                hb_color_line_t *color_line,
                                float cx, float cy,
                                float start_angle, float end_angle,
                                void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  hb_vector_paint_ensure_initialized (paint);

  hb_vector_t<hb_color_stop_t> &stops = paint->color_stops_scratch;
  if (!hb_svg_get_color_stops (paint, color_line, &stops) || !stops.length)
    return;

  qsort (stops.arrayZ, stops.length, sizeof (hb_color_stop_t), hb_svg_color_stop_cmp);

  hb_color_t c = stops.arrayZ[stops.length - 1].color;
  if (start_angle > end_angle)
    c = stops.arrayZ[0].color;

  auto &body = paint->current_body ();
  hb_svg_append_str (&body, "<path d=\"M");
  hb_svg_append_num (&body, cx, paint->precision);
  hb_svg_append_c (&body, ',');
  hb_svg_append_num (&body, cy, paint->precision);
  hb_svg_append_str (&body, " m-32767,0 a32767,32767 0 1,0 65534,0 a32767,32767 0 1,0 -65534,0\" fill=\"");
  hb_svg_append_color (&body, c, true);
  hb_svg_append_str (&body, "\"/>\n");
}

static void
hb_vector_paint_push_group (hb_paint_funcs_t *,
                            void *paint_data,
                            void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  hb_vector_paint_ensure_initialized (paint);
  paint->group_stack.push (hb_vector_t<char> {});
}

static void
hb_vector_paint_pop_group (hb_paint_funcs_t *,
                           void *paint_data,
                           hb_paint_composite_mode_t mode,
                           void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  hb_vector_paint_ensure_initialized (paint);
  if (paint->group_stack.length < 2)
    return;

  hb_vector_t<char> group = paint->group_stack.pop ();
  auto &body = paint->current_body ();

  const char *blend = hb_svg_composite_mode_str (mode);
  if (blend)
  {
    hb_svg_append_str (&body, "<g style=\"mix-blend-mode:");
    hb_svg_append_str (&body, blend);
    hb_svg_append_str (&body, "\">\n");
    hb_svg_append_len (&body, group.arrayZ, group.length);
    hb_svg_append_str (&body, "</g>\n");
  }
  else
    hb_svg_append_len (&body, group.arrayZ, group.length);
}

static hb_bool_t
hb_vector_paint_color_glyph (hb_paint_funcs_t *,
                             void *paint_data,
                             hb_codepoint_t glyph,
                             hb_font_t *font,
                             void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  hb_vector_paint_ensure_initialized (paint);
  hb_codepoint_t old_gid = paint->current_svg_image_glyph;
  paint->current_svg_image_glyph = glyph;
  hb_font_paint_glyph (font, glyph,
                       hb_vector_paint_funcs_singleton (),
                       paint,
                       paint->palette,
                       paint->foreground);
  paint->current_svg_image_glyph = old_gid;
  return true;
}


hb_vector_draw_t *
hb_vector_draw_create_or_fail (void)
{
  hb_vector_draw_t *draw = hb_object_create<hb_vector_draw_t> ();
  if (unlikely (!draw))
    return nullptr;
  draw->defined_glyphs = hb_set_create ();
  return draw;
}

hb_vector_draw_t *
hb_vector_draw_reference (hb_vector_draw_t *draw)
{
  return hb_object_reference (draw);
}

void
hb_vector_draw_destroy (hb_vector_draw_t *draw)
{
  if (!hb_object_destroy (draw)) return;
  hb_blob_destroy (draw->recycled_blob);
  hb_set_destroy (draw->defined_glyphs);
  hb_free (draw);
}

hb_bool_t
hb_vector_draw_set_user_data (hb_vector_draw_t   *draw,
                              hb_user_data_key_t *key,
                              void               *data,
                              hb_destroy_func_t   destroy,
                              hb_bool_t           replace)
{
  return hb_object_set_user_data (draw, key, data, destroy, replace);
}

void *
hb_vector_draw_get_user_data (hb_vector_draw_t   *draw,
                              hb_user_data_key_t *key)
{
  return hb_object_get_user_data (draw, key);
}

void
hb_vector_draw_set_format (hb_vector_draw_t *draw,
                           hb_vector_format_t format)
{
  draw->format = format;
}

void
hb_vector_draw_set_transform (hb_vector_draw_t *draw,
                              float xx, float yx,
                              float xy, float yy,
                              float dx, float dy)
{
  draw->transform = {xx, yx, xy, yy, dx, dy};
}

void
hb_vector_draw_set_scale_factor (hb_vector_draw_t *draw,
                                 float x_scale_factor,
                                 float y_scale_factor)
{
  draw->x_scale_factor = x_scale_factor > 0.f ? x_scale_factor : 1.f;
  draw->y_scale_factor = y_scale_factor > 0.f ? y_scale_factor : 1.f;
}

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

hb_draw_funcs_t *
hb_vector_draw_get_funcs (void)
{
  return hb_vector_draw_funcs_singleton ();
}

hb_bool_t
hb_vector_draw_glyph (hb_vector_draw_t *draw,
                      hb_font_t *font,
                      hb_codepoint_t glyph,
                      float pen_x,
                      float pen_y,
                      hb_vector_extents_mode_t extents_mode)
{
  if (draw->format != HB_VECTOR_FORMAT_SVG)
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

  bool needs_def = !draw->flat && !hb_set_has (draw->defined_glyphs, glyph);
  if (needs_def)
  {
    hb_set_add (draw->defined_glyphs, glyph);
    draw->path.clear ();
    hb_svg_path_sink_t sink = {&draw->path, draw->precision};
    hb_font_draw_glyph (font, glyph, hb_svg_path_draw_funcs_singleton (), &sink);
    if (!draw->path.length)
      return false;
    hb_svg_append_str (&draw->defs, "<path id=\"p");
    hb_svg_append_unsigned (&draw->defs, glyph);
    hb_svg_append_str (&draw->defs, "\" d=\"");
    hb_svg_append_len (&draw->defs, draw->path.arrayZ, draw->path.length);
    hb_svg_append_str (&draw->defs, "\"/>\n");
  }

  if (draw->flat)
  {
    draw->path.clear ();
    hb_svg_path_sink_t sink = {&draw->path, draw->precision};
    hb_font_draw_glyph (font, glyph, hb_svg_path_draw_funcs_singleton (), &sink);

    if (!draw->path.length)
      return false;

    float xx = draw->transform.xx / draw->x_scale_factor;
    float yx = draw->transform.yx / draw->y_scale_factor;
    float xy = draw->transform.xy / draw->x_scale_factor;
    float yy = draw->transform.yy / draw->y_scale_factor;
    float tx = (draw->transform.x0 + draw->transform.xx * pen_x + draw->transform.xy * pen_y) / draw->x_scale_factor;
    float ty = (draw->transform.y0 + draw->transform.yx * pen_x + draw->transform.yy * pen_y) / draw->y_scale_factor;

    hb_svg_append_str (&draw->body, "<path d=\"");
    hb_svg_append_len (&draw->body, draw->path.arrayZ, draw->path.length);
    hb_svg_append_str (&draw->body, "\" transform=\"");
    if (draw->transform.xx == 1.f && draw->transform.yx == 0.f &&
	draw->transform.xy == 0.f && draw->transform.yy == 1.f)
    {
      float sx = 1.f / draw->x_scale_factor;
      float sy = 1.f / draw->y_scale_factor;
      unsigned sprec = hb_svg_scale_precision (draw->precision);
      hb_svg_append_str (&draw->body, "translate(");
      hb_svg_append_num (&draw->body, tx, draw->precision);
      hb_svg_append_c (&draw->body, ',');
      hb_svg_append_num (&draw->body, ty, draw->precision);
      hb_svg_append_str (&draw->body, ") scale(");
      hb_svg_append_num (&draw->body, sx, sprec, true);
      hb_svg_append_c (&draw->body, ',');
      hb_svg_append_num (&draw->body, -sy, sprec, true);
      hb_svg_append_c (&draw->body, ')');
    }
    else
    {
      unsigned sprec = hb_svg_scale_precision (draw->precision);
      hb_svg_append_str (&draw->body, "matrix(");
      hb_svg_append_num (&draw->body, xx, sprec, true);
      hb_svg_append_c (&draw->body, ',');
      hb_svg_append_num (&draw->body, yx, sprec, true);
      hb_svg_append_c (&draw->body, ',');
      hb_svg_append_num (&draw->body, -xy, sprec, true);
      hb_svg_append_c (&draw->body, ',');
      hb_svg_append_num (&draw->body, -yy, sprec, true);
      hb_svg_append_c (&draw->body, ',');
      hb_svg_append_num (&draw->body, tx, draw->precision);
      hb_svg_append_c (&draw->body, ',');
      hb_svg_append_num (&draw->body, ty, draw->precision);
      hb_svg_append_c (&draw->body, ')');
    }
    hb_svg_append_str (&draw->body, "\"/>\n");
    return true;
  }

  float xx = draw->transform.xx / draw->x_scale_factor;
  float yx = draw->transform.yx / draw->y_scale_factor;
  float xy = draw->transform.xy / draw->x_scale_factor;
  float yy = draw->transform.yy / draw->y_scale_factor;
  float tx = (draw->transform.x0 + draw->transform.xx * pen_x + draw->transform.xy * pen_y) / draw->x_scale_factor;
  float ty = (draw->transform.y0 + draw->transform.yx * pen_x + draw->transform.yy * pen_y) / draw->y_scale_factor;

  hb_svg_append_str (&draw->body, "<use href=\"#p");
  hb_svg_append_unsigned (&draw->body, glyph);
  hb_svg_append_str (&draw->body, "\" transform=\"");
  if (draw->transform.xx == 1.f && draw->transform.yx == 0.f &&
      draw->transform.xy == 0.f && draw->transform.yy == 1.f)
  {
    float sx = 1.f / draw->x_scale_factor;
    float sy = 1.f / draw->y_scale_factor;
    unsigned sprec = hb_svg_scale_precision (draw->precision);
    hb_svg_append_str (&draw->body, "translate(");
    hb_svg_append_num (&draw->body, tx, draw->precision);
    hb_svg_append_c (&draw->body, ',');
    hb_svg_append_num (&draw->body, ty, draw->precision);
    hb_svg_append_str (&draw->body, ") scale(");
    hb_svg_append_num (&draw->body, sx, sprec, true);
    hb_svg_append_c (&draw->body, ',');
    hb_svg_append_num (&draw->body, -sy, sprec, true);
    hb_svg_append_c (&draw->body, ')');
  }
  else
  {
    unsigned sprec = hb_svg_scale_precision (draw->precision);
    hb_svg_append_str (&draw->body, "matrix(");
    hb_svg_append_num (&draw->body, xx, sprec, true);
    hb_svg_append_c (&draw->body, ',');
    hb_svg_append_num (&draw->body, yx, sprec, true);
    hb_svg_append_c (&draw->body, ',');
    hb_svg_append_num (&draw->body, -xy, sprec, true);
    hb_svg_append_c (&draw->body, ',');
    hb_svg_append_num (&draw->body, -yy, sprec, true);
    hb_svg_append_c (&draw->body, ',');
    hb_svg_append_num (&draw->body, tx, draw->precision);
    hb_svg_append_c (&draw->body, ',');
    hb_svg_append_num (&draw->body, ty, draw->precision);
    hb_svg_append_c (&draw->body, ')');
  }
  hb_svg_append_str (&draw->body, "\"/>\n");
  return true;
}

void
hb_vector_svg_set_flat (hb_vector_draw_t *draw,
                        hb_bool_t flat)
{
  draw->flat = !!flat;
}

void
hb_vector_svg_set_precision (hb_vector_draw_t *draw,
                             unsigned precision)
{
  draw->precision = hb_min (precision, 12u);
}

hb_blob_t *
hb_vector_draw_render (hb_vector_draw_t *draw)
{
  if (draw->format != HB_VECTOR_FORMAT_SVG)
    return nullptr;
  if (!draw->has_extents)
    return nullptr;

  hb_vector_t<char> out;
  hb_svg_recover_recycled_buffer (draw->recycled_blob, &out);
  unsigned estimated = draw->defs.length +
                       (draw->body.length ? draw->body.length : draw->path.length) +
                       256;
  out.alloc (estimated);
  hb_svg_append_str (&out, "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"");
  hb_svg_append_num (&out, draw->extents.x, draw->precision);
  hb_svg_append_c (&out, ' ');
  hb_svg_append_num (&out, draw->extents.y, draw->precision);
  hb_svg_append_c (&out, ' ');
  hb_svg_append_num (&out, draw->extents.width, draw->precision);
  hb_svg_append_c (&out, ' ');
  hb_svg_append_num (&out, draw->extents.height, draw->precision);
  hb_svg_append_str (&out, "\" width=\"");
  hb_svg_append_num (&out, draw->extents.width, draw->precision);
  hb_svg_append_str (&out, "\" height=\"");
  hb_svg_append_num (&out, draw->extents.height, draw->precision);
  hb_svg_append_str (&out, "\">\n");

  if (draw->defs.length)
  {
    hb_svg_append_str (&out, "<defs>\n");
    hb_svg_append_len (&out, draw->defs.arrayZ, draw->defs.length);
    hb_svg_append_str (&out, "</defs>\n");
  }

  if (draw->body.length)
  {
    hb_svg_append_len (&out, draw->body.arrayZ, draw->body.length);
  }
  else if (draw->path.length)
  {
    hb_svg_append_str (&out, "<path d=\"");
    hb_svg_append_len (&out, draw->path.arrayZ, draw->path.length);
    hb_svg_append_str (&out, "\"/>\n");
  }

  hb_svg_append_str (&out, "</svg>\n");

  hb_blob_t *blob = hb_svg_blob_from_buffer (&draw->recycled_blob, &out);

  draw->path.clear ();
  draw->defs.clear ();
  draw->body.clear ();
  hb_set_clear (draw->defined_glyphs);
  draw->has_extents = false;
  draw->extents = {0, 0, 0, 0};

  return blob;
}

void
hb_vector_draw_reset (hb_vector_draw_t *draw)
{
  draw->format = HB_VECTOR_FORMAT_SVG;
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


hb_vector_paint_t *
hb_vector_paint_create_or_fail (void)
{
  hb_vector_paint_t *paint = hb_object_create<hb_vector_paint_t> ();
  if (unlikely (!paint))
    return nullptr;

  paint->defined_outlines = hb_set_create ();
  paint->defined_clips = hb_set_create ();

  return paint;
}

hb_vector_paint_t *
hb_vector_paint_reference (hb_vector_paint_t *paint)
{
  return hb_object_reference (paint);
}

void
hb_vector_paint_destroy (hb_vector_paint_t *paint)
{
  if (!hb_object_destroy (paint)) return;
  hb_blob_destroy (paint->recycled_blob);
  hb_set_destroy (paint->defined_outlines);
  hb_set_destroy (paint->defined_clips);
  hb_free (paint);
}

hb_bool_t
hb_vector_paint_set_user_data (hb_vector_paint_t  *paint,
                               hb_user_data_key_t *key,
                               void               *data,
                               hb_destroy_func_t   destroy,
                               hb_bool_t           replace)
{
  return hb_object_set_user_data (paint, key, data, destroy, replace);
}

void *
hb_vector_paint_get_user_data (hb_vector_paint_t  *paint,
                               hb_user_data_key_t *key)
{
  return hb_object_get_user_data (paint, key);
}

void
hb_vector_paint_set_format (hb_vector_paint_t *paint,
                            hb_vector_format_t format)
{
  paint->format = format;
}

void
hb_vector_paint_set_transform (hb_vector_paint_t *paint,
                               float xx, float yx,
                               float xy, float yy,
                               float dx, float dy)
{
  paint->transform = {xx, yx, xy, yy, dx, dy};
}

void
hb_vector_paint_set_scale_factor (hb_vector_paint_t *paint,
                                  float x_scale_factor,
                                  float y_scale_factor)
{
  paint->x_scale_factor = x_scale_factor > 0.f ? x_scale_factor : 1.f;
  paint->y_scale_factor = y_scale_factor > 0.f ? y_scale_factor : 1.f;
}

void
hb_vector_paint_set_extents (hb_vector_paint_t *paint,
                             const hb_vector_extents_t *extents)
{
  if (!extents)
  {
    paint->extents = {0, 0, 0, 0};
    paint->has_extents = false;
    return;
  }

  if (!(extents->width > 0.f && extents->height > 0.f))
    return;

  if (paint->has_extents)
  {
    float x0 = hb_min (paint->extents.x, extents->x);
    float y0 = hb_min (paint->extents.y, extents->y);
    float x1 = hb_max (paint->extents.x + paint->extents.width,
		       extents->x + extents->width);
    float y1 = hb_max (paint->extents.y + paint->extents.height,
		       extents->y + extents->height);
    paint->extents = {x0, y0, x1 - x0, y1 - y0};
  }
  else
  {
    paint->extents = *extents;
    paint->has_extents = true;
  }
}

hb_bool_t
hb_vector_paint_set_glyph_extents (hb_vector_paint_t *paint,
                                   const hb_glyph_extents_t *glyph_extents)
{
  hb_bool_t has_extents = paint->has_extents;
  hb_bool_t ret = hb_svg_set_glyph_extents_common (paint->transform,
						   paint->x_scale_factor,
						   paint->y_scale_factor,
						   glyph_extents,
						   &paint->extents,
						   &has_extents);
  paint->has_extents = has_extents;
  return ret;
}

void
hb_vector_paint_set_foreground (hb_vector_paint_t *paint,
                                hb_color_t foreground)
{
  paint->foreground = foreground;
}

void
hb_vector_paint_set_palette (hb_vector_paint_t *paint,
                             int palette)
{
  paint->palette = palette;
}

hb_paint_funcs_t *
hb_vector_paint_get_funcs (void)
{
  return hb_vector_paint_funcs_singleton ();
}

hb_bool_t
hb_vector_paint_glyph (hb_vector_paint_t *paint,
		       hb_font_t         *font,
		       hb_codepoint_t     glyph,
		       float              pen_x,
		       float              pen_y,
		       hb_vector_extents_mode_t extents_mode,
		       unsigned           palette,
		       hb_color_t         foreground)
{
  paint->palette = (int) palette;
  paint->foreground = foreground;

  float xx = paint->transform.xx;
  float yx = paint->transform.yx;
  float xy = paint->transform.xy;
  float yy = paint->transform.yy;
  float tx = paint->transform.x0 + xx * pen_x + xy * pen_y;
  float ty = paint->transform.y0 + yx * pen_x + yy * pen_y;

  if (extents_mode == HB_VECTOR_EXTENTS_MODE_EXPAND)
  {
    hb_glyph_extents_t ge;
    if (hb_font_get_glyph_extents (font, glyph, &ge))
    {
      hb_bool_t has_extents = paint->has_extents;
      hb_transform_t<> extents_transform = {xx, yx, -xy, -yy, tx, ty};
      hb_bool_t ret = hb_svg_set_glyph_extents_common (extents_transform,
							paint->x_scale_factor,
							paint->y_scale_factor,
							&ge,
							&paint->extents,
							&has_extents);
      paint->has_extents = has_extents;
      if (!ret)
	return false;
    }
  }

  hb_vector_paint_ensure_initialized (paint);

  bool can_cache = !paint->flat;
  uint64_t cache_key = hb_svg_color_glyph_cache_key (glyph, palette, foreground);
  if (can_cache)
  {
    if (paint->defined_color_glyphs.has (cache_key))
    {
      uint64_t entry = paint->defined_color_glyphs.get (cache_key);
      unsigned def_id = hb_svg_cache_entry_def_id (entry);
      bool image_like = hb_svg_cache_entry_image_like (entry);
      auto &body = paint->current_body ();
      hb_svg_append_str (&body, "<use href=\"#cg");
      hb_svg_append_unsigned (&body, def_id);
      hb_svg_append_str (&body, "\" transform=\"");
      if (image_like)
        hb_svg_append_image_instance_translate (&body, paint->precision,
                                                paint->x_scale_factor,
                                                paint->y_scale_factor,
                                                tx, ty);
      else
        hb_svg_append_instance_transform (&body, paint->precision,
                                          paint->x_scale_factor,
                                          paint->y_scale_factor,
                                          xx, yx, xy, yy, tx, ty);
      hb_svg_append_str (&body, "\"/>\n");
      return true;
    }
  }

  hb_vector_t<char> captured;
  bool has_svg_image = false;
  if (can_cache)
  {
    paint->group_stack.push (hb_vector_t<char> {});
    paint->current_color_glyph_has_svg_image = false;
  }

  if (can_cache)
  {
    hb_codepoint_t old_gid = paint->current_svg_image_glyph;
    paint->current_svg_image_glyph = glyph;
    hb_bool_t ret = hb_font_paint_glyph_or_fail (font, glyph,
						  hb_vector_paint_get_funcs (), paint,
						  palette, foreground);
    paint->current_svg_image_glyph = old_gid;
    if (unlikely (!ret))
    {
      paint->group_stack.pop ();
      return false;
    }

    captured = paint->group_stack.pop ();
    has_svg_image = paint->current_color_glyph_has_svg_image ||
		    hb_svg_buffer_contains (captured, "<svg");
    if (unlikely (!captured.length))
      return false;

    unsigned def_id = paint->color_glyph_counter++;
    if (unlikely (!paint->defined_color_glyphs.set (cache_key,
                                                    hb_svg_pack_color_glyph_cache_entry (def_id,
                                                                                         has_svg_image))))
      return false;

    hb_svg_append_str (&paint->defs, "<g id=\"cg");
    hb_svg_append_unsigned (&paint->defs, def_id);
    hb_svg_append_str (&paint->defs, "\">\n");
    hb_svg_append_len (&paint->defs, captured.arrayZ, captured.length);
    hb_svg_append_str (&paint->defs, "</g>\n");

    auto &body = paint->current_body ();
    hb_svg_append_str (&body, "<use href=\"#cg");
    hb_svg_append_unsigned (&body, def_id);
    hb_svg_append_str (&body, "\" transform=\"");
    if (has_svg_image)
      hb_svg_append_image_instance_translate (&body, paint->precision,
                                              paint->x_scale_factor,
                                              paint->y_scale_factor,
                                              tx, ty);
    else
      hb_svg_append_instance_transform (&body, paint->precision,
                                        paint->x_scale_factor,
                                        paint->y_scale_factor,
                                        xx, yx, xy, yy, tx, ty);
    hb_svg_append_str (&body, "\"/>\n");
    return true;
  }

  auto &body = paint->current_body ();
  hb_svg_append_str (&body, "<g transform=\"");
  hb_svg_append_instance_transform (&body, paint->precision,
				    paint->x_scale_factor,
				    paint->y_scale_factor,
				    xx, yx, xy, yy, tx, ty);
  hb_svg_append_str (&body, "\">\n");
  hb_codepoint_t old_gid = paint->current_svg_image_glyph;
  paint->current_svg_image_glyph = glyph;
  hb_bool_t ret = hb_font_paint_glyph_or_fail (font, glyph,
						hb_vector_paint_get_funcs (), paint,
						palette, foreground);
  paint->current_svg_image_glyph = old_gid;
  hb_svg_append_str (&body, "</g>\n");
  return ret;
}

void
hb_vector_svg_paint_set_flat (hb_vector_paint_t *paint,
                              hb_bool_t flat)
{
  paint->flat = !!flat;
}

void
hb_vector_svg_paint_set_precision (hb_vector_paint_t *paint,
                                   unsigned precision)
{
  paint->precision = hb_min (precision, 12u);
}

hb_blob_t *
hb_vector_paint_render (hb_vector_paint_t *paint)
{
  if (paint->format != HB_VECTOR_FORMAT_SVG)
    return nullptr;
  if (!paint->has_extents)
    return nullptr;

  hb_vector_paint_ensure_initialized (paint);

  hb_vector_t<char> out;
  hb_svg_recover_recycled_buffer (paint->recycled_blob, &out);
  unsigned estimated = paint->defs.length +
                       paint->group_stack.arrayZ[0].length +
                       320;
  out.alloc (estimated);
  hb_svg_append_str (&out, "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" viewBox=\"");
  hb_svg_append_num (&out, paint->extents.x, paint->precision);
  hb_svg_append_c (&out, ' ');
  hb_svg_append_num (&out, paint->extents.y, paint->precision);
  hb_svg_append_c (&out, ' ');
  hb_svg_append_num (&out, paint->extents.width, paint->precision);
  hb_svg_append_c (&out, ' ');
  hb_svg_append_num (&out, paint->extents.height, paint->precision);
  hb_svg_append_str (&out, "\" width=\"");
  hb_svg_append_num (&out, paint->extents.width, paint->precision);
  hb_svg_append_str (&out, "\" height=\"");
  hb_svg_append_num (&out, paint->extents.height, paint->precision);
  hb_svg_append_str (&out, "\">\n");

  if (paint->defs.length)
  {
    hb_svg_append_str (&out, "<defs>\n");
    hb_svg_append_len (&out, paint->defs.arrayZ, paint->defs.length);
    hb_svg_append_str (&out, "</defs>\n");
  }

  paint->append_global_transform_prefix (&out);
  hb_svg_append_len (&out, paint->group_stack.arrayZ[0].arrayZ, paint->group_stack.arrayZ[0].length);
  paint->append_global_transform_suffix (&out);

  hb_svg_append_str (&out, "</svg>\n");

  hb_blob_t *blob = hb_svg_blob_from_buffer (&paint->recycled_blob, &out);

  hb_vector_paint_reset (paint);

  return blob;
}

void
hb_vector_paint_reset (hb_vector_paint_t *paint)
{
  paint->format = HB_VECTOR_FORMAT_SVG;
  paint->transform = {1, 0, 0, 1, 0, 0};
  paint->x_scale_factor = 1.f;
  paint->y_scale_factor = 1.f;
  paint->extents = {0, 0, 0, 0};
  paint->has_extents = false;
  paint->foreground = HB_COLOR (0, 0, 0, 255);
  paint->palette = 0;
  paint->precision = 2;
  paint->flat = false;

  paint->defs.clear ();
  paint->body.clear ();
  paint->path.clear ();
  paint->group_stack.clear ();
  paint->transform_group_open_mask = 0;
  paint->transform_group_depth = 0;
  paint->transform_group_overflow_depth = 0;
  paint->clip_rect_counter = 0;
  paint->gradient_counter = 0;
  paint->color_glyph_counter = 0;
  paint->current_color_glyph_has_svg_image = false;
  paint->current_svg_image_glyph = HB_CODEPOINT_INVALID;
  paint->svg_image_counter = 0;
  hb_set_clear (paint->defined_outlines);
  hb_set_clear (paint->defined_clips);
  paint->defined_color_glyphs.clear ();
  paint->color_stops_scratch.clear ();
}

void
hb_vector_paint_recycle_blob (hb_vector_paint_t *paint,
                              hb_blob_t *blob)
{
  hb_blob_destroy (paint->recycled_blob);
  paint->recycled_blob = nullptr;
  if (!blob || blob == hb_blob_get_empty ())
    return;
  paint->recycled_blob = blob;
}
