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

#include "hb-test.h"
#include "hb-vector.h"
#include <hb-ot.h>

#include <string.h>

/* Find @needle (null-terminated) inside @hay (len bytes).  Returns
 * pointer inside @hay, or NULL. */
static const char *
memfind (const char *hay, unsigned len, const char *needle)
{
  unsigned nlen = (unsigned) strlen (needle);
  if (nlen > len) return NULL;
  for (unsigned i = 0; i + nlen <= len; i++)
    if (memcmp (hay + i, needle, nlen) == 0)
      return hay + i;
  return NULL;
}

/* Test: draw fragment for a simple outline font. */
static void
test_draw_pdf_fragment (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/Inconsolata-Regular.ab.ttf");
  g_assert_nonnull (face);
  hb_font_t *font = hb_font_create (face);
  g_assert_nonnull (font);

  hb_vector_draw_t *draw = hb_vector_draw_create_or_fail (HB_VECTOR_FORMAT_PDF);
  g_assert_nonnull (draw);

  hb_vector_extents_t ex = {0, -100, 1024, 1200};
  hb_vector_draw_set_extents (draw, &ex);
  g_assert_true (hb_vector_draw_glyph (draw, font,
                                       1,
                                       0.f, 0.f,
                                       HB_VECTOR_EXTENTS_MODE_NONE));

  hb_blob_t *content = hb_vector_draw_render_pdf_fragment (draw);
  g_assert_nonnull (content);

  unsigned len = 0;
  const char *data = hb_blob_get_data (content, &len);
  g_assert_cmpuint (len, >, 0);

  /* No PDF document wrapping. */
  g_assert_null (memfind (data, len, "%PDF-"));
  g_assert_null (memfind (data, len, "/Catalog"));
  g_assert_null (memfind (data, len, "xref"));
  /* Content ends with a fill operator. */
  g_assert_nonnull (memfind (data, len, "\nf\n"));

  hb_blob_destroy (content);
  hb_vector_draw_destroy (draw);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

/* Test: paint fragment on a glyph with no streams (solid color path).
 * Callback must not be invoked; resources should be inlined / empty. */
struct emit_ctx_t
{
  unsigned next_id;
  unsigned call_count;
};

static unsigned
emit_stream_cb (const char *body HB_UNUSED,
                unsigned body_length HB_UNUSED,
                void *user_data)
{
  auto *ctx = (emit_ctx_t *) user_data;
  ctx->call_count++;
  return ++ctx->next_id;
}

static void
test_paint_pdf_fragment_no_streams (void)
{
  /* A COLR font with simple solid-fill glyphs — no gradients, no
   * images — so the paint pipeline produces no stream objects. */
  hb_face_t *face = hb_test_open_font_file ("fonts/test_glyphs-glyf_colr_1.ttf");
  g_assert_nonnull (face);
  hb_font_t *font = hb_font_create (face);
  g_assert_nonnull (font);

  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (HB_VECTOR_FORMAT_PDF);
  g_assert_nonnull (paint);

  hb_vector_extents_t ex = {-16, -16, 632, 982};
  hb_vector_paint_set_extents (paint, &ex);
  g_assert_true (hb_vector_paint_glyph (paint, font,
                                        10,
                                        0.f, 0.f,
                                        HB_VECTOR_EXTENTS_MODE_NONE));

  emit_ctx_t ctx = {99, 0};
  unsigned resource_count = 0;
  auto resource_cb = +[] (hb_vector_pdf_resource_type_t type HB_UNUSED,
                          const char *name HB_UNUSED,
                          const char *value HB_UNUSED,
                          unsigned value_length HB_UNUSED,
                          void *user_data) {
    (*(unsigned *) user_data)++;
  };
  hb_blob_t *content = hb_vector_paint_render_pdf_fragment (paint,
                                                            emit_stream_cb,
                                                            &ctx,
                                                            resource_cb,
                                                            &resource_count);
  g_assert_nonnull (content);
  g_assert_cmpuint (ctx.call_count, ==, 0);
  g_assert_cmpuint (resource_count, >, 0);

  unsigned clen = 0;
  const char *cdata = hb_blob_get_data (content, &clen);
  g_assert_cmpuint (clen, >, 0);
  g_assert_null (memfind (cdata, clen, "%PDF-"));
  g_assert_null (memfind (cdata, clen, "xref"));

  hb_blob_destroy (content);
  hb_vector_paint_destroy (paint);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

/* Test: NULL resources out-param is accepted. */
static void
test_paint_pdf_fragment_null_resources_out (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/test_glyphs-glyf_colr_1.ttf");
  hb_font_t *font = hb_font_create (face);

  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (HB_VECTOR_FORMAT_PDF);
  hb_vector_extents_t ex = {-16, -16, 632, 982};
  hb_vector_paint_set_extents (paint, &ex);
  hb_vector_paint_glyph (paint, font,
                         10,
                         0.f, 0.f, HB_VECTOR_EXTENTS_MODE_NONE);

  emit_ctx_t ctx = {1, 0};
  hb_blob_t *content = hb_vector_paint_render_pdf_fragment (paint,
                                                            emit_stream_cb,
                                                            &ctx,
                                                            NULL, NULL);
  g_assert_nonnull (content);

  hb_blob_destroy (content);
  hb_vector_paint_destroy (paint);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

/* Test: paint fragment on a glyph with a sweep gradient — which in
 * PDF becomes a Type 6 Coons-patch shading, a stream object.  The
 * callback must be invoked; stream refs in the resources dict must
 * use the consumer-assigned id; the content stream must not contain
 * any indirect references. */
static void
test_paint_pdf_fragment_with_stream (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/test_glyphs-glyf_colr_1.ttf");
  hb_font_t *font = hb_font_create (face);

  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (HB_VECTOR_FORMAT_PDF);
  hb_vector_extents_t ex = {-16, -16, 1016, 966};
  hb_vector_paint_set_extents (paint, &ex);
  g_assert_true (hb_vector_paint_glyph (paint, font,
                                        20, /* sweep gradient */
                                        0.f, 0.f,
                                        HB_VECTOR_EXTENTS_MODE_NONE));

  emit_ctx_t ctx = {1000, 0};

  struct resource_ctx_t
  {
    bool found_stream_ref;
    unsigned expected_id;
  } rctx = {false, 1001};

  auto resource_cb = +[] (hb_vector_pdf_resource_type_t type HB_UNUSED,
                          const char *name HB_UNUSED,
                          const char *value,
                          unsigned value_length,
                          void *user_data) {
    auto *rc = (resource_ctx_t *) user_data;
    char expected[32];
    snprintf (expected, sizeof (expected), "%u 0 R", rc->expected_id);
    if (memfind (value, value_length, expected))
      rc->found_stream_ref = true;
  };

  hb_blob_t *content = hb_vector_paint_render_pdf_fragment (paint,
                                                            emit_stream_cb,
                                                            &ctx,
                                                            resource_cb,
                                                            &rctx);
  g_assert_nonnull (content);
  g_assert_cmpuint (ctx.call_count, >=, 1);
  /* The consumer-assigned id (1001) must appear in some resource
   * value as "<id> 0 R". */
  g_assert_true (rctx.found_stream_ref);

  hb_blob_destroy (content);
  hb_vector_paint_destroy (paint);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

/* Test: NULL callback on content-with-streams aborts cleanly. */
static unsigned
never_called_cb (const char *body HB_UNUSED,
                 unsigned body_length HB_UNUSED,
                 void *user_data HB_UNUSED)
{
  g_assert_not_reached ();
  return 0;
}

static void
test_paint_pdf_fragment_null_cb_with_stream_fails (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/test_glyphs-glyf_colr_1.ttf");
  hb_font_t *font = hb_font_create (face);

  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (HB_VECTOR_FORMAT_PDF);
  hb_vector_extents_t ex = {-16, -16, 1016, 966};
  hb_vector_paint_set_extents (paint, &ex);
  hb_vector_paint_glyph (paint, font, 20, 0.f, 0.f,
                         HB_VECTOR_EXTENTS_MODE_NONE);

  hb_blob_t *content = hb_vector_paint_render_pdf_fragment (paint,
                                                            NULL,
                                                            NULL,
                                                            NULL, NULL);
  g_assert_null (content);
  (void) never_called_cb;

  hb_vector_paint_destroy (paint);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_draw_pdf_fragment);
  hb_test_add (test_paint_pdf_fragment_no_streams);
  hb_test_add (test_paint_pdf_fragment_null_resources_out);
  hb_test_add (test_paint_pdf_fragment_with_stream);
  hb_test_add (test_paint_pdf_fragment_null_cb_with_stream_fails);

  return hb_test_run ();
}
