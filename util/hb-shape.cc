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

#include "hb-view.hh"

struct output_buffer_t : output_options_t, format_options_t
{
  output_buffer_t (option_parser_t *parser)
		  : output_options_t (parser),
		    format_options_t (parser) {}

  void init (const font_options_t *font_opts);
  void consume_line (hb_buffer_t  *buffer,
		     const char   *text,
		     unsigned int  text_len);
  void finish (const font_options_t *font_opts);

  protected:
  GString *gs;
  hb_font_t *font;
  hb_buffer_t *scratch;
};

void
output_buffer_t::init (const font_options_t *font_opts)
{
  get_file_handle ();
  font = hb_font_reference (font_opts->get_font ());
  gs = g_string_new (NULL);
  scratch = hb_buffer_create ();
}

void
output_buffer_t::consume_line (hb_buffer_t  *buffer,
			       const char   *text,
			       unsigned int  text_len)
{
  g_string_set_size (gs, 0);

  if (show_text) {
    g_string_append_len (gs, text, text_len);
    g_string_append_c (gs, '\n');
  }

  if (show_unicode) {
    hb_buffer_reset (scratch);
    hb_buffer_add_utf8 (scratch, text, text_len, 0, -1);
    serialize_unicode (buffer, gs);
    g_string_append_c (gs, '\n');
  }

  serialize_glyphs (buffer, font, gs);
  fprintf (fp, "%s\n", gs->str);
}

void
output_buffer_t::finish (const font_options_t *font_opts)
{
  hb_buffer_destroy (scratch);
  scratch = NULL;
  g_string_free (gs, TRUE);
  gs = NULL;
  hb_font_destroy (font);
  font = NULL;
}

int
main (int argc, char **argv)
{
  return hb_view_t<output_buffer_t>::main (argc, argv);
}
