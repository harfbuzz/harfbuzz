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

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <string.h>

#include <hb.h>
#include <hb-vector.h>

/* A translucent color is painted through an ExtGState that sets its
 * alpha, and a gradient with translucent stops through one that sets
 * a soft mask.  Neither may still be set for what is painted next. */

static unsigned
gradient_color_stops (hb_color_line_t *cl, void *cl_data,
		      unsigned start, unsigned *count,
		      hb_color_stop_t *stops, void *user_data)
{
  if (count && stops)
  {
    unsigned n = 0;
    for (; n < *count && start + n < 2; n++)
    {
      stops[n].offset = (float) (start + n);
      stops[n].is_foreground = 0;
      stops[n].color = HB_COLOR (0, 255, 0, start + n ? 0 : 255);
    }
    *count = n;
  }
  return 2;
}

static hb_paint_extend_t
gradient_extend (hb_color_line_t *cl, void *cl_data, void *user_data)
{ return HB_PAINT_EXTEND_PAD; }

typedef enum { COLOR, LINEAR, RADIAL, SWEEP, N_FILLS } fill_t;
typedef enum { SAME_SCOPE, NESTED_CLIP, GROUP, N_FOLLOWERS } follower_t;

/* Paints a translucent fill, then something opaque. */
static hb_blob_t *
render (fill_t fill, follower_t follower)
{
  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (HB_VECTOR_FORMAT_PDF);
  assert (paint);
  hb_vector_extents_t extents = {0, 0, 256, 256};
  hb_vector_paint_set_extents (paint, &extents);
  hb_paint_funcs_t *funcs = hb_vector_paint_get_funcs (paint);

  hb_color_line_t line = {0};
  line.get_color_stops = gradient_color_stops;
  line.get_extend = gradient_extend;

  hb_paint_push_clip_rectangle (funcs, paint, 20, 20, 220, 220);
  switch (fill)
  {
    case COLOR:
      hb_paint_color (funcs, paint, 0, HB_COLOR (255, 0, 0, 128));
      break;
    case LINEAR:
      hb_paint_linear_gradient (funcs, paint, &line, 20, 0, 220, 0, 20, 200);
      break;
    case RADIAL:
      hb_paint_radial_gradient (funcs, paint, &line, 120, 120, 0, 120, 120, 100);
      break;
    case SWEEP:
      hb_paint_sweep_gradient (funcs, paint, &line, 120, 120, 0.f, 6.2831853f);
      break;
    case N_FILLS: default: assert (0);
  }
  switch (follower)
  {
    case SAME_SCOPE:
      hb_paint_color (funcs, paint, 0, HB_COLOR (0, 0, 255, 255));
      break;
    case NESTED_CLIP:
      hb_paint_push_clip_rectangle (funcs, paint, 60, 60, 160, 160);
      hb_paint_color (funcs, paint, 0, HB_COLOR (0, 0, 255, 255));
      hb_paint_pop_clip (funcs, paint);
      break;
    case GROUP:
      hb_paint_push_group (funcs, paint);
      hb_paint_color (funcs, paint, 0, HB_COLOR (0, 0, 255, 255));
      hb_paint_pop_group (funcs, paint, HB_PAINT_COMPOSITE_MODE_SRC_OVER);
      break;
    case N_FOLLOWERS: default: assert (0);
  }
  hb_paint_pop_clip (funcs, paint);

  hb_blob_t *blob = hb_vector_paint_render (paint);
  assert (blob);
  hb_vector_paint_destroy (paint);
  return blob;
}

static const char *
find (const char *data, unsigned len, const char *needle)
{
  unsigned n = (unsigned) strlen (needle);
  for (unsigned i = 0; i + n <= len; i++)
    if (!memcmp (data + i, needle, n))
      return data + i;
  return NULL;
}

/* Walks the content stream of the page, and writes for each painting
 * operator a '1' if an ExtGState is set where it paints, '0' if not. */
static void
painted_states (hb_blob_t *blob, char *states, unsigned size)
{
  unsigned len;
  const char *pdf = hb_blob_get_data (blob, &len);
  const char *end = pdf + len;

  const char *p = find (pdf, len, "/Contents ");
  assert (p);
  char header[32] = "\n";
  unsigned n = 1;
  for (p += strlen ("/Contents "); *p >= '0' && *p <= '9' && n < 16; p++)
    header[n++] = *p;
  memcpy (header + n, " 0 obj\n", sizeof (" 0 obj\n"));
  p = find (pdf, len, header);
  assert (p);
  p = find (p, (unsigned) (end - p), "stream\n");
  assert (p);
  p += strlen ("stream\n");
  end = find (p, (unsigned) (end - p), "endstream");
  assert (end);

  hb_bool_t set[64] = {0};
  unsigned depth = 0;
  unsigned count = 0;
  while (p < end)
  {
    const char *token = p;
    while (p < end && *p != ' ' && *p != '\n')
      p++;
    unsigned l = (unsigned) (p - token);
    p++;

    if (l == 1 && *token == 'q')
    {
      assert (depth + 1 < 64);
      set[depth + 1] = set[depth];
      depth++;
    }
    else if (l == 1 && *token == 'Q')
    {
      assert (depth);
      depth--;
    }
    else if (l == 2 && !memcmp (token, "gs", 2))
      set[depth] = 1;
    else if ((l == 1 && *token == 'f') ||
	     (l == 2 && (!memcmp (token, "sh", 2) || !memcmp (token, "Do", 2))))
    {
      assert (count + 1 < size);
      states[count++] = set[depth] ? '1' : '0';
    }
  }
  assert (!depth);
  states[count] = '\0';
}

int
main (void)
{
  for (unsigned fill = 0; fill < N_FILLS; fill++)
    for (unsigned follower = 0; follower < N_FOLLOWERS; follower++)
    {
      hb_blob_t *blob = render ((fill_t) fill, (follower_t) follower);
      char states[16];
      painted_states (blob, states, sizeof (states));
      /* The translucent fill, and what follows it. */
      assert (!strcmp (states, "10"));
      hb_blob_destroy (blob);
    }

  return 0;
}
