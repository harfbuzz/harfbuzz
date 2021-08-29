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

#ifndef SHAPE_OPTIONS_HH
#define SHAPE_OPTIONS_HH

#include "options.hh"

struct shape_options_t
{
  ~shape_options_t ()
  {
    g_free (direction);
    g_free (language);
    g_free (script);
    free (features);
    g_strfreev (shapers);
  }

  void add_options (option_parser_t *parser);

  void setup_buffer (hb_buffer_t *buffer)
  {
    hb_buffer_set_direction (buffer, hb_direction_from_string (direction, -1));
    hb_buffer_set_script (buffer, hb_script_from_string (script, -1));
    hb_buffer_set_language (buffer, hb_language_from_string (language, -1));
    hb_buffer_set_flags (buffer, (hb_buffer_flags_t)
				 (HB_BUFFER_FLAG_DEFAULT |
				  (bot ? HB_BUFFER_FLAG_BOT : 0) |
				  (eot ? HB_BUFFER_FLAG_EOT : 0) |
				  (preserve_default_ignorables ? HB_BUFFER_FLAG_PRESERVE_DEFAULT_IGNORABLES : 0) |
				  (remove_default_ignorables ? HB_BUFFER_FLAG_REMOVE_DEFAULT_IGNORABLES : 0) |
				  0));
    hb_buffer_set_invisible_glyph (buffer, invisible_glyph);
    hb_buffer_set_cluster_level (buffer, cluster_level);
    hb_buffer_guess_segment_properties (buffer);
  }

  static void copy_buffer_properties (hb_buffer_t *dst, hb_buffer_t *src)
  {
    hb_segment_properties_t props;
    hb_buffer_get_segment_properties (src, &props);
    hb_buffer_set_segment_properties (dst, &props);
    hb_buffer_set_flags (dst, hb_buffer_get_flags (src));
    hb_buffer_set_cluster_level (dst, hb_buffer_get_cluster_level (src));
  }

  void populate_buffer (hb_buffer_t *buffer, const char *text, int text_len,
			const char *text_before, const char *text_after)
  {
    hb_buffer_clear_contents (buffer);
    if (text_before) {
      unsigned int len = strlen (text_before);
      hb_buffer_add_utf8 (buffer, text_before, len, len, 0);
    }
    hb_buffer_add_utf8 (buffer, text, text_len, 0, text_len);
    if (text_after) {
      hb_buffer_add_utf8 (buffer, text_after, -1, 0, 0);
    }

    if (!utf8_clusters) {
      /* Reset cluster values to refer to Unicode character index
       * instead of UTF-8 index. */
      unsigned int num_glyphs = hb_buffer_get_length (buffer);
      hb_glyph_info_t *info = hb_buffer_get_glyph_infos (buffer, nullptr);
      for (unsigned int i = 0; i < num_glyphs; i++)
      {
	info->cluster = i;
	info++;
      }
    }

    setup_buffer (buffer);
  }

  hb_bool_t shape (hb_font_t *font, hb_buffer_t *buffer, const char **error=nullptr)
  {
    hb_buffer_t *text_buffer = nullptr;
    if (verify)
    {
      text_buffer = hb_buffer_create ();
      hb_buffer_append (text_buffer, buffer, 0, -1);
    }

    if (!hb_shape_full (font, buffer, features, num_features, shapers))
    {
      if (error)
	*error = "All shapers failed.";
      goto fail;
    }

    if (normalize_glyphs)
      hb_buffer_normalize_glyphs (buffer);

    if (verify && !verify_buffer (buffer, text_buffer, font, error))
      goto fail;

    if (text_buffer)
      hb_buffer_destroy (text_buffer);

    return true;

  fail:
    if (text_buffer)
      hb_buffer_destroy (text_buffer);

    return false;
  }

  bool verify_buffer (hb_buffer_t  *buffer,
		      hb_buffer_t  *text_buffer,
		      hb_font_t    *font,
		      const char  **error=nullptr)
  {
    if (!verify_buffer_monotone (buffer, error))
      return false;
    if (!verify_buffer_safe_to_break (buffer, text_buffer, font, error))
      return false;
    return true;
  }

  bool verify_buffer_monotone (hb_buffer_t *buffer, const char **error=nullptr)
  {
    /* Check that clusters are monotone. */
    if (cluster_level == HB_BUFFER_CLUSTER_LEVEL_MONOTONE_GRAPHEMES ||
	cluster_level == HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS)
    {
      bool is_forward = HB_DIRECTION_IS_FORWARD (hb_buffer_get_direction (buffer));

      unsigned int num_glyphs;
      hb_glyph_info_t *info = hb_buffer_get_glyph_infos (buffer, &num_glyphs);

      for (unsigned int i = 1; i < num_glyphs; i++)
	if (info[i-1].cluster != info[i].cluster &&
	    (info[i-1].cluster < info[i].cluster) != is_forward)
	{
	  if (error)
	    *error = "clusters are not monotone.";
	  return false;
	}
    }

    return true;
  }

  bool verify_buffer_safe_to_break (hb_buffer_t  *buffer,
				    hb_buffer_t  *text_buffer,
				    hb_font_t    *font,
				    const char  **error=nullptr)
  {
    if (cluster_level != HB_BUFFER_CLUSTER_LEVEL_MONOTONE_GRAPHEMES &&
	cluster_level != HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS)
    {
      /* Cannot perform this check without monotone clusters.
       * Then again, unsafe-to-break flag is much harder to use without
       * monotone clusters. */
      return true;
    }

    /* Check that breaking up shaping at safe-to-break is indeed safe. */

    hb_buffer_t *fragment = hb_buffer_create ();
    hb_buffer_t *reconstruction = hb_buffer_create ();
    copy_buffer_properties (reconstruction, buffer);

    unsigned int num_glyphs;
    hb_glyph_info_t *info = hb_buffer_get_glyph_infos (buffer, &num_glyphs);

    unsigned int num_chars;
    hb_glyph_info_t *text = hb_buffer_get_glyph_infos (text_buffer, &num_chars);

    /* Chop text and shape fragments. */
    bool forward = HB_DIRECTION_IS_FORWARD (hb_buffer_get_direction (buffer));
    unsigned int start = 0;
    unsigned int text_start = forward ? 0 : num_chars;
    unsigned int text_end = text_start;
    for (unsigned int end = 1; end < num_glyphs + 1; end++)
    {
      if (end < num_glyphs &&
	  (info[end].cluster == info[end-1].cluster ||
	   info[end-(forward?0:1)].mask & HB_GLYPH_FLAG_UNSAFE_TO_BREAK))
	  continue;

      /* Shape segment corresponding to glyphs start..end. */
      if (end == num_glyphs)
      {
	if (forward)
	  text_end = num_chars;
	else
	  text_start = 0;
      }
      else
      {
	if (forward)
	{
	  unsigned int cluster = info[end].cluster;
	  while (text_end < num_chars && text[text_end].cluster < cluster)
	    text_end++;
	}
	else
	{
	  unsigned int cluster = info[end - 1].cluster;
	  while (text_start && text[text_start - 1].cluster >= cluster)
	    text_start--;
	}
      }
      assert (text_start < text_end);

      if (0)
	printf("start %d end %d text start %d end %d\n", start, end, text_start, text_end);

      hb_buffer_clear_contents (fragment);
      copy_buffer_properties (fragment, buffer);

      hb_buffer_flags_t flags = hb_buffer_get_flags (fragment);
      if (0 < text_start)
	flags = (hb_buffer_flags_t) (flags & ~HB_BUFFER_FLAG_BOT);
      if (text_end < num_chars)
	flags = (hb_buffer_flags_t) (flags & ~HB_BUFFER_FLAG_EOT);
      hb_buffer_set_flags (fragment, flags);

      hb_buffer_append (fragment, text_buffer, text_start, text_end);
      if (!hb_shape_full (font, fragment, features, num_features, shapers))
      {
	if (error)
	  *error = "All shapers failed while shaping fragment.";
	hb_buffer_destroy (reconstruction);
	hb_buffer_destroy (fragment);
	return false;
      }
      hb_buffer_append (reconstruction, fragment, 0, -1);

      start = end;
      if (forward)
	text_start = text_end;
      else
	text_end = text_start;
    }

    bool ret = true;
    hb_buffer_diff_flags_t diff = hb_buffer_diff (reconstruction, buffer, (hb_codepoint_t) -1, 0);
    if (diff)
    {
      if (error)
	*error = "Safe-to-break test failed.";
      ret = false;

      /* Return the reconstructed result instead so it can be inspected. */
      hb_buffer_set_length (buffer, 0);
      hb_buffer_append (buffer, reconstruction, 0, -1);
    }

    hb_buffer_destroy (reconstruction);
    hb_buffer_destroy (fragment);

    return ret;
  }

  void shape_closure (const char *text, int text_len,
		      hb_font_t *font, hb_buffer_t *buffer,
		      hb_set_t *glyphs)
  {
    hb_buffer_reset (buffer);
    hb_buffer_add_utf8 (buffer, text, text_len, 0, text_len);
    setup_buffer (buffer);
    hb_ot_shape_glyphs_closure (font, buffer, features, num_features, glyphs);
  }

  /* Buffer properties */
  char *direction = nullptr;
  char *language = nullptr;
  char *script = nullptr;

  /* Buffer flags */
  hb_bool_t bot = false;
  hb_bool_t eot = false;
  hb_bool_t preserve_default_ignorables = false;
  hb_bool_t remove_default_ignorables = false;

  hb_feature_t *features = nullptr;
  unsigned int num_features = 0;
  char **shapers = nullptr;
  hb_bool_t utf8_clusters = false;
  hb_codepoint_t invisible_glyph = 0;
  hb_buffer_cluster_level_t cluster_level = HB_BUFFER_CLUSTER_LEVEL_DEFAULT;
  hb_bool_t normalize_glyphs = false;
  hb_bool_t verify = false;
  unsigned int num_iterations = 1;
};


static gboolean
parse_shapers (const char *name G_GNUC_UNUSED,
	       const char *arg,
	       gpointer    data,
	       GError    **error)
{
  shape_options_t *shape_opts = (shape_options_t *) data;
  char **shapers = g_strsplit (arg, ",", 0);

  for (char **shaper = shapers; *shaper; shaper++)
  {
    bool found = false;
    for (const char **hb_shaper = hb_shape_list_shapers (); *hb_shaper; hb_shaper++) {
      if (strcmp (*shaper, *hb_shaper) == 0)
      {
	found = true;
	break;
      }
    }
    if (!found)
    {
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
		   "Unknown or unsupported shaper: %s", *shaper);
      g_strfreev (shapers);
      return false;
    }
  }

  g_strfreev (shape_opts->shapers);
  shape_opts->shapers = shapers;
  return true;
}

static G_GNUC_NORETURN gboolean
list_shapers (const char *name G_GNUC_UNUSED,
	      const char *arg G_GNUC_UNUSED,
	      gpointer    data G_GNUC_UNUSED,
	      GError    **error G_GNUC_UNUSED)
{
  for (const char **shaper = hb_shape_list_shapers (); *shaper; shaper++)
    g_printf ("%s\n", *shaper);

  exit(0);
}


static gboolean
parse_features (const char *name G_GNUC_UNUSED,
		const char *arg,
		gpointer    data,
		GError    **error G_GNUC_UNUSED)
{
  shape_options_t *shape_opts = (shape_options_t *) data;
  char *s = (char *) arg;
  size_t l = strlen (s);
  char *p;

  shape_opts->num_features = 0;
  g_free (shape_opts->features);
  shape_opts->features = nullptr;

  /* if the string is quoted, strip the quotes */
  if (s[0] == s[l - 1] && (s[0] == '\"' || s[0] == '\''))
  {
    s[l - 1] = '\0';
    s++;
  }

  if (!*s)
    return true;

  /* count the features first, so we can allocate memory */
  p = s;
  do {
    shape_opts->num_features++;
    p = strchr (p, ',');
    if (p)
      p++;
  } while (p);

  shape_opts->features = (hb_feature_t *) calloc (shape_opts->num_features, sizeof (*shape_opts->features));
  if (!shape_opts->features)
    return false;

  /* now do the actual parsing */
  p = s;
  shape_opts->num_features = 0;
  while (p && *p) {
    char *end = strchr (p, ',');
    if (hb_feature_from_string (p, end ? end - p : -1, &shape_opts->features[shape_opts->num_features]))
      shape_opts->num_features++;
    p = end ? end + 1 : nullptr;
  }

  return true;
}

void
shape_options_t::add_options (option_parser_t *parser)
{
  GOptionEntry entries[] =
  {
    {"list-shapers",	0, G_OPTION_FLAG_NO_ARG,
			      G_OPTION_ARG_CALLBACK,	(gpointer) &list_shapers,	"List available shapers and quit",	nullptr},
    {"shaper",		0, G_OPTION_FLAG_HIDDEN,
			      G_OPTION_ARG_CALLBACK,	(gpointer) &parse_shapers,	"Hidden duplicate of --shapers",	nullptr},
    {"shapers",		0, 0, G_OPTION_ARG_CALLBACK,	(gpointer) &parse_shapers,	"Set comma-separated list of shapers to try","list"},
    {"direction",	0, 0, G_OPTION_ARG_STRING,	&this->direction,		"Set text direction (default: auto)",	"ltr/rtl/ttb/btt"},
    {"language",	0, 0, G_OPTION_ARG_STRING,	&this->language,		"Set text language (default: $LANG)",	"BCP 47 tag"},
    {"script",		0, 0, G_OPTION_ARG_STRING,	&this->script,			"Set text script (default: auto)",	"ISO-15924 tag"},
    {"bot",		0, 0, G_OPTION_ARG_NONE,	&this->bot,			"Treat text as beginning-of-paragraph",	nullptr},
    {"eot",		0, 0, G_OPTION_ARG_NONE,	&this->eot,			"Treat text as end-of-paragraph",	nullptr},
    {"preserve-default-ignorables",0, 0, G_OPTION_ARG_NONE,	&this->preserve_default_ignorables,	"Preserve Default-Ignorable characters",	nullptr},
    {"remove-default-ignorables",0, 0, G_OPTION_ARG_NONE,	&this->remove_default_ignorables,	"Remove Default-Ignorable characters",	nullptr},
    {"invisible-glyph",	0, 0, G_OPTION_ARG_INT,		&this->invisible_glyph,		"Glyph value to replace Default-Ignorables with",	nullptr},
    {"utf8-clusters",	0, 0, G_OPTION_ARG_NONE,	&this->utf8_clusters,		"Use UTF8 byte indices, not char indices",	nullptr},
    {"cluster-level",	0, 0, G_OPTION_ARG_INT,		&this->cluster_level,		"Cluster merging level (default: 0)",	"0/1/2"},
    {"normalize-glyphs",0, 0, G_OPTION_ARG_NONE,	&this->normalize_glyphs,	"Rearrange glyph clusters in nominal order",	nullptr},
    {"verify",		0, 0, G_OPTION_ARG_NONE,	&this->verify,			"Perform sanity checks on shaping results",	nullptr},
    {"num-iterations", 'n', G_OPTION_FLAG_IN_MAIN,
			      G_OPTION_ARG_INT,		&this->num_iterations,		"Run shaper N times (default: 1)",	"N"},
    {nullptr}
  };
  parser->add_group (entries,
		     "shape",
		     "Shape options:",
		     "Options for the shaping process",
		     this);

  const gchar *features_help = "Comma-separated list of font features\n"
    "\n"
    "    Features can be enabled or disabled, either globally or limited to\n"
    "    specific character ranges.  The format for specifying feature settings\n"
    "    follows.  All valid CSS font-feature-settings values other than 'normal'\n"
    "    and the global values are also accepted, though not documented below.\n"
    "    CSS string escapes are not supported."
    "\n"
    "    The range indices refer to the positions between Unicode characters,\n"
    "    unless the --utf8-clusters is provided, in which case range indices\n"
    "    refer to UTF-8 byte indices. The position before the first character\n"
    "    is always 0.\n"
    "\n"
    "    The format is Python-esque.  Here is how it all works:\n"
    "\n"
    "      Syntax:       Value:    Start:    End:\n"
    "\n"
    "    Setting value:\n"
    "      \"kern\"        1         0         ∞         # Turn feature on\n"
    "      \"+kern\"       1         0         ∞         # Turn feature on\n"
    "      \"-kern\"       0         0         ∞         # Turn feature off\n"
    "      \"kern=0\"      0         0         ∞         # Turn feature off\n"
    "      \"kern=1\"      1         0         ∞         # Turn feature on\n"
    "      \"aalt=2\"      2         0         ∞         # Choose 2nd alternate\n"
    "\n"
    "    Setting index:\n"
    "      \"kern[]\"      1         0         ∞         # Turn feature on\n"
    "      \"kern[:]\"     1         0         ∞         # Turn feature on\n"
    "      \"kern[5:]\"    1         5         ∞         # Turn feature on, partial\n"
    "      \"kern[:5]\"    1         0         5         # Turn feature on, partial\n"
    "      \"kern[3:5]\"   1         3         5         # Turn feature on, range\n"
    "      \"kern[3]\"     1         3         3+1       # Turn feature on, single char\n"
    "\n"
    "    Mixing it all:\n"
    "\n"
    "      \"aalt[3:5]=2\" 2         3         5         # Turn 2nd alternate on for range";

  GOptionEntry entries2[] =
  {
    {"features",	0, 0, G_OPTION_ARG_CALLBACK,	(gpointer) &parse_features,	features_help,	"list"},
    {nullptr}
  };
  parser->add_group (entries2,
		     "features",
		     "Features options:",
		     "Options for font features used",
		     this);
}

#endif
