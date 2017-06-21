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
 * Google Author(s): Alexander Aprelev
 */

#include "SkCanvas.h"
#include "SkDocument.h"
#include "SkFontMgr.h"
#include "SkGradientShader.h"
#include "SkPaint.h"
#include "SkStream.h"
#include "SkString.h"
#include "SkTextBlob.h"
#include "SkTypeface.h"

#include "options.hh"

#ifndef VIEW_SKIA_HH
#define VIEW_SKIA_HH

extern const char *helper_skia_supported_formats[];

struct view_skia_t
{
  view_skia_t (option_parser_t *parser)
               : output_options (parser, helper_skia_supported_formats),
                 view_options (parser),
                 direction (HB_DIRECTION_INVALID),
                 scale_bits (0) {}

  ~view_skia_t (void) {}

  void init (const font_options_t *_font_opts);

  void new_line (void) {}

  void consume_text (hb_buffer_t  *buffer,
                     const char   *text,
                     unsigned int  text_len,
                     hb_bool_t     utf8_clusters) {}

  void shape_failed (hb_buffer_t  *buffer,
                     const char   *text,
                     unsigned int  text_len,
                     hb_bool_t     utf8_clusters) {
    fail (false, "all shapers failed");
  }

  void consume_glyphs (hb_buffer_t  *buffer,
                       const char   *text,
                       unsigned int  text_len,
                       hb_bool_t     utf8_clusters);

  void finish (const font_options_t *font_opts);

 protected:
  output_options_t output_options;
  view_options_t view_options;
  const font_options_t *font_opts;

  hb_direction_t direction; // Remove this, make segment_properties accessible
  GArray *lines;
  int scale_bits;

  SkPaint glyph_paint;
};

#endif
