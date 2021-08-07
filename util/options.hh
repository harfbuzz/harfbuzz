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

#ifndef OPTIONS_HH
#define OPTIONS_HH

#include "hb.hh"

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <locale.h>
#include <errno.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* for isatty() */
#endif
#if defined(_WIN32) || defined(__CYGWIN__)
#include <io.h> /* for setmode() under Windows */
#endif

#include <hb.h>
#include <hb-ot.h>
#include <glib.h>
#include <glib/gprintf.h>


static inline void fail (hb_bool_t suggest_help, const char *format, ...) G_GNUC_NORETURN G_GNUC_PRINTF (2, 3);

static inline void
fail (hb_bool_t suggest_help, const char *format, ...)
{
  const char *msg;

  va_list vap;
  va_start (vap, format);
  msg = g_strdup_vprintf (format, vap);
  va_end (vap);
  const char *prgname = g_get_prgname ();
  g_printerr ("%s: %s\n", prgname, msg);
  if (suggest_help)
    g_printerr ("Try `%s --help' for more information.\n", prgname);

  exit (1);
}


struct option_parser_t
{
  option_parser_t (const char *usage)
  : usage_str (usage),
    context (g_option_context_new (usage)),
    to_free (g_ptr_array_new ())
  {
    add_main_options ();
  }

  static void _g_free_g_func (void *p, void * G_GNUC_UNUSED) { g_free (p); }

  ~option_parser_t ()
  {
    g_option_context_free (context);
    g_ptr_array_foreach (to_free, _g_free_g_func, nullptr);
    g_ptr_array_free (to_free, TRUE);
  }

  void add_main_options ();

  static void
  post_parse_ (void *thiz, GError **error) {}
  template <typename Type>
  static auto
  post_parse_ (Type *thiz, GError **error) -> decltype (thiz->post_parse (error))
  { thiz->post_parse (error); }
  template <typename Type>
  static gboolean
  post_parse (GOptionContext *context G_GNUC_UNUSED,
	      GOptionGroup *group G_GNUC_UNUSED,
	      gpointer data,
	      GError **error)
  {
    option_parser_t::post_parse_ (static_cast<Type *> (data), error);
    return !*error;
  }

  template <typename Type>
  void add_group (GOptionEntry   *entries,
		  const gchar    *name,
		  const gchar    *description,
		  const gchar    *help_description,
		  Type           *closure)
  {
    GOptionGroup *group = g_option_group_new (name, description, help_description,
					      static_cast<gpointer>(closure), nullptr);
    g_option_group_add_entries (group, entries);
    g_option_group_set_parse_hooks (group, nullptr, post_parse<Type>);
    g_option_context_add_group (context, group);
  }

  void free_later (char *p) {
    g_ptr_array_add (to_free, p);
  }

  void parse (int *argc, char ***argv);

  G_GNUC_NORETURN void usage () {
    g_printerr ("Usage: %s [OPTION...] %s\n", g_get_prgname (), usage_str);
    exit (1);
  }

  private:
  const char *usage_str;
  GOptionContext *context;
  GPtrArray *to_free;
};


static inline gchar *
shapers_to_string ()
{
  GString *shapers = g_string_new (nullptr);
  const char **shaper_list = hb_shape_list_shapers ();

  for (; *shaper_list; shaper_list++) {
    g_string_append (shapers, *shaper_list);
    g_string_append_c (shapers, ',');
  }
  g_string_truncate (shapers, MAX (0, (gint)shapers->len - 1));

  return g_string_free (shapers, false);
}

static G_GNUC_NORETURN gboolean
show_version (const char *name G_GNUC_UNUSED,
	      const char *arg G_GNUC_UNUSED,
	      gpointer    data G_GNUC_UNUSED,
	      GError    **error G_GNUC_UNUSED)
{
  g_printf ("%s (%s) %s\n", g_get_prgname (), PACKAGE_NAME, PACKAGE_VERSION);

  char *shapers = shapers_to_string ();
  g_printf ("Available shapers: %s\n", shapers);
  g_free (shapers);
  if (strcmp (HB_VERSION_STRING, hb_version_string ()))
    g_printf ("Linked HarfBuzz library has a different version: %s\n", hb_version_string ());

  exit(0);
}

inline void
option_parser_t::add_main_options ()
{
  GOptionEntry entries[] =
  {
    {"version",		0, G_OPTION_FLAG_NO_ARG,
			      G_OPTION_ARG_CALLBACK,	(gpointer) &show_version,	"Show version numbers",			nullptr},
    {nullptr}
  };
  g_option_context_add_main_entries (context, entries, nullptr);
}

inline void
option_parser_t::parse (int *argc, char ***argv)
{
  setlocale (LC_ALL, "");

  GError *parse_error = nullptr;
  if (!g_option_context_parse (context, argc, argv, &parse_error))
  {
    if (parse_error)
    {
      fail (true, "%s", parse_error->message);
      //g_error_free (parse_error);
    }
    else
      fail (true, "Option parse error");
  }
}

// XXXXXXXXXXXX

#define FONT_SIZE_UPEM 0x7FFFFFFF
#define FONT_SIZE_NONE 0

extern const unsigned DEFAULT_FONT_SIZE;
extern const unsigned SUBPIXEL_BITS;

struct face_options_t
{
  void add_options (option_parser_t *parser);

  hb_blob_t *get_blob () const;
  hb_face_t *get_face () const;

  static struct cache_t
  {
    ~cache_t ()
    {
      free ((void *) font_path);
      hb_blob_destroy (blob);
      hb_face_destroy (face);
    }

    const char *font_path = nullptr;
    hb_blob_t *blob = nullptr;
    unsigned face_index = (unsigned) -1;
    hb_face_t *face = nullptr;
  } cache;

  char *font_file = nullptr;
  unsigned face_index = 0;
  private:
  mutable hb_face_t *face = nullptr;
};

struct font_options_t : face_options_t
{
  ~font_options_t ()
  {
    g_free (font_file);
    free (variations);
    g_free (font_funcs);
    hb_font_destroy (font);
  }

  void add_options (option_parser_t *parser);

  hb_font_t *get_font () const;

  hb_variation_t *variations = nullptr;
  unsigned int num_variations = 0;
  int x_ppem = 0;
  int y_ppem = 0;
  double ptem = 0.;
  unsigned int subpixel_bits = SUBPIXEL_BITS;
  mutable double font_size_x = DEFAULT_FONT_SIZE;
  mutable double font_size_y = DEFAULT_FONT_SIZE;
  char *font_funcs = nullptr;
  int ft_load_flags = 2;

  private:
  mutable hb_font_t *font = nullptr;
};


struct text_options_t
{
  ~text_options_t ()
  {
    g_free (text_before);
    g_free (text_after);
    g_free (text);
    g_free (text_file);
    if (gs)
      g_string_free (gs, true);
    if (fp && fp != stdin)
      fclose (fp);
  }

  void add_options (option_parser_t *parser);

  void post_parse (GError **error G_GNUC_UNUSED)
  {
    if (text && text_file)
      g_set_error (error,
		   G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
		   "Only one of text and text-file can be set");
  }

  const char *get_line (unsigned int *len, int eol = '\n');

  char *text_before = nullptr;
  char *text_after = nullptr;

  int text_len = -1;
  char *text = nullptr;
  char *text_file = nullptr;

  private:
  FILE *fp = nullptr;
  GString *gs = nullptr;
  char *line = nullptr;
  unsigned int line_len = UINT_MAX;
};

struct output_options_t
{
  ~output_options_t ()
  {
    g_free (output_file);
    g_free (output_format);
    if (fp && fp != stdout)
      fclose (fp);
  }

  void add_options (option_parser_t *parser,
		    const char **supported_formats = nullptr);

  void post_parse (GError **error G_GNUC_UNUSED)
  {
    if (output_format)
      explicit_output_format = true;

    if (output_file && !output_format) {
      output_format = strrchr (output_file, '.');
      if (output_format)
      {
	  output_format++; /* skip the dot */
	  output_format = g_strdup (output_format);
      }
    }

    if (output_file && 0 == strcmp (output_file, "-"))
      output_file = nullptr; /* STDOUT */
  }

  FILE *get_file_handle ();

  char *output_file = nullptr;
  char *output_format = nullptr;
  bool explicit_output_format = false;

  mutable FILE *fp = nullptr;
};


/* fallback implementation for scalbn()/scalbnf() for pre-2013 MSVC */
#if defined (_MSC_VER) && (_MSC_VER < 1800)

#ifndef FLT_RADIX
#define FLT_RADIX 2
#endif

__inline long double scalbn (long double x, int exp)
{
  return x * (pow ((long double) FLT_RADIX, exp));
}

__inline float scalbnf (float x, int exp)
{
  return x * (pow ((float) FLT_RADIX, exp));
}
#endif

#endif
