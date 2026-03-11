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
 */

#ifndef HB_ZLIB_HH
#define HB_ZLIB_HH

#include "hb-blob.hh"

#ifdef HAVE_ZLIB
#include <zlib.h>
#endif

static inline bool
hb_blob_is_gzip (const char *data,
                 unsigned    data_len)
{
  return data_len >= 3 &&
         (unsigned char) data[0] == 0x1Fu &&
         (unsigned char) data[1] == 0x8Bu &&
         (unsigned char) data[2] == 0x08u;
}

static inline bool
hb_gzip_get_uncompressed_size (const char *data,
                               unsigned    data_len,
                               uint32_t   *size)
{
  if (data_len < 4)
    return false;

  const unsigned char *trailer = (const unsigned char *) data + data_len - 4;
  if (size)
    *size = (uint32_t) trailer[0] |
            ((uint32_t) trailer[1] << 8) |
            ((uint32_t) trailer[2] << 16) |
            ((uint32_t) trailer[3] << 24);
  return true;
}

#ifdef HAVE_ZLIB
static inline hb_blob_t *
hb_blob_decompress_gzip (hb_blob_t *blob,
                         unsigned   max_output_len)
{
  unsigned compressed_len = 0;
  const uint8_t *compressed = (const uint8_t *) hb_blob_get_data (blob, &compressed_len);
  if (!compressed || !compressed_len)
    return nullptr;

  z_stream stream = {};
  stream.next_in = (Bytef *) compressed;
  stream.avail_in = compressed_len;

  if (inflateInit2 (&stream, 16 + MAX_WBITS) != Z_OK)
    return nullptr;

  uint32_t expected_size = 0;
  hb_gzip_get_uncompressed_size ((const char *) compressed,
                                 compressed_len,
                                 &expected_size);

  size_t allocated = hb_min ((size_t) hb_max (expected_size, 4096u),
                             (size_t) max_output_len);
  char *output = (char *) hb_malloc (allocated);
  if (!output)
  {
    inflateEnd (&stream);
    return nullptr;
  }

  int status = Z_OK;
  while (true)
  {
    size_t produced = (size_t) stream.total_out;
    if (unlikely (produced >= (size_t) max_output_len))
      goto fail;

    if (produced == allocated)
    {
      size_t new_allocated = hb_min (allocated * 2, (size_t) max_output_len);
      if (unlikely (new_allocated <= allocated))
        goto fail;

      char *new_output = (char *) hb_realloc (output, new_allocated);
      if (unlikely (!new_output))
        goto fail;

      output = new_output;
      allocated = new_allocated;
    }

    stream.next_out = (Bytef *) output + stream.total_out;
    stream.avail_out = (uInt) (allocated - (size_t) stream.total_out);

    status = inflate (&stream, Z_FINISH);
    if (status == Z_STREAM_END)
      break;

    if ((status == Z_OK || status == Z_BUF_ERROR) && stream.avail_out == 0)
      continue;

    goto fail;
  }

  inflateEnd (&stream);

  if (unlikely ((size_t) stream.total_out > (size_t) max_output_len))
    goto fail_free;

  return hb_blob_create_or_fail (output,
                                 (unsigned) stream.total_out,
                                 HB_MEMORY_MODE_WRITABLE,
                                 output,
                                 hb_free);

fail:
  inflateEnd (&stream);
fail_free:
  hb_free (output);
  return nullptr;
}
#endif

#endif /* HB_ZLIB_HH */
