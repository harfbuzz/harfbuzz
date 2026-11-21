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

/* A clip to an outline that has no points lets nothing through.  In
 * PDF, `W` with no path before it would leave the clip as it was, and
 * what is painted under it would cover the page.
 *
 * A transform that has no inverse leaves no area to paint on.  Not
 * every viewer paints nothing under a `cm` of one, so everything is
 * clipped out instead. */

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <string.h>

#include <hb.h>
#include <hb-vector.h>

static const char *
find (const char *data, unsigned len, const char *needle)
{
  unsigned n = (unsigned) strlen (needle);
  for (unsigned i = 0; i + n <= len; i++)
    if (!memcmp (data + i, needle, n))
      return data + i;
  return NULL;
}

/* Paints a color under a clip path of @points points, and something
 * else so that the document is not empty. */
static hb_blob_t *
render (unsigned points)
{
  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (HB_VECTOR_FORMAT_PDF);
  assert (paint);
  hb_vector_extents_t extents = {0, 0, 100, 100};
  hb_vector_paint_set_extents (paint, &extents);
  hb_paint_funcs_t *funcs = hb_vector_paint_get_funcs (paint);

  void *draw_data;
  hb_draw_funcs_t *draw = hb_paint_push_clip_path_start (funcs, paint, &draw_data);
  assert (draw);
  if (points)
  {
    hb_draw_state_t st = HB_DRAW_STATE_DEFAULT;
    hb_draw_move_to (draw, draw_data, &st, 10, 10);
    hb_draw_line_to (draw, draw_data, &st, 90, 10);
    hb_draw_line_to (draw, draw_data, &st, 50, 90);
    hb_draw_close_path (draw, draw_data, &st);
  }
  hb_paint_push_clip_path_end (funcs, paint);
  hb_paint_color (funcs, paint, 0, HB_COLOR (0, 0, 255, 255));
  hb_paint_pop_clip (funcs, paint);

  hb_paint_push_clip_rectangle (funcs, paint, 0, 0, 10, 10);
  hb_paint_color (funcs, paint, 0, HB_COLOR (255, 0, 0, 255));
  hb_paint_pop_clip (funcs, paint);

  hb_blob_t *blob = hb_vector_paint_render (paint);
  assert (blob);
  hb_vector_paint_destroy (paint);
  return blob;
}

/* Paints a color under a transform, and something else so that the
 * document is not empty. */
static hb_blob_t *
render_under_transform (float xx, float yx, float xy, float yy)
{
  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (HB_VECTOR_FORMAT_PDF);
  assert (paint);
  hb_vector_extents_t extents = {0, 0, 100, 100};
  hb_vector_paint_set_extents (paint, &extents);
  hb_paint_funcs_t *funcs = hb_vector_paint_get_funcs (paint);

  hb_paint_push_transform (funcs, paint, xx, yx, xy, yy, 5, 7);
  hb_paint_push_clip_rectangle (funcs, paint, 20, 20, 80, 80);
  hb_paint_color (funcs, paint, 0, HB_COLOR (0, 0, 255, 255));
  hb_paint_pop_clip (funcs, paint);
  hb_paint_pop_transform (funcs, paint);

  hb_paint_push_clip_rectangle (funcs, paint, 0, 0, 10, 10);
  hb_paint_color (funcs, paint, 0, HB_COLOR (255, 0, 0, 255));
  hb_paint_pop_clip (funcs, paint);

  hb_blob_t *blob = hb_vector_paint_render (paint);
  assert (blob);
  hb_vector_paint_destroy (paint);
  return blob;
}

int
main (void)
{
  unsigned len;
  const char *data;

  /* No points: a path that encloses nothing takes their place. */
  hb_blob_t *blob = render (0);
  data = hb_blob_get_data (blob, &len);
  assert (find (data, len, "q\n0 0 0 0 re\nW n\n"));
  hb_blob_destroy (blob);

  /* A triangle is its own path. */
  blob = render (3);
  data = hb_blob_get_data (blob, &len);
  assert (!find (data, len, "0 0 0 0 re"));
  assert (find (data, len, "h\nW n\n"));
  hb_blob_destroy (blob);

  /* A transform that has an inverse is written. */
  blob = render_under_transform (2, 0, 0, 3);
  data = hb_blob_get_data (blob, &len);
  assert (find (data, len, "q\n2 0 0 3 5 7 cm\n"));
  assert (!find (data, len, "0 0 0 0 re"));
  hb_blob_destroy (blob);

  /* One that has none is not: a scale by zero, a scale by zero one way,
   * and one that puts everything onto a line. */
  static const float none[3][4] = {{0, 0, 0, 0}, {0, 0, 0, 3}, {1, 2, 2, 4}};
  for (unsigned i = 0; i < 3; i++)
  {
    blob = render_under_transform (none[i][0], none[i][1], none[i][2], none[i][3]);
    data = hb_blob_get_data (blob, &len);
    assert (!find (data, len, " 5 7 cm"));
    assert (find (data, len, "q\n0 0 0 0 re W n\n"));
    /* What is painted after it is. */
    assert (find (data, len, "1 0 0 rg"));
    hb_blob_destroy (blob);
  }

  return 0;
}
