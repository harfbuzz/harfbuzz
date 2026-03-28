/*
 * Copyright (C) 2026  Behdad Esfahbod
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
 * Author(s): Behdad Esfahbod
 */

#include "font-options.hh"
#include "main-font-text.hh"
#include "shape-consumer.hh"
#include "text-options.hh"
#include "gpu-output.hh"

const unsigned DEFAULT_FONT_SIZE = FONT_SIZE_UPEM;
const unsigned SUBPIXEL_BITS = 0;

#include "gpu/default-text.hh"

struct gpu_text_options_t : shape_text_options_t
{
  const char *default_text () override
  {
    return ::default_text;
  }
};

struct gpu_font_options_t : font_options_t
{
  hb_face_t *default_face () override
  {
#ifdef _WIN32
    return nullptr; /* Will fail; user must provide font on Windows. */
#else
    #include "gpu/default-font.hh"
    hb_blob_t *blob = hb_blob_create ((const char *) default_font,
				      sizeof (default_font),
				      HB_MEMORY_MODE_READONLY,
				      nullptr, nullptr);
    hb_face_t *face_ = hb_face_create (blob, 0);
    hb_blob_destroy (blob);
    return face_;
#endif
  }
};

using base_t = main_font_text_t<shape_consumer_t<gpu_output_t>,
				gpu_font_options_t,
				gpu_text_options_t>;

struct gpu_main_t : base_t
{
  int operator () (int argc, char **argv)
  {
    add_options ();
    add_exit_code (RETURN_VALUE_OPERATION_FAILED, "Operation failed.");

    parse (&argc, &argv);

    this->init (this);

    while (this->consume_line (*this))
      ;

    this->finish (this);

    if (this->failed && return_value == RETURN_VALUE_SUCCESS)
      return_value = RETURN_VALUE_OPERATION_FAILED;

    return return_value;
  }
};

int
main (int argc, char **argv)
{
  gpu_main_t driver;
  return driver (argc, argv);
}
