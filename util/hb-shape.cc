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
		     unsigned int  text_len,
		     hb_bool_t     utf8_clusters);
  void finish (const font_options_t *font_opts);

  protected:
  GString *gs;
  hb_font_t *font;
  unsigned int line_no;
};

void
output_buffer_t::init (const font_options_t *font_opts)
{
  get_file_handle ();
  font = hb_font_reference (font_opts->get_font ());
  gs = g_string_new (NULL);
  line_no = 0;
}

void
output_buffer_t::consume_line (hb_buffer_t  *buffer,
			       const char   *text,
			       unsigned int  text_len,
			       hb_bool_t     utf8_clusters)
{
  line_no++;
  g_string_set_size (gs, 0);
  serialize_line (buffer, line_no, text, text_len, font, utf8_clusters, gs);
  fprintf (fp, "%s", gs->str);
}

void
output_buffer_t::finish (const font_options_t *font_opts)
{
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
