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
 */

/* SVG has no sweep gradient, so one is written as a fan of wedges of
 * one color each.  Check that neighbors are close in color however
 * little of the circle the color line takes, that the wedges are not
 * to be antialiased one by one, and that opaque ones overlap for a
 * viewer that does so anyway. */

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <hb.h>
#include <hb-vector.h>

typedef struct
{
  float offset[2];
  hb_color_t color[2];
} line_t;

static unsigned
gradient_color_stops (hb_color_line_t *cl, void *cl_data,
		      unsigned start, unsigned *count,
		      hb_color_stop_t *stops, void *user_data)
{
  const line_t *line = (const line_t *) cl_data;
  if (count && stops)
  {
    unsigned n = 0;
    for (; n < *count && start + n < 2; n++)
    {
      stops[n].offset = line->offset[start + n];
      stops[n].is_foreground = 0;
      stops[n].color = line->color[start + n];
    }
    *count = n;
  }
  return 2;
}

static hb_paint_extend_t
gradient_extend (hb_color_line_t *cl, void *cl_data, void *user_data)
{ return HB_PAINT_EXTEND_PAD; }

/* Paints a sweep all the way around with the stops of @line, and
 * checks its wedges.  Returns how many there are. */
static unsigned
check (line_t *line)
{
  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (HB_VECTOR_FORMAT_SVG);
  assert (paint);
  hb_vector_extents_t extents = {0, 0, 100, 100};
  hb_vector_paint_set_extents (paint, &extents);
  hb_paint_funcs_t *funcs = hb_vector_paint_get_funcs (paint);

  hb_color_line_t color_line = {0};
  color_line.data = line;
  color_line.get_color_stops = gradient_color_stops;
  color_line.get_extend = gradient_extend;

  hb_paint_push_clip_rectangle (funcs, paint, 0, 0, 100, 100);
  hb_paint_sweep_gradient (funcs, paint, &color_line, 50, 50, 0.f, 6.2831853f);
  hb_paint_pop_clip (funcs, paint);

  hb_blob_t *blob = hb_vector_paint_render (paint);
  assert (blob);
  unsigned len;
  const char *data = hb_blob_get_data (blob, &len);
  char *doc = (char *) malloc (len + 1);
  assert (doc);
  memcpy (doc, data, len);
  doc[len] = '\0';
  hb_blob_destroy (blob);
  hb_vector_paint_destroy (paint);

  int opaque = hb_color_get_alpha (line->color[0]) == 255 &&
	       hb_color_get_alpha (line->color[1]) == 255;
  unsigned n = 0, overlapping = 0, groups = 0;

  /* A group of wedges for each stretch of the color line. */
  static const char group[] = "<g shape-rendering=\"crispEdges\">";
  for (const char *g = doc; (g = strstr (g, group)); g++)
  {
    const char *g_end = strstr (g, "</g>");
    assert (g_end);
    groups++;

    unsigned in_group = 0;
    unsigned prev[3] = {0, 0, 0};
    for (const char *p = g; (p = strstr (p, "<path d=\"M")) && p < g_end; p++)
    {
      const char *end = strchr (p, '>');
      assert (end);

      /* Three corners, or four if it reaches under the next wedge. */
      unsigned corners = 1;
      for (const char *q = p; q < end; q++)
	if (*q == 'L')
	  corners++;
      assert (corners == 3 || corners == 4);
      if (corners == 4)
	overlapping++;

      /* fill="#RGB" or fill="#RRGGBB" */
      const char *fill = strstr (p, "fill=\"#");
      assert (fill && fill < end);
      fill += strlen ("fill=\"#");
      unsigned digits = (unsigned) strcspn (fill, "\"");
      unsigned v = (unsigned) strtoul (fill, NULL, 16);
      unsigned c[3];
      if (digits == 3)
      {
	c[0] = ((v >> 8) & 0xF) * 17;
	c[1] = ((v >> 4) & 0xF) * 17;
	c[2] = (v & 0xF) * 17;
      }
      else
      {
	assert (digits == 6);
	c[0] = (v >> 16) & 0xFF;
	c[1] = (v >> 8) & 0xFF;
	c[2] = v & 0xFF;
      }

      /* Neighbors are a few levels apart. */
      if (in_group)
	for (unsigned k = 0; k < 3; k++)
	{
	  int d = (int) c[k] - (int) prev[k];
	  assert (d >= -8 && d <= 8);
	}
      memcpy (prev, c, sizeof (c));
      in_group++;
      n++;
    }
    assert (in_group);
  }
  assert (groups);
  if (opaque)
    assert (overlapping * 2 > n);
  else
    assert (!overlapping);

  free (doc);
  return n;
}

int
main (void)
{
  /* All the way around from red to blue. */
  line_t wide = {{0.f, 1.f}, {HB_COLOR (0, 0, 255, 255), HB_COLOR (255, 0, 0, 255)}};
  assert (check (&wide) >= 60);

  /* The same in a tenth of the circle: as many steps of color there,
   * and the rest padded. */
  line_t narrow = {{.4f, .5f}, {HB_COLOR (0, 0, 255, 255), HB_COLOR (255, 0, 0, 255)}};
  assert (check (&narrow) >= 60);

  /* Translucent wedges do not overlap: that would show. */
  line_t translucent = {{0.f, 1.f}, {HB_COLOR (0, 0, 255, 128), HB_COLOR (255, 0, 0, 128)}};
  assert (check (&translucent) >= 60);

  return 0;
}
