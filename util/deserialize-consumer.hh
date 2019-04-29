/*
 * Copyright © 2019  Ebrahim Byagowi
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
 */

#ifndef HB_DESERIALIZE_CONSUMER_HH
#define HB_DESERIALIZE_CONSUMER_HH

#include "hb.hh"
#include "options.hh"


template <typename output_t>
struct deserialize_consumer_t
{
  deserialize_consumer_t (option_parser_t *parser)
			: failed (false),
			  shaper (parser),
			  output (parser),
			  font (nullptr),
			  buffer (nullptr) {}

  void init (hb_buffer_t  *buffer_,
	     const font_options_t *font_opts)
  {
    font = hb_font_reference (font_opts->get_font ());
    failed = false;
    buffer = hb_buffer_reference (buffer_);

    output.init (buffer, font_opts);
  }
  void consume_line (const char   *text,
		     unsigned int  text_len,
		     const char   *text_before,
		     const char   *text_after)
  {
    output.new_line ();

    hb_buffer_deserialize_glyphs (buffer, text, text_len, &text, font, HB_BUFFER_SERIALIZE_FORMAT_TEXT);

    output.consume_glyphs (buffer, text, text_len, shaper.utf8_clusters);
  }
  void finish (const font_options_t *font_opts)
  {
    output.finish (buffer, font_opts);
    hb_font_destroy (font);
    font = nullptr;
    hb_buffer_destroy (buffer);
    buffer = nullptr;
  }

  public:
  bool failed;

  protected:
  shape_options_t shaper;
  output_t output;

  hb_font_t *font;
  hb_buffer_t *buffer;
};


#endif
