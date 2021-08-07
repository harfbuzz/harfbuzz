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
  const char *text = nullptr;

  if (supported_formats)
  {
    char *items = g_strjoinv ("/", const_cast<char **> (supported_formats));
    text = g_strdup_printf ("Set output format\n\n    Supported output formats are: %s", items);
    g_free (items);
    parser->free_later ((char *) text);
  }

  GOptionEntry entries[] =
  {
    {"output-file",   'o', 0, G_OPTION_ARG_STRING,	&this->output_file,		"Set output file-name (default: stdout)","filename"},
    {"output-format", 'O', supported_formats ? 0 : G_OPTION_FLAG_HIDDEN,
			      G_OPTION_ARG_STRING,	&this->output_format,		text,					"format"},
    {nullptr}
  };
  parser->add_group (entries,
		     "output",
		     "Output destination & format options:",
		     "Options for the destination & form of the output",
		     this);
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
