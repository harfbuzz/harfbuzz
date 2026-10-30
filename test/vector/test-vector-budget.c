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

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <hb.h>
#include <hb-vector.h>

static void
test_format (hb_vector_format_t format, hb_font_t *font)
{
  hb_vector_draw_t *draw = hb_vector_draw_create_or_fail (format);
  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (format);
  assert (draw);
  assert (paint);

  hb_draw_funcs_t *draw_funcs = hb_vector_draw_get_funcs (draw);
  hb_paint_funcs_t *paint_funcs = hb_vector_paint_get_funcs (paint);

  assert (hb_draw_get_budget (draw_funcs, draw) == HB_BUDGET_DEFAULT);
  assert (hb_draw_get_budget_remaining (draw_funcs, draw) > 0);
  assert (hb_draw_get_budget_remaining (draw_funcs, draw) < HB_BUDGET_UNLIMITED);
  assert (hb_paint_get_budget (paint_funcs, paint) == HB_BUDGET_DEFAULT);
  assert (hb_paint_get_budget_remaining (paint_funcs, paint) > 0);
  assert (hb_paint_get_budget_remaining (paint_funcs, paint) < HB_BUDGET_UNLIMITED);

  assert (hb_draw_set_budget (draw_funcs, draw, 1));
  hb_vector_draw_glyph (draw, font, 0, HB_VECTOR_EXTENTS_MODE_NONE);
  assert (hb_draw_get_budget_remaining (draw_funcs, draw) < 0);
  hb_vector_draw_clear (draw);
  assert (hb_draw_get_budget (draw_funcs, draw) == 1);
  assert (hb_draw_get_budget_remaining (draw_funcs, draw) == 1);
  hb_vector_draw_reset (draw);
  assert (hb_draw_get_budget (draw_funcs, draw) == HB_BUDGET_DEFAULT);
  assert (hb_draw_get_budget_remaining (draw_funcs, draw) > 0);

  assert (hb_paint_set_budget (paint_funcs, paint, 1));
  hb_vector_paint_glyph (paint, font, 0, HB_VECTOR_EXTENTS_MODE_NONE);
  assert (hb_paint_get_budget_remaining (paint_funcs, paint) < 0);
  hb_vector_paint_clear (paint);
  assert (hb_paint_get_budget (paint_funcs, paint) == 1);
  assert (hb_paint_get_budget_remaining (paint_funcs, paint) == 1);
  hb_vector_paint_reset (paint);
  assert (hb_paint_get_budget (paint_funcs, paint) == HB_BUDGET_DEFAULT);
  assert (hb_paint_get_budget_remaining (paint_funcs, paint) > 0);

  assert (hb_draw_set_budget (draw_funcs, draw, HB_BUDGET_UNLIMITED));
  assert (hb_draw_get_budget (draw_funcs, draw) == HB_BUDGET_UNLIMITED);
  assert (hb_draw_get_budget_remaining (draw_funcs, draw) == HB_BUDGET_UNLIMITED);
  assert (hb_paint_set_budget (paint_funcs, paint, HB_BUDGET_UNLIMITED));
  assert (hb_paint_get_budget (paint_funcs, paint) == HB_BUDGET_UNLIMITED);
  assert (hb_paint_get_budget_remaining (paint_funcs, paint) == HB_BUDGET_UNLIMITED);

  hb_vector_draw_destroy (draw);
  hb_vector_paint_destroy (paint);
}

/* Embedded bitmaps are not outline work, so their base64 output is bounded
 * by the document-size cap, not the outline budget: painting an oversized
 * image must keep the serialized document bounded instead of ballooning
 * with the image.  Exercised through SVG, whose image path base64-embeds
 * the raw blob directly; PDF assembles its whole document through the same
 * capped buffer (hb_vector_buf_t), but validates the PNG, so a bogus blob
 * cannot drive it here. */
static void
test_document_cap (void)
{
  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (HB_VECTOR_FORMAT_SVG);
  hb_paint_funcs_t *funcs = hb_vector_paint_get_funcs (paint);

  unsigned len = 40u << 20;           /* 40 MB, well past the 16 MB cap */
  char *data = (char *) calloc (len, 1);
  assert (data);
  hb_blob_t *image = hb_blob_create (data, len, HB_MEMORY_MODE_READONLY, data, free);

  hb_glyph_extents_t ext = {0, 100, 100, -100};
  hb_vector_paint_set_glyph_extents (paint, &ext);
  hb_paint_image (funcs, paint, image, 100, 100,
		  HB_PAINT_IMAGE_FORMAT_PNG, 0.f, &ext);

  hb_blob_t *out = hb_vector_paint_render (paint);
  /* The cap either makes render refuse the over-budget document (null) or
   * bounds its size; either way it must not balloon to the base64 of the
   * whole 40 MB image (~53 MB), which is what an uncapped buffer produces. */
  assert (!out || hb_blob_get_length (out) < (32u << 20));
  hb_blob_destroy (out);
  hb_blob_destroy (image);

  /* Reusing the context after an over-cap glyph must recover: the sticky
   * overflow flag has to be cleared, or every later render fails forever. */
  hb_vector_paint_clear (paint);
  hb_vector_paint_set_glyph_extents (paint, &ext);
  unsigned small_len = 12;
  char *small = (char *) calloc (small_len, 1);
  assert (small);
  hb_blob_t *small_image = hb_blob_create (small, small_len, HB_MEMORY_MODE_READONLY, small, free);
  hb_paint_image (funcs, paint, small_image, 4, 3,
		  HB_PAINT_IMAGE_FORMAT_PNG, 0.f, &ext);
  hb_blob_t *out2 = hb_vector_paint_render (paint);
  assert (out2);
  hb_blob_destroy (out2);
  hb_blob_destroy (small_image);

  hb_vector_paint_destroy (paint);
}

/* Minimal LSB-first bit writer, enough to hand-roll a zlib stream. */
typedef struct { unsigned char *data; unsigned len; unsigned bits; unsigned nbits; } bit_writer_t;

static void
put_bits (bit_writer_t *w, unsigned value, unsigned n)
{
  w->bits |= value << w->nbits;
  w->nbits += n;
  while (w->nbits >= 8)
  {
    w->data[w->len++] = (unsigned char) w->bits;
    w->bits >>= 8;
    w->nbits -= 8;
  }
}

/* Huffman codes are packed most-significant bit first. */
static void
put_code (bit_writer_t *w, unsigned code, unsigned n)
{
  unsigned rev = 0;
  for (unsigned i = 0; i < n; i++)
    rev |= ((code >> i) & 1u) << (n - 1 - i);
  put_bits (w, rev, n);
}

/* zlib-wrap n zero bytes as one fixed-Huffman deflate block: a literal 0
 * followed by <length 258, distance 1> copies, so ~13 bits per 258 bytes. */
static unsigned
zlib_zeros (unsigned char *out, unsigned n)
{
  bit_writer_t w = {out, 0, 0, 0};
  put_bits (&w, 0x78, 8); put_bits (&w, 0x01, 8); /* CMF, FLG */
  put_bits (&w, 1, 1);                            /* BFINAL */
  put_bits (&w, 1, 2);                            /* BTYPE: fixed Huffman */
  unsigned done = 0;
  if (done < n) { put_code (&w, 0x30, 8); done++; }            /* literal 0 */
  for (; n - done >= 258; done += 258)
  {
    put_code (&w, 0xC5, 8);                       /* length 258 (symbol 285) */
    put_code (&w, 0, 5);                          /* distance 1 (symbol 0) */
  }
  for (; done < n; done++) put_code (&w, 0x30, 8);             /* literal 0 */
  put_code (&w, 0, 7);                            /* end of block (symbol 256) */
  put_bits (&w, 0, (8 - w.nbits) % 8);
  unsigned adler = ((n % 65521u) << 16) | 1u;     /* adler32 of n zero bytes */
  for (int i = 3; i >= 0; i--) put_bits (&w, (adler >> (8 * i)) & 0xFF, 8);
  return w.len;
}

static void
png_u32 (unsigned char *p, unsigned v)
{
  p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v;
}

/* Append a PNG chunk; the PDF backend does not verify chunk CRCs. */
static unsigned
png_chunk (unsigned char *out, const char *type, const unsigned char *data, unsigned len)
{
  png_u32 (out, len);
  memcpy (out + 4, type, 4);
  if (len) memcpy (out + 8, data, len);
  memset (out + 8 + len, 0, 4);
  return 12 + len;
}

/* The PDF backend decodes an indexed PNG with tRNS into an uncompressed soft
 * mask of width * height bytes, which is bounded by the document-size cap
 * like everything else it emits.  A mask that cannot fit must be refused up
 * front, not inflated from a tiny IDAT into gigabytes and dropped afterwards;
 * and an image whose mask was dropped must not reference the empty object. */
static void
test_indexed_smask_cap (void)
{
  unsigned width = 4097, height = 4096; /* 16 MB + 4 KB, just past the cap */
  unsigned raw_len = (width + 1) * height;
  unsigned char *idat = (unsigned char *) malloc (raw_len / 258 * 2 + 1024);
  assert (idat);
  unsigned idat_len = zlib_zeros (idat, raw_len);

  unsigned char *png = (unsigned char *) malloc (idat_len + 128);
  assert (png);
  static const unsigned char png_sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
  unsigned char ihdr[13] = {0};
  png_u32 (ihdr, width);
  png_u32 (ihdr + 4, height);
  ihdr[8] = 8; /* bit depth */
  ihdr[9] = 3; /* indexed */
  static const unsigned char plte[3] = {0, 0, 0};
  static const unsigned char trns[1] = {0};
  unsigned len = 0;
  memcpy (png, png_sig, 8); len += 8;
  len += png_chunk (png + len, "IHDR", ihdr, sizeof (ihdr));
  len += png_chunk (png + len, "PLTE", plte, sizeof (plte));
  len += png_chunk (png + len, "tRNS", trns, sizeof (trns));
  len += png_chunk (png + len, "IDAT", idat, idat_len);
  len += png_chunk (png + len, "IEND", NULL, 0);
  free (idat);
  hb_blob_t *image = hb_blob_create ((char *) png, len, HB_MEMORY_MODE_READONLY, png, free);

  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (HB_VECTOR_FORMAT_PDF);
  hb_paint_funcs_t *funcs = hb_vector_paint_get_funcs (paint);
  hb_glyph_extents_t ext = {0, 100, 100, -100};
  hb_vector_paint_set_glyph_extents (paint, &ext);
  hb_paint_image (funcs, paint, image, width, height,
		  HB_PAINT_IMAGE_FORMAT_PNG, 0.f, &ext);
  hb_blob_t *out = hb_vector_paint_render (paint);
  assert (out);

  /* The image is embedded (its IDAT is small), but without a soft mask. */
  unsigned out_len;
  const char *out_data = hb_blob_get_data (out, &out_len);
  assert (out_len > len);
  for (unsigned i = 0; i + 6 <= out_len; i++)
    assert (memcmp (out_data + i, "/SMask", 6) != 0);

  hb_blob_destroy (out);
  hb_blob_destroy (image);
  hb_vector_paint_destroy (paint);
}

#define LEVELS 30
#define FAN_OUT 3

static void
put (char *p, unsigned size, unsigned value)
{
  while (size--)
  {
    p[size] = (char) (value & 255);
    value >>= 8;
  }
}

/* A chain of COLRv1 glyphs that draw nothing, each referring to the next
 * one FAN_OUT times.  The graph limits of the top-level glyph must cover the
 * nested glyphs as well, or this takes FAN_OUT ^ LEVELS steps. */
static void
test_nested_color_glyphs (hb_vector_format_t format)
{
  enum {
    BASE_GLYPH_LIST = 34,
    COLR_LAYERS = 4 + (LEVELS + 1) * 6, /* From BaseGlyphList. */
    LAYER_LIST = BASE_GLYPH_LIST + COLR_LAYERS + (LEVELS + 1) * 6,
    COLR_GLYPHS = 4 + LEVELS * FAN_OUT * 4, /* From LayerList. */
    SIZE = LAYER_LIST + COLR_GLYPHS + LEVELS * 3
  };
  char colr[SIZE] = {0};
  char *base_glyph_list = colr + BASE_GLYPH_LIST;
  char *layer_list = colr + LAYER_LIST;

  put (colr + 0, 2, 1); /* version */
  put (colr + 14, 4, BASE_GLYPH_LIST);
  put (colr + 18, 4, LAYER_LIST);
  put (base_glyph_list, 4, LEVELS + 1);
  put (layer_list, 4, LEVELS * FAN_OUT);
  for (unsigned i = 0; i <= LEVELS; i++)
  {
    put (base_glyph_list + 4 + i * 6, 2, i);
    put (base_glyph_list + 4 + i * 6 + 2, 4, COLR_LAYERS + i * 6);
    put (base_glyph_list + COLR_LAYERS + i * 6, 1, 1); /* PaintColrLayers */
    if (i == LEVELS)
      break; /* The last glyph has no layers. */
    put (base_glyph_list + COLR_LAYERS + i * 6 + 1, 1, FAN_OUT);
    put (base_glyph_list + COLR_LAYERS + i * 6 + 2, 4, i * FAN_OUT);
    for (unsigned j = 0; j < FAN_OUT; j++)
      put (layer_list + 4 + (i * FAN_OUT + j) * 4, 4, COLR_GLYPHS + i * 3);
    put (layer_list + COLR_GLYPHS + i * 3, 1, 11); /* PaintColrGlyph */
    put (layer_list + COLR_GLYPHS + i * 3 + 1, 2, i + 1);
  }

  hb_face_t *face = hb_face_builder_create ();
  hb_blob_t *blob = hb_blob_create (colr, SIZE, HB_MEMORY_MODE_READONLY, NULL, NULL);
  hb_face_builder_add_table (face, HB_TAG ('C','O','L','R'), blob);
  hb_blob_destroy (blob);
  hb_face_set_glyph_count (face, LEVELS + 1);
  hb_font_t *font = hb_font_create (face);

  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (format);
  assert (hb_vector_paint_glyph_or_fail (paint, font, 0, HB_VECTOR_EXTENTS_MODE_NONE));
  hb_vector_paint_destroy (paint);

  hb_font_destroy (font);
  hb_face_destroy (face);
}

#define GRADIENT_STOPS 1000

static unsigned
gradient_color_stops (hb_color_line_t *cl, void *cl_data,
		      unsigned start, unsigned *count,
		      hb_color_stop_t *stops, void *user_data)
{
  if (count && stops)
  {
    unsigned n = 0;
    for (; n < *count && start + n < GRADIENT_STOPS; n++)
    {
      stops[n].offset = (float) (start + n) / (GRADIENT_STOPS - 1);
      stops[n].is_foreground = 0;
      stops[n].color = HB_COLOR ((start + n) & 0xFF, 0, 0, 255);
    }
    *count = n;
  }
  return GRADIENT_STOPS;
}

static hb_paint_extend_t
gradient_extend (hb_color_line_t *cl, void *cl_data, void *user_data)
{ return HB_PAINT_EXTEND_PAD; }

/* The PDF backend emits one function object per pair of color stops for
 * every gradient paint, each object a buffer of its own.  The document cap
 * must bound their total, or a COLRv1 font that paints one large gradient
 * thousands of times allocates gigabytes that are only refused at render. */
static void
test_pdf_objects_cap (void)
{
  hb_color_line_t color_line = {0};
  color_line.get_color_stops = gradient_color_stops;
  color_line.get_extend = gradient_extend;

  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (HB_VECTOR_FORMAT_PDF);
  hb_paint_funcs_t *funcs = hb_vector_paint_get_funcs (paint);
  hb_glyph_extents_t ext = {0, 100, 100, -100};
  hb_vector_paint_set_glyph_extents (paint, &ext);
  for (unsigned i = 0; i < 500; i++)
    hb_paint_linear_gradient (funcs, paint, &color_line, 0, 0, 100, 0, 0, 100);
  /* ~500k function objects, far over the cap in total: render must refuse
   * rather than return a document built from objects that were dropped. */
  assert (!hb_vector_paint_render (paint));
  hb_vector_paint_destroy (paint);
}

int
main (int argc, char **argv)
{
  assert (argc == 2);
  hb_blob_t *blob = hb_blob_create_from_file_or_fail (argv[1]);
  assert (blob);
  hb_face_t *face = hb_face_create (blob, 0);
  hb_font_t *font = hb_font_create (face);

  test_format (HB_VECTOR_FORMAT_SVG, font);
  test_format (HB_VECTOR_FORMAT_PDF, font);
  test_document_cap ();
  test_indexed_smask_cap ();
  test_nested_color_glyphs (HB_VECTOR_FORMAT_SVG);
  test_nested_color_glyphs (HB_VECTOR_FORMAT_PDF);
  test_pdf_objects_cap ();

  hb_font_destroy (font);
  hb_face_destroy (face);
  hb_blob_destroy (blob);
  return 0;
}
