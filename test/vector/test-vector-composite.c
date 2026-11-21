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

/* Compositing modes in vector paint output.  What the documents look
 * like when rendered is not checked here: these tests are about which
 * of the two groups a mode keeps, and about the document staying whole
 * however the groups, transforms and clips are nested. */

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hb.h>
#include <hb-vector.h>

#define RED  HB_COLOR (0, 0, 255, 255)
#define BLUE HB_COLOR (255, 0, 0, 255)

#define GREEN HB_COLOR (0, 255, 0, 255)

/* How the two colors are written in SVG and in PDF. */
#define SVG_RED  "fill=\"#F00\""
#define SVG_BLUE "fill=\"#00F\""
#define PDF_RED  "\n1 0 0 rg\n"
#define PDF_BLUE "\n0 0 1 rg\n"

typedef void (*scene_func_t) (hb_paint_funcs_t *funcs, void *paint, int arg);

static char *
render (hb_vector_format_t format, scene_func_t scene, int arg)
{
  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (format);
  assert (paint);
  hb_vector_extents_t extents = {0, 0, 100, 100};
  hb_vector_paint_set_extents (paint, &extents);

  scene (hb_vector_paint_get_funcs (paint), paint, arg);

  hb_blob_t *blob = hb_vector_paint_render (paint);
  assert (blob);
  unsigned len;
  const char *data = hb_blob_get_data (blob, &len);
  assert (data && len);
  char *copy = (char *) malloc (len + 1);
  assert (copy);
  memcpy (copy, data, len);
  copy[len] = '\0';
  hb_blob_destroy (blob);
  hb_vector_paint_destroy (paint);
  return copy;
}

/* strstr(), for a string that ends at @end.  The documents here run to
 * megabytes, and the strstr() of the sanitizers measures the whole
 * string at every call: called once per match, that is quadratic. */
static const char *
find (const char *p, const char *end, const char *text)
{
  size_t len = strlen (text);
  while ((size_t) (end - p) >= len &&
	 (p = (const char *) memchr (p, text[0], (size_t) (end - p) - len + 1)))
  {
    if (!memcmp (p, text, len))
      return p;
    p++;
  }
  return NULL;
}

static unsigned
count (const char *doc, const char *text)
{
  unsigned n = 0;
  size_t len = strlen (text);
  const char *end = doc + strlen (doc);
  for (const char *p = doc; (p = find (p, end, text)); p += len)
    n++;
  return n;
}

/* Asserts that @doc is well-formed as far as its elements go: every tag
 * is closed, in order, and every id referred to is defined. */
static void
check_svg (const char *doc)
{
  const char *stack[256];
  size_t stack_len[256];
  unsigned depth = 0;

  for (const char *p = doc; (p = strchr (p, '<')); )
  {
    const char *end = strchr (p, '>');
    assert (end);
    if (p[1] == '/')
    {
      assert (depth);
      depth--;
      assert ((size_t) (end - (p + 2)) == stack_len[depth] &&
	      !strncmp (p + 2, stack[depth], stack_len[depth]));
    }
    else if (end[-1] != '/')
    {
      size_t n = strcspn (p + 1, " >");
      assert (depth < 256);
      stack[depth] = p + 1;
      stack_len[depth] = n;
      depth++;
    }
    p = end + 1;
  }
  assert (!depth);

  const char *doc_end = doc + strlen (doc);
  static const char *const refs[] = {"url(#", "href=\"#"};
  for (unsigned i = 0; i < 2; i++)
    for (const char *p = doc; (p = find (p, doc_end, refs[i])); )
    {
      p += strlen (refs[i]);
      size_t n = strcspn (p, ")\"");
      char def[64];
      assert (n < sizeof (def) - 8);
      snprintf (def, sizeof (def), " id=\"%.*s\"", (int) n, p);
      assert (find (doc, doc_end, def));
    }
}

/* Asserts that @doc holds together as a PDF: every content stream
 * restores as many graphics states as it saves, every resource that is
 * used is in the resource dictionary, and every object referred to
 * exists. */
static void
check_pdf (const char *doc)
{
  unsigned n_objects = count (doc, " 0 obj\n");
  assert (n_objects >= 4);
  const char *doc_end = doc + strlen (doc);

  for (const char *p = doc; (p = find (p, doc_end, " 0 R")); p += 4)
  {
    const char *q = p;
    while (q > doc && q[-1] >= '0' && q[-1] <= '9')
      q--;
    unsigned id = (unsigned) atoi (q);
    assert (id >= 1 && id <= n_objects);
  }

  for (const char *p = doc; (p = find (p, doc_end, "\nstream\n")); )
  {
    p += 8;
    const char *end = find (p, doc_end, "endstream");
    assert (end);
    int depth = 0;
    while (p < end)
    {
      const char *eol = strchr (p, '\n');
      assert (eol && eol <= end);
      size_t len = (size_t) (eol - p);
      if (len == 1 && p[0] == 'q')
	depth++;
      else if (len == 1 && p[0] == 'Q')
      {
	depth--;
	assert (depth >= 0);
      }
      else if ((len > 3 && !strncmp (eol - 3, " Do", 3)) ||
	       (len > 3 && !strncmp (eol - 3, " gs", 3)))
      {
	/* "/Fm3 Do" or "/GS2 gs": the name must be defined. */
	char def[32];
	size_t n = len - 3;
	assert (p[0] == '/' && n < sizeof (def) - 1);
	memcpy (def, p, n);
	def[n] = ' ';
	def[n + 1] = '\0';
	const char *found = find (doc, doc_end, def);
	assert (found && found[n + 1] >= '0' && found[n + 1] <= '9');
      }
      p = eol + 1;
    }
    assert (!depth);
  }
}

static void
square (hb_paint_funcs_t *funcs, void *paint,
	float x, float y, hb_color_t color)
{
  hb_paint_push_clip_rectangle (funcs, paint, x, y, x + 50, y + 50);
  hb_paint_color (funcs, paint, 0, color);
  hb_paint_pop_clip (funcs, paint);
}

/* The way COLR composites: a blue backdrop and a red source, in a group
 * of their own. */
static void
composite_scene (hb_paint_funcs_t *funcs, void *paint, int mode)
{
  hb_paint_push_group (funcs, paint);
  square (funcs, paint, 10, 10, BLUE);
  hb_paint_push_group (funcs, paint);
  square (funcs, paint, 40, 40, RED);
  hb_paint_pop_group (funcs, paint, (hb_paint_composite_mode_t) mode);
  hb_paint_pop_group (funcs, paint, HB_PAINT_COMPOSITE_MODE_SRC_OVER);
}

static void
test_modes_svg (void)
{
  for (int mode = HB_PAINT_COMPOSITE_MODE_CLEAR; mode <= HB_PAINT_COMPOSITE_MODE_PLUS; mode++)
  {
    char *doc = render (HB_VECTOR_FORMAT_SVG, composite_scene, mode);
    check_svg (doc);

    const char *body = strstr (doc, "</defs>");
    assert (body);
    unsigned red = count (doc, SVG_RED), blue = count (doc, SVG_BLUE);
    unsigned body_red = count (body, SVG_RED), body_blue = count (body, SVG_BLUE);
    const char *first_red = strstr (body, SVG_RED), *first_blue = strstr (body, SVG_BLUE);

    switch (mode)
    {
      case HB_PAINT_COMPOSITE_MODE_CLEAR:
	assert (!red && !blue);
	break;
      case HB_PAINT_COMPOSITE_MODE_SRC:
	assert (red == 1 && !blue);
	break;
      case HB_PAINT_COMPOSITE_MODE_DEST:
	assert (!red && blue == 1);
	break;
      case HB_PAINT_COMPOSITE_MODE_SRC_OVER:
	assert (body_red == 1 && body_blue == 1 && first_blue < first_red);
	assert (!strstr (doc, "<mask"));
	break;
      case HB_PAINT_COMPOSITE_MODE_DEST_OVER:
	assert (body_red == 1 && body_blue == 1 && first_red < first_blue);
	assert (!strstr (doc, "<mask"));
	break;
      case HB_PAINT_COMPOSITE_MODE_SRC_IN:
      case HB_PAINT_COMPOSITE_MODE_SRC_OUT:
	/* The source is painted; the backdrop is only a mask. */
	assert (red == 1 && body_red == 1 && blue == 1 && !body_blue);
	assert (count (body, " mask=\"url(#") == 1);
	break;
      case HB_PAINT_COMPOSITE_MODE_DEST_IN:
      case HB_PAINT_COMPOSITE_MODE_DEST_OUT:
	assert (blue == 1 && body_blue == 1 && red == 1 && !body_red);
	assert (count (body, " mask=\"url(#") == 1);
	break;
      case HB_PAINT_COMPOSITE_MODE_SRC_ATOP:
      case HB_PAINT_COMPOSITE_MODE_DEST_ATOP:
      case HB_PAINT_COMPOSITE_MODE_XOR:
	/* Each group is defined once, and used as content and as a mask. */
	assert (red == 1 && blue == 1 && !body_red && !body_blue);
	assert (count (doc, "<use ") == 4 && count (body, "<use ") == 2);
	assert (count (body, "plus-lighter") == 1);
	assert (strstr (body, "isolation:isolate"));
	break;
      case HB_PAINT_COMPOSITE_MODE_PLUS:
	assert (body_red == 1 && body_blue == 1 && first_blue < first_red);
	assert (count (body, "plus-lighter") == 1);
	assert (strstr (body, "isolation:isolate"));
	break;
    }

    /* An inverted mask is what tells OUT from IN. */
    unsigned white = count (doc, "fill=\"#FFF\"/>\n<rect ");
    switch (mode)
    {
      case HB_PAINT_COMPOSITE_MODE_SRC_OUT:
      case HB_PAINT_COMPOSITE_MODE_DEST_OUT:
      case HB_PAINT_COMPOSITE_MODE_SRC_ATOP:
      case HB_PAINT_COMPOSITE_MODE_DEST_ATOP:
	assert (white == 1);
	break;
      case HB_PAINT_COMPOSITE_MODE_XOR:
	assert (white == 2);
	break;
      default:
	assert (!white);
	break;
    }

    free (doc);
  }
}

/* A group composited onto a parent that has a transform and a clip open:
 * the backdrop includes what was painted before them, the source was
 * painted under them, and so is what follows. */
static void
open_scopes_scene (hb_paint_funcs_t *funcs, void *paint, int mode)
{
  square (funcs, paint, 0, 0, BLUE);
  hb_paint_push_transform (funcs, paint, 0.5f, 0, 0, 0.5f, 7, 9);
  hb_paint_push_clip_rectangle (funcs, paint, 0, 0, 90, 90);
  square (funcs, paint, 20, 20, BLUE);
  hb_paint_push_group (funcs, paint);
  square (funcs, paint, 40, 40, RED);
  hb_paint_pop_group (funcs, paint, (hb_paint_composite_mode_t) mode);
  square (funcs, paint, 30, 30, RED);
  hb_paint_pop_clip (funcs, paint);
  hb_paint_pop_transform (funcs, paint);
}

static void
test_open_scopes_svg (void)
{
  static const char transform[] = "<g transform=\"matrix(0.5,0,0,0.5,7,9)\">";
  for (int mode = HB_PAINT_COMPOSITE_MODE_CLEAR; mode <= HB_PAINT_COMPOSITE_MODE_HSL_LUMINOSITY; mode++)
  {
    char *doc = render (HB_VECTOR_FORMAT_SVG, open_scopes_scene, mode);
    check_svg (doc);
    const char *body = strstr (doc, "</defs>");
    assert (body);

    /* What follows the group is painted under the transform, whatever
     * became of what preceded it. */
    const char *last_transform = NULL;
    for (const char *p = body; (p = strstr (p, transform)); p++)
      last_transform = p;
    const char *last_red = NULL;
    for (const char *p = body; (p = strstr (p, SVG_RED)); p++)
      last_red = p;
    assert (last_transform && last_red && last_transform < last_red);

    switch (mode)
    {
      case HB_PAINT_COMPOSITE_MODE_CLEAR:
	/* Both blue squares are gone, and so is the source. */
	assert (!count (doc, SVG_BLUE) && count (doc, SVG_RED) == 1);
	assert (count (doc, transform) == 1);
	break;
      case HB_PAINT_COMPOSITE_MODE_SRC:
	assert (!count (doc, SVG_BLUE) && count (doc, SVG_RED) == 2);
	break;
      case HB_PAINT_COMPOSITE_MODE_DEST:
      case HB_PAINT_COMPOSITE_MODE_SRC_OVER:
	/* Nothing to rearrange. */
	assert (count (doc, transform) == 1);
	break;
      case HB_PAINT_COMPOSITE_MODE_SRC_IN:
	/* The backdrop, a mask now, has both blue squares, the second
	 * under the transform; so has the source; and it is open again. */
	assert (count (doc, SVG_BLUE) == 2 && !count (body, SVG_BLUE));
	assert (count (doc, transform) == 3 && count (body, transform) == 2);
	break;
      case HB_PAINT_COMPOSITE_MODE_MULTIPLY:
	/* A blend mode reaches the first blue square only from outside
	 * the clip, which isolates what is in it. */
	assert (count (body, transform) == 3);
	break;
      default:
	break;
    }
    free (doc);
  }
}

/* With no group around them, the result takes the place of everything
 * painted so far, and what blends must not reach the background. */
static void
root_scene (hb_paint_funcs_t *funcs, void *paint, int mode)
{
  square (funcs, paint, 10, 10, BLUE);
  hb_paint_push_group (funcs, paint);
  square (funcs, paint, 40, 40, RED);
  hb_paint_pop_group (funcs, paint, (hb_paint_composite_mode_t) mode);
}

static void
test_root_svg (void)
{
  static const char isolated[] = "<g transform=\"scale(1,-1)\" style=\"isolation:isolate\">";
  static const struct { int mode; hb_bool_t isolated; } tests[] = {
    {HB_PAINT_COMPOSITE_MODE_SRC_OVER, 0},
    {HB_PAINT_COMPOSITE_MODE_SRC_IN, 0},
    {HB_PAINT_COMPOSITE_MODE_DEST_OUT, 0},
    {HB_PAINT_COMPOSITE_MODE_XOR, 1},
    {HB_PAINT_COMPOSITE_MODE_PLUS, 1},
    {HB_PAINT_COMPOSITE_MODE_MULTIPLY, 1},
  };
  for (unsigned i = 0; i < sizeof (tests) / sizeof (tests[0]); i++)
  {
    char *doc = render (HB_VECTOR_FORMAT_SVG, root_scene, tests[i].mode);
    check_svg (doc);
    assert (!strstr (doc, isolated) == !tests[i].isolated);
    free (doc);
  }
}

/* Groups, transforms and clips that are not popped in the order they
 * were pushed: the document is whole all the same. */
static void
unbalanced_scene (hb_paint_funcs_t *funcs, void *paint, int mode)
{
  hb_paint_push_group (funcs, paint);
  square (funcs, paint, 10, 10, BLUE);
  hb_paint_push_group (funcs, paint);
  hb_paint_push_transform (funcs, paint, 2, 0, 0, 2, 0, 0);
  hb_paint_push_clip_rectangle (funcs, paint, 0, 0, 40, 40);
  hb_paint_color (funcs, paint, 0, RED);
  /* The source group is popped with its transform and clip open... */
  hb_paint_pop_group (funcs, paint, (hb_paint_composite_mode_t) mode);
  /* ...which are popped in its parent, where they are not open... */
  hb_paint_pop_clip (funcs, paint);
  hb_paint_pop_transform (funcs, paint);
  hb_paint_pop_group (funcs, paint, HB_PAINT_COMPOSITE_MODE_SRC_OVER);
  /* ...and there are more pops than pushes. */
  hb_paint_pop_group (funcs, paint, (hb_paint_composite_mode_t) mode);
  hb_paint_pop_clip (funcs, paint);
  square (funcs, paint, 60, 60, BLUE);
}

static void
test_unbalanced_svg (void)
{
  for (int mode = HB_PAINT_COMPOSITE_MODE_CLEAR; mode <= HB_PAINT_COMPOSITE_MODE_PLUS; mode++)
  {
    char *doc = render (HB_VECTOR_FORMAT_SVG, unbalanced_scene, mode);
    check_svg (doc);
    free (doc);
  }
}

/* SRC_ATOP on SRC_ATOP on SRC_ATOP: every level uses what is under it
 * twice, so the document takes its viewer twice the work.  @arg is how
 * much is painted at each level. */
#define NESTING 12
static void
nested_scene (hb_paint_funcs_t *funcs, void *paint, int arg)
{
  hb_paint_push_group (funcs, paint);
  hb_paint_push_clip_rectangle (funcs, paint, 0, 0, 100, 100);
  for (int i = 0; i < arg; i++)
    hb_paint_color (funcs, paint, 0, BLUE);
  hb_paint_pop_clip (funcs, paint);
  for (unsigned level = 0; level < NESTING; level++)
  {
    hb_paint_push_group (funcs, paint);
    hb_paint_push_clip_rectangle (funcs, paint, 0, 0, 100, 100);
    for (int i = 0; i < arg; i++)
      hb_paint_color (funcs, paint, 0, RED);
    hb_paint_pop_clip (funcs, paint);
    hb_paint_pop_group (funcs, paint, HB_PAINT_COMPOSITE_MODE_SRC_ATOP);
  }
  hb_paint_pop_group (funcs, paint, HB_PAINT_COMPOSITE_MODE_SRC_OVER);
}

static void
test_nested_svg (void)
{
  /* Small groups can be nested this deep: each level refers to the
   * level under it and to its source, twice each. */
  char *doc = render (HB_VECTOR_FORMAT_SVG, nested_scene, 1);
  check_svg (doc);
  assert (count (doc, "<use ") == 4 * NESTING);
  free (doc);

  /* Large ones cannot: 2^12 times a megabyte is more than a document
   * may hold, so it may not make its viewer render that much either.
   * The levels past that are painted source-over. */
  doc = render (HB_VECTOR_FORMAT_SVG, nested_scene, 12000);
  check_svg (doc);
  unsigned uses = count (doc, "<use ");
  assert (uses && uses < 4 * NESTING && uses % 4 == 0);
  assert (count (doc, SVG_RED) == 12000 * NESTING);
  free (doc);
}

/* A PDF with nothing painted is not rendered, so paint something else
 * first for the modes that leave nothing. */
static void
composite_scene_pdf (hb_paint_funcs_t *funcs, void *paint, int mode)
{
  square (funcs, paint, 0, 0, GREEN);
  composite_scene (funcs, paint, mode);
}

static void
test_modes_pdf (void)
{
  for (int mode = HB_PAINT_COMPOSITE_MODE_CLEAR; mode <= HB_PAINT_COMPOSITE_MODE_PLUS; mode++)
  {
    char *doc = render (HB_VECTOR_FORMAT_PDF, composite_scene_pdf, mode);
    check_pdf (doc);

    unsigned red = count (doc, PDF_RED), blue = count (doc, PDF_BLUE);
    unsigned masks = count (doc, "/SMask"), inverted = count (doc, "/S /Luminosity");
    unsigned screen = count (doc, "/BM /Screen");
    switch (mode)
    {
      case HB_PAINT_COMPOSITE_MODE_CLEAR:
	assert (!red && !blue && !masks && !screen);
	break;
      case HB_PAINT_COMPOSITE_MODE_SRC:
	assert (red == 1 && !blue && !masks && !screen);
	break;
      case HB_PAINT_COMPOSITE_MODE_DEST:
	assert (!red && blue == 1 && !masks && !screen);
	break;
      case HB_PAINT_COMPOSITE_MODE_SRC_OVER:
      case HB_PAINT_COMPOSITE_MODE_DEST_OVER:
	assert (red == 1 && blue == 1 && !masks && !screen);
	break;
      case HB_PAINT_COMPOSITE_MODE_SRC_IN:
      case HB_PAINT_COMPOSITE_MODE_DEST_IN:
      case HB_PAINT_COMPOSITE_MODE_SRC_ATOP:
      case HB_PAINT_COMPOSITE_MODE_DEST_ATOP:
	/* One group through the alpha of the other. */
	assert (red == 1 && blue == 1 && masks == 1 && !inverted && !screen);
	break;
      case HB_PAINT_COMPOSITE_MODE_SRC_OUT:
      case HB_PAINT_COMPOSITE_MODE_DEST_OUT:
	/* Through the inverse of it, which takes a second mask. */
	assert (red == 1 && blue == 1 && masks == 2 && inverted == 1 && !screen);
	break;
      case HB_PAINT_COMPOSITE_MODE_XOR:
	assert (red == 1 && blue == 1 && masks == 4 && inverted == 2 && screen == 1);
	break;
      case HB_PAINT_COMPOSITE_MODE_PLUS:
	assert (red == 1 && blue == 1 && !masks && screen == 1);
	break;
    }

    /* Which group is painted, and which is only a mask, or painted
     * first: the group that the outer one paints last. */
    const char *outer = NULL;
    for (const char *p = doc; (p = strstr (p, "\nstream\n")); p++)
      outer = p;
    assert (outer);
    unsigned painted = count (outer, " Do\n");
    switch (mode)
    {
      case HB_PAINT_COMPOSITE_MODE_SRC_IN:
      case HB_PAINT_COMPOSITE_MODE_SRC_OUT:
      case HB_PAINT_COMPOSITE_MODE_DEST_IN:
      case HB_PAINT_COMPOSITE_MODE_DEST_OUT:
      case HB_PAINT_COMPOSITE_MODE_SRC:
	assert (painted == 1);
	break;
      case HB_PAINT_COMPOSITE_MODE_DEST_OVER:
      case HB_PAINT_COMPOSITE_MODE_SRC_ATOP:
      case HB_PAINT_COMPOSITE_MODE_DEST_ATOP:
      case HB_PAINT_COMPOSITE_MODE_XOR:
	assert (painted == 2);
	break;
      default:
	break;
    }

    free (doc);
  }
}

static void
test_open_scopes_pdf (void)
{
  static const char transform[] = "\n0.5 0 0 0.5 7 9 cm\n";
  for (int mode = HB_PAINT_COMPOSITE_MODE_CLEAR; mode <= HB_PAINT_COMPOSITE_MODE_HSL_LUMINOSITY; mode++)
  {
    char *doc = render (HB_VECTOR_FORMAT_PDF, open_scopes_scene, mode);
    check_pdf (doc);
    switch (mode)
    {
      case HB_PAINT_COMPOSITE_MODE_CLEAR:
	assert (!count (doc, PDF_BLUE) && count (doc, PDF_RED) == 1);
	assert (count (doc, transform) == 1);
	break;
      case HB_PAINT_COMPOSITE_MODE_SRC_IN:
	/* In the backdrop, around the source, and open again. */
	assert (count (doc, PDF_BLUE) == 2 && count (doc, PDF_RED) == 2);
	assert (count (doc, transform) == 3);
	break;
      case HB_PAINT_COMPOSITE_MODE_SRC_OVER:
      case HB_PAINT_COMPOSITE_MODE_MULTIPLY:
	/* Nothing to rearrange: a blend mode blends with all that the
	 * group has painted, whatever is open. */
	assert (count (doc, transform) == 1);
	break;
      default:
	break;
    }
    free (doc);
  }
}

static void
test_root_pdf (void)
{
  static const struct { int mode; hb_bool_t isolated; } tests[] = {
    {HB_PAINT_COMPOSITE_MODE_SRC_OVER, 0},
    {HB_PAINT_COMPOSITE_MODE_SRC_IN, 0},
    {HB_PAINT_COMPOSITE_MODE_DEST_OUT, 0},
    {HB_PAINT_COMPOSITE_MODE_XOR, 1},
    {HB_PAINT_COMPOSITE_MODE_PLUS, 1},
    {HB_PAINT_COMPOSITE_MODE_MULTIPLY, 1},
  };
  for (unsigned i = 0; i < sizeof (tests) / sizeof (tests[0]); i++)
  {
    char *doc = render (HB_VECTOR_FORMAT_PDF, root_scene, tests[i].mode);
    check_pdf (doc);
    /* The page's own content stream is the first.  When something in
     * the root blends, it holds nothing but the root as one group. */
    const char *page = strstr (doc, "\nstream\n");
    assert (page);
    page += 8;
    assert (!strncmp (page, "q\n/Fm", 5) == !!tests[i].isolated);
    if (tests[i].isolated)
    {
      const char *end = strstr (page, "endstream");
      assert (end && count (page, " Do\n") >= 1 && strstr (page, " Do\nQ\n") + 6 == end);
    }
    free (doc);
  }
}

static void
test_unbalanced_pdf (void)
{
  for (int mode = HB_PAINT_COMPOSITE_MODE_CLEAR; mode <= HB_PAINT_COMPOSITE_MODE_PLUS; mode++)
  {
    char *doc = render (HB_VECTOR_FORMAT_PDF, unbalanced_scene, mode);
    check_pdf (doc);
    free (doc);
  }
}

static void
test_nested_pdf (void)
{
  /* Each level paints what is under it, and the source through its
   * alpha. */
  char *doc = render (HB_VECTOR_FORMAT_PDF, nested_scene, 1);
  check_pdf (doc);
  assert (count (doc, "/SMask") == NESTING);
  free (doc);

  doc = render (HB_VECTOR_FORMAT_PDF, nested_scene, 12000);
  check_pdf (doc);
  unsigned masks = count (doc, "/SMask");
  assert (masks && masks < NESTING);
  assert (count (doc, PDF_RED) == 12000 * NESTING);
  free (doc);
}

/* A context gives the same document again after it has rendered one. */
static void
test_reuse (hb_vector_format_t format)
{
  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (format);
  assert (paint);
  hb_blob_t *blobs[2];
  for (unsigned i = 0; i < 2; i++)
  {
    hb_vector_extents_t extents = {0, 0, 100, 100};
    hb_vector_paint_set_extents (paint, &extents);
    composite_scene (hb_vector_paint_get_funcs (paint), paint, HB_PAINT_COMPOSITE_MODE_XOR);
    open_scopes_scene (hb_vector_paint_get_funcs (paint), paint, HB_PAINT_COMPOSITE_MODE_SRC_OUT);
    blobs[i] = hb_vector_paint_render (paint);
    assert (blobs[i]);
  }
  unsigned len0, len1;
  const char *data0 = hb_blob_get_data (blobs[0], &len0);
  const char *data1 = hb_blob_get_data (blobs[1], &len1);
  assert (len0 == len1 && !memcmp (data0, data1, len0));
  hb_blob_destroy (blobs[0]);
  hb_blob_destroy (blobs[1]);
  hb_vector_paint_destroy (paint);
}

int
main (void)
{
  test_modes_svg ();
  test_open_scopes_svg ();
  test_root_svg ();
  test_unbalanced_svg ();
  test_nested_svg ();
  test_reuse (HB_VECTOR_FORMAT_SVG);

  test_modes_pdf ();
  test_open_scopes_pdf ();
  test_root_pdf ();
  test_unbalanced_pdf ();
  test_nested_pdf ();
  test_reuse (HB_VECTOR_FORMAT_PDF);
  return 0;
}
