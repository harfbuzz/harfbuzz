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

void fail (hb_bool_t suggest_help, const char *format, ...) G_GNUC_NORETURN G_GNUC_PRINTF (2, 3);


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

struct format_options_t
{
  void add_options (option_parser_t *parser);

  void serialize (hb_buffer_t  *buffer,
			 hb_font_t    *font,
			 hb_buffer_serialize_format_t format,
			 hb_buffer_serialize_flags_t flags,
			 GString      *gs);
  void serialize_line_no (unsigned int  line_no,
			  GString      *gs);
  void serialize_buffer_of_text (hb_buffer_t  *buffer,
				 unsigned int  line_no,
				 const char   *text,
				 unsigned int  text_len,
				 hb_font_t    *font,
				 GString      *gs);
  void serialize_message (unsigned int  line_no,
			  const char   *type,
			  const char   *msg,
			  GString      *gs);
  void serialize_buffer_of_glyphs (hb_buffer_t  *buffer,
				   unsigned int  line_no,
				   const char   *text,
				   unsigned int  text_len,
				   hb_font_t    *font,
				   hb_buffer_serialize_format_t output_format,
				   hb_buffer_serialize_flags_t format_flags,
				   GString      *gs);


  hb_bool_t show_glyph_names = true;
  hb_bool_t show_positions = true;
  hb_bool_t show_advances = true;
  hb_bool_t show_clusters = true;
  hb_bool_t show_text = false;
  hb_bool_t show_unicode = false;
  hb_bool_t show_line_num = false;
  hb_bool_t show_extents = false;
  hb_bool_t show_flags = false;
  hb_bool_t trace = false;
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
