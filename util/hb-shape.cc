/*
 * Copyright © 2010  Behdad Esfahbod
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

#include "main-font-text.hh"
#include "shape-consumer.hh"

struct output_buffer_t
{
  output_buffer_t (option_parser_t *parser)
		  : options (parser),
		    format (parser) {}

  void init (const font_options_t *font_opts)
  {
    options.get_file_handle ();
    gs = g_string_new (NULL);
    line_no = 0;
    font = hb_font_reference (font_opts->get_font ());
  }
  void new_line (void)
  {
    line_no++;
  }
  void consume_text (hb_buffer_t  *buffer,
		     const char   *text,
		     unsigned int  text_len,
		     hb_bool_t     utf8_clusters)
  {
    g_string_set_size (gs, 0);
    format.serialize_buffer_of_text (buffer, line_no, text, text_len, font, utf8_clusters, gs);
    fprintf (options.fp, "%s", gs->str);
  }
  void shape_failed (hb_buffer_t  *buffer,
		     const char   *text,
		     unsigned int  text_len,
		     hb_bool_t     utf8_clusters)
  {
    g_string_set_size (gs, 0);
    format.serialize_message (line_no, "msg: all shapers failed", gs);
    fprintf (options.fp, "%s", gs->str);
  }
  void consume_glyphs (hb_buffer_t  *buffer,
		       const char   *text,
		       unsigned int  text_len,
		       hb_bool_t     utf8_clusters)
  {
    g_string_set_size (gs, 0);
    format.serialize_buffer_of_glyphs (buffer, line_no, text, text_len, font, utf8_clusters, gs);
    fprintf (options.fp, "%s", gs->str);
  }
  void finish (const font_options_t *font_opts)
  {
    hb_font_destroy (font);
    g_string_free (gs, true);
    gs = NULL;
    font = NULL;
  }

  protected:
  output_options_t options;
  format_options_t format;

  GString *gs;
  unsigned int line_no;
  hb_font_t *font;
};

int
main (int argc, char **argv)
{
  main_font_text_t<shape_consumer_t<output_buffer_t> > driver;
  return driver.main (argc, argv);
}
