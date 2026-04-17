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

#ifndef HB_VECTOR_UTILS_HH
#define HB_VECTOR_UTILS_HH

#include "hb.hh"
#include "hb-vector.hh"
#include <math.h>
#include <stdio.h>
#include <string.h>

HB_INTERNAL const char *
hb_vector_decimal_point_get (void);

static inline bool
hb_buf_append_len (hb_vector_t<char> *buf,
                   const char *s,
                   unsigned len)
{
  unsigned old_len = buf->length;
  if (unlikely (!buf->resize_dirty ((int) (old_len + len))))
    return false;
  hb_memcpy (buf->arrayZ + old_len, s, len);
  return true;
}

static inline bool
hb_buf_append_c (hb_vector_t<char> *buf, char c)
{
  return buf->push_or_fail (c);
}

static inline void
hb_buf_append_num (hb_vector_t<char> *buf,
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

  if (!(v == v) || !std::isfinite (v))
  {
    hb_buf_append_c (buf, '0');
    return;
  }

  static const char float_formats[13][6] = {
    "%.0f",  "%.1f",  "%.2f",  "%.3f",  "%.4f",  "%.5f",  "%.6f",
    "%.7f",  "%.8f",  "%.9f",  "%.10f", "%.11f", "%.12f",
  };
  char out[128];
  snprintf (out, sizeof (out), float_formats[effective_precision], (double) v);

  const char *decimal_point = hb_vector_decimal_point_get ();

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

  hb_buf_append_len (buf, out, (unsigned) strlen (out));
}

static inline unsigned
hb_vector_scale_precision (unsigned precision)
{
  return precision < 7 ? 7 : precision;
}

struct hb_buf_t : hb_vector_t<char>
{
  unsigned precision = 2;

  void append_num (float v)
  { hb_buf_append_num (this, v, precision); }

  void append_num (float v, unsigned p)
  { hb_buf_append_num (this, v, p); }

  bool append_c (char ch)
  { return push_or_fail (ch); }

  bool append_str (const char *s)
  { return hb_buf_append_len (this, s, (unsigned) strlen (s)); }

  bool append_len (const char *s, unsigned l)
  { return hb_buf_append_len (this, s, l); }

  bool append_unsigned (unsigned v)
  {
    char tmp[16];
    snprintf (tmp, sizeof (tmp), "%u", v);
    return hb_buf_append_len (this, tmp, (unsigned) strlen (tmp));
  }

  bool append_hex_byte (unsigned v)
  {
    char tmp[2] = {"0123456789ABCDEF"[(v >> 4) & 15],
		   "0123456789ABCDEF"[v & 15]};
    return append_len (tmp, 2);
  }

  void append_svg_color (hb_color_t color, bool with_alpha)
  {
    unsigned r = hb_color_get_red (color);
    unsigned g = hb_color_get_green (color);
    unsigned b = hb_color_get_blue (color);
    unsigned a = hb_color_get_alpha (color);
    append_c ('#');
    if (((r >> 4) == (r & 0xF)) &&
	((g >> 4) == (g & 0xF)) &&
	((b >> 4) == (b & 0xF)))
    {
      append_c ("0123456789ABCDEF"[r & 0xF]);
      append_c ("0123456789ABCDEF"[g & 0xF]);
      append_c ("0123456789ABCDEF"[b & 0xF]);
    }
    else
    {
      append_hex_byte (r);
      append_hex_byte (g);
      append_hex_byte (b);
    }
    if (with_alpha && a != 255)
    {
      append_str ("\" fill-opacity=\"");
      append_num (a / 255.f, 4);
    }
  }

  bool append_base64 (const uint8_t *data, unsigned len)
  {
    unsigned out_len = ((len + 2) / 3) * 4;
    unsigned old_len = length;
    if (unlikely (!resize_dirty ((int) (old_len + out_len))))
      return false;

    char *dst = arrayZ + old_len;
    unsigned di = 0;
    unsigned i = 0;
    while (i + 2 < len)
    {
      unsigned v = ((unsigned) data[i] << 16) |
		   ((unsigned) data[i + 1] << 8) |
		   ((unsigned) data[i + 2]);
      dst[di++] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(v >> 18) & 63];
      dst[di++] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(v >> 12) & 63];
      dst[di++] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(v >> 6) & 63];
      dst[di++] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[v & 63];
      i += 3;
    }

    if (i < len)
    {
      unsigned v = (unsigned) data[i] << 16;
      if (i + 1 < len)
	v |= (unsigned) data[i + 1] << 8;
      dst[di++] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(v >> 18) & 63];
      dst[di++] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(v >> 12) & 63];
      dst[di++] = (i + 1 < len) ? "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(v >> 6) & 63] : '=';
      dst[di++] = '=';
    }

    return true;
  }
};

#endif /* HB_VECTOR_UTILS_HH */
