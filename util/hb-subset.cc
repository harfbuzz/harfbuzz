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
 * Google Author(s): Garret Rieger, Rod Sheeter
 */

#include <unistd.h>

#include "main-font-text.hh"

/*
 * Command line interface to the harfbuzz font subsetter.
 */

struct subset_consumer_t
{
  subset_consumer_t (option_parser_t *parser)
      : failed (false), options(parser) {}

  void init (hb_buffer_t  *buffer_,
             const font_options_t *font_opts)
  {
    font = hb_font_reference (font_opts->get_font ());
  }

  void consume_line (const char   *text,
                     unsigned int  text_len,
                     const char   *text_before,
                     const char   *text_after)
  {
  }

  void finish (const font_options_t *font_opts)
  {
    hb_face_t *face = hb_font_get_face (font);
    hb_blob_t *result = hb_face_reference_blob (face);
    unsigned int data_length;
    const char* data = hb_blob_get_data (result, &data_length);

    int fd_out = open(options.output_file, O_CREAT | O_WRONLY, S_IRWXU);
    if (fd_out != -1) {
      ssize_t bytes_written = write(fd_out, data, data_length);
      if (bytes_written == -1) {
        fprintf(stderr, "Unable to write output file");
        failed = true;
      } else if (bytes_written != data_length) {
        fprintf(stderr, "Wrong number of bytes written");
        failed = true;
      }
    } else {
      fprintf(stderr, "Unable to open output file");
      failed = true;
    }

    hb_blob_destroy (result);
    hb_font_destroy (font);
  }

  public:
  bool failed;

  private:
  output_options_t options;
  hb_font_t *font;
};

int
main (int argc, char **argv)
{
  main_font_text_t<subset_consumer_t, 10, 0> driver;
  return driver.main (argc, argv);
}
