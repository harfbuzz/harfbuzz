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

#include "common.hh"
#include "options.hh"

#include "view-cairo.hh"

int
main (int argc, char **argv)
{
  setlocale (LC_ALL, "");

  option_parser_t options ("[FONT-FILE] [TEXT]");

  shape_options_t shaper (&options);
  font_options_t font_opts (&options);
  text_options_t input (&options);

  view_cairo_t output (&options);

  options.parse (&argc, &argv);

  argc--, argv++;
  if (argc && !font_opts.font_file) font_opts.font_file = argv[0], argc--, argv++;
  if (argc && !input.text && !input.text_file) input.text = argv[0], argc--, argv++;
  if (argc)
    fail (TRUE, "Too many arguments on the command line");
  if (!font_opts.font_file || (!input.text && !input.text_file))
    options.usage ();

  output.init (&font_opts);

  hb_buffer_t *buffer = hb_buffer_create ();
  unsigned int text_len;
  const char *text;
  while ((text = input.get_line (&text_len)))
  {
    if (!shaper.shape (text, text_len,
			   font_opts.get_font (),
			   buffer))
      fail (FALSE, "All shapers failed");

    output.consume_line (buffer, text, text_len);
  }
  hb_buffer_destroy (buffer);

  output.finish (&font_opts);

  return 0;
}
