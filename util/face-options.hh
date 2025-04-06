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

#ifndef FACE_OPTIONS_HH
#define FACE_OPTIONS_HH

#include "options.hh"

struct face_options_t
{
  ~face_options_t ()
  {
    g_free (font_file);
    g_free (face_loader);
    hb_face_destroy (face);
  }

  void set_face (hb_face_t *face_)
  {
    hb_face_destroy (face);
    face = hb_face_reference (face_);
  }

  void add_options (option_parser_t *parser);

  void post_parse (GError **error);

  static struct cache_t
  {
    ~cache_t ()
    {
      g_free (font_path);
      g_free (face_loader);
      hb_face_destroy (face);
    }

    char *font_path = nullptr;
    char *face_loader = nullptr;
    unsigned face_index = (unsigned) -1;
    hb_face_t *face = nullptr;
  } cache;

  char *font_file = nullptr;
  unsigned face_index = 0;
  char *face_loader = nullptr;

  hb_face_t *face = nullptr;
};


face_options_t::cache_t face_options_t::cache {};

void
face_options_t::post_parse (GError **error)
{
  if (!font_file)
  {
    g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_FAILED,
		 "No font file set");
    return;
  }

  assert (font_file);

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

  if ((!cache.font_path || 0 != strcmp (cache.font_path, font_path)) ||
      (cache.face_loader != face_loader &&
       (cache.face_loader && face_loader && 0 != strcmp (cache.face_loader, face_loader))) ||
      cache.face_index != face_index)
  {
    hb_face_destroy (cache.face);
    cache.face = hb_face_create_from_file_or_fail_using (font_path, face_index, face_loader);
    cache.face_index = face_index;

    g_free ((char *) cache.font_path);
    g_free ((char *) cache.face_loader);
    cache.font_path = g_strdup (font_path);
    cache.face_loader = face_loader ? g_strdup (face_loader) : nullptr;

    if (!cache.face)
    {
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_FAILED,
		   "%s: Failed loading font face", font_path);
      return_value = RETURN_VALUE_FACE_LOAD_FAILED;
      return;
    }
  }

  set_face (cache.face);
}

static G_GNUC_NORETURN gboolean
list_face_loaders (const char *name G_GNUC_UNUSED,
		   const char *arg G_GNUC_UNUSED,
		   gpointer    data G_GNUC_UNUSED,
		   GError    **error G_GNUC_UNUSED)
{
  for (const char **funcs = hb_face_list_loaders (); *funcs; funcs++)
    g_printf ("%s\n", *funcs);

  exit(0);
}

void
face_options_t::add_options (option_parser_t *parser)
{
  char *face_loaders_text = nullptr;
  {
    const char **supported_face_loaders = hb_face_list_loaders ();
    GString *s = g_string_new (nullptr);
    if (unlikely (!supported_face_loaders))
      g_string_printf (s, "Set face loader to use (default: none)\n    No supported face loaders found");
    else
    {
      char *supported_str = g_strjoinv ("/", (char **) supported_face_loaders);
      g_string_printf (s, "Set face loader to use (default: %s)\n    Supported face loaders are: %s",
		       supported_face_loaders[0],
		       supported_str);
      g_free (supported_str);
    }
    face_loaders_text = g_string_free (s, FALSE);
    parser->free_later (face_loaders_text);
  }

  GOptionEntry entries[] =
  {
    {"font-file",	0, 0, G_OPTION_ARG_STRING,	&this->font_file,		"Set font file-name",				"filename"},
    {"face-index",	'y', 0, G_OPTION_ARG_INT,	&this->face_index,		"Set face index (default: 0)",			"index"},
    {"face-loader",	0, 0, G_OPTION_ARG_STRING,	&this->face_loader,		face_loaders_text,				"loader"},
    {"list-face-loaders",0, G_OPTION_FLAG_NO_ARG,
			      G_OPTION_ARG_CALLBACK,	(gpointer) &list_face_loaders,	"List available face loaders and quit",		nullptr},
    {nullptr}
  };
  parser->add_group (entries,
		     "face",
		     "Font-face options:",
		     "Options for the font face",
		     this);

  parser->add_environ("HB_FACE_LOADER=face-loader; Overrides the default face loader.");
  parser->add_exit_code (RETURN_VALUE_FACE_LOAD_FAILED, "Failed loading font face.");
}

#endif
