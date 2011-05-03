/*
 * Copyright © 2010  Behdad Esfahbod
 * Copyright © 2011  Google, Inc.
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
 * Google Author(s): Behdad Esfahbod
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <locale.h>

#include <glib.h>

#include <cairo-ft.h>
#include <hb-ft.h>

HB_BEGIN_DECLS


/* Controlled by cmd-line options */
static int margin_t = 10;
static int margin_b = 10;
static int margin_l = 10;
static int margin_r = 10;
static int line_space = 0;
static double font_size = 18;
static const char *fore = "#000000";
static const char *back = "#ffffff";
static const char *text = NULL;
static const char *font_file = NULL;
static const char *out_file = "/dev/stdout";
static const char *direction = NULL;
static const char *script = NULL;
static const char *language = NULL;
static hb_feature_t *features = NULL;
static unsigned int num_features;
static hb_bool_t debug = FALSE;

/* Ugh, global vars.  Ugly, but does the job */
static int width = 0;
static int height = 0;
static cairo_surface_t *surface = NULL;
static cairo_pattern_t *fore_pattern = NULL;
static cairo_pattern_t *back_pattern = NULL;
static FT_Library ft_library;
static FT_Face ft_face;
static cairo_font_face_t *cairo_face;


G_GNUC_NORETURN static void
usage (FILE *f, int status)
{
  fprintf (f, "usage: hb-view [OPTS...] font-file text\n");
  exit (status);
}

G_GNUC_NORETURN static void
version (void)
{
  printf ("hb-view (harfbuzz) %s\n", PACKAGE_VERSION);
  exit (0);
}

static void parse_features (char *s);

static void
parse_opts (int argc, char **argv)
{
  argv[0] = (char *) "hb-view";
  while (1)
    {
      int option_index = 0, c;
      static struct option long_options[] = {
	{"background", 1, 0, 'B'},
	{"debug", 0, &debug, TRUE},
	{"direction", 1, 0, 'd'},
	{"features", 1, 0, 'f'},
	{"font-size", 1, 0, 's'},
	{"foreground", 1, 0, 'F'},
	{"help", 0, 0, 'h'},
	{"language", 1, 0, 'L'},
	{"line-space", 1, 0, 'l'},
	{"margin", 1, 0, 'm'},
	{"output", 1, 0, 'o'},
	{"script", 1, 0, 'S'},
	{"version", 0, 0, 'v'},
	{0, 0, 0, 0}
      };

      c = getopt_long (argc, argv, "", long_options, &option_index);
      if (c == -1)
	break;

      switch (c)
	{
	case 0:
	  break;
	case 'h':
	  usage (stdout, 0);
	  break;
	case 'v':
	  version ();
	  break;
	case 'l':
	  line_space = atoi (optarg);
	  break;
	case 'm':
	  switch (sscanf (optarg, "%d %d %d %d", &margin_t, &margin_r, &margin_b, &margin_l)) {
	    case 1: margin_r = margin_t;
	    case 2: margin_b = margin_t;
	    case 3: margin_l = margin_r;
	  }
	  break;
	case 's':
	  font_size = strtod (optarg, NULL);
	  break;
	case 'f':
	  parse_features (optarg);
	  break;
	case 'F':
	  fore = optarg;
	  break;
	case 'B':
	  back = optarg;
	  break;
	case 't':
	  text = optarg;
	  break;
	case 'd':
	  direction = optarg;
	  break;
	case 'S':
	  script = optarg;
	  break;
	case 'L':
	  language = optarg;
	  break;
	case 'o':
	  out_file = optarg;
	  break;
	case '?':
	  usage (stdout, 1);
	  break;
	default:
	  break;
	}
    }
  if (optind + 2 != argc)
    usage (stderr, 1);
  font_file = argv[optind++];
  text = argv[optind++];
}


static void
parse_space (char **pp)
{
  char c;
#define ISSPACE(c) ((c)==' '||(c)=='\f'||(c)=='\n'||(c)=='\r'||(c)=='\t'||(c)=='\v')
  while (c = **pp, ISSPACE (c))
    (*pp)++;
#undef ISSPACE
}

static hb_bool_t
parse_char (char **pp, char c)
{
  parse_space (pp);

  if (**pp != c)
    return FALSE;

  (*pp)++;
  return TRUE;
}

static hb_bool_t
parse_uint (char **pp, unsigned int *pv)
{
  char *p = *pp;
  unsigned int v;

  v = strtol (p, pp, 0);

  if (p == *pp)
    return FALSE;

  *pv = v;
  return TRUE;
}


static hb_bool_t
parse_feature_value_prefix (char **pp, hb_feature_t *feature)
{
  if (parse_char (pp, '-'))
    feature->value = 0;
  else {
    parse_char (pp, '+');
    feature->value = 1;
  }

  return TRUE;
}

static hb_bool_t
parse_feature_tag (char **pp, hb_feature_t *feature)
{
  char *p = *pp, c;

  parse_space (pp);

#define ISALNUM(c) (('a' <= (c) && (c) <= 'z') || ('A' <= (c) && (c) <= 'Z') || ('0' <= (c) && (c) <= '9'))
  while (c = **pp, ISALNUM(c))
    (*pp)++;
#undef ISALNUM

  if (p == *pp)
    return FALSE;

  **pp = '\0';
  feature->tag = hb_tag_from_string (p);
  **pp = c;

  return TRUE;
}

static hb_bool_t
parse_feature_indices (char **pp, hb_feature_t *feature)
{
  hb_bool_t has_start;

  feature->start = 0;
  feature->end = (unsigned int) -1;

  if (!parse_char (pp, '['))
    return TRUE;

  has_start = parse_uint (pp, &feature->start);

  if (parse_char (pp, ':')) {
    parse_uint (pp, &feature->end);
  } else {
    if (has_start)
      feature->end = feature->start + 1;
  }

  return parse_char (pp, ']');
}

static hb_bool_t
parse_feature_value_postfix (char **pp, hb_feature_t *feature)
{
  return !parse_char (pp, '=') || parse_uint (pp, &feature->value);
}


static hb_bool_t
parse_one_feature (char **pp, hb_feature_t *feature)
{
  return parse_feature_value_prefix (pp, feature) &&
	 parse_feature_tag (pp, feature) &&
	 parse_feature_indices (pp, feature) &&
	 parse_feature_value_postfix (pp, feature) &&
	 (parse_char (pp, ',') || **pp == '\0');
}

static void
skip_one_feature (char **pp)
{
  char *e;
  e = strchr (*pp, ',');
  if (e)
    *pp = e + 1;
  else
    *pp = *pp + strlen (*pp);
}

static void parse_features (char *s)
{
  char *p;

  num_features = 0;
  features = NULL;

  if (!*s)
    return;

  /* count the features first, so we can allocate memory */
  p = s;
  do {
    num_features++;
    p = strchr (p, ',');
    if (p)
      p++;
  } while (p);

  features = (hb_feature_t *) calloc (num_features, sizeof (*features));

  /* now do the actual parsing */
  p = s;
  num_features = 0;
  while (*p) {
    if (parse_one_feature (&p, &features[num_features]))
      num_features++;
    else
      skip_one_feature (&p);
  }
}


static cairo_glyph_t *
_hb_cr_text_glyphs (cairo_t *cr,
		    const char *text, int len,
		    unsigned int *pnum_glyphs)
{
  cairo_scaled_font_t *scaled_font = cairo_get_scaled_font (cr);
  FT_Face ft_face = cairo_ft_scaled_font_lock_face (scaled_font);
  hb_font_t *hb_font = hb_ft_font_create (ft_face, NULL);
  hb_buffer_t *hb_buffer;
  cairo_glyph_t *cairo_glyphs;
  hb_glyph_info_t *hb_glyph;
  hb_glyph_position_t *hb_position;
  unsigned int num_glyphs, i;
  hb_position_t x;

  hb_buffer = hb_buffer_create (0);

  if (direction)
    hb_buffer_set_direction (hb_buffer, hb_direction_from_string (direction));
  if (script)
    hb_buffer_set_script (hb_buffer, hb_script_from_string (script));
  if (language)
    hb_buffer_set_language (hb_buffer, hb_language_from_string (language));

  if (len < 0)
    len = strlen (text);
  hb_buffer_add_utf8 (hb_buffer, text, len, 0, len);

  hb_shape (hb_font, hb_buffer, features, num_features);

  num_glyphs = hb_buffer_get_length (hb_buffer);
  hb_glyph = hb_buffer_get_glyph_infos (hb_buffer, NULL);
  hb_position = hb_buffer_get_glyph_positions (hb_buffer, NULL);
  cairo_glyphs = cairo_glyph_allocate (num_glyphs + 1);
  x = 0;
  for (i = 0; i < num_glyphs; i++)
    {
      cairo_glyphs[i].index = hb_glyph->codepoint;
      cairo_glyphs[i].x = (hb_position->x_offset + x) * (1./64);
      cairo_glyphs[i].y = -(hb_position->y_offset)    * (1./64);
      x += hb_position->x_advance;

      hb_glyph++;
      hb_position++;
    }
  cairo_glyphs[i].x = x * (1./64);
  hb_buffer_destroy (hb_buffer);
  hb_font_destroy (hb_font);
  cairo_ft_scaled_font_unlock_face (scaled_font);

  if (pnum_glyphs)
    *pnum_glyphs = num_glyphs;
  return cairo_glyphs;
}

static cairo_t *
create_context (void)
{
  cairo_t *cr;
  unsigned int fr, fg, fb, fa, br, bg, bb, ba;

  if (surface)
    cairo_surface_destroy (surface);
  if (back_pattern)
    cairo_pattern_destroy (back_pattern);
  if (fore_pattern)
    cairo_pattern_destroy (fore_pattern);

  br = bg = bb = ba = 255;
  sscanf (back + (*back=='#'), "%2x%2x%2x%2x", &br, &bg, &bb, &ba);
  fr = fg = fb = 0; fa = 255;
  sscanf (fore + (*fore=='#'), "%2x%2x%2x%2x", &fr, &fg, &fb, &fa);

  if (ba == 255 && fa == 255 && br == bg && bg == bb && fr == fg && fg == fb) {
    /* grayscale.  use A8 surface */
    surface = cairo_image_surface_create (CAIRO_FORMAT_A8, width, height);
    cr = cairo_create (surface);
    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba (cr, 1., 1., 1., br / 255.);
    cairo_paint (cr);
    back_pattern = cairo_pattern_reference (cairo_get_source (cr));
    cairo_set_source_rgba (cr, 1., 1., 1., fr / 255.);
    fore_pattern = cairo_pattern_reference (cairo_get_source (cr));
  } else {
    /* color.  use (A)RGB surface */
    if (ba != 255)
      surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
    else
      surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, width, height);
    cr = cairo_create (surface);
    cairo_set_source_rgba (cr, br / 255., bg / 255., bb / 255., ba / 255.);
    cairo_paint (cr);
    back_pattern = cairo_pattern_reference (cairo_get_source (cr));
    cairo_set_source_rgba (cr, fr / 255., fg / 255., fb / 255., fa / 255.);
    fore_pattern = cairo_pattern_reference (cairo_get_source (cr));
  }

  cairo_set_font_face (cr, cairo_face);

  return cr;
}

static void
draw (void)
{
  cairo_t *cr;
  cairo_font_extents_t font_extents;

  cairo_glyph_t *glyphs = NULL;
  unsigned int num_glyphs = 0;

  const char *end, *p = text;
  double x, y;

  cr= create_context ();

  cairo_set_font_size (cr, font_size);
  cairo_font_extents (cr, &font_extents);

  height = 0;
  width = 0;

  x = margin_l;
  y = margin_t;

  do {
    cairo_text_extents_t extents;

    end = strchr (p, '\n');
    if (!end)
      end = p + strlen (p);

    if (p != text)
	y += line_space;

    if (p != end) {
      glyphs = _hb_cr_text_glyphs (cr, p, end - p, &num_glyphs);

      cairo_glyph_extents (cr, glyphs, num_glyphs, &extents);

      y += ceil (font_extents.ascent);
      width = MAX (width, extents.x_advance);
      cairo_save (cr);
      cairo_translate (cr, x, y);
      cairo_show_glyphs (cr, glyphs, num_glyphs);
      cairo_restore (cr);
      y += ceil (font_extents.height - ceil (font_extents.ascent));

      cairo_glyph_free (glyphs);
    }

    p = end + 1;
  } while (*end);

  height = y + margin_b;
  width += margin_l + margin_r;

  cairo_destroy (cr);
}



int
main (int argc, char **argv)
{
  cairo_status_t status;

  setlocale (LC_ALL, "");

  parse_opts (argc, argv);

  FT_Init_FreeType (&ft_library);
  if (FT_New_Face (ft_library, font_file, 0, &ft_face)) {
    fprintf (stderr, "Failed to open font file `%s'\n", font_file);
    exit (1);
  }
  cairo_face = cairo_ft_font_face_create_for_ft_face (ft_face, 0);

  draw ();
  draw ();

  status = cairo_surface_write_to_png (surface, out_file);
  if (status != CAIRO_STATUS_SUCCESS) {
    fprintf (stderr, "Failed to write output file `%s': %s\n",
	     out_file, cairo_status_to_string (status));
    exit (1);
  }

  if (debug) {
    free (features);

    cairo_pattern_destroy (fore_pattern);
    cairo_pattern_destroy (back_pattern);
    cairo_surface_destroy (surface);
    cairo_font_face_destroy (cairo_face);
    cairo_debug_reset_static_data ();

    FT_Done_Face (ft_face);
    FT_Done_FreeType (ft_library);
  }

  return 0;
}


HB_END_DECLS
