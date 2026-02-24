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
#include "hb-machinery.hh"
#include "hb-map.hh"
#include "hb-vector-svg-subset.hh"
#include "hb-vector-svg-utils.hh"

#include <algorithm>
#include <math.h>
#include <string.h>


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

HB_UNUSED static bool
hb_svg_append_base64 (hb_vector_t<char> *buf,
                      const uint8_t *data,
                      unsigned len)
{
  static const char b64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  unsigned out_len = ((len + 2) / 3) * 4;
  unsigned old_len = buf->length;
  if (unlikely (!buf->resize_dirty ((int) (old_len + out_len))))
    return false;

  char *dst = buf->arrayZ + old_len;
  unsigned di = 0;
  unsigned i = 0;
  while (i + 2 < len)
  {
    unsigned v = ((unsigned) data[i] << 16) |
                 ((unsigned) data[i + 1] << 8) |
                 ((unsigned) data[i + 2]);
    dst[di++] = b64[(v >> 18) & 63];
    dst[di++] = b64[(v >> 12) & 63];
    dst[di++] = b64[(v >> 6) & 63];
    dst[di++] = b64[v & 63];
    i += 3;
  }

  if (i < len)
  {
    unsigned v = (unsigned) data[i] << 16;
    if (i + 1 < len)
      v |= (unsigned) data[i + 1] << 8;
    dst[di++] = b64[(v >> 18) & 63];
    dst[di++] = b64[(v >> 12) & 63];
    dst[di++] = (i + 1 < len) ? b64[(v >> 6) & 63] : '=';
    dst[di++] = '=';
  }

  return true;
}

static void
hb_svg_append_num (hb_vector_t<char> *buf,
                   float v,
                   unsigned precision,
                   bool keep_nonzero = false)
{
  unsigned effective_precision = precision;
  if (effective_precision > 12)
    effective_precision = 12;
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

  static const char float_formats[13][6] = {
    "%.0f",  "%.1f",  "%.2f",  "%.3f",  "%.4f",  "%.5f",  "%.6f",
    "%.7f",  "%.8f",  "%.9f",  "%.10f", "%.11f", "%.12f",
  };
  char out[128];
  snprintf (out, sizeof (out), float_formats[effective_precision], (double) v);

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

HB_UNUSED static void
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

struct hb_svg_color_glyph_cache_key_t
{
  hb_codepoint_t glyph = HB_CODEPOINT_INVALID;
  unsigned palette = 0;
  hb_color_t foreground = 0;

  hb_svg_color_glyph_cache_key_t () = default;
  hb_svg_color_glyph_cache_key_t (hb_codepoint_t g, unsigned p, hb_color_t f)
    : glyph (g), palette (p), foreground (f) {}

  bool operator == (const hb_svg_color_glyph_cache_key_t &o) const
  {
    return glyph == o.glyph &&
           palette == o.palette &&
           foreground == o.foreground;
  }

  uint32_t hash () const
  {
    uint32_t h = hb_hash (glyph);
    h = h * 31u + hb_hash (palette);
    h = h * 31u + hb_hash (foreground);
    return h;
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
  hb_hashmap_t<hb_svg_color_glyph_cache_key_t, uint64_t> defined_color_glyphs;
  hb_vector_t<hb_color_stop_t> color_stops_scratch;
  hb_vector_t<char> subset_body_scratch;
  hb_vector_t<char> captured_scratch;
  hb_blob_t *recycled_blob = nullptr;
  bool current_color_glyph_has_svg_image = false;
  hb_codepoint_t current_svg_image_glyph = HB_CODEPOINT_INVALID;
  hb_face_t *current_face = nullptr;
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

HB_UNUSED static inline uint64_t
hb_svg_pack_color_glyph_cache_entry (unsigned def_id,
                                     bool image_like)
{
  return ((uint64_t) def_id << 1) | (image_like ? 1ull : 0ull);
}

HB_UNUSED static inline unsigned
hb_svg_cache_entry_def_id (uint64_t v)
{
  return (unsigned) (v >> 1);
}

HB_UNUSED static inline bool
hb_svg_cache_entry_image_like (uint64_t v)
{
  return !!(v & 1ull);
}

HB_UNUSED static inline hb_svg_color_glyph_cache_key_t
hb_svg_color_glyph_cache_key (hb_codepoint_t glyph,
                              unsigned palette,
                              hb_color_t foreground)
{
  return {glyph, palette, foreground};
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

HB_UNUSED static inline void
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

static inline void free_static_vector_draw_funcs ();
static inline void free_static_svg_path_draw_funcs ();

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

static struct hb_svg_path_draw_funcs_lazy_loader_t
  : hb_draw_funcs_lazy_loader_t<hb_svg_path_draw_funcs_lazy_loader_t>
{
  static hb_draw_funcs_t *create ()
  {
    hb_draw_funcs_t *funcs = hb_draw_funcs_create ();
    hb_draw_funcs_set_move_to_func (funcs, (hb_draw_move_to_func_t) hb_svg_path_move_to, nullptr, nullptr);
    hb_draw_funcs_set_line_to_func (funcs, (hb_draw_line_to_func_t) hb_svg_path_line_to, nullptr, nullptr);
    hb_draw_funcs_set_quadratic_to_func (funcs, (hb_draw_quadratic_to_func_t) hb_svg_path_quadratic_to, nullptr, nullptr);
    hb_draw_funcs_set_cubic_to_func (funcs, (hb_draw_cubic_to_func_t) hb_svg_path_cubic_to, nullptr, nullptr);
    hb_draw_funcs_set_close_path_func (funcs, (hb_draw_close_path_func_t) hb_svg_path_close_path, nullptr, nullptr);
    hb_draw_funcs_make_immutable (funcs);
    hb_atexit (free_static_svg_path_draw_funcs);
    return funcs;
  }
} static_svg_path_draw_funcs;

static inline void
free_static_vector_draw_funcs ()
{
  static_vector_draw_funcs.free_instance ();
}

static inline void
free_static_svg_path_draw_funcs ()
{
  static_svg_path_draw_funcs.free_instance ();
}

static hb_draw_funcs_t *
hb_vector_draw_funcs_get ()
{
  return static_vector_draw_funcs.get_unconst ();
}

static hb_draw_funcs_t *
hb_svg_path_draw_funcs_get ()
{
  return static_svg_path_draw_funcs.get_unconst ();
}

hb_vector_draw_t *
hb_vector_draw_create_or_fail (hb_vector_format_t format)
{
  if (format != HB_VECTOR_FORMAT_SVG)
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
 * XSince: REPLACEME
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
 * XSince: REPLACEME
 */
void
hb_vector_draw_get_scale_factor (hb_vector_draw_t *draw,
                                 float *x_scale_factor,
                                 float *y_scale_factor)
{
  if (x_scale_factor) *x_scale_factor = draw->x_scale_factor;
  if (y_scale_factor) *y_scale_factor = draw->y_scale_factor;
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

/**
 * hb_vector_draw_get_extents:
 * @draw: a draw context.
 * @extents: (out) (nullable): where to store current output extents.
 *
 * Gets current output extents from @draw.
 *
 * Return value: `true` if extents are set, `false` otherwise.
 *
 * XSince: REPLACEME
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
  return hb_vector_draw_funcs_get ();
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
    draw->path.clear ();
    hb_svg_path_sink_t sink = {&draw->path, draw->precision};
    hb_font_draw_glyph (font, glyph, hb_svg_path_draw_funcs_get (), &sink);
    if (!draw->path.length)
      return false;
    hb_svg_append_str (&draw->defs, "<path id=\"p");
    hb_svg_append_unsigned (&draw->defs, glyph);
    hb_svg_append_str (&draw->defs, "\" d=\"");
    hb_svg_append_len (&draw->defs, draw->path.arrayZ, draw->path.length);
    hb_svg_append_str (&draw->defs, "\"/>\n");
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

    hb_svg_append_str (&draw->body, "<path d=\"");
    hb_svg_append_len (&draw->body, draw->path.arrayZ, draw->path.length);
    hb_svg_append_str (&draw->body, "\" transform=\"");
    hb_svg_append_instance_transform (&draw->body,
                                      draw->precision,
                                      draw->x_scale_factor,
                                      draw->y_scale_factor,
                                      xx, yx, xy, yy, tx, ty);
    hb_svg_append_str (&draw->body, "\"/>\n");
    return true;
  }

  float xx = draw->transform.xx;
  float yx = draw->transform.yx;
  float xy = draw->transform.xy;
  float yy = draw->transform.yy;
  float tx = draw->transform.x0 + xx * pen_x + xy * pen_y;
  float ty = draw->transform.y0 + yx * pen_x + yy * pen_y;

  hb_svg_append_str (&draw->body, "<use href=\"#p");
  hb_svg_append_unsigned (&draw->body, glyph);
  hb_svg_append_str (&draw->body, "\" transform=\"");
  hb_svg_append_instance_transform (&draw->body,
                                    draw->precision,
                                    draw->x_scale_factor,
                                    draw->y_scale_factor,
                                    xx, yx, xy, yy, tx, ty);
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
