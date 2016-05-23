/*
 * Copyright © 2016  Google, Inc.
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

#include "view-skia.hh"

#include <assert.h>

const char *helper_skia_supported_formats[] =
{
  "pdf",
  NULL
};

void view_skia_t::init (const font_options_t *_font_opts) {
  font_opts = _font_opts;

  lines = g_array_new (false, false, sizeof (SkTextBlob*));
  scale_bits = -font_opts->subpixel_bits;

  sk_sp<SkTypeface> fSkiaTypeface;
  auto data = SkData::MakeFromFileName(font_opts->font_file);
  assert(data);
  if (!data) { return; }
  fSkiaTypeface = SkTypeface::MakeFromStream(new SkMemoryStream(data), font_opts->face_index);
  assert(fSkiaTypeface);
  if (!fSkiaTypeface) { return; }

  glyph_paint.setFlags(
    SkPaint::kAntiAlias_Flag |
    SkPaint::kSubpixelText_Flag);  // ... avoid waggly text when rotating.
  glyph_paint.setColor(SK_ColorBLACK);
  glyph_paint.setTextSize(font_opts->font_size_x);
  SkAutoTUnref<SkFontMgr> fm(SkFontMgr::RefDefault());
  glyph_paint.setTypeface(fSkiaTypeface);
  glyph_paint.setTextEncoding(SkPaint::kGlyphID_TextEncoding);
}

void view_skia_t::consume_glyphs (hb_buffer_t  *buffer,
                                  const char   *text,
                                  unsigned int  text_len,
                                  hb_bool_t     utf8_clusters) {
  direction = hb_buffer_get_direction (buffer);

  SkTextBlobBuilder text_blob_builder;
  unsigned len = hb_buffer_get_length (buffer);
  if (len == 0) {
    return;
  }
  hb_glyph_info_t *info = hb_buffer_get_glyph_infos (buffer, NULL);
  hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (buffer, NULL);
  auto run_buffer = text_blob_builder.allocRunPos(glyph_paint, len);

  double x = 0;
  double y = 0;
  for (unsigned int i = 0; i < len; i++)
  {
    run_buffer.glyphs[i] = info[i].codepoint;

    reinterpret_cast<SkPoint*>(run_buffer.pos)[i] = SkPoint::Make(
      x + scalbn((double)pos[i].x_offset, scale_bits),
      y - scalbn((double)pos[i].y_offset, scale_bits));
    x += scalbn((double)pos[i].x_advance, scale_bits);
    y += scalbn((double)pos[i].y_advance, scale_bits);
  }
  const SkTextBlob *text_blob = text_blob_builder.build();
  g_array_append_val (lines, text_blob);
}

void view_skia_t::finish (const font_options_t *font_opts) {
  auto data = SkData::MakeFromFileName(font_opts->font_file);
  sk_sp<SkTypeface> fSkiaTypeface = fSkiaTypeface = SkTypeface::MakeFromStream(
      new SkMemoryStream(data), font_opts->face_index);

  SkDocument::PDFMetadata pdf_info;
  SkTime::DateTime now;
  SkTime::GetDateTime(&now);
  pdf_info.fCreation.fEnabled = true;
  pdf_info.fCreation.fDateTime = now;
  pdf_info.fModified.fEnabled = true;
  pdf_info.fModified.fDateTime = now;

  SkFILEWStream *file_stream = nullptr;
  SkDynamicMemoryWStream *memory_stream = nullptr;
  SkWStream *output_stream = nullptr;
  if (output_options.output_file != nullptr) {
    file_stream = new SkFILEWStream(output_options.output_file);
    output_stream = file_stream;
  } else {
    // when writing to stdout, write into memory stream first because
    // there seems to be no way to point Skia stream to stdout.
    memory_stream = new SkDynamicMemoryWStream();
    output_stream = memory_stream;
  }

  sk_sp<SkDocument> pdfDocument = SkDocument::MakePDF(
      output_stream, SK_ScalarDefaultRasterDPI,
      pdf_info, nullptr, true);
  assert(pdfDocument);

  SkPaint background_paint;
  background_paint.setColor(SK_ColorWHITE);

  hb_font_t *font = font_opts->get_font();
  hb_font_extents_t extents;
  hb_font_get_extents_for_direction (font, direction, &extents);

  double ascent = scalbn ((double) extents.ascender, scale_bits);
  double descent = -scalbn ((double) extents.descender, scale_bits);
  double font_height = scalbn ((double) extents.ascender - extents.descender + extents.line_gap, scale_bits);
  double leading = font_height + view_options.line_space;

  // Calculate width and height of one page that fits all consumed text.
  //
  // Use (1.0f,1.0f) as minimum (width, height) so that drawPaint doesn't
  // crash below when input is empty.
  double h = MAX((double)1.0f, lines->len * leading + descent + view_options.line_space);
  double w = 1.0f;
  for (unsigned int i = 0; i < lines->len; i++) {
    SkTextBlob *text_blob = g_array_index (lines, SkTextBlob*, i);
    SkRect bounds = text_blob->bounds();
    double right = bounds.right();
    w =  MAX (w, right);
  }

  SkCanvas* pageCanvas = pdfDocument->beginPage(w, h);
  pageCanvas->drawPaint(background_paint);

  double current_x = 0;
  double current_y = leading;

  for (unsigned int i = 0; i < lines->len; i++) {
    const SkTextBlob *text_blob = g_array_index (lines, const SkTextBlob*, i);
    pageCanvas->drawTextBlob(text_blob, current_x, current_y, glyph_paint);
    text_blob->unref();

    current_y += leading;
  }

  pdfDocument->endPage();
  pdfDocument->close();

  if (memory_stream != nullptr) {
    SkAutoDataUnref data(memory_stream->copyToData());
    fwrite(data->data(), sizeof(char), data->size(), stdout);
  }

#if GLIB_CHECK_VERSION (2, 22, 0)
  g_array_unref (lines);
#else
  g_array_free (lines, TRUE);
#endif
}
