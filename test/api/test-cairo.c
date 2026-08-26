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

#include "hb-test.h"

#include <hb-cairo.h>

static void
test_hb_cairo_budget (void)
{
  hb_face_t *face = hb_test_open_font_file ("../fuzzing/fonts/clusterfuzz-testcase-minimized-hb-vector-fuzzer-4886959609413632");
  cairo_font_face_t *cairo_face = hb_cairo_font_face_create_for_face (face);
  cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 64, 64);
  cairo_t *cr = cairo_create (surface);

  cairo_set_font_face (cr, cairo_face);
  cairo_set_font_size (cr, 30);

  cairo_glyph_t glyph = {0, 0, 0};
  cairo_show_glyphs (cr, &glyph, 1);

  g_assert_cmpint (cairo_status (cr), ==, CAIRO_STATUS_SUCCESS);
  g_assert_cmpint (cairo_surface_status (surface), ==, CAIRO_STATUS_SUCCESS);

  cairo_destroy (cr);
  cairo_surface_destroy (surface);
  cairo_font_face_destroy (cairo_face);
  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);
  hb_test_add (test_hb_cairo_budget);

  return hb_test_run ();
}
