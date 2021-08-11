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

#ifndef HB_MAIN_FONT_TEXT_HH
#define HB_MAIN_FONT_TEXT_HH

#include "options.hh"

/* main() body for utilities taking font and processing text.*/

template <typename consumer_t, typename font_options_t, typename text_options_t>
struct main_font_text : font_options_t, text_options_t, consumer_t
{
  void add_options (struct option_parser_t *parser)
  {
    font_options_t::add_options (parser);
    text_options_t::add_options (parser);
    consumer_t::add_options (parser);
  }

  int
  operator () (int argc, char **argv)
  {
    option_parser_t options ("[FONT-FILE] [TEXT]");
    add_options (&options);
    options.parse (&argc, &argv);

    argc--, argv++;
    if (argc && !this->font_file) this->font_file = locale_to_utf8 (argv[0]), argc--, argv++;
    if (argc && !this->text && !this->text_file) this->text = locale_to_utf8 (argv[0]), argc--, argv++;
    if (argc)
      fail (true, "Too many arguments on the command line");
    if (!this->font_file)
      options.usage ();
    if (!this->text && !this->text_file)
      this->text_file = g_strdup ("-");

    this->init (this);

    unsigned int line_len;
    const char *line;
    while ((line = this->get_line (&line_len)))
      this->consume_line (line, line_len, this->text_before, this->text_after);

    this->finish (this);

    return this->failed ? 1 : 0;
  }
};

#endif
