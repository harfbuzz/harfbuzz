/*
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

#ifndef FONT_OPTIONS_HH
#define FONT_OPTIONS_HH

#include "face-options.hh"

#ifdef HAVE_FREETYPE
#include <hb-ft.h>
#endif

#define FONT_SIZE_UPEM 0x7FFFFFFF
#define FONT_SIZE_NONE 0

extern const unsigned DEFAULT_FONT_SIZE;
extern const unsigned SUBPIXEL_BITS;

struct font_options_t : face_options_t
{
  ~font_options_t ()
  {
#ifndef HB_NO_VAR
    free (variations);
#endif
    g_free (font_funcs);
    hb_font_destroy (font);
  }

  void add_options (option_parser_t *parser);

  void post_parse (GError **error);

  hb_bool_t sub_font = false;
#ifndef HB_NO_VAR
  hb_variation_t *variations = nullptr;
  unsigned int num_variations = 0;
#endif
  int x_ppem = 0;
  int y_ppem = 0;
  double ptem = 0.;
  double x_embolden = 0.;
  double y_embolden = 0.;
  hb_bool_t embolden_in_place = false;
  double slant = 0.;
  unsigned int subpixel_bits = SUBPIXEL_BITS;
  mutable double font_size_x = DEFAULT_FONT_SIZE;
  mutable double font_size_y = DEFAULT_FONT_SIZE;
  char *font_funcs = nullptr;
  int ft_load_flags = 2;
  unsigned int named_instance = HB_FONT_NO_VAR_NAMED_INSTANCE;

  hb_font_t *font = nullptr;
};


void
font_options_t::post_parse (GError **error)
{
  assert (!font);
  font = hb_font_create (face);

  if (font_size_x == FONT_SIZE_UPEM)
    font_size_x = hb_face_get_upem (face);
  if (font_size_y == FONT_SIZE_UPEM)
    font_size_y = hb_face_get_upem (face);

  hb_font_set_ppem (font, x_ppem, y_ppem);
  hb_font_set_ptem (font, ptem);

  hb_font_set_synthetic_bold (font,
			      (float) x_embolden, (float) y_embolden,
			      embolden_in_place);
  hb_font_set_synthetic_slant (font, slant);

  int scale_x = (int) scalbnf (font_size_x, subpixel_bits);
  int scale_y = (int) scalbnf (font_size_y, subpixel_bits);
  hb_font_set_scale (font, scale_x, scale_y);

#ifndef HB_NO_VAR
  if (named_instance != HB_FONT_NO_VAR_NAMED_INSTANCE)
    hb_font_set_var_named_instance (font, named_instance);
  hb_font_set_variations (font, variations, num_variations);
#endif

  if (font_funcs)
  {
    if (!hb_font_set_funcs_using (font, font_funcs))
    {
      return_value = RETURN_VALUE_FONT_FUNCS_FAILED;
      const char **supported_font_funcs = hb_font_list_funcs ();
      if (unlikely (!supported_font_funcs[0]))
      {
        g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
		     "Unknown font function implementation `%s'; no supported values found",
		     font_funcs);
	return;
      }
      char *supported_str = g_strjoinv ("/", (char **) supported_font_funcs);
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
		   "Unknown font function implementation `%s'; supported values are: %s; default is %s",
		   font_funcs,
		   supported_str,
		   supported_font_funcs[0]);
      g_free (supported_str);
      return;
    }
  }
#ifdef HAVE_FREETYPE
  hb_ft_font_set_load_flags (font, ft_load_flags);
#endif

  if (sub_font)
  {
    hb_font_t *old_font = font;
    font = hb_font_create_sub_font (old_font);
    hb_font_set_scale (old_font, scale_x * 2, scale_y * 2);
    hb_font_destroy (old_font);
  }
}

static G_GNUC_NORETURN gboolean
list_font_funcs (const char *name G_GNUC_UNUSED,
		 const char *arg G_GNUC_UNUSED,
		 gpointer    data G_GNUC_UNUSED,
		 GError    **error G_GNUC_UNUSED)
{
  for (const char **funcs = hb_font_list_funcs (); *funcs; funcs++)
    g_printf ("%s\n", *funcs);

  exit(0);
}

#ifndef HB_NO_VAR
static gboolean
parse_variations (const char *name G_GNUC_UNUSED,
		  const char *arg,
		  gpointer    data,
		  GError    **error G_GNUC_UNUSED)
{
  font_options_t *font_opts = (font_options_t *) data;
  char *s = (char *) arg;
  char *p;

  font_opts->num_variations = 0;
  g_free (font_opts->variations);
  font_opts->variations = nullptr;

  if (!*s)
    return true;

  /* count the variations first, so we can allocate memory */
  p = s;
  do {
    font_opts->num_variations++;
    p = strpbrk (p, ", ");
    if (p)
      p++;
  } while (p);

  font_opts->variations = (hb_variation_t *) calloc (font_opts->num_variations, sizeof (*font_opts->variations));
  if (!font_opts->variations)
    return false;

  /* now do the actual parsing */
  p = s;
  font_opts->num_variations = 0;
  while (p && *p) {
    char *end = strpbrk (p, ", ");
    if (hb_variation_from_string (p, end ? end - p : -1, &font_opts->variations[font_opts->num_variations]))
      font_opts->num_variations++;
    p = end ? end + 1 : nullptr;
  }

  return true;
}
#endif

static gboolean
parse_font_size (const char *name G_GNUC_UNUSED,
		 const char *arg,
		 gpointer    data,
		 GError    **error G_GNUC_UNUSED)
{
  font_options_t *font_opts = (font_options_t *) data;
  if (0 == strcmp (arg, "upem"))
  {
    font_opts->font_size_y = font_opts->font_size_x = FONT_SIZE_UPEM;
    return true;
  }
  switch (sscanf (arg, "%lf%*[ ,]%lf", &font_opts->font_size_x, &font_opts->font_size_y)) {
    case 1: font_opts->font_size_y = font_opts->font_size_x; HB_FALLTHROUGH;
    case 2: return true;
    default:
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
		   "%s argument should be one or two space-separated numbers",
		   name);
      return false;
  }
}

static gboolean
parse_font_ppem (const char *name G_GNUC_UNUSED,
		 const char *arg,
		 gpointer    data,
		 GError    **error G_GNUC_UNUSED)
{
  font_options_t *font_opts = (font_options_t *) data;
  switch (sscanf (arg, "%d%*[ ,]%d", &font_opts->x_ppem, &font_opts->y_ppem)) {
    case 1: font_opts->y_ppem = font_opts->x_ppem; HB_FALLTHROUGH;
    case 2: return true;
    default:
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
		   "%s argument should be one or two space-separated numbers",
		   name);
      return false;
  }
}

static gboolean
parse_font_embolden (const char *name G_GNUC_UNUSED,
		     const char *arg,
		     gpointer    data,
		     GError    **error G_GNUC_UNUSED)
{
  font_options_t *font_opts = (font_options_t *) data;
  switch (sscanf (arg, "%lf%*[ ,]%lf", &font_opts->x_embolden, &font_opts->y_embolden)) {
    case 1: font_opts->y_embolden = font_opts->x_embolden; HB_FALLTHROUGH;
    case 2: return true;
    default:
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
		   "%s argument should be one or two space-separated numbers",
		   name);
      return false;
  }
}

static gboolean
parse_font_bold (const char *name G_GNUC_UNUSED,
		 const char *arg,
		 gpointer    data,
		 GError    **error G_GNUC_UNUSED)
{
  font_options_t *font_opts = (font_options_t *) data;
  font_opts->embolden_in_place = false;
  return parse_font_embolden ( name, arg, data, error);
}

static gboolean
parse_font_grade (const char *name G_GNUC_UNUSED,
		  const char *arg,
		  gpointer    data,
		  GError    **error G_GNUC_UNUSED)
{
  font_options_t *font_opts = (font_options_t *) data;
  font_opts->embolden_in_place = true;
  return parse_font_embolden ( name, arg, data, error);
}

void
font_options_t::add_options (option_parser_t *parser)
{
  face_options_t::add_options (parser);

  char *font_funcs_text = nullptr;
  {
    const char **supported_font_funcs = hb_font_list_funcs ();
    GString *s = g_string_new (nullptr);
    if (unlikely (!supported_font_funcs[0]))
      g_string_printf (s, "Set font functions implementation to use (default: none)\n    No supported font function implementations found");
    else
    {
      char *supported_str = g_strjoinv ("/", (char **) supported_font_funcs);
      g_string_printf (s, "Set font functions implementation to use (default: %s)\n    Supported font function implementations are: %s",
		       supported_font_funcs[0],
		       supported_str);
      g_free (supported_str);
    }
    font_funcs_text = g_string_free (s, FALSE);
    parser->free_later (font_funcs_text);
  }

  char *font_size_text;
  if (DEFAULT_FONT_SIZE == FONT_SIZE_UPEM)
    font_size_text = (char *) "Font size (default: upem)";
  else
  {
    font_size_text = g_strdup_printf ("Font size (default: %u)", DEFAULT_FONT_SIZE);
    parser->free_later (font_size_text);
  }

  int font_size_flags = DEFAULT_FONT_SIZE == FONT_SIZE_NONE ? G_OPTION_FLAG_HIDDEN : 0;
  GOptionEntry entries[] =
  {
    {"font-size",	0, font_size_flags,
			      G_OPTION_ARG_CALLBACK,	(gpointer) &parse_font_size,	font_size_text,					"1/2 integers or 'upem'"},
    {"font-ppem",	0, font_size_flags,
			      G_OPTION_ARG_CALLBACK,	(gpointer) &parse_font_ppem,	"Set x,y pixels per EM (default: 0; disabled)",	"1/2 integers"},
    {"font-ptem",	0, font_size_flags,
			      G_OPTION_ARG_DOUBLE,	&this->ptem,			"Set font point-size (default: 0; disabled)",	"point-size"},
    {"font-bold",	0, font_size_flags,
			      G_OPTION_ARG_CALLBACK,	(gpointer) &parse_font_bold,	"Set synthetic bold (default: 0)",		"1/2 numbers; eg. 0.05"},
    {"font-grade",	0, font_size_flags,
			      G_OPTION_ARG_CALLBACK,	(gpointer) &parse_font_grade,	"Set synthetic grade (default: 0)",		"1/2 numbers; eg. 0.05"},
    {"font-slant",	0, font_size_flags,
			      G_OPTION_ARG_DOUBLE,	&this->slant,			"Set synthetic slant (default: 0)",		 "slant ratio; eg. 0.2"},
    {"font-funcs",	0, 0, G_OPTION_ARG_STRING,	&this->font_funcs,		font_funcs_text,				"impl"},
    {"list-font-funcs",	0, G_OPTION_FLAG_NO_ARG,
			      G_OPTION_ARG_CALLBACK,	(gpointer) &list_font_funcs,	"List available font functions and quit",	nullptr},
    {"sub-font",	0, G_OPTION_FLAG_HIDDEN,
			      G_OPTION_ARG_NONE,	&this->sub_font,		"Create a sub-font (default: false)",		"boolean"},
    {"ft-load-flags",	0, 0, G_OPTION_ARG_INT,		&this->ft_load_flags,		"Set FreeType load-flags (default: 2)",		"integer"},
    {nullptr}
  };
  parser->add_group (entries,
		     "font",
		     "Font-instance options:",
		     "Options for the font instance",
		     this,
		     false /* We add below. */);

#ifndef HB_NO_VAR
  const gchar *variations_help = "Comma-separated list of font variations\n"
    "\n"
    "    Variations are set globally. The format for specifying variation settings\n"
    "    follows.  All valid CSS font-variation-settings values other than 'normal'\n"
    "    and 'inherited' are also accepted, though, not documented below.\n"
    "\n"
    "    The format is a tag, optionally followed by an equals sign, followed by a\n"
    "    number. For example:\n"
    "\n"
    "      \"wght=500\"\n"
    "      \"slnt=-7.5\"";

  GOptionEntry entries2[] =
  {
    {"named-instance",	0, 0, G_OPTION_ARG_INT,         &this->named_instance,		"Set named-instance index (default: none)",	"index"},
    {"variations",	0, 0, G_OPTION_ARG_CALLBACK,	(gpointer) &parse_variations,	variations_help,	"list"},
    {nullptr}
  };
  parser->add_group (entries2,
		     "variations",
		     "Variations options:",
		     "Options for font variations used",
		     this);
#endif

  parser->add_environ("HB_FONT_FUNCS=font-funcs; Overrides the default font-funcs.");
  parser->add_exit_code (RETURN_VALUE_FONT_FUNCS_FAILED, "Failed setting font functions.");
}

#endif
