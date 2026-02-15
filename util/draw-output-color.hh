/*
 * Copyright © 2026  Google, Inc.
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
 */

/* This file is included inside draw_output_t in draw-output.hh.
 * It provides color font rendering via the hb_paint API → SVG. */

#ifndef HB_PI
#define HB_PI 3.14159265358979323846f
#endif
#ifndef HB_2_PI
#define HB_2_PI (2.f * HB_PI)
#endif

/* ===== SVG Paint Context ===== */

struct svg_paint_data_t
{
  draw_output_t *that;
  hb_font_t *font;
  GString *defs;
  std::vector<GString *> body_stack;

  GString *body () { return body_stack.back (); }

  void push_body ()
  {
    body_stack.push_back (g_string_new (nullptr));
  }

  GString *pop_body ()
  {
    GString *top = body_stack.back ();
    body_stack.pop_back ();
    return top;
  }
};

/* ===== Color / point helpers ===== */

struct color_t { float r, g, b, a; };
struct point_t { float x, y; };

static color_t color_from_hb (hb_color_t c)
{
  return {
    hb_color_get_red (c) / 255.f,
    hb_color_get_green (c) / 255.f,
    hb_color_get_blue (c) / 255.f,
    hb_color_get_alpha (c) / 255.f
  };
}

static void premultiply (color_t *c)
{
  c->r *= c->a;
  c->g *= c->a;
  c->b *= c->a;
}

static void unpremultiply (color_t *c)
{
  if (c->a != 0.f)
  {
    c->r /= c->a;
    c->g /= c->a;
    c->b /= c->a;
  }
}

static void interpolate_colors (color_t *c0, color_t *c1, float k, color_t *c)
{
  premultiply (c0);
  premultiply (c1);
  c->r = c0->r + k * (c1->r - c0->r);
  c->g = c0->g + k * (c1->g - c0->g);
  c->b = c0->b + k * (c1->b - c0->b);
  c->a = c0->a + k * (c1->a - c0->a);
  unpremultiply (c);
}

static float interpolate_f (float f0, float f1, float f)
{
  return f0 + f * (f1 - f0);
}

static float dot (point_t p, point_t q)
{
  return p.x * q.x + p.y * q.y;
}

static point_t normalize_pt (point_t p)
{
  float len = sqrtf (dot (p, p));
  return {p.x / len, p.y / len};
}

static point_t sum (point_t p, point_t q)
{
  return {p.x + q.x, p.y + q.y};
}

static point_t diff (point_t p, point_t q)
{
  return {p.x - q.x, p.y - q.y};
}

static point_t scale_pt (point_t p, float f)
{
  return {p.x * f, p.y * f};
}

/* ===== Format a number into a GString ===== */

void append_num_to (GString *str, float v, int requested_precision) const
{
  float rounded_zero_threshold = 0.5f;
  for (int i = 0; i < requested_precision; i++)
    rounded_zero_threshold *= 0.1f;
  if (fabsf (v) < rounded_zero_threshold)
    v = 0.f;

  char fmt[20];
  g_snprintf (fmt, sizeof (fmt), "%%.%df", requested_precision);
  char buf[128];
  g_ascii_formatd (buf, sizeof (buf), fmt, (double) v);

  char *d = strchr (buf, '.');
  if (d)
  {
    char *end = buf + strlen (buf) - 1;
    while (end > d && *end == '0')
      *end-- = '\0';
    if (end == d)
      *end = '\0';
  }

  g_string_append (str, buf);
}

/* ===== Gradient helpers ===== */

static int cmp_color_stop (const void *p1, const void *p2)
{
  const hb_color_stop_t *c1 = (const hb_color_stop_t *) p1;
  const hb_color_stop_t *c2 = (const hb_color_stop_t *) p2;
  if (c1->offset < c2->offset) return -1;
  if (c1->offset > c2->offset) return 1;
  return 0;
}

static void normalize_color_line (hb_color_stop_t *stops, unsigned len,
				  float *omin, float *omax)
{
  qsort (stops, len, sizeof (hb_color_stop_t), cmp_color_stop);
  float mn = stops[0].offset, mx = stops[0].offset;
  for (unsigned i = 1; i < len; i++)
  {
    if (stops[i].offset < mn) mn = stops[i].offset;
    if (stops[i].offset > mx) mx = stops[i].offset;
  }
  if (mn != mx)
    for (unsigned i = 0; i < len; i++)
      stops[i].offset = (stops[i].offset - mn) / (mx - mn);
  *omin = mn;
  *omax = mx;
}

static void reduce_anchors (float x0, float y0, float x1, float y1,
			    float x2, float y2,
			    float *xx0, float *yy0, float *xx1, float *yy1)
{
  float q2x = x2 - x0, q2y = y2 - y0;
  float q1x = x1 - x0, q1y = y1 - y0;
  float s = q2x * q2x + q2y * q2y;
  if (s < 0.000001f)
  {
    *xx0 = x0; *yy0 = y0;
    *xx1 = x1; *yy1 = y1;
    return;
  }
  float k = (q2x * q1x + q2y * q1y) / s;
  *xx0 = x0; *yy0 = y0;
  *xx1 = x1 - k * q2x; *yy1 = y1 - k * q2y;
}

void get_color_stops (svg_paint_data_t *pd HB_UNUSED,
		      hb_color_line_t *color_line,
		      unsigned *count,
		      hb_color_stop_t **stops)
{
  unsigned len = hb_color_line_get_color_stops (color_line, 0, nullptr, nullptr);
  if (len > *count)
    *stops = (hb_color_stop_t *) malloc (len * sizeof (hb_color_stop_t));
  hb_color_line_get_color_stops (color_line, 0, &len, *stops);

  for (unsigned i = 0; i < len; i++)
    if ((*stops)[i].is_foreground)
      (*stops)[i].color = HB_COLOR (hb_color_get_blue (foreground),
				    hb_color_get_green (foreground),
				    hb_color_get_red (foreground),
				    (unsigned) hb_color_get_alpha ((*stops)[i].color) *
				    hb_color_get_alpha (foreground) / 255);
  *count = len;
}

static const char *extend_mode_str (hb_paint_extend_t ext)
{
  switch (ext)
  {
    case HB_PAINT_EXTEND_PAD:     return "pad";
    case HB_PAINT_EXTEND_REPEAT:  return "repeat";
    case HB_PAINT_EXTEND_REFLECT: return "reflect";
    default: return "pad";
  }
}

void emit_svg_stops (GString *defs, hb_color_stop_t *stops, unsigned len) const
{
  for (unsigned i = 0; i < len; i++)
  {
    color_t c = color_from_hb (stops[i].color);
    g_string_append (defs, "<stop offset=\"");
    append_num_to (defs, stops[i].offset, 4);
    g_string_append_printf (defs,
			    "\" stop-color=\"rgb(%u,%u,%u)\"",
			    (unsigned) roundf (c.r * 255.f),
			    (unsigned) roundf (c.g * 255.f),
			    (unsigned) roundf (c.b * 255.f));
    if (c.a < 1.f)
    {
      g_string_append (defs, " stop-opacity=\"");
      append_num_to (defs, c.a, 4);
      g_string_append_c (defs, '"');
    }
    g_string_append (defs, "/>\n");
  }
}

static void emit_fill_rect (GString *body, const char *fill_attr)
{
  g_string_append_printf (body,
			  "<rect x=\"-32767\" y=\"-32767\" width=\"65534\" height=\"65534\" %s/>\n",
			  fill_attr);
}

/* ===== Composite mode → CSS mix-blend-mode ===== */

static const char *composite_mode_str (hb_paint_composite_mode_t mode)
{
  switch (mode)
  {
    case HB_PAINT_COMPOSITE_MODE_CLEAR:          return nullptr;
    case HB_PAINT_COMPOSITE_MODE_SRC:             return nullptr;
    case HB_PAINT_COMPOSITE_MODE_DEST:            return nullptr;
    case HB_PAINT_COMPOSITE_MODE_SRC_OVER:        return "normal";
    case HB_PAINT_COMPOSITE_MODE_DEST_OVER:       return nullptr;
    case HB_PAINT_COMPOSITE_MODE_SRC_IN:          return nullptr;
    case HB_PAINT_COMPOSITE_MODE_DEST_IN:         return nullptr;
    case HB_PAINT_COMPOSITE_MODE_SRC_OUT:         return nullptr;
    case HB_PAINT_COMPOSITE_MODE_DEST_OUT:        return nullptr;
    case HB_PAINT_COMPOSITE_MODE_SRC_ATOP:        return nullptr;
    case HB_PAINT_COMPOSITE_MODE_DEST_ATOP:       return nullptr;
    case HB_PAINT_COMPOSITE_MODE_XOR:             return nullptr;
    case HB_PAINT_COMPOSITE_MODE_PLUS:            return nullptr;
    case HB_PAINT_COMPOSITE_MODE_SCREEN:          return "screen";
    case HB_PAINT_COMPOSITE_MODE_OVERLAY:         return "overlay";
    case HB_PAINT_COMPOSITE_MODE_DARKEN:          return "darken";
    case HB_PAINT_COMPOSITE_MODE_LIGHTEN:         return "lighten";
    case HB_PAINT_COMPOSITE_MODE_COLOR_DODGE:     return "color-dodge";
    case HB_PAINT_COMPOSITE_MODE_COLOR_BURN:      return "color-burn";
    case HB_PAINT_COMPOSITE_MODE_HARD_LIGHT:      return "hard-light";
    case HB_PAINT_COMPOSITE_MODE_SOFT_LIGHT:      return "soft-light";
    case HB_PAINT_COMPOSITE_MODE_DIFFERENCE:      return "difference";
    case HB_PAINT_COMPOSITE_MODE_EXCLUSION:       return "exclusion";
    case HB_PAINT_COMPOSITE_MODE_MULTIPLY:        return "multiply";
    case HB_PAINT_COMPOSITE_MODE_HSL_HUE:         return "hue";
    case HB_PAINT_COMPOSITE_MODE_HSL_SATURATION:  return "saturation";
    case HB_PAINT_COMPOSITE_MODE_HSL_COLOR:       return "color";
    case HB_PAINT_COMPOSITE_MODE_HSL_LUMINOSITY:  return "luminosity";
    default: return nullptr;
  }
}

/* ===== Sweep gradient decomposition ===== */

#define MAX_ANGLE (HB_PI / 16.f)

void add_sweep_patch (GString *body,
		      float cx, float cy, float radius,
		      float a0, color_t *c0,
		      float a1, color_t *c1) const
{
  point_t center = {cx, cy};
  int num_splits = (int) ceilf (fabsf (a1 - a0) / MAX_ANGLE);
  point_t p0 = {cosf (a0), sinf (a0)};
  color_t color0 = *c0;

  for (int a = 0; a < num_splits; a++)
  {
    float k = (a + 1.f) / num_splits;
    float angle1 = interpolate_f (a0, a1, k);
    color_t color1;
    interpolate_colors (c0, c1, k, &color1);

    point_t p1 = {cosf (angle1), sinf (angle1)};
    point_t sp0 = sum (center, scale_pt (p0, radius));
    point_t sp1 = sum (center, scale_pt (p1, radius));

    point_t A = normalize_pt (sum (p0, p1));
    point_t U = {-A.y, A.x};
    point_t C0 = sum (A, scale_pt (U, dot (diff (p0, A), p0) / dot (U, p0)));
    point_t C1 = sum (A, scale_pt (U, dot (diff (p1, A), p1) / dot (U, p1)));

    point_t sc0 = sum (center, scale_pt (sum (C0, scale_pt (diff (C0, p0), 0.33333f)), radius));
    point_t sc1 = sum (center, scale_pt (sum (C1, scale_pt (diff (C1, p1), 0.33333f)), radius));

    /* Emit a pie-slice path with the interpolated color. */
    color_t mid_color;
    color_t c0_copy = color0, c1_copy = color1;
    interpolate_colors (&c0_copy, &c1_copy, 0.5f, &mid_color);

    g_string_append (body, "<path d=\"M");
    append_num_to (body, center.x, precision);
    g_string_append_c (body, ',');
    append_num_to (body, center.y, precision);
    g_string_append (body, "L");
    append_num_to (body, sp0.x, precision);
    g_string_append_c (body, ',');
    append_num_to (body, sp0.y, precision);
    g_string_append (body, "C");
    append_num_to (body, sc0.x, precision);
    g_string_append_c (body, ',');
    append_num_to (body, sc0.y, precision);
    g_string_append_c (body, ' ');
    append_num_to (body, sc1.x, precision);
    g_string_append_c (body, ',');
    append_num_to (body, sc1.y, precision);
    g_string_append_c (body, ' ');
    append_num_to (body, sp1.x, precision);
    g_string_append_c (body, ',');
    append_num_to (body, sp1.y, precision);
    g_string_append_printf (body,
			    "Z\" fill=\"rgb(%u,%u,%u)\"",
			    (unsigned) roundf (mid_color.r * 255.f),
			    (unsigned) roundf (mid_color.g * 255.f),
			    (unsigned) roundf (mid_color.b * 255.f));
    if (mid_color.a < 1.f)
    {
      g_string_append (body, " fill-opacity=\"");
      append_num_to (body, mid_color.a, 4);
      g_string_append_c (body, '"');
    }
    g_string_append (body, "/>\n");

    p0 = p1;
    color0 = color1;
  }
}

void add_sweep_gradient_patches (GString *body,
				 hb_color_stop_t *stops, unsigned n_stops,
				 hb_paint_extend_t extend,
				 float cx, float cy, float radius,
				 float start_angle, float end_angle) const
{
  color_t colors_buf[16];
  float angles_buf[16];
  color_t *colors = colors_buf;
  float *angles = angles_buf;
  color_t color0, color1;

  if (start_angle == end_angle)
  {
    if (extend == HB_PAINT_EXTEND_PAD)
    {
      color_t c;
      if (start_angle > 0)
      {
	c = color_from_hb (stops[0].color);
	add_sweep_patch (body, cx, cy, radius, 0.f, &c, start_angle, &c);
      }
      if (end_angle < HB_2_PI)
      {
	c = color_from_hb (stops[n_stops - 1].color);
	add_sweep_patch (body, cx, cy, radius, end_angle, &c, HB_2_PI, &c);
      }
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

  if (n_stops > 16)
  {
    angles = (float *) malloc (sizeof (float) * n_stops);
    colors = (color_t *) malloc (sizeof (color_t) * n_stops);
    if (!angles || !colors)
    {
      free (angles);
      free (colors);
      return;
    }
  }

  for (unsigned i = 0; i < n_stops; i++)
  {
    angles[i] = start_angle + stops[i].offset * (end_angle - start_angle);
    colors[i] = color_from_hb (stops[i].color);
  }

  if (extend == HB_PAINT_EXTEND_PAD)
  {
    unsigned pos;
    color0 = colors[0];
    for (pos = 0; pos < n_stops; pos++)
    {
      if (angles[pos] >= 0)
      {
	if (pos > 0)
	{
	  float f = (0.f - angles[pos - 1]) / (angles[pos] - angles[pos - 1]);
	  interpolate_colors (&colors[pos - 1], &colors[pos], f, &color0);
	}
	break;
      }
    }
    if (pos == n_stops)
    {
      color0 = colors[n_stops - 1];
      add_sweep_patch (body, cx, cy, radius, 0.f, &color0, HB_2_PI, &color0);
      goto done;
    }

    add_sweep_patch (body, cx, cy, radius, 0.f, &color0, angles[pos], &colors[pos]);

    for (pos++; pos < n_stops; pos++)
    {
      if (angles[pos] <= HB_2_PI)
	add_sweep_patch (body, cx, cy, radius, angles[pos - 1], &colors[pos - 1], angles[pos], &colors[pos]);
      else
      {
	float f = (HB_2_PI - angles[pos - 1]) / (angles[pos] - angles[pos - 1]);
	interpolate_colors (&colors[pos - 1], &colors[pos], f, &color1);
	add_sweep_patch (body, cx, cy, radius, angles[pos - 1], &colors[pos - 1], HB_2_PI, &color1);
	break;
      }
    }

    if (pos == n_stops)
    {
      color0 = colors[n_stops - 1];
      add_sweep_patch (body, cx, cy, radius, angles[n_stops - 1], &color0, HB_2_PI, &color0);
      goto done;
    }
  }
  else
  {
    int k;
    float span = angles[n_stops - 1] - angles[0];
    if (fabsf (span) < 1e-6f)
      goto done;

    k = 0;
    if (angles[0] >= 0)
    {
      float ss = angles[0];
      while (ss > 0)
      {
	if (span > 0) { ss -= span; k--; }
	else { ss += span; k++; }
      }
    }
    else
    {
      float ee = angles[n_stops - 1];
      while (ee < 0)
      {
	if (span > 0) { ee += span; k++; }
	else { ee -= span; k--; }
      }
    }

    span = fabsf (span);

    for (int l = k; l < 1000; l++)
    {
      for (unsigned i = 1; i < n_stops; i++)
      {
	float a0_l, a1_l;
	color_t *col0, *col1;

	if ((l % 2 != 0) && (extend == HB_PAINT_EXTEND_REFLECT))
	{
	  a0_l = angles[0] + angles[n_stops - 1] - angles[n_stops - 1 - (i - 1)] + l * span;
	  a1_l = angles[0] + angles[n_stops - 1] - angles[n_stops - 1 - i] + l * span;
	  col0 = &colors[n_stops - 1 - (i - 1)];
	  col1 = &colors[n_stops - 1 - i];
	}
	else
	{
	  a0_l = angles[i - 1] + l * span;
	  a1_l = angles[i] + l * span;
	  col0 = &colors[i - 1];
	  col1 = &colors[i];
	}

	if (a1_l < 0) continue;
	if (a0_l < 0)
	{
	  color_t c;
	  float f = (0.f - a0_l) / (a1_l - a0_l);
	  interpolate_colors (col0, col1, f, &c);
	  add_sweep_patch (body, cx, cy, radius, 0.f, &c, a1_l, col1);
	}
	else if (a1_l >= HB_2_PI)
	{
	  color_t c;
	  float f = (HB_2_PI - a0_l) / (a1_l - a0_l);
	  interpolate_colors (col0, col1, f, &c);
	  add_sweep_patch (body, cx, cy, radius, a0_l, col0, HB_2_PI, &c);
	  goto done;
	}
	else
	  add_sweep_patch (body, cx, cy, radius, a0_l, col0, a1_l, col1);
      }
    }
  }

done:
  if (angles != angles_buf) free (angles);
  if (colors != colors_buf) free (colors);
}

#undef MAX_ANGLE

/* ===== Paint callbacks ===== */

static void
paint_push_transform (hb_paint_funcs_t *, svg_paint_data_t *data,
		      float xx, float yx, float xy, float yy, float dx, float dy,
		      void *)
{
  GString *body = data->body ();
  g_string_append (body, "<g transform=\"matrix(");
  data->that->append_num_to (body, xx, data->that->scale_precision ());
  g_string_append_c (body, ',');
  data->that->append_num_to (body, yx, data->that->scale_precision ());
  g_string_append_c (body, ',');
  data->that->append_num_to (body, xy, data->that->scale_precision ());
  g_string_append_c (body, ',');
  data->that->append_num_to (body, yy, data->that->scale_precision ());
  g_string_append_c (body, ',');
  data->that->append_num_to (body, dx, data->that->scale_precision ());
  g_string_append_c (body, ',');
  data->that->append_num_to (body, dy, data->that->scale_precision ());
  g_string_append (body, ")\">\n");
}

static void
paint_pop_transform (hb_paint_funcs_t *, svg_paint_data_t *data,
		     void *)
{
  g_string_append (data->body (), "</g>\n");
}

static void
paint_push_clip_glyph (hb_paint_funcs_t *, svg_paint_data_t *data,
		       hb_codepoint_t glyph, hb_font_t *font,
		       void *)
{
  /* Define outline path once per glyph, reuse for all clips. */
  if (data->that->defined_outlines.insert (glyph).second)
  {
    draw_data_t dd = {data->that, 0.f, 0.f, 1.f, 1.f};
    g_string_set_size (data->that->path, 0);
    hb_font_draw_glyph (font, glyph, data->that->draw_funcs, &dd);

    g_string_append_printf (data->defs,
			    "<path id=\"p%u\" d=\"%s\"/>\n",
			    glyph,
			    data->that->path->str);
  }

  if (data->that->defined_clips.insert (glyph).second)
    g_string_append_printf (data->defs,
			    "<clipPath id=\"clip-g%u\"><use href=\"#p%u\"/></clipPath>\n",
			    glyph, glyph);

  g_string_append_printf (data->body (),
			  "<g clip-path=\"url(#clip-g%u)\">\n",
			  glyph);
}

static void
paint_push_clip_rectangle (hb_paint_funcs_t *, svg_paint_data_t *data,
			   float xmin, float ymin, float xmax, float ymax,
			   void *)
{
  unsigned clip_id = data->that->rect_clip_counter++;

  g_string_append_printf (data->defs, "<clipPath id=\"c%u\"><rect x=\"", clip_id);
  data->that->append_num_to (data->defs, xmin, data->that->precision);
  g_string_append (data->defs, "\" y=\"");
  data->that->append_num_to (data->defs, ymin, data->that->precision);
  g_string_append (data->defs, "\" width=\"");
  data->that->append_num_to (data->defs, xmax - xmin, data->that->precision);
  g_string_append (data->defs, "\" height=\"");
  data->that->append_num_to (data->defs, ymax - ymin, data->that->precision);
  g_string_append (data->defs, "\"/></clipPath>\n");

  g_string_append_printf (data->body (),
			  "<g clip-path=\"url(#c%u)\">\n",
			  clip_id);
}

static void
paint_pop_clip (hb_paint_funcs_t *, svg_paint_data_t *data,
		void *)
{
  g_string_append (data->body (), "</g>\n");
}

static void
paint_color (hb_paint_funcs_t *, svg_paint_data_t *data,
	     hb_bool_t is_foreground, hb_color_t color,
	     void *)
{
  float r, g, b, a;
  if (is_foreground)
  {
    r = hb_color_get_red (data->that->foreground) / 255.f;
    g = hb_color_get_green (data->that->foreground) / 255.f;
    b = hb_color_get_blue (data->that->foreground) / 255.f;
    a = hb_color_get_alpha (data->that->foreground) / 255.f *
	hb_color_get_alpha (color) / 255.f;
  }
  else
  {
    r = hb_color_get_red (color) / 255.f;
    g = hb_color_get_green (color) / 255.f;
    b = hb_color_get_blue (color) / 255.f;
    a = hb_color_get_alpha (color) / 255.f;
  }

  GString *body = data->body ();
  g_string_append_printf (body,
			  "<rect x=\"-32767\" y=\"-32767\" width=\"65534\" height=\"65534\" "
			  "fill=\"rgb(%u,%u,%u)\"",
			  (unsigned) roundf (r * 255.f),
			  (unsigned) roundf (g * 255.f),
			  (unsigned) roundf (b * 255.f));
  if (a < 1.f)
  {
    g_string_append (body, " fill-opacity=\"");
    data->that->append_num_to (body, a, 4);
    g_string_append_c (body, '"');
  }
  g_string_append (body, "/>\n");
}

/* ===== Probe callbacks (no-op, just return success) ===== */

static hb_bool_t
probe_paint_image (hb_paint_funcs_t *, void *,
		   hb_blob_t *, unsigned, unsigned,
		   hb_tag_t, float,
		   hb_glyph_extents_t *,
		   void *)
{
  return true;
}

/* ===== Actual paint callbacks ===== */

static hb_bool_t
paint_image (hb_paint_funcs_t *, svg_paint_data_t *data,
	     hb_blob_t *image, unsigned width, unsigned height,
	     hb_tag_t format, float slant HB_UNUSED,
	     hb_glyph_extents_t *extents,
	     void *)
{
  if (format == HB_TAG ('s','v','g',' '))
  {
    unsigned len;
    const char *svg_data = hb_blob_get_data (image, &len);
    if (!len) return false;

    GString *body = data->body ();
    if (extents)
    {
      g_string_append (body, "<g transform=\"translate(");
      data->that->append_num_to (body, (float) extents->x_bearing, data->that->precision);
      g_string_append_c (body, ',');
      data->that->append_num_to (body, (float) extents->y_bearing, data->that->precision);
      g_string_append (body, ") scale(");
      data->that->append_num_to (body, (float) extents->width / width, data->that->precision);
      g_string_append_c (body, ',');
      data->that->append_num_to (body, (float) extents->height / height, data->that->precision);
      g_string_append (body, ")\">\n");
    }

    g_string_append_len (body, svg_data, len);
    g_string_append_c (body, '\n');

    if (extents)
      g_string_append (body, "</g>\n");

    return true;
  }

  if (format == HB_TAG ('p','n','g',' '))
  {
    if (!extents) return false;

    unsigned len;
    const char *png_data = hb_blob_get_data (image, &len);
    if (!len) return false;

    gchar *b64 = g_base64_encode ((const guchar *) png_data, len);

    GString *body = data->body ();
    g_string_append (body, "<g transform=\"translate(");
    data->that->append_num_to (body, (float) extents->x_bearing, data->that->precision);
    g_string_append_c (body, ',');
    data->that->append_num_to (body, (float) (extents->y_bearing + extents->height), data->that->precision);
    g_string_append (body, ") scale(");
    data->that->append_num_to (body, (float) extents->width / width, data->that->precision);
    g_string_append_c (body, ',');
    data->that->append_num_to (body, (float) extents->height / height, data->that->precision);
    g_string_append (body, ")\">\n");

    g_string_append (body, "<image href=\"data:image/png;base64,");
    g_string_append (body, b64);
    g_string_append (body, "\" width=\"");
    data->that->append_num_to (body, (float) width, data->that->precision);
    g_string_append (body, "\" height=\"");
    data->that->append_num_to (body, (float) height, data->that->precision);
    g_string_append (body, "\"/>\n");

    g_string_append (body, "</g>\n");

    g_free (b64);
    return true;
  }

  return false;
}

static void
paint_linear_gradient (hb_paint_funcs_t *, svg_paint_data_t *data,
		       hb_color_line_t *color_line,
		       float x0, float y0, float x1, float y1, float x2, float y2,
		       void *)
{
  unsigned len = 16;
  hb_color_stop_t stops_buf[16];
  hb_color_stop_t *stops = stops_buf;
  float mn, mx;

  data->that->get_color_stops (data, color_line, &len, &stops);
  normalize_color_line (stops, len, &mn, &mx);

  float xx0, yy0, xx1, yy1;
  reduce_anchors (x0, y0, x1, y1, x2, y2, &xx0, &yy0, &xx1, &yy1);

  float gx0 = xx0 + mn * (xx1 - xx0);
  float gy0 = yy0 + mn * (yy1 - yy0);
  float gx1 = xx0 + mx * (xx1 - xx0);
  float gy1 = yy0 + mx * (yy1 - yy0);

  unsigned grad_id = data->that->gradient_counter++;
  hb_paint_extend_t ext = hb_color_line_get_extend (color_line);

  GString *defs = data->defs;
  g_string_append_printf (defs, "<linearGradient id=\"gr%u\" gradientUnits=\"userSpaceOnUse\" x1=\"", grad_id);
  data->that->append_num_to (defs, gx0, data->that->precision);
  g_string_append (defs, "\" y1=\"");
  data->that->append_num_to (defs, gy0, data->that->precision);
  g_string_append (defs, "\" x2=\"");
  data->that->append_num_to (defs, gx1, data->that->precision);
  g_string_append (defs, "\" y2=\"");
  data->that->append_num_to (defs, gy1, data->that->precision);
  g_string_append_printf (defs, "\" spreadMethod=\"%s\">\n", extend_mode_str (ext));
  data->that->emit_svg_stops (defs, stops, len);
  g_string_append (defs, "</linearGradient>\n");

  char fill_attr[64];
  g_snprintf (fill_attr, sizeof (fill_attr), "fill=\"url(#gr%u)\"", grad_id);
  emit_fill_rect (data->body (), fill_attr);

  if (stops != stops_buf) free (stops);
}

static void
paint_radial_gradient (hb_paint_funcs_t *, svg_paint_data_t *data,
		       hb_color_line_t *color_line,
		       float x0, float y0, float r0,
		       float x1, float y1, float r1,
		       void *)
{
  unsigned len = 16;
  hb_color_stop_t stops_buf[16];
  hb_color_stop_t *stops = stops_buf;
  float mn, mx;

  data->that->get_color_stops (data, color_line, &len, &stops);
  normalize_color_line (stops, len, &mn, &mx);

  float gx0 = x0 + mn * (x1 - x0);
  float gy0 = y0 + mn * (y1 - y0);
  float gr0 = r0 + mn * (r1 - r0);
  float gx1 = x0 + mx * (x1 - x0);
  float gy1 = y0 + mx * (y1 - y0);
  float gr1 = r0 + mx * (r1 - r0);

  unsigned grad_id = data->that->gradient_counter++;
  hb_paint_extend_t ext = hb_color_line_get_extend (color_line);

  GString *defs = data->defs;
  g_string_append_printf (defs, "<radialGradient id=\"gr%u\" gradientUnits=\"userSpaceOnUse\" cx=\"", grad_id);
  data->that->append_num_to (defs, gx1, data->that->precision);
  g_string_append (defs, "\" cy=\"");
  data->that->append_num_to (defs, gy1, data->that->precision);
  g_string_append (defs, "\" r=\"");
  data->that->append_num_to (defs, gr1, data->that->precision);
  g_string_append (defs, "\" fx=\"");
  data->that->append_num_to (defs, gx0, data->that->precision);
  g_string_append (defs, "\" fy=\"");
  data->that->append_num_to (defs, gy0, data->that->precision);
  if (gr0 > 0.f)
  {
    g_string_append (defs, "\" fr=\"");
    data->that->append_num_to (defs, gr0, data->that->precision);
  }
  g_string_append_printf (defs, "\" spreadMethod=\"%s\">\n", extend_mode_str (ext));
  data->that->emit_svg_stops (defs, stops, len);
  g_string_append (defs, "</radialGradient>\n");

  char fill_attr[64];
  g_snprintf (fill_attr, sizeof (fill_attr), "fill=\"url(#gr%u)\"", grad_id);
  emit_fill_rect (data->body (), fill_attr);

  if (stops != stops_buf) free (stops);
}

static void
paint_sweep_gradient (hb_paint_funcs_t *, svg_paint_data_t *data,
		      hb_color_line_t *color_line,
		      float cx, float cy,
		      float start_angle, float end_angle,
		      void *)
{
  unsigned len = 16;
  hb_color_stop_t stops_buf[16];
  hb_color_stop_t *stops = stops_buf;

  data->that->get_color_stops (data, color_line, &len, &stops);
  qsort (stops, len, sizeof (hb_color_stop_t), cmp_color_stop);

  float radius = 32767.f;
  hb_paint_extend_t ext = hb_color_line_get_extend (color_line);

  data->that->add_sweep_gradient_patches (data->body (), stops, len, ext,
					  cx, cy, radius,
					  start_angle, end_angle);

  if (stops != stops_buf) free (stops);
}

static void
paint_push_group (hb_paint_funcs_t *, svg_paint_data_t *data,
		  void *)
{
  data->push_body ();
}

static void
paint_pop_group (hb_paint_funcs_t *, svg_paint_data_t *data,
		 hb_paint_composite_mode_t mode,
		 void *)
{
  GString *group = data->pop_body ();
  GString *body = data->body ();

  const char *blend = composite_mode_str (mode);
  if (blend)
  {
    g_string_append_printf (body, "<g style=\"mix-blend-mode:%s\">\n", blend);
    g_string_append_len (body, group->str, group->len);
    g_string_append (body, "</g>\n");
  }
  else
    g_string_append_len (body, group->str, group->len);

  g_string_free (group, true);
}

static hb_bool_t
paint_color_glyph (hb_paint_funcs_t *, svg_paint_data_t *data,
		   hb_codepoint_t glyph, hb_font_t *font,
		   void *)
{
  hb_font_paint_glyph (font, glyph,
		       data->that->paint_funcs, data,
		       data->that->palette,
		       data->that->foreground);
  return true;
}

/* ===== Try painting a glyph with color; returns true if successful ===== */

bool paint_glyph_to_svg (hb_codepoint_t gid,
			 GString *defs_out,
			 GString *body_out)
{
  svg_paint_data_t pd;
  pd.that = this;
  pd.font = upem_font;
  pd.defs = defs_out;
  pd.body_stack.push_back (body_out);

  return hb_font_paint_glyph_or_fail (upem_font, gid,
				      paint_funcs, &pd,
				      palette, foreground);
}
