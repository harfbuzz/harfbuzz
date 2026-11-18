/*
 * Copyright © 2026  Behdad Esfahbod
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

#ifndef HB_VECTOR_PAINT_PDF_IMAGE_HH
#define HB_VECTOR_PAINT_PDF_IMAGE_HH

#include "hb-vector-paint.hh"

#ifdef HAVE_ZLIB
#include <zlib.h>
#endif

/* Inflate noninterlaced 8-bit PNG rows and undo their byte filters in place.
 * Bound decoded bytes before allocating, and charge work even for bad streams.
 * Empty DEFLATE blocks can make input much larger than the decoded rows. */
static bool
hb_pdf_decode_png_rows (hb_vector_t<uint8_t> *out,
                       const char *idat_data, unsigned idat_len,
                       unsigned width, unsigned height, unsigned channels,
                       int64_t *budget)
{
#ifndef HAVE_ZLIB
  (void) out; (void) idat_data; (void) idat_len;
  (void) width; (void) height; (void) channels; (void) budget;
  return false;
#else
  unsigned row_bytes, stride, data_len, raw_len;
  if (!width || !height || !channels || channels > 4 ||
      hb_unsigned_mul_overflows (width, channels, &row_bytes) ||
      hb_unsigned_add_overflows (row_bytes, 1u, &stride) ||
      hb_unsigned_mul_overflows (row_bytes, height, &data_len) ||
      data_len > HB_VECTOR_MAX_DOCUMENT_SIZE ||
      hb_unsigned_mul_overflows (stride, height, &raw_len) ||
      !hb_budget_spend (*budget, HB_BUDGET_1 * 2, raw_len) ||
      !hb_budget_spend (*budget, HB_BUDGET_1, idat_len))
    return false;

  if (!out->resize (raw_len)) return false;
  z_stream stream = {};
  stream.next_in = (Bytef *) idat_data;
  stream.avail_in = idat_len;
  stream.next_out = out->arrayZ;
  stream.avail_out = raw_len;
  if (inflateInit (&stream) != Z_OK) return false;
  int status = inflate (&stream, Z_FINISH);
  unsigned long total_out = stream.total_out;
  inflateEnd (&stream);
  if (status != Z_STREAM_END || total_out != raw_len) return false;

  for (unsigned y = 0; y < height; y++)
  {
    uint8_t *row = out->arrayZ + y * stride;
    uint8_t filter = *row++;
    if (filter > 4) return false;
    const uint8_t *prev = y ? row - stride : nullptr;
    for (unsigned x = 0; x < row_bytes; x++)
    {
      uint8_t a = x >= channels ? row[x - channels] : 0;
      uint8_t b = prev ? prev[x] : 0;
      uint8_t c = prev && x >= channels ? prev[x - channels] : 0;
      uint8_t predictor = 0;
      switch (filter)
      {
      case 1: predictor = a; break;
      case 2: predictor = b; break;
      case 3: predictor = (uint8_t) (((unsigned) a + b) / 2); break;
      case 4:
      {
        int p = (int) a + b - c;
        int pa = abs (p - a), pb = abs (p - b), pc = abs (p - c);
        predictor = pa <= pb && pa <= pc ? a : pb <= pc ? b : c;
        break;
      }
      }
      row[x] = (uint8_t) (row[x] + predictor);
    }
  }
  return true;
#endif
}

/* Small streams do not justify setting up a deflater. Keep uncompressed data
 * if compression cannot shrink it or allocation fails. */
static bool
hb_pdf_compress_image_stream (hb_vector_buf_t *data, int64_t *budget)
{
#ifndef HAVE_ZLIB
  (void) data; (void) budget;
  return false;
#else
  if (data->length < 256) return false;
  if (!hb_budget_spend (*budget, HB_BUDGET_1, data->length)) return false;
  uLongf size = compressBound (data->length);
  if (size > INT_MAX) return false;
  hb_vector_t<uint8_t> compressed;
  if (!compressed.resize ((unsigned) size)) return false;
  if (compress2 (compressed.arrayZ, &size, (const Bytef *) data->arrayZ,
                 data->length, Z_BEST_SPEED) != Z_OK || size >= data->length)
    return false;
  if (!data->resize ((unsigned) size)) return false;
  hb_memcpy (data->arrayZ, compressed.arrayZ, (unsigned) size);
  return true;
#endif
}

/* PDF image samples do not carry alpha: emit separate color and mask streams. */
static bool
hb_pdf_split_png_alpha (hb_vector_buf_t *color, hb_vector_buf_t *alpha,
                        const char *idat_data, unsigned idat_len,
                        unsigned width, unsigned height, unsigned channels,
                        int64_t *budget)
{
  if (channels != 2 && channels != 4) return false;
  hb_vector_t<uint8_t> rows;
  if (!hb_pdf_decode_png_rows (&rows, idat_data, idat_len,
                              width, height, channels, budget))
    return false;
  /* The decoder checked these products while bounding all channels together. */
  unsigned pixels = width * height;
  unsigned colors = channels - 1;
  if (!color->resize (pixels * colors) || !alpha->resize (pixels))
    return false;
  unsigned stride = width * channels + 1;
  for (unsigned y = 0; y < height; y++)
    for (unsigned x = 0; x < width; x++)
    {
      const uint8_t *pixel = rows.arrayZ + y * stride + 1 + x * channels;
      unsigned index = y * width + x;
      hb_memcpy (color->arrayZ + index * colors, pixel, colors);
      alpha->arrayZ[index] = (char) pixel[colors];
    }
  return true;
}

static bool
hb_pdf_build_indexed_smask (hb_vector_buf_t *out,
                            const char *idat_data, unsigned idat_len,
                            unsigned width, unsigned height,
                            const uint8_t *trns, unsigned trns_len,
                            int64_t *budget)
{
  hb_vector_t<uint8_t> rows;
  if (!hb_pdf_decode_png_rows (&rows, idat_data, idat_len,
                              width, height, 1, budget) ||
      !out->resize (width * height))
    return false;
  for (unsigned y = 0; y < height; y++)
    for (unsigned x = 0; x < width; x++)
    {
      uint8_t index = rows.arrayZ[y * (width + 1) + 1 + x];
      out->arrayZ[y * width + x] = (char) (index < trns_len ? trns[index] : 0xFF);
    }
  return true;
}

#endif /* HB_VECTOR_PAINT_PDF_IMAGE_HH */
