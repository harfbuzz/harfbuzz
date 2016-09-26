/*
 * Copyright © 2016  Igalia S.L.
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
 * Igalia Author(s): Frédéric Wang
 */


#include "hb-test.h"

#include "hb-ft.h"
#include "hb-ot.h"

/* Unit tests for hb-ot-shape.h - OpenType MATH table  */

static FT_Face ft_face;
static FT_Library ft_library;
static hb_buffer_t *hb_buffer;
static hb_face_t *hb_face;
static hb_font_t *hb_font;
static unsigned int len;
static hb_glyph_info_t *info;
static hb_glyph_position_t *pos;

static const hb_codepoint_t arrowleftCodePoint = 0x2190;
static const hb_codepoint_t arrowupCodePoint = 0x2191;
static const hb_codepoint_t arrowdownCodePoint = 0x2193;
static const hb_codepoint_t arrowbothCodePoint = 0x2194;
static const unsigned int cluster = 77;
static const int fontSize = 1000;

static void
initFreeType()
{
  FT_Error ft_error;
  if ((ft_error = FT_Init_FreeType (&ft_library)))
    abort();
}

static void
cleanupFreeType()
{
  FT_Done_FreeType (ft_library);
}

static void
openFont(const char* fontFile)
{
  gchar* path = g_test_build_filename(G_TEST_DIST, fontFile, NULL);

  FT_Error ft_error;
  if ((ft_error = FT_New_Face (ft_library, path, 0, &ft_face))) {
    g_free(path);
    abort();
  }
  g_free(path);
  if ((ft_error = FT_Set_Char_Size (ft_face, fontSize, fontSize, 0, 0)))
    abort();
  hb_font = hb_ft_font_create (ft_face, NULL);
  hb_face = hb_ft_face_create_cached(ft_face);
}

static void
closeFont()
{
  hb_font_destroy (hb_font);
  FT_Done_Face (ft_face);
}

static void
createBuffer (hb_codepoint_t codepoint, hb_bool_t pre_shape)
{
  hb_buffer = hb_buffer_create ();
  hb_buffer_add (hb_buffer, codepoint, cluster);
  hb_buffer_set_content_type (hb_buffer, HB_BUFFER_CONTENT_TYPE_UNICODE);
  hb_buffer_set_direction (hb_buffer, HB_DIRECTION_LTR);
  if (pre_shape) hb_shape (hb_font, hb_buffer, NULL, 0);
}

static void
getBufferData()
{
  len = hb_buffer_get_length (hb_buffer);
  info = hb_buffer_get_glyph_infos (hb_buffer, NULL);
  pos = hb_buffer_get_glyph_positions (hb_buffer, NULL);
}

static void
destroyBuffer()
{
  hb_buffer_destroy (hb_buffer);
}

static void
test_shape_math_stretchy_none (void)
{
  hb_codepoint_t glyph;

  initFreeType();
  openFont("fonts/MathTestFontFull.otf");
  g_assert(hb_ot_layout_has_math_data (hb_face));

  /* Stretch arrowleft vertically */
  createBuffer (arrowleftCodePoint, TRUE);
  g_assert (hb_ot_shape_math_stretchy (hb_font, hb_buffer, FALSE, 2 * fontSize));
  g_assert_cmpint (hb_buffer_get_content_type (hb_buffer), ==, HB_BUFFER_CONTENT_TYPE_GLYPHS);

  getBufferData();
  g_assert_cmpint (len, ==, 1);
  g_assert (hb_font_get_glyph_from_name (hb_font, "arrowleft", -1, &glyph));
  g_assert_cmpint (info[0].codepoint, ==, glyph);
  g_assert_cmpint (info[0].cluster, ==, cluster);
  g_assert_cmpint (pos[0].x_offset, ==, 0);
  g_assert_cmpint (pos[0].y_offset, ==, 0);
  g_assert_cmpint (pos[0].x_advance, ==,
                   hb_font_get_glyph_h_advance(hb_font, info[0].codepoint));
  g_assert_cmpint (pos[0].y_advance, ==,
                   hb_font_get_glyph_v_advance(hb_font, info[0].codepoint));
  destroyBuffer();

  /* Stretch arrowup horizontally */
  createBuffer (arrowupCodePoint, TRUE);
  g_assert (hb_ot_shape_math_stretchy (hb_font, hb_buffer, TRUE, 2 * fontSize));
  g_assert_cmpint (hb_buffer_get_content_type (hb_buffer), ==, HB_BUFFER_CONTENT_TYPE_GLYPHS);
  getBufferData();
  g_assert_cmpint (len, ==, 1);
  g_assert (hb_font_get_glyph_from_name (hb_font, "arrowup", -1, &glyph));
  g_assert_cmpint (info[0].codepoint, ==, glyph);
  g_assert_cmpint (info[0].cluster, ==, cluster);
  g_assert_cmpint (pos[0].x_offset, ==, 0);
  g_assert_cmpint (pos[0].y_offset, ==, 0);
  g_assert_cmpint (pos[0].x_advance, ==,
                   hb_font_get_glyph_h_advance(hb_font, info[0].codepoint));
  g_assert_cmpint (pos[0].y_advance, ==,
                   hb_font_get_glyph_v_advance(hb_font, info[0].codepoint));
  destroyBuffer();

  closeFont();
  cleanupFreeType();
}

static void
test_shape_math_stretchy_horizontal_glyph_variants (void)
{
  hb_codepoint_t glyph;

  initFreeType();
  openFont("fonts/MathTestFontFull.otf");
  g_assert(hb_ot_layout_has_math_data (hb_face));

  /* base size */
  createBuffer (arrowleftCodePoint, TRUE);
  g_assert (hb_ot_shape_math_stretchy (hb_font, hb_buffer, TRUE, .8 * fontSize));
  g_assert_cmpint (hb_buffer_get_content_type (hb_buffer), ==, HB_BUFFER_CONTENT_TYPE_GLYPHS);
  getBufferData();
  g_assert_cmpint (len, ==, 1);
  g_assert (hb_font_get_glyph_from_name (hb_font, "arrowleft", -1, &glyph));
  g_assert_cmpint (info[0].codepoint, ==, glyph);
  g_assert_cmpint (info[0].cluster, ==, cluster);
  g_assert_cmpint (pos[0].x_offset, ==, 0);
  g_assert_cmpint (pos[0].y_offset, ==, 0);
  g_assert_cmpint (pos[0].x_advance, ==,
                   hb_font_get_glyph_h_advance(hb_font, info[0].codepoint));
  g_assert_cmpint (pos[0].y_advance, ==,
                   hb_font_get_glyph_v_advance(hb_font, info[0].codepoint));
  destroyBuffer();

  /* size 2 */
  createBuffer (arrowleftCodePoint, TRUE);
  g_assert (hb_ot_shape_math_stretchy (hb_font, hb_buffer, TRUE, 1.5 * fontSize));
  g_assert_cmpint (hb_buffer_get_content_type (hb_buffer), ==, HB_BUFFER_CONTENT_TYPE_GLYPHS);
  getBufferData();
  g_assert_cmpint (len, ==, 1);
  g_assert (hb_font_get_glyph_from_name (hb_font, "uni2190_size2", -1, &glyph));
  g_assert_cmpint (info[0].codepoint, ==, glyph);
  g_assert_cmpint (info[0].cluster, ==, cluster);
  g_assert_cmpint (pos[0].x_offset, ==, 0);
  g_assert_cmpint (pos[0].y_offset, ==, 0);
  g_assert_cmpint (pos[0].x_advance, ==,
                   hb_font_get_glyph_h_advance(hb_font, info[0].codepoint));
  g_assert_cmpint (pos[0].y_advance, ==,
                   hb_font_get_glyph_v_advance(hb_font, info[0].codepoint));
  destroyBuffer();

  /* size 3 */
  createBuffer (arrowleftCodePoint, TRUE);
  g_assert (hb_ot_shape_math_stretchy (hb_font, hb_buffer, TRUE, 2.2 * fontSize));
  g_assert_cmpint (hb_buffer_get_content_type (hb_buffer), ==, HB_BUFFER_CONTENT_TYPE_GLYPHS);
  getBufferData();
  g_assert_cmpint (len, ==, 1);
  g_assert (hb_font_get_glyph_from_name (hb_font, "uni2190_size3", -1, &glyph));
  g_assert_cmpint (info[0].codepoint, ==, glyph);
  g_assert_cmpint (info[0].cluster, ==, cluster);
  g_assert_cmpint (pos[0].x_offset, ==, 0);
  g_assert_cmpint (pos[0].y_offset, ==, 0);
  g_assert_cmpint (pos[0].x_advance, ==,
                   hb_font_get_glyph_h_advance(hb_font, info[0].codepoint));
  g_assert_cmpint (pos[0].y_advance, ==,
                   hb_font_get_glyph_v_advance(hb_font, info[0].codepoint));
  destroyBuffer();

  /* size 4 */
  createBuffer (arrowleftCodePoint, TRUE);
  g_assert (hb_ot_shape_math_stretchy (hb_font, hb_buffer, TRUE, 2.9 * fontSize));
  g_assert_cmpint (hb_buffer_get_content_type (hb_buffer), ==, HB_BUFFER_CONTENT_TYPE_GLYPHS);
  getBufferData();
  g_assert_cmpint (len, ==, 1);
  g_assert (hb_font_get_glyph_from_name (hb_font, "uni2190_size4", -1, &glyph));
  g_assert_cmpint (info[0].codepoint, ==, glyph);
  g_assert_cmpint (info[0].cluster, ==, cluster);
  g_assert_cmpint (pos[0].x_offset, ==, 0);
  g_assert_cmpint (pos[0].y_offset, ==, 0);
  g_assert_cmpint (pos[0].x_advance, ==,
                   hb_font_get_glyph_h_advance(hb_font, info[0].codepoint));
  g_assert_cmpint (pos[0].y_advance, ==,
                   hb_font_get_glyph_v_advance(hb_font, info[0].codepoint));
  destroyBuffer();

  closeFont();
  cleanupFreeType();
}

static void
test_shape_math_stretchy_vertical_glyph_variants (void)
{
  hb_codepoint_t glyph;

  initFreeType();
  openFont("fonts/MathTestFontFull.otf");
  g_assert(hb_ot_layout_has_math_data (hb_face));

  /* base size */
  createBuffer (arrowupCodePoint, TRUE);
  g_assert (hb_ot_shape_math_stretchy (hb_font, hb_buffer, FALSE, fontSize));
  g_assert_cmpint (hb_buffer_get_content_type (hb_buffer), ==, HB_BUFFER_CONTENT_TYPE_GLYPHS);
  getBufferData();
  g_assert_cmpint (len, ==, 1);
  g_assert (hb_font_get_glyph_from_name (hb_font, "arrowup", -1, &glyph));
  g_assert_cmpint (info[0].codepoint, ==, glyph);
  g_assert_cmpint (info[0].cluster, ==, cluster);
  g_assert_cmpint (pos[0].x_offset, ==, 0);
  g_assert_cmpint (pos[0].y_offset, ==, 0);
  g_assert_cmpint (pos[0].x_advance, ==,
                   hb_font_get_glyph_h_advance(hb_font, info[0].codepoint));
  g_assert_cmpint (pos[0].y_advance, ==,
                   hb_font_get_glyph_v_advance(hb_font, info[0].codepoint));
  destroyBuffer();

  /* size 2 */
  createBuffer (arrowupCodePoint, TRUE);
  g_assert (hb_ot_shape_math_stretchy (hb_font, hb_buffer, FALSE, 2 * fontSize));
  g_assert_cmpint (hb_buffer_get_content_type (hb_buffer), ==, HB_BUFFER_CONTENT_TYPE_GLYPHS);
  getBufferData();
  g_assert_cmpint (len, ==, 1);
  g_assert (hb_font_get_glyph_from_name (hb_font, "uni2191_size2", -1, &glyph));
  g_assert_cmpint (info[0].codepoint, ==, glyph);
  g_assert_cmpint (info[0].cluster, ==, cluster);
  g_assert_cmpint (pos[0].x_offset, ==, 0);
  g_assert_cmpint (pos[0].y_offset, ==, 0);
  g_assert_cmpint (pos[0].x_advance, ==,
                   hb_font_get_glyph_h_advance(hb_font, info[0].codepoint));
  g_assert_cmpint (pos[0].y_advance, ==,
                   hb_font_get_glyph_v_advance(hb_font, info[0].codepoint));
  destroyBuffer();

  /* size 3 */
  createBuffer (arrowupCodePoint, TRUE);
  g_assert (hb_ot_shape_math_stretchy (hb_font, hb_buffer, FALSE, 2.3 * fontSize));
  g_assert_cmpint (hb_buffer_get_content_type (hb_buffer), ==, HB_BUFFER_CONTENT_TYPE_GLYPHS);
  getBufferData();
  g_assert_cmpint (len, ==, 1);
  g_assert (hb_font_get_glyph_from_name (hb_font, "uni2191_size3", -1, &glyph));
  g_assert_cmpint (info[0].codepoint, ==, glyph);
  g_assert_cmpint (info[0].cluster, ==, cluster);
  g_assert_cmpint (pos[0].x_offset, ==, 0);
  g_assert_cmpint (pos[0].y_offset, ==, 0);
  g_assert_cmpint (pos[0].x_advance, ==,
                   hb_font_get_glyph_h_advance(hb_font, info[0].codepoint));
  g_assert_cmpint (pos[0].y_advance, ==,
                   hb_font_get_glyph_v_advance(hb_font, info[0].codepoint));
  destroyBuffer();

  /* size 4 */
  createBuffer (arrowupCodePoint, TRUE);
  g_assert (hb_ot_shape_math_stretchy (hb_font, hb_buffer, FALSE, 3 * fontSize));
  g_assert_cmpint (hb_buffer_get_content_type (hb_buffer), ==, HB_BUFFER_CONTENT_TYPE_GLYPHS);
  getBufferData();
  g_assert_cmpint (len, ==, 1);
  g_assert (hb_font_get_glyph_from_name (hb_font, "uni2191_size4", -1, &glyph));
  g_assert_cmpint (info[0].codepoint, ==, glyph);
  g_assert_cmpint (info[0].cluster, ==, cluster);
  g_assert_cmpint (pos[0].x_offset, ==, 0);
  g_assert_cmpint (pos[0].y_offset, ==, 0);
  g_assert_cmpint (pos[0].x_advance, ==,
                   hb_font_get_glyph_h_advance(hb_font, info[0].codepoint));
  g_assert_cmpint (pos[0].y_advance, ==,
                   hb_font_get_glyph_v_advance(hb_font, info[0].codepoint));
  destroyBuffer();

  /* size 5 */
  createBuffer (arrowupCodePoint, TRUE);
  g_assert (hb_ot_shape_math_stretchy (hb_font, hb_buffer, FALSE, 3.5 * fontSize));
  g_assert_cmpint (hb_buffer_get_content_type (hb_buffer), ==, HB_BUFFER_CONTENT_TYPE_GLYPHS);
  getBufferData();
  g_assert_cmpint (len, ==, 1);
  g_assert (hb_font_get_glyph_from_name (hb_font, "uni2191_size5", -1, &glyph));
  g_assert_cmpint (info[0].codepoint, ==, glyph);
  g_assert_cmpint (info[0].cluster, ==, cluster);
  g_assert_cmpint (pos[0].x_offset, ==, 0);
  g_assert_cmpint (pos[0].y_offset, ==, 0);
  g_assert_cmpint (pos[0].x_advance, ==,
                   hb_font_get_glyph_h_advance(hb_font, info[0].codepoint));
  g_assert_cmpint (pos[0].y_advance, ==,
                   hb_font_get_glyph_v_advance(hb_font, info[0].codepoint));
  destroyBuffer();

  closeFont();
  cleanupFreeType();
}

static void
test_shape_math_stretchy_horizontal_glyph_assembly (void)
{
  initFreeType();
  openFont("fonts/MathTestFontFull.otf");
  g_assert(hb_ot_layout_has_math_data (hb_face));

  hb_codepoint_t arrowboth, left, right, center, horizontal;
  g_assert (hb_font_get_glyph_from_name (hb_font, "arrowboth", -1, &arrowboth));
  g_assert (hb_font_get_glyph_from_name (hb_font, "left", -1, &left));
  g_assert (hb_font_get_glyph_from_name (hb_font, "right", -1, &right));
  g_assert (hb_font_get_glyph_from_name (hb_font, "center", -1, &center));
  g_assert (hb_font_get_glyph_from_name (hb_font, "horizontal", -1, &horizontal));

  // base size
  createBuffer (arrowbothCodePoint, TRUE);
  g_assert (hb_ot_shape_math_stretchy (hb_font, hb_buffer, TRUE, fontSize));
  g_assert_cmpint (hb_buffer_get_content_type (hb_buffer), ==, HB_BUFFER_CONTENT_TYPE_GLYPHS);
  getBufferData();
  g_assert_cmpint (len, ==, 1);
  g_assert_cmpint (info[0].codepoint, ==, arrowboth);
  g_assert_cmpint (info[0].cluster, ==, cluster);
  g_assert_cmpint (pos[0].x_offset, ==, 0);
  g_assert_cmpint (pos[0].y_offset, ==, 0);
  g_assert_cmpint (pos[0].x_advance, ==,
                   hb_font_get_glyph_h_advance(hb_font, info[0].codepoint));
  g_assert_cmpint (pos[0].y_advance, ==,
                   hb_font_get_glyph_v_advance(hb_font, info[0].codepoint));
  destroyBuffer();

  // small size
  createBuffer (arrowbothCodePoint, TRUE);
  g_assert (hb_ot_shape_math_stretchy (hb_font, hb_buffer, TRUE, 3.8 * fontSize));
  g_assert_cmpint (hb_buffer_get_content_type (hb_buffer), ==, HB_BUFFER_CONTENT_TYPE_GLYPHS);
  getBufferData();
  g_assert_cmpint (len, ==, 4);

  g_assert_cmpint (info[0].codepoint, ==, left);
  g_assert_cmpint (info[0].cluster, ==, cluster);
  g_assert_cmpint (pos[0].x_offset, ==, 0);
  g_assert_cmpint (pos[0].y_offset, ==, 0);
  g_assert_cmpint (pos[0].x_advance, ==, 0);
  g_assert_cmpint (pos[0].y_advance, ==, 0);

  g_assert_cmpint (info[1].codepoint, ==, center);
  g_assert_cmpint (info[1].cluster, ==, cluster);
  g_assert_cmpint (pos[1].x_offset, ==, 934);
  g_assert_cmpint (pos[1].y_offset, ==, 0);
  g_assert_cmpint (pos[1].x_advance, ==, 0);
  g_assert_cmpint (pos[1].y_advance, ==, 0);

  g_assert_cmpint (info[2].codepoint, ==, center);
  g_assert_cmpint (info[2].cluster, ==, cluster);
  g_assert_cmpint (pos[2].x_offset, ==, 1868);
  g_assert_cmpint (pos[2].y_offset, ==, 0);
  g_assert_cmpint (pos[2].x_advance, ==, 0);
  g_assert_cmpint (pos[2].y_advance, ==, 0);

  g_assert_cmpint (info[3].codepoint, ==, right);
  g_assert_cmpint (info[3].cluster, ==, cluster);
  g_assert_cmpint (pos[3].x_offset, ==, 2802);
  g_assert_cmpint (pos[3].y_offset, ==, 0);
  g_assert_cmpint (pos[3].x_advance, ==, 3802);
  g_assert_cmpint (pos[3].y_advance, ==, -fontSize);

  destroyBuffer();

  /* large size */
  createBuffer (arrowbothCodePoint, TRUE);
  g_assert (hb_ot_shape_math_stretchy (hb_font, hb_buffer, TRUE, 11 * fontSize));
  g_assert_cmpint (hb_buffer_get_content_type (hb_buffer), ==, HB_BUFFER_CONTENT_TYPE_GLYPHS);
  getBufferData();
  g_assert_cmpint (len, ==, 13);

  g_assert_cmpint (info[0].codepoint, ==, left);
  g_assert_cmpint (info[0].cluster, ==, cluster);
  g_assert_cmpint (pos[0].x_offset, ==, 0);
  g_assert_cmpint (info[1].codepoint, ==, horizontal);
  g_assert_cmpint (info[1].cluster, ==, cluster);
  g_assert_cmpint (pos[1].x_offset, ==, 911);
  g_assert_cmpint (info[2].codepoint, ==, horizontal);
  g_assert_cmpint (info[2].cluster, ==, cluster);
  g_assert_cmpint (pos[2].x_offset, ==, 1822);
  g_assert_cmpint (info[3].codepoint, ==, horizontal);
  g_assert_cmpint (info[3].cluster, ==, cluster);
  g_assert_cmpint (pos[3].x_offset, ==, 2733);
  g_assert_cmpint (info[4].codepoint, ==, center);
  g_assert_cmpint (info[4].cluster, ==, cluster);
  g_assert_cmpint (pos[4].x_offset, ==, 3644);
  g_assert_cmpint (info[5].codepoint, ==, horizontal);
  g_assert_cmpint (info[5].cluster, ==, cluster);
  g_assert_cmpint (pos[5].x_offset, ==, 4555);
  g_assert_cmpint (info[6].codepoint, ==, horizontal);
  g_assert_cmpint (info[6].cluster, ==, cluster);
  g_assert_cmpint (pos[6].x_offset, ==, 5466);
  g_assert_cmpint (info[7].codepoint, ==, horizontal);
  g_assert_cmpint (info[7].cluster, ==, cluster);
  g_assert_cmpint (pos[7].x_offset, ==, 6377);
  g_assert_cmpint (info[8].codepoint, ==, center);
  g_assert_cmpint (info[8].cluster, ==, cluster);
  g_assert_cmpint (pos[8].x_offset, ==, 7288);
  g_assert_cmpint (info[9].codepoint, ==, horizontal);
  g_assert_cmpint (info[9].cluster, ==, cluster);
  g_assert_cmpint (pos[9].x_offset, ==, 8199);
  g_assert_cmpint (info[10].codepoint, ==, horizontal);
  g_assert_cmpint (info[10].cluster, ==, cluster);
  g_assert_cmpint (pos[10].x_offset, ==, 9110);
  g_assert_cmpint (info[11].codepoint, ==, horizontal);
  g_assert_cmpint (info[11].cluster, ==, cluster);
  g_assert_cmpint (pos[11].x_offset, ==, 10021);
  g_assert_cmpint (info[12].codepoint, ==, right);
  g_assert_cmpint (info[12].cluster, ==, cluster);
  g_assert_cmpint (pos[12].x_offset, ==, 10932);

  g_assert_cmpint (pos[12].x_advance, ==, 11932);
  g_assert_cmpint (pos[12].y_advance, ==, -fontSize);

  destroyBuffer();

  closeFont();
  cleanupFreeType();
}

static void
test_shape_math_stretchy_vertical_glyph_assembly (void)
{
  initFreeType();
  openFont("fonts/MathTestFontFull.otf");
  g_assert(hb_ot_layout_has_math_data (hb_face));

  hb_codepoint_t size6, arrowdown, top, bottom, center, vertical;
  g_assert (hb_font_get_glyph_from_name (hb_font, "uni2193_size6", -1, &size6));
  g_assert (hb_font_get_glyph_from_name (hb_font, "arrowdown", -1, &arrowdown));
  g_assert (hb_font_get_glyph_from_name (hb_font, "top", -1, &top));
  g_assert (hb_font_get_glyph_from_name (hb_font, "bottom", -1, &bottom));
  g_assert (hb_font_get_glyph_from_name (hb_font, "center", -1, &center));
  g_assert (hb_font_get_glyph_from_name (hb_font, "vertical", -1, &vertical));

  // base size is used before glyph assembly
  createBuffer (arrowdownCodePoint, TRUE);
  g_assert (hb_ot_shape_math_stretchy (hb_font, hb_buffer, FALSE, fontSize));
  g_assert_cmpint (hb_buffer_get_content_type (hb_buffer), ==, HB_BUFFER_CONTENT_TYPE_GLYPHS);
  getBufferData();
  g_assert_cmpint (len, ==, 1);
  g_assert_cmpint (info[0].codepoint, ==, arrowdown);
  g_assert_cmpint (info[0].cluster, ==, cluster);
  g_assert_cmpint (pos[0].x_offset, ==, 0);
  g_assert_cmpint (pos[0].y_offset, ==, 0);
  g_assert_cmpint (pos[0].x_advance, ==,
                   hb_font_get_glyph_h_advance(hb_font, info[0].codepoint));
  g_assert_cmpint (pos[0].y_advance, ==,
                   hb_font_get_glyph_v_advance(hb_font, info[0].codepoint));
  destroyBuffer();

  // size variant is used before glyph assembly
  createBuffer (arrowdownCodePoint, TRUE);
  g_assert (hb_ot_shape_math_stretchy (hb_font, hb_buffer, FALSE, 4 * fontSize));
  g_assert_cmpint (hb_buffer_get_content_type (hb_buffer), ==, HB_BUFFER_CONTENT_TYPE_GLYPHS);
  getBufferData();
  g_assert_cmpint (len, ==, 1);
  g_assert_cmpint (info[0].codepoint, ==, size6);
  g_assert_cmpint (info[0].cluster, ==, cluster);
  g_assert_cmpint (pos[0].x_offset, ==, 0);
  g_assert_cmpint (pos[0].y_offset, ==, 0);
  g_assert_cmpint (pos[0].x_advance, ==,
                   hb_font_get_glyph_h_advance(hb_font, info[0].codepoint));
  g_assert_cmpint (pos[0].y_advance, ==,
                   hb_font_get_glyph_v_advance(hb_font, info[0].codepoint));
  destroyBuffer();

  // glyph assembly
  createBuffer (arrowdownCodePoint, TRUE);
  g_assert (hb_ot_shape_math_stretchy (hb_font, hb_buffer, FALSE, 5 * fontSize));
  g_assert_cmpint (hb_buffer_get_content_type (hb_buffer), ==, HB_BUFFER_CONTENT_TYPE_GLYPHS);
  getBufferData();
  g_assert_cmpint (len, ==, 7);

  g_assert_cmpint (info[0].codepoint, ==, bottom);
  g_assert_cmpint (info[0].cluster, ==, cluster);
  g_assert_cmpint (pos[0].x_offset, ==, 0);
  g_assert_cmpint (pos[0].y_offset, ==, 0);
  g_assert_cmpint (pos[0].x_advance, ==, 0);
  g_assert_cmpint (pos[0].y_advance, ==, 0);

  g_assert_cmpint (info[1].codepoint, ==, vertical);
  g_assert_cmpint (info[1].cluster, ==, cluster);
  g_assert_cmpint (pos[1].x_offset, ==, 0);
  g_assert_cmpint (pos[1].y_offset, ==, 946);
  g_assert_cmpint (pos[1].x_advance, ==, 0);
  g_assert_cmpint (pos[1].y_advance, ==, 0);

  g_assert_cmpint (info[2].codepoint, ==, vertical);
  g_assert_cmpint (info[2].cluster, ==, cluster);
  g_assert_cmpint (pos[2].x_offset, ==, 0);
  g_assert_cmpint (pos[2].y_offset, ==, 1892);
  g_assert_cmpint (pos[2].x_advance, ==, 0);
  g_assert_cmpint (pos[2].y_advance, ==, 0);

  g_assert_cmpint (info[3].codepoint, ==, center);
  g_assert_cmpint (info[3].cluster, ==, cluster);
  g_assert_cmpint (pos[3].x_offset, ==, 0);
  g_assert_cmpint (pos[3].y_offset, ==, 2838);
  g_assert_cmpint (pos[3].x_advance, ==, 0);
  g_assert_cmpint (pos[3].y_advance, ==, 0);

  g_assert_cmpint (info[4].codepoint, ==, vertical);
  g_assert_cmpint (info[4].cluster, ==, cluster);
  g_assert_cmpint (pos[4].x_offset, ==, 0);
  g_assert_cmpint (pos[4].y_offset, ==, 3784);
  g_assert_cmpint (pos[4].x_advance, ==, 0);
  g_assert_cmpint (pos[4].y_advance, ==, 0);

  g_assert_cmpint (info[5].codepoint, ==, vertical);
  g_assert_cmpint (info[5].cluster, ==, cluster);
  g_assert_cmpint (pos[5].x_offset, ==, 0);
  g_assert_cmpint (pos[5].y_offset, ==, 4730);
  g_assert_cmpint (pos[5].x_advance, ==, 0);
  g_assert_cmpint (pos[5].y_advance, ==, 0);

  g_assert_cmpint (info[6].codepoint, ==, top);
  g_assert_cmpint (info[6].cluster, ==, cluster);
  g_assert_cmpint (pos[6].x_offset, ==, 0);
  g_assert_cmpint (pos[6].y_offset, ==, 5676);
  g_assert_cmpint (pos[6].x_advance, ==, fontSize);
  g_assert_cmpint (pos[6].y_advance, ==, -6676);

  destroyBuffer();

  closeFont();
  cleanupFreeType();
}

static void
test_shape_math_stretchy_edge (void)
{
  hb_codepoint_t glyph;

  initFreeType();

  // Bad input buffer: fail.
  hb_buffer = hb_buffer_create ();
  hb_buffer_set_content_type (hb_buffer, HB_BUFFER_CONTENT_TYPE_INVALID);
  g_assert (!hb_ot_shape_math_stretchy (hb_font, hb_buffer, FALSE, 0));
  destroyBuffer();
  hb_buffer = hb_buffer_create ();
  hb_buffer_add_utf8 (hb_buffer, "text", -1, 0, -1);
  g_assert (!hb_ot_shape_math_stretchy (hb_font, hb_buffer, FALSE, 0));
  destroyBuffer();

  // Unicode input: fail */
  openFont("fonts/MathTestFontFull.otf");
  createBuffer ('{', FALSE);
  g_assert (!hb_ot_shape_math_stretchy (hb_font, hb_buffer, FALSE, fontSize));
  closeFont();

  // Missing base glyph: fail.
  openFont("fonts/MathTestFontNone.otf");
  createBuffer ('{', FALSE);
  g_assert (!hb_ot_shape_math_stretchy (hb_font, hb_buffer, FALSE, fontSize));
  destroyBuffer();
  createBuffer ('{', TRUE);
  g_assert (!hb_ot_shape_math_stretchy (hb_font, hb_buffer, FALSE, fontSize));
  destroyBuffer();
  closeFont();

  // Missing math data: fail.
  openFont("fonts/MathTestFontNone.otf");
  createBuffer (' ', TRUE);
  g_assert (!hb_ot_shape_math_stretchy (hb_font, hb_buffer, FALSE, fontSize));
  closeFont();

  // No MathGlyphConstruction: use the base glyph.
  openFont("fonts/MathTestFontPartial2.otf");
  createBuffer (' ', TRUE);
  g_assert (hb_ot_shape_math_stretchy (hb_font, hb_buffer, FALSE, fontSize));
  g_assert_cmpint (hb_buffer_get_content_type (hb_buffer), ==, HB_BUFFER_CONTENT_TYPE_GLYPHS);
  getBufferData();
  g_assert_cmpint (len, ==, 1);
  g_assert (hb_font_get_glyph_from_name (hb_font, "space", -1, &glyph));
  g_assert_cmpint (info[0].codepoint, ==, glyph);
  destroyBuffer();
  closeFont();

  // variantCount = PartCount = 0: use the base glyph.
  openFont("fonts/MathTestFontPartial4.otf");
  createBuffer (' ', TRUE);
  g_assert (hb_ot_shape_math_stretchy (hb_font, hb_buffer, FALSE, fontSize));
  g_assert_cmpint (hb_buffer_get_content_type (hb_buffer), ==, HB_BUFFER_CONTENT_TYPE_GLYPHS);
  getBufferData();
  g_assert_cmpint (len, ==, 1);
  g_assert (hb_font_get_glyph_from_name (hb_font, "space", -1, &glyph));
  g_assert_cmpint (info[0].codepoint, ==, glyph);
  destroyBuffer();
  closeFont();

  // too many glyphs needed to build the assembly: use the largest variant.
  openFont("fonts/MathTestFontFull.otf");
  createBuffer (arrowleftCodePoint, TRUE);
  g_assert (hb_ot_shape_math_stretchy (hb_font, hb_buffer, TRUE, HB_OT_MATH_MAXIMUM_PART_COUNT_IN_GLYPH_ASSEMBLY * fontSize));
  g_assert_cmpint (hb_buffer_get_content_type (hb_buffer), ==, HB_BUFFER_CONTENT_TYPE_GLYPHS);
  getBufferData();
  g_assert_cmpint (len, ==, 1);
  g_assert (hb_font_get_glyph_from_name (hb_font, "uni2190_size4", -1, &glyph));
  g_assert_cmpint (info[0].codepoint, ==, glyph);
  g_assert_cmpint (info[0].cluster, ==, cluster);
  g_assert_cmpint (pos[0].x_offset, ==, 0);
  g_assert_cmpint (pos[0].y_offset, ==, 0);
  g_assert_cmpint (pos[0].x_advance, ==,
                   hb_font_get_glyph_h_advance(hb_font, info[0].codepoint));
  g_assert_cmpint (pos[0].y_advance, ==,
                   hb_font_get_glyph_v_advance(hb_font, info[0].codepoint));
  destroyBuffer();
  closeFont();

  cleanupFreeType();
}

static void
test_shape_math_stretchy_max_orthogonal_advance (void)
{
  initFreeType();

  // No math data: return 0.
  openFont("fonts/MathTestFontNone.otf");
  createBuffer (' ', TRUE);
  g_assert_cmpint (hb_ot_shape_math_stretchy_max_orthogonal_advance (hb_font, hb_buffer, FALSE), ==, 0);
  destroyBuffer();
  closeFont();

  openFont("fonts/MathTestFontFull.otf");

  // Bad input buffer: return 0
  hb_buffer = hb_buffer_create ();
  hb_buffer_set_content_type (hb_buffer, HB_BUFFER_CONTENT_TYPE_INVALID);
  g_assert_cmpint (hb_ot_shape_math_stretchy_max_orthogonal_advance (hb_font, hb_buffer, FALSE), ==, 0);
  destroyBuffer();
  hb_buffer = hb_buffer_create ();
  hb_buffer_add_utf8 (hb_buffer, "text", -1, 0, -1);
  g_assert_cmpint (hb_ot_shape_math_stretchy_max_orthogonal_advance (hb_font, hb_buffer, FALSE), ==, 0);
  destroyBuffer();

  // Unicode
  createBuffer ('{', FALSE);
  g_assert_cmpint (hb_ot_shape_math_stretchy_max_orthogonal_advance (hb_font, hb_buffer, FALSE), ==, 0);
  destroyBuffer();

  // MATH Table
  createBuffer (arrowdownCodePoint, TRUE);
  g_assert_cmpint (hb_ot_shape_math_stretchy_max_orthogonal_advance (hb_font, hb_buffer, FALSE), ==, -fontSize);
  destroyBuffer();
  createBuffer (arrowleftCodePoint, TRUE);
  g_assert_cmpint (hb_ot_shape_math_stretchy_max_orthogonal_advance (hb_font, hb_buffer, TRUE), ==, fontSize);
  destroyBuffer();

  closeFont();
  cleanupFreeType();
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_shape_math_stretchy_none);
  hb_test_add (test_shape_math_stretchy_horizontal_glyph_variants);
  hb_test_add (test_shape_math_stretchy_vertical_glyph_variants);
  hb_test_add (test_shape_math_stretchy_horizontal_glyph_assembly);
  hb_test_add (test_shape_math_stretchy_vertical_glyph_assembly);
  hb_test_add (test_shape_math_stretchy_edge);
  hb_test_add (test_shape_math_stretchy_max_orthogonal_advance);

  return hb_test_run();
}
