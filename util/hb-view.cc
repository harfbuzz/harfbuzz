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
#include <glib/gprintf.h>

#include <cairo-ft.h>
#include <hb-ft.h>



/* Controlled by cmd-line options */
struct margin_t {
  double t, r, b, l;
};
static margin_t opt_margin = {18, 18, 18, 18};
static double line_space = 0;
static int face_index = 0;
static double font_size = 36;
static const char *fore = "#000000";
static const char *back = "#ffffff";
static const char *text = NULL;
static const char *font_file = NULL;
static const char *out_file = "/dev/stdout";
static const char *direction = NULL;
static const char *script = NULL;
static const char *language = NULL;
static hb_bool_t annotate = FALSE;
static hb_bool_t debug = FALSE;

static hb_feature_t *features = NULL;
static unsigned int num_features;

/* Ugh, global vars.  Ugly, but does the job */
static int width = 0;
static int height = 0;
static cairo_surface_t *surface = NULL;
static cairo_pattern_t *fore_pattern = NULL;
static cairo_pattern_t *back_pattern = NULL;
static cairo_font_face_t *cairo_face;


G_GNUC_NORETURN static void
usage (FILE *f, int status)
{
  fprintf (f, "usage: hb-view [OPTS...] font-file text\n");
  exit (status);
}

static G_GNUC_NORETURN gboolean
show_version (const char *name G_GNUC_UNUSED,
	      const char *arg G_GNUC_UNUSED,
	      gpointer    data G_GNUC_UNUSED,
	      GError    **error G_GNUC_UNUSED)
{
  g_printf("%s (%s) %s\n", g_get_prgname (), PACKAGE_NAME, PACKAGE_VERSION);

  if (strcmp (HB_VERSION_STRING, hb_version_string ()))
    g_printf("Linked HarfBuzz library has a different version: %s\n", hb_version_string ());

  exit(0);
}


static gboolean
parse_features (const char *name G_GNUC_UNUSED,
	        const char *arg,
	        gpointer    data G_GNUC_UNUSED,
	        GError    **error G_GNUC_UNUSED);

static gboolean
parse_margin (const char *name G_GNUC_UNUSED,
	      const char *arg,
	      gpointer    data G_GNUC_UNUSED,
	      GError    **error G_GNUC_UNUSED)
{
  switch (sscanf (arg, "%f %f %f %f", &opt_margin.t, &opt_margin.r, &opt_margin.b, &opt_margin.l)) {
    case 1: opt_margin.r = opt_margin.t;
    case 2: opt_margin.b = opt_margin.t;
    case 3: opt_margin.l = opt_margin.r;
    case 4: return TRUE;
    default:
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
		   "%s argument should be one to four space-separated numbers",
		   name);
      return FALSE;
  }
}


void
fail (const char *format, ...)
{
  const char *msg;

  va_list vap;
  va_start (vap, format);
  msg = g_strdup_vprintf (format, vap);
  g_printerr ("%s: %s\n", g_get_prgname (), msg);

  exit (1);
}

static gchar *
shapers_to_string (void)
{
  GString *shapers = g_string_new (NULL);
  const char **shaper_list = hb_shape_list_shapers ();

  for (; *shaper_list; shaper_list++) {
    g_string_append (shapers, *shaper_list);
    g_string_append_c (shapers, ',');
  }
  g_string_truncate (shapers, MAX (0, (gint)shapers->len - 1));

  return g_string_free(shapers, FALSE);
}

void
parse_options (int argc, char *argv[])
{
  gchar *shapers_options = shapers_to_string ();
  GOptionEntry entries[] =
  {
    {"version",		0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, (gpointer) &show_version,
     "Show version numbers",						NULL},
    {"debug",		0, 0, G_OPTION_ARG_NONE,			&debug,
     "Free all resources before exit",					NULL},
    {"output",		0, 0, G_OPTION_ARG_STRING,			&out_file,
     "Set output file name",				      "filename"},

    {"annotate",	0, 0, G_OPTION_ARG_NONE,			&annotate,
     "Annotate output rendering",				    NULL},
    {"background",	0, 0, G_OPTION_ARG_STRING,			&back,
     "Set background color",			 "red/#rrggbb/#rrggbbaa"},
    {"foreground",	0, 0, G_OPTION_ARG_STRING,			&fore,
     "Set foreground color",			 "red/#rrggbb/#rrggbbaa"},
    {"line-space",	0, 0, G_OPTION_ARG_DOUBLE,			&font_size,
     "Set space between lines (default: 0)",			 "units"},
    {"margin",		0, 0, G_OPTION_ARG_CALLBACK,			(gpointer) &parse_margin,
     "Margin around output",			   "one to four numbers"},

    {"direction",	0, 0, G_OPTION_ARG_STRING,			&direction,
     "Set text direction (default: auto)",	       "ltr/rtl/ttb/btt"},
    {"language",	0, 0, G_OPTION_ARG_STRING,			&language,
     "Set text language (default: $LANG)",		       "langstr"},
    {"script",		0, 0, G_OPTION_ARG_STRING,			&script,
     "Set text script (default: auto)",			 "ISO-15924 tag"},
    {"features",	0, 0, G_OPTION_ARG_CALLBACK,			(gpointer) &parse_features,
     "Font features to apply to text",				  "TODO"},

    {"face-index",	0, 0, G_OPTION_ARG_INT,				&face_index,
     "Face index (default: 0)",					 "index"},
    {"font-size",	0, 0, G_OPTION_ARG_DOUBLE,			&font_size,
     "Font size",						  "size"},

    {NULL}
  };
  GError *error = NULL;
  GError *parse_error = NULL;
  GOptionContext *context;
  size_t len;

  context = g_option_context_new ("- FONT-FILE TEXT");

  g_option_context_add_main_entries (context, entries, NULL);

  if (!g_option_context_parse (context, &argc, &argv, &parse_error))
  {
    if (parse_error != NULL)
      fail ("%s", parse_error->message);
    else
      fail ("Option parse error");
    exit(1);
  }
  g_option_context_free(context);
  g_free(shapers_options);

  if (argc != 3) {
    g_printerr ("Usage: %s [OPTION...] FONT-FILE TEXT\n", g_get_prgname ());
    exit (1);
  }

  font_file = argv[1];
  text = g_locale_to_utf8 (argv[2], -1, NULL, NULL, &error);
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

static gboolean
parse_features (const char *name G_GNUC_UNUSED,
	        const char *arg,
	        gpointer    data G_GNUC_UNUSED,
	        GError    **error G_GNUC_UNUSED)
{
  char *s = (char *) arg;
  char *p;

  num_features = 0;
  features = NULL;

  if (!*s)
    return TRUE;

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

  return TRUE;
}


static cairo_glyph_t *
_hb_cr_text_glyphs (cairo_t *cr,
		    const char *utf8, int len,
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
  hb_position_t x, y;

  hb_buffer = hb_buffer_create (0);

  if (direction)
    hb_buffer_set_direction (hb_buffer, hb_direction_from_string (direction));
  if (script)
    hb_buffer_set_script (hb_buffer, hb_script_from_string (script));
  if (language)
    hb_buffer_set_language (hb_buffer, hb_language_from_string (language));

  if (len < 0)
    len = strlen (utf8);
  hb_buffer_add_utf8 (hb_buffer, utf8, len, 0, len);

  hb_shape (hb_font, hb_buffer, features, num_features);

  num_glyphs = hb_buffer_get_length (hb_buffer);
  hb_glyph = hb_buffer_get_glyph_infos (hb_buffer, NULL);
  hb_position = hb_buffer_get_glyph_positions (hb_buffer, NULL);
  cairo_glyphs = cairo_glyph_allocate (num_glyphs);
  x = 0;
  y = 0;
  for (i = 0; i < num_glyphs; i++)
    {
      cairo_glyphs[i].index = hb_glyph->codepoint;
      cairo_glyphs[i].x = ( hb_position->x_offset + x) * (1./64);
      cairo_glyphs[i].y = (-hb_position->y_offset + y) * (1./64);
      x +=  hb_position->x_advance;
      y += -hb_position->y_advance;

      hb_glyph++;
      hb_position++;
    }
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

  if (!annotate && ba == 255 && fa == 255 && br == bg && bg == bb && fr == fg && fg == fb) {
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

  x = opt_margin.l;
  y = opt_margin.t;

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
      if (annotate) {
        unsigned int i;
        cairo_save (cr);

	/* Draw actual glyph origins */
	cairo_set_source_rgba (cr, 1., 0., 0., .5);
	cairo_set_line_width (cr, 5);
	cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
	for (i = 0; i < num_glyphs; i++) {
	  cairo_move_to (cr, glyphs[i].x, glyphs[i].y);
	  cairo_rel_line_to (cr, 0, 0);
	}
	cairo_stroke (cr);

        cairo_restore (cr);
      }
      cairo_show_glyphs (cr, glyphs, num_glyphs);
      cairo_restore (cr);
      y += ceil (font_extents.height - ceil (font_extents.ascent));

      cairo_glyph_free (glyphs);
    }

    p = end + 1;
  } while (*end);

  height = y + opt_margin.b;
  width += opt_margin.l + opt_margin.r;

  cairo_destroy (cr);
}



int
main (int argc, char **argv)
{
  static FT_Library ft_library;
  static FT_Face ft_face;
  cairo_status_t status;

  setlocale (LC_ALL, "");

  parse_options (argc, argv);

  FT_Init_FreeType (&ft_library);
  if (FT_New_Face (ft_library, font_file, face_index, &ft_face)) {
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


