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

#ifdef HAVE_FREETYPE
#include <hb-ft.h>
#endif
#ifdef HAVE_CORETEXT
#include <hb-coretext.h>
#endif

struct face_options_t
{
  ~face_options_t ()
  {
    g_free (face_loader);
    g_free (font_file);
  }

  void set_face (hb_face_t *face_)
  { face = face_; }

  void add_options (option_parser_t *parser);

  void post_parse (GError **error);

  static struct cache_t
  {
    ~cache_t ()
    {
      g_free (font_path);
      hb_face_destroy (face);
    }

    char *font_path = nullptr;
    unsigned face_index = (unsigned) -1;
    hb_face_t *face = nullptr;
  } cache;

  char *font_file = nullptr;
  unsigned face_index = 0;
  char *face_loader = nullptr;

  hb_face_t *face = nullptr;
};


face_options_t::cache_t face_options_t::cache {};

static struct supported_face_loaders_t {
	char name[9];
	hb_face_t * (*func) (const char *font_file, unsigned face_index);
} supported_face_loaders[] =
{
  {"ot",	hb_face_create_from_file_or_fail},
#ifdef HAVE_FREETYPE
  {"ft",	hb_ft_face_create_from_file_or_fail},
#endif
#ifdef HAVE_CORETEXT
  {"coretext",	hb_coretext_face_create_from_file_or_fail},
#endif
};

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

  hb_face_t * (*face_load) (const char *file_name, unsigned face_index) = nullptr;
  if (!face_loader)
  {
    face_load = supported_face_loaders[0].func;
  }
  else
  {
    for (unsigned int i = 0; i < ARRAY_LENGTH (supported_face_loaders); i++)
      if (0 == g_ascii_strcasecmp (face_loader, supported_face_loaders[i].name))
      {
	face_load = supported_face_loaders[i].func;
	break;
      }
    if (!face_load)
    {
      GString *s = g_string_new (nullptr);
      for (unsigned int i = 0; i < ARRAY_LENGTH (supported_face_loaders); i++)
      {
	if (i)
	  g_string_append_c (s, '/');
	g_string_append (s, supported_face_loaders[i].name);
      }
      g_string_append_c (s, '\n');
      char *p = g_string_free (s, FALSE);
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
		   "Unknown face loader `%s'; supported values are: %s; default is %s",
		   face_loader,
		   p,
		   supported_face_loaders[0].name);
      free (p);
      return;
    }
  }

  if (!cache.font_path ||
      0 != strcmp (cache.font_path, font_path) ||
      cache.face_index != face_index)
  {
    hb_face_destroy (cache.face);
    cache.face = face_load (font_path, face_index);
    cache.face_index = face_index;

    free ((char *) cache.font_path);
    cache.font_path = g_strdup (font_path);

    if (!cache.face)
    {
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_FAILED,
		   "%s: Failed loading font face", font_path);
      return;
    }
  }

  face = cache.face;
}

void
face_options_t::add_options (option_parser_t *parser)
{
  char *face_loaders_text = nullptr;
  {
    static_assert ((ARRAY_LENGTH_CONST (supported_face_loaders) > 0),
		   "No supported face-loaders found.");
    GString *s = g_string_new (nullptr);
    g_string_printf (s, "Set face loader to use (default: %s)\n\n    Supported face loaders are: %s",
		     supported_face_loaders[0].name,
		     supported_face_loaders[0].name);
    for (unsigned int i = 1; i < ARRAY_LENGTH (supported_face_loaders); i++)
    {
      g_string_append_c (s, '/');
      g_string_append (s, supported_face_loaders[i].name);
    }
    face_loaders_text = g_string_free (s, FALSE);
    parser->free_later (face_loaders_text);
  }

  GOptionEntry entries[] =
  {
    {"font-file",	0, 0, G_OPTION_ARG_STRING,	&this->font_file,		"Set font file-name",				"filename"},
    {"face-index",	'y', 0, G_OPTION_ARG_INT,	&this->face_index,		"Set face index (default: 0)",			"index"},
    {"face-loader",	0, 0, G_OPTION_ARG_STRING,	&this->face_loader,		face_loaders_text,				"loader"},
    {nullptr}
  };
  parser->add_group (entries,
		     "face",
		     "Font-face options:",
		     "Options for the font face",
		     this);
}

#endif
