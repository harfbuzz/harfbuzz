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

/* PDF shadings do not repeat, so a linear or radial gradient whose
 * color line repeats or reflects is written one period at a time.
 * Check which periods are written, and how. */

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <string.h>

#include <hb.h>
#include <hb-vector.h>

/* ---- a color line of two stops ---- */

typedef struct
{
  hb_color_t colors[2];
  hb_paint_extend_t extend;
} line_t;

static unsigned
get_stops (hb_color_line_t *color_line,
	   void *color_line_data,
	   unsigned start,
	   unsigned *count,
	   hb_color_stop_t *stops,
	   void *user_data)
{
  line_t *line = (line_t *) color_line_data;
  (void) color_line;
  (void) user_data;
  if (count)
  {
    unsigned n = 0;
    for (unsigned i = start; i < 2 && n < *count; i++, n++)
    {
      stops[n].offset = (float) i;
      stops[n].is_foreground = 0;
      stops[n].color = line->colors[i];
    }
    *count = n;
  }
  return 2;
}

static hb_paint_extend_t
get_extend (hb_color_line_t *color_line,
	    void *color_line_data,
	    void *user_data)
{
  (void) color_line;
  (void) user_data;
  return ((line_t *) color_line_data)->extend;
}

/* ---- a picture ---- */

static hb_vector_paint_t *paint;
static hb_paint_funcs_t *funcs;
static line_t line;
static hb_color_line_t color_line;

static void
begin (hb_paint_extend_t extend, unsigned alpha)
{
  paint = hb_vector_paint_create_or_fail (HB_VECTOR_FORMAT_PDF);
  assert (paint);
  hb_vector_extents_t extents = {-100, -100, 200, 200};
  hb_vector_paint_set_extents (paint, &extents);
  funcs = hb_vector_paint_get_funcs (paint);

  line.colors[0] = HB_COLOR (0, 0, 255, 255);
  line.colors[1] = HB_COLOR (255, 0, 0, alpha);
  line.extend = extend;
  memset (&color_line, 0, sizeof (color_line));
  color_line.data = &line;
  color_line.get_color_stops = get_stops;
  color_line.get_extend = get_extend;
}

/* A gradient along the x axis from @x0 to @x1. */
static void
linear (float x0, float x1)
{
  hb_paint_linear_gradient (funcs, paint, &color_line,
			    x0, 0, x1, 0, x0, x1 - x0);
}

/* What was painted: the document, and the content of its page. */
static hb_blob_t *blob;
static const char *doc, *page;
static unsigned doc_len, page_len;

static const char *
find_in (const char *data, unsigned len, const char *needle)
{
  unsigned n = (unsigned) strlen (needle);
  for (unsigned i = 0; i + n <= len; i++)
    if (!memcmp (data + i, needle, n))
      return data + i;
  return NULL;
}

static unsigned
count_in (const char *data, unsigned len, const char *needle)
{
  unsigned n = (unsigned) strlen (needle), count = 0;
  for (unsigned i = 0; i + n <= len; i++)
    if (!memcmp (data + i, needle, n))
      count++;
  return count;
}

static void
end (void)
{
  blob = hb_vector_paint_render (paint);
  assert (blob);
  hb_vector_paint_destroy (paint);
  doc = hb_blob_get_data (blob, &doc_len);

  page = find_in (doc, doc_len, "\n4 0 obj\n");
  assert (page);
  page = find_in (page, doc_len - (unsigned) (page - doc), "stream\n");
  assert (page);
  page += strlen ("stream\n");
  const char *stop = find_in (page, doc_len - (unsigned) (page - doc), "endstream");
  assert (stop);
  page_len = (unsigned) (stop - page);
}

#define IN_PAGE(s) (find_in (page, page_len, s) != NULL)
#define IN_DOC(s) (find_in (doc, doc_len, s) != NULL)
#define PAGE_COUNT(s) count_in (page, page_len, s)
#define DOC_COUNT(s) count_in (doc, doc_len, s)

int
main (void)
{
  /* Ten periods fill the clip: one shading, painted in ten places. */
  begin (HB_PAINT_EXTEND_REPEAT, 255);
  hb_paint_push_clip_rectangle (funcs, paint, 0, 0, 100, 10);
  linear (0, 10);
  hb_paint_pop_clip (funcs, paint);
  end ();
  assert (PAGE_COUNT (" sh\n") == 10);
  assert (DOC_COUNT ("/ShadingType 2") == 1);
  assert (IN_PAGE ("1 0 0 1 10 0 cm\n"));
  assert (IN_PAGE ("1 0 0 1 90 0 cm\n"));
  assert (!IN_PAGE ("1 0 0 1 100 0 cm\n"));
  assert (!IN_DOC ("/Extend"));
  hb_blob_destroy (blob);

  /* The color line is backwards in every other one if it reflects,
   * also before its own start. */
  begin (HB_PAINT_EXTEND_REFLECT, 255);
  hb_paint_push_clip_rectangle (funcs, paint, 0, 0, 100, 10);
  linear (25, 35);
  hb_paint_pop_clip (funcs, paint);
  end ();
  assert (PAGE_COUNT (" sh\n") == 11);
  assert (DOC_COUNT ("/ShadingType 2") == 2);
  assert (IN_DOC ("/Encode [0 1 1 0]"));
  assert (IN_DOC ("/Encode [1 0 0 1]"));
  assert (IN_PAGE ("1 0 0 1 -30 0 cm\n/SH1 sh\n"));
  assert (IN_PAGE ("1 0 0 1 -20 0 cm\n/SH0 sh\n"));
  assert (IN_PAGE ("1 0 0 1 70 0 cm\n/SH1 sh\n"));
  assert (!IN_PAGE ("1 0 0 1 80 0 cm\n"));
  hb_blob_destroy (blob);

  /* A color line much shorter than what it fills: no more periods
   * than the limit, around the color line itself. */
  begin (HB_PAINT_EXTEND_REPEAT, 255);
  hb_paint_push_clip_rectangle (funcs, paint, 0, 0, 100, 10);
  linear (50, 50.01f);
  hb_paint_pop_clip (funcs, paint);
  end ();
  assert (PAGE_COUNT (" sh\n") == 256);
  assert (DOC_COUNT ("/ShadingType 2") == 1);
  hb_blob_destroy (blob);

  /* The same goes for a gradient that nothing clips. */
  begin (HB_PAINT_EXTEND_REPEAT, 255);
  linear (0, 10);
  end ();
  assert (PAGE_COUNT (" sh\n") == 256);
  assert (IN_PAGE ("1 0 0 1 -1280 0 cm\n"));
  assert (IN_PAGE ("1 0 0 1 1270 0 cm\n"));
  hb_blob_destroy (blob);

  /* Nothing is painted under a clip that lets nothing through. */
  begin (HB_PAINT_EXTEND_REPEAT, 255);
  hb_paint_push_clip_rectangle (funcs, paint, 10, 0, 10, 10);
  linear (0, 10);
  hb_paint_pop_clip (funcs, paint);
  end ();
  assert (PAGE_COUNT (" sh\n") == 0);
  assert (DOC_COUNT ("/ShadingType") == 0);
  hb_blob_destroy (blob);

  /* Under a transform that rotates, the periods are painted in the
   * space of the root: the transform is undone, and they step along
   * the rotated line.  The bounds of the clip are kept in that space,
   * where they are wider than the clip, by a period at each end. */
  begin (HB_PAINT_EXTEND_REPEAT, 255);
  hb_paint_push_transform (funcs, paint, 0.8f, 0.6f, -0.6f, 0.8f, 0, 0);
  hb_paint_push_clip_rectangle (funcs, paint, 0, 0, 100, 10);
  linear (0, 10);
  hb_paint_pop_clip (funcs, paint);
  hb_paint_pop_transform (funcs, paint);
  end ();
  assert (PAGE_COUNT (" sh\n") == 12);
  assert (IN_PAGE ("0.8 -0.6 0.6 0.8 0 0 cm\n"));
  assert (IN_PAGE ("1 0 0 1 -8 -6 cm\n"));
  assert (IN_PAGE ("1 0 0 1 80 60 cm\n"));
  hb_blob_destroy (blob);

  /* Through a soft mask of their alphas, the periods are painted as
   * one group. */
  begin (HB_PAINT_EXTEND_REPEAT, 128);
  hb_paint_push_clip_rectangle (funcs, paint, 0, 0, 100, 10);
  linear (0, 10);
  hb_paint_pop_clip (funcs, paint);
  end ();
  assert (PAGE_COUNT (" sh\n") == 0);
  assert (PAGE_COUNT (" Do\n") == 1);
  assert (PAGE_COUNT (" gs\n") == 1);
  assert (DOC_COUNT (" sh\n") == 20);
  assert (DOC_COUNT ("/ShadingType 2 /ColorSpace /DeviceRGB") == 1);
  assert (DOC_COUNT ("/ShadingType 2 /ColorSpace /DeviceGray") == 1);
  assert (IN_DOC ("/SMask << /Type /Mask /S /Luminosity"));
  hb_blob_destroy (blob);

  /* Circles around one center are rings: a shading each, with its hole
   * filled, from the outside in.  They are painted on top of each
   * other, so as one group. */
  begin (HB_PAINT_EXTEND_REPEAT, 255);
  hb_paint_push_clip_rectangle (funcs, paint, -50, -50, 50, 50);
  hb_paint_radial_gradient (funcs, paint, &color_line, 0, 0, 0, 0, 0, 10);
  hb_paint_pop_clip (funcs, paint);
  end ();
  assert (PAGE_COUNT (" sh\n") == 0);
  assert (PAGE_COUNT (" Do\n") == 1);
  assert (PAGE_COUNT (" gs\n") == 0);
  assert (DOC_COUNT (" sh\n") == 8);
  assert (DOC_COUNT ("/ShadingType 3") == 8);
  assert (DOC_COUNT ("/Extend [true false]") == 8);
  assert (IN_DOC ("/SH0 sh\n/SH1 sh\n"));
  assert (find_in (doc, doc_len, "/Coords [0 0 70 0 0 80]") <
	  find_in (doc, doc_len, "/Coords [0 0 0 0 0 10.16]"));
  hb_blob_destroy (blob);

  /* Rings that shrink along the color line are written growing, with
   * the color line backwards. */
  begin (HB_PAINT_EXTEND_REPEAT, 255);
  hb_paint_push_clip_rectangle (funcs, paint, -50, -50, 50, 50);
  hb_paint_radial_gradient (funcs, paint, &color_line, 0, 0, 20, 0, 0, 10);
  hb_paint_pop_clip (funcs, paint);
  end ();
  assert (PAGE_COUNT (" Do\n") == 1);
  assert (DOC_COUNT (" sh\n") == 8);
  assert (DOC_COUNT ("/Extend [true false]") == 8);
  assert (IN_DOC ("/Encode [1 0 1 0]"));
  assert (find_in (doc, doc_len, "/Coords [0 0 70 0 0 80]") <
	  find_in (doc, doc_len, "/Coords [0 0 0 0 0 10.16]"));
  hb_blob_destroy (blob);

  /* Circles that are not inside one another are painted later over
   * earlier, as they are, each period half way into the next, and
   * stop where the radius is zero. */
  begin (HB_PAINT_EXTEND_REPEAT, 255);
  hb_paint_push_clip_rectangle (funcs, paint, -50, -50, 50, 50);
  hb_paint_radial_gradient (funcs, paint, &color_line, 0, 0, 5, 20, 0, 10);
  hb_paint_pop_clip (funcs, paint);
  end ();
  assert (PAGE_COUNT (" Do\n") == 1);
  assert (!IN_DOC ("/Extend"));
  assert (IN_DOC ("/Coords [-20 0 0 10 0 7.5]\n"));
  assert (find_in (doc, doc_len, "/Coords [-20 0 0 10 0 7.5]") <
	  find_in (doc, doc_len, "/Coords [0 0 5 30 0 12.5]"));
  hb_blob_destroy (blob);

  return 0;
}
