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
#include <hb-ot.h>

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
  hb_bool_t list_features = false;
#ifndef HB_NO_VAR
  hb_bool_t list_variations = false;
  hb_variation_t *variations = nullptr;
  unsigned int num_variations = 0;
#endif
  int x_ppem = 0;
  int y_ppem = 0;
  double ptem = 0.;
  double slant = 0.;
  unsigned int subpixel_bits = SUBPIXEL_BITS;
  mutable double font_size_x = DEFAULT_FONT_SIZE;
  mutable double font_size_y = DEFAULT_FONT_SIZE;
  char *font_funcs = nullptr;
  int ft_load_flags = 2;
  unsigned int palette = 0;
  unsigned int named_instance = HB_FONT_NO_VAR_NAMED_INSTANCE;

  hb_font_t *font = nullptr;
};


static struct supported_font_funcs_t {
	char name[4];
	void (*func) (hb_font_t *);
} supported_font_funcs[] =
{
  {"ot",	hb_ot_font_set_funcs},
#ifdef HAVE_FREETYPE
  {"ft",	hb_ft_font_set_funcs},
#endif
};


static G_GNUC_NORETURN void _list_features (hb_face_t *face);
#ifndef HB_NO_VAR
static G_GNUC_NORETURN void _list_variations (hb_face_t *face);
#endif

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

  hb_font_set_synthetic_slant (font, slant);

  int scale_x = (int) scalbnf (font_size_x, subpixel_bits);
  int scale_y = (int) scalbnf (font_size_y, subpixel_bits);
  hb_font_set_scale (font, scale_x, scale_y);

#ifndef HB_NO_VAR
  hb_font_set_var_named_instance (font, named_instance);
  hb_font_set_variations (font, variations, num_variations);
#endif

  void (*set_font_funcs) (hb_font_t *) = nullptr;
  if (!font_funcs)
  {
    set_font_funcs = supported_font_funcs[0].func;
  }
  else
  {
    for (unsigned int i = 0; i < ARRAY_LENGTH (supported_font_funcs); i++)
      if (0 == g_ascii_strcasecmp (font_funcs, supported_font_funcs[i].name))
      {
	set_font_funcs = supported_font_funcs[i].func;
	break;
      }
    if (!set_font_funcs)
    {
      GString *s = g_string_new (nullptr);
      for (unsigned int i = 0; i < ARRAY_LENGTH (supported_font_funcs); i++)
      {
	if (i)
	  g_string_append_c (s, '/');
	g_string_append (s, supported_font_funcs[i].name);
      }
      g_string_append_c (s, '\n');
      char *p = g_string_free (s, FALSE);
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
		   "Unknown font function implementation `%s'; supported values are: %s; default is %s",
		   font_funcs,
		   p,
		   supported_font_funcs[0].name);
      free (p);
      return;
    }
  }
  set_font_funcs (font);
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

  if (list_features)
    _list_features (face);

#ifndef HB_NO_VAR
  if (list_variations)
    _list_variations (face);
#endif
}

static G_GNUC_NORETURN void
_list_features (hb_face_t *face)
{
  hb_tag_t table_tags[] = {HB_OT_TAG_GSUB, HB_OT_TAG_GPOS, HB_TAG_NONE};
  auto language = hb_language_get_default ();
  hb_set_t *features = hb_set_create ();

  for (unsigned int i = 0; table_tags[i]; i++)
  {
    printf ("Table: %c%c%c%c\n", HB_UNTAG (table_tags[i]));

    hb_tag_t script_array[32];
    unsigned script_count = sizeof script_array / sizeof script_array[0];
    unsigned script_offset = 0;
    do
    {
      hb_ot_layout_table_get_script_tags (face, table_tags[i],
					  script_offset,
					  &script_count,
					  script_array);

      for (unsigned script_index = 0; script_index < script_count; script_index++)
      {
        printf ("  Script: %c%c%c%c\n", HB_UNTAG (script_array[script_index]));

	hb_tag_t language_array[32];
	unsigned language_count = sizeof language_array / sizeof language_array[0];
	unsigned language_offset = 0;
	do
	{
	  hb_ot_layout_script_get_language_tags (face, table_tags[i],
						 script_offset + script_index,
						 language_offset,
						 &language_count,
						 language_array);

	  for (unsigned language_index = 0; language_index < language_count; language_index++)
	  {
	    printf ("    Language: %c%c%c%c\n", HB_UNTAG (language_array[language_index]));
	  }

	  language_offset += language_count;
	}
	while (language_count == sizeof language_array / sizeof language_array[0]);
      }

      script_offset += script_count;
    }
    while (script_count == sizeof script_array / sizeof script_array[0]);

    hb_set_clear (features);
    hb_tag_t feature_array[32];
    unsigned feature_count = sizeof feature_array / sizeof feature_array[0];
    unsigned feature_offset = 0;
    do
    {
      hb_ot_layout_table_get_feature_tags (face, table_tags[i],
					   feature_offset,
					   &feature_count,
					   feature_array);

      for (unsigned feature_index = 0; feature_index < feature_count; feature_index++)
      {
        if (hb_set_has (features, feature_array[feature_index]))
	  continue;
	hb_set_add (features, feature_array[feature_index]);

	hb_ot_name_id_t label_id;

	hb_ot_layout_feature_get_name_ids (face,
					   table_tags[i],
					   feature_offset + feature_index,
					   &label_id,
					   nullptr,
					   nullptr,
					   nullptr,
					   nullptr);

	char name[64];
	unsigned name_len = sizeof name;

	hb_ot_name_get_utf8 (face, label_id,
			     language,
			     &name_len, name);

	printf ("  Feature: %c%c%c%c", HB_UNTAG (feature_array[feature_index]));

	if (*name)
	  printf (" \"%s\"", name);

	printf ("\n");
      }

      feature_offset += feature_count;
    }
    while (feature_count == sizeof feature_array / sizeof feature_array[0]);
  }

  hb_set_destroy (features);

  exit (0);
}

#ifndef HB_NO_VAR
static G_GNUC_NORETURN void
_list_variations (hb_face_t *face)
{
  hb_ot_var_axis_info_t *axes;

  unsigned count = hb_ot_var_get_axis_infos (face, 0, nullptr, nullptr);
  axes = (hb_ot_var_axis_info_t *) calloc (count, sizeof (hb_ot_var_axis_info_t));
  hb_ot_var_get_axis_infos (face, 0, &count, axes);

  auto language = hb_language_get_default ();
  bool has_hidden = false;

  printf ("Varitation axes:\n");
  printf ("Tag:	Minimum	Default	Maximum	Name\n\n");
  for (unsigned i = 0; i < count; i++)
  {
    const auto &axis = axes[i];
    if (axis.flags & HB_OT_VAR_AXIS_FLAG_HIDDEN)
      has_hidden = true;

    char name[64];
    unsigned name_len = sizeof name;

    hb_ot_name_get_utf8 (face, axis.name_id,
			 language,
			 &name_len, name);

    printf ("%c%c%c%c%s:	%g	%g	%g	%s\n",
	    HB_UNTAG (axis.tag),
	    axis.flags & HB_OT_VAR_AXIS_FLAG_HIDDEN ? "*" : "",
	    (double) axis.min_value,
	    (double) axis.default_value,
	    (double) axis.max_value,
	    name);
  }
  if (has_hidden)
    printf ("\n[*] Hidden axis\n");

  free (axes);
  axes = nullptr;

  count = hb_ot_var_get_named_instance_count (face);
  if (count)
  {
    printf ("\n\nNamed instances: \n\n");

    for (unsigned i = 0; i < count; i++)
    {
      char name[64];
      unsigned name_len = sizeof name;

      hb_ot_name_id_t name_id = hb_ot_var_named_instance_get_subfamily_name_id (face, i);
      hb_ot_name_get_utf8 (face, name_id,
			   language,
			   &name_len, name);

      unsigned coords_count = hb_ot_var_named_instance_get_design_coords (face, i, nullptr, nullptr);
      float* coords;
      coords = (float *) calloc (coords_count, sizeof (float));
      hb_ot_var_named_instance_get_design_coords (face, i, &coords_count, coords);

      printf ("%u. %-32s", i, name);
      for (unsigned j = 0; j < coords_count; j++)
	printf ("%g, ", (double) coords[j]);
      printf ("\n");

      free (coords);
    }
  }

  exit(0);
}

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

void
font_options_t::add_options (option_parser_t *parser)
{
  face_options_t::add_options (parser);

  char *text = nullptr;

  {
    static_assert ((ARRAY_LENGTH_CONST (supported_font_funcs) > 0),
		   "No supported font-funcs found.");
    GString *s = g_string_new (nullptr);
    g_string_printf (s, "Set font functions implementation to use (default: %s)\n\n    Supported font function implementations are: %s",
		     supported_font_funcs[0].name,
		     supported_font_funcs[0].name);
    for (unsigned int i = 1; i < ARRAY_LENGTH (supported_font_funcs); i++)
    {
      g_string_append_c (s, '/');
      g_string_append (s, supported_font_funcs[i].name);
    }
    text = g_string_free (s, FALSE);
    parser->free_later (text);
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
    {"font-ptem",	0, 0,
			      G_OPTION_ARG_DOUBLE,	&this->ptem,			"Set font point-size (default: 0; disabled)",	"point-size"},
    {"font-slant",	0, 0,
			      G_OPTION_ARG_DOUBLE,	&this->slant,			"Set synthetic slant (default: 0)",		 "slant ratio; eg. 0.2"},
    {"font-palette",    0, 0, G_OPTION_ARG_INT,         &this->palette,                 "Set font palette (default: 0)",                "index"},
    {"font-funcs",	0, 0, G_OPTION_ARG_STRING,	&this->font_funcs,		text,						"impl"},
    {"sub-font",	0, G_OPTION_FLAG_HIDDEN,
			      G_OPTION_ARG_NONE,	&this->sub_font,		"Create a sub-font (default: false)",		"boolean"},
    {"ft-load-flags",	0, 0, G_OPTION_ARG_INT,		&this->ft_load_flags,		"Set FreeType load-flags (default: 2)",		"integer"},
    {"list-features",	0, 0, G_OPTION_ARG_NONE,	&this->list_features,		"List available font features and quit",	nullptr},
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
    {"list-variations",	0, 0, G_OPTION_ARG_NONE,	&this->list_variations,		"List available font variations and quit",	nullptr},
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
}

#endif
