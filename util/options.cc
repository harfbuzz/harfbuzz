/*
 * Copyright © 2011,2012  Google, Inc.
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

#include "options.hh"

#ifdef HAVE_FREETYPE
#include <hb-ft.h>
#endif
#include <hb-ot.h>

static struct supported_font_funcs_t {
	char name[4];
	void (*func) (hb_font_t *);
} supported_font_funcs[] =
{
#ifdef HAVE_FREETYPE
  {"ft",	hb_ft_font_set_funcs},
#endif
  {"ot",	hb_ot_font_set_funcs},
};


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
    p = strchr (p, ',');
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
    char *end = strchr (p, ',');
    if (hb_variation_from_string (p, end ? end - p : -1, &font_opts->variations[font_opts->num_variations]))
      font_opts->num_variations++;
    p = end ? end + 1 : nullptr;
  }

  return true;
}

static gboolean
parse_text (const char *name G_GNUC_UNUSED,
	    const char *arg,
	    gpointer    data,
	    GError    **error G_GNUC_UNUSED)
{
  text_options_t *text_opts = (text_options_t *) data;

  if (text_opts->text)
  {
    g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
		 "Either --text or --unicodes can be provided but not both");
    return false;
  }

  text_opts->text_len = -1;
  text_opts->text = g_strdup (arg);
  return true;
}


static gboolean
parse_unicodes (const char *name G_GNUC_UNUSED,
		const char *arg,
		gpointer    data,
		GError    **error G_GNUC_UNUSED)
{
  text_options_t *text_opts = (text_options_t *) data;

  if (text_opts->text)
  {
    g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
		 "Either --text or --unicodes can be provided but not both");
    return false;
  }

  GString *gs = g_string_new (nullptr);
  if (0 == strcmp (arg, "*"))
  {
    g_string_append_c (gs, '*');
  }
  else
  {

    char *s = (char *) arg;
    char *p;

    while (s && *s)
    {
#define DELIMITERS "<+>{},;&#\\xXuUnNiI\n\t\v\f\r "

      while (*s && strchr (DELIMITERS, *s))
	s++;
      if (!*s)
	break;

      errno = 0;
      hb_codepoint_t u = strtoul (s, &p, 16);
      if (errno || s == p)
      {
	g_string_free (gs, TRUE);
	g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
		     "Failed parsing Unicode values at: '%s'", s);
	return false;
      }

      g_string_append_unichar (gs, u);

      s = p;
    }
  }

  text_opts->text_len = gs->len;
  text_opts->text = g_string_free (gs, FALSE);
  return true;
}

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
face_options_t::add_options (option_parser_t *parser)
{
  GOptionEntry entries[] =
  {
    {"font-file",	0, 0, G_OPTION_ARG_STRING,	&this->font_file,		"Set font file-name",				"filename"},
    {"face-index",	0, 0, G_OPTION_ARG_INT,		&this->face_index,		"Set face index (default: 0)",			"index"},
    {nullptr}
  };
  parser->add_group (entries,
		     "face",
		     "Font-face options:",
		     "Options for the font face",
		     this);
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
    font_size_text = g_strdup_printf ("Font size (default: %d)", DEFAULT_FONT_SIZE);
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
    {"font-funcs",	0, 0, G_OPTION_ARG_STRING,	&this->font_funcs,		text,						"impl"},
    {"ft-load-flags",	0, 0, G_OPTION_ARG_INT,		&this->ft_load_flags,		"Set FreeType load-flags (default: 2)",		"integer"},
    {nullptr}
  };
  parser->add_group (entries,
		     "font",
		     "Font-instance options:",
		     "Options for the font instance",
		     this);

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
    "      \"slnt=-7.5\"\n";

  GOptionEntry entries2[] =
  {
    {"variations",	0, 0, G_OPTION_ARG_CALLBACK,	(gpointer) &parse_variations,	variations_help,	"list"},
    {nullptr}
  };
  parser->add_group (entries2,
		     "variations",
		     "Variations options:",
		     "Options for font variations used",
		     this);
}

void
text_options_t::add_options (option_parser_t *parser)
{
  GOptionEntry entries[] =
  {
    {"text",		0, 0, G_OPTION_ARG_CALLBACK,	(gpointer) &parse_text,		"Set input text",			"string"},
    {"text-file",	0, 0, G_OPTION_ARG_STRING,	&this->text_file,		"Set input text file-name\n\n    If no text is provided, standard input is used for input.\n",		"filename"},
    {"unicodes",      'u', 0, G_OPTION_ARG_CALLBACK,	(gpointer) &parse_unicodes,		"Set input Unicode codepoints",		"list of hex numbers"},
    {"text-before",	0, 0, G_OPTION_ARG_STRING,	&this->text_before,		"Set text context before each line",	"string"},
    {"text-after",	0, 0, G_OPTION_ARG_STRING,	&this->text_after,		"Set text context after each line",	"string"},
    {nullptr}
  };
  parser->add_group (entries,
		     "text",
		     "Text options:",
		     "Options for the input text",
		     this);
}

void
output_options_t::add_options (option_parser_t *parser,
			       const char **supported_formats)
{
  const char *text;

  if (!supported_formats)
    text = "Set output serialization format";
  else
  {
    char *items = g_strjoinv ("/", const_cast<char **> (supported_formats));
    text = g_strdup_printf ("Set output format\n\n    Supported output formats are: %s", items);
    g_free (items);
    parser->free_later ((char *) text);
  }

  GOptionEntry entries[] =
  {
    {"output-file",   'o', 0, G_OPTION_ARG_STRING,	&this->output_file,		"Set output file-name (default: stdout)","filename"},
    {"output-format", 'O', 0, G_OPTION_ARG_STRING,	&this->output_format,		text,					"format"},
    {nullptr}
  };
  parser->add_group (entries,
		     "output",
		     "Output destination & format options:",
		     "Options for the destination & form of the output",
		     this);
}


face_options_t::cache_t face_options_t::cache {};

hb_blob_t *
face_options_t::get_blob () const
{
  // XXX This does the job for now; will move to post_parse.
  return cache.blob;
}

hb_face_t *
face_options_t::get_face () const
{
  if (face)
    return face;

  if (!font_file)
    fail (true, "No font file set");

  const char *font_path = font_file;

  if (0 == strcmp (font_path, "-"))
  {
#if defined(_WIN32) || defined(__CYGWIN__)
    setmode (fileno (stdin), O_BINARY);
    font_path = "STDIN";
#else
    font_path = "/dev/stdin";
#endif
  }

  if (!cache.font_path || 0 != strcmp (cache.font_path, font_path))
  {
    hb_blob_destroy (cache.blob);
    cache.blob = hb_blob_create_from_file_or_fail (font_path);

    free ((char *) cache.font_path);
    cache.font_path = strdup (font_path);

    if (!cache.blob)
      fail (false, "%s: Failed reading file", font_path);

    hb_face_destroy (cache.face);
    cache.face = nullptr;
    cache.face_index = (unsigned) -1;
  }

  if (cache.face_index != face_index)
  {
    hb_face_destroy (cache.face);
    cache.face = hb_face_create (cache.blob, face_index);
    cache.face_index = face_index;
  }

  face = cache.face;

  return face;
}


hb_font_t *
font_options_t::get_font () const
{
  if (font)
    return font;

  auto *face = get_face ();

  font = hb_font_create (face);

  if (font_size_x == FONT_SIZE_UPEM)
    font_size_x = hb_face_get_upem (face);
  if (font_size_y == FONT_SIZE_UPEM)
    font_size_y = hb_face_get_upem (face);

  hb_font_set_ppem (font, x_ppem, y_ppem);
  hb_font_set_ptem (font, ptem);

  int scale_x = (int) scalbnf (font_size_x, subpixel_bits);
  int scale_y = (int) scalbnf (font_size_y, subpixel_bits);
  hb_font_set_scale (font, scale_x, scale_y);

  hb_font_set_variations (font, variations, num_variations);

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
      char *p = g_string_free (s, FALSE);
      fail (false, "Unknown font function implementation `%s'; supported values are: %s; default is %s",
	    font_funcs,
	    p,
	    supported_font_funcs[0].name);
      //free (p);
    }
  }
  set_font_funcs (font);
#ifdef HAVE_FREETYPE
  hb_ft_font_set_load_flags (font, ft_load_flags);
#endif

  return font;
}


const char *
text_options_t::get_line (unsigned int *len, int eol)
{
  if (text) {
    if (!line)
    {
      line = text;
      line_len = text_len;
    }
    if (line_len == UINT_MAX)
      line_len = strlen (line);

    if (!line_len) {
      *len = 0;
      return nullptr;
    }

    const char *ret = line;
    const char *p = (const char *) memchr (line, eol, line_len);
    unsigned int ret_len;
    if (!p) {
      ret_len = line_len;
      line += ret_len;
      line_len = 0;
    } else {
      ret_len = p - ret;
      line += ret_len + 1;
      line_len -= ret_len + 1;
    }

    *len = ret_len;
    return ret;
  }

  if (!fp) {
    if (!text_file)
      fail (true, "At least one of text or text-file must be set");

    if (0 != strcmp (text_file, "-"))
      fp = fopen (text_file, "r");
    else
      fp = stdin;

    if (!fp)
      fail (false, "Failed opening text file `%s': %s",
	    text_file, strerror (errno));

    gs = g_string_new (nullptr);
  }

  g_string_set_size (gs, 0);
  char buf[BUFSIZ];
  while (fgets (buf, sizeof (buf), fp))
  {
    unsigned bytes = strlen (buf);
    if (bytes && (int) (unsigned char) buf[bytes - 1] == eol)
    {
      bytes--;
      g_string_append_len (gs, buf, bytes);
      break;
    }
    g_string_append_len (gs, buf, bytes);
  }
  if (ferror (fp))
    fail (false, "Failed reading text: %s", strerror (errno));
  *len = gs->len;
  return !*len && feof (fp) ? nullptr : gs->str;
}


FILE *
output_options_t::get_file_handle ()
{
  if (fp)
    return fp;

  if (output_file)
    fp = fopen (output_file, "wb");
  else {
#if defined(_WIN32) || defined(__CYGWIN__)
    setmode (fileno (stdout), O_BINARY);
#endif
    fp = stdout;
  }
  if (!fp)
    fail (false, "Cannot open output file `%s': %s",
	  g_filename_display_name (output_file), strerror (errno));

  return fp;
}
