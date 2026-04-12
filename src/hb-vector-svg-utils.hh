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

#ifndef HB_VECTOR_SVG_UTILS_HH
#define HB_VECTOR_SVG_UTILS_HH

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

static inline hb_color_t
hb_color_lerp (hb_color_t c0, hb_color_t c1, float t)
{
  auto lerp = [&] (unsigned shift) -> unsigned {
    unsigned v0 = (c0 >> shift) & 0xFF;
    unsigned v1 = (c1 >> shift) & 0xFF;
    return (unsigned) (v0 + t * ((float) v1 - (float) v0) + 0.5f);
  };
  return HB_COLOR (lerp (0), lerp (8), lerp (16), lerp (24));
}

/* Iterate sweep gradient color stop tiling across the full 0..2π range,
 * calling emit_patch (a0, c0, a1, c1) for each sector.
 *
 * Handles pad, repeat, and reflect extend modes.  Stops must be
 * pre-sorted by offset. */
template <typename Func>
static inline void
hb_vector_sweep_gradient_tiles (hb_color_stop_t *stops,
			 unsigned n_stops,
			 hb_paint_extend_t extend,
			 float start_angle,
			 float end_angle,
			 Func emit_patch)
{
  if (!n_stops) return;

  if (start_angle == end_angle)
  {
    if (extend == HB_PAINT_EXTEND_PAD)
    {
      if (start_angle > 0.f)
	emit_patch (0.f, stops[0].color, start_angle, stops[0].color);
      if (end_angle < HB_2_PI)
	emit_patch (end_angle, stops[n_stops - 1].color, HB_2_PI, stops[n_stops - 1].color);
    }
    return;
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
      return;
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
	  color0 = hb_color_lerp (colors[pos - 1], colors[pos], f);
	}
	break;
      }
    }
    if (pos == n_stops)
    {
      color0 = colors[n_stops - 1];
      emit_patch (0.f, color0, HB_2_PI, color0);
      goto done;
    }
    emit_patch (0.f, color0, angles[pos], colors[pos]);
    for (pos++; pos < n_stops; pos++)
    {
      if (angles[pos] <= HB_2_PI)
	emit_patch (angles[pos - 1], colors[pos - 1], angles[pos], colors[pos]);
      else
      {
	float f = (HB_2_PI - angles[pos - 1]) / (angles[pos] - angles[pos - 1]);
	hb_color_t color1 = hb_color_lerp (colors[pos - 1], colors[pos], f);
	emit_patch (angles[pos - 1], colors[pos - 1], HB_2_PI, color1);
	break;
      }
    }
    if (pos == n_stops)
    {
      color0 = colors[n_stops - 1];
      emit_patch (angles[n_stops - 1], color0, HB_2_PI, color0);
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
	  hb_color_t c = hb_color_lerp (col0, col1, f);
	  emit_patch (0.f, c, a1_l, col1);
	}
	else if (a1_l >= HB_2_PI)
	{
	  float f = (HB_2_PI - a0_l) / (a1_l - a0_l);
	  hb_color_t c = hb_color_lerp (col0, col1, f);
	  emit_patch (a0_l, col0, HB_2_PI, c);
	  goto done;
	}
	else
	  emit_patch (a0_l, col0, a1_l, col1);
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

#endif /* HB_VECTOR_SVG_UTILS_HH */
