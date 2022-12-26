/*
 * Copyright © 2016  Google, Inc.
 * Copyright © 2018  Ebrahim Byagowi
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
 * Google Author(s): Sascha Brawer
 */

#include "hb-test.h"

#include <hb-ot.h>

/* Unit tests for hb-ot-color.h */

/* Test font with the following CPAL v0 table, as TTX and manual disassembly:

  <CPAL>
    <version value="0"/>
    <numPaletteEntries value="2"/>
    <palette index="0">
      <color index="0" value="#000000FF"/>
      <color index="1" value="#66CCFFFF"/>
    </palette>
    <palette index="1">
      <color index="0" value="#000000FF"/>
      <color index="1" value="#800000FF"/>
    </palette>
  </CPAL>

   0 | 0000                           # version=0
   2 | 0002                           # numPaletteEntries=2
   4 | 0002                           # numPalettes=2
   6 | 0004                           # numColorRecords=4
   8 | 00000010                       # offsetToFirstColorRecord=16
  12 | 0000 0002                      # colorRecordIndex=[0, 2]
  16 | 000000ff ffcc66ff              # colorRecord #0, #1 (BGRA)
  24 | 000000ff 000080ff              # colorRecord #2, #3 (BGRA)
 */
static hb_face_t *cpal_v0 = NULL;

/* Test font with the following CPAL v1 table, as TTX and manual disassembly:

  <CPAL>
    <version value="1"/>
    <numPaletteEntries value="2"/>
    <palette index="0" label="257" type="2">
      <color index="0" value="#000000FF"/>
      <color index="1" value="#66CCFFFF"/>
    </palette>
    <palette index="1" label="65535" type="1">
      <color index="0" value="#000000FF"/>
      <color index="1" value="#FFCC66FF"/>
    </palette>
    <palette index="2" label="258" type="0">
      <color index="0" value="#000000FF"/>
      <color index="1" value="#800000FF"/>
    </palette>
    <paletteEntryLabels>
      <label index="0" value="65535"/>
      <label index="1" value="256"/>
    </paletteEntryLabels>
  </CPAL>

   0 | 0001                           # version=1
   2 | 0002                           # numPaletteEntries=2
   4 | 0003                           # numPalettes=3
   6 | 0006                           # numColorRecords=6
   8 | 0000001e                       # offsetToFirstColorRecord=30
  12 | 0000 0002 0004                 # colorRecordIndex=[0, 2, 4]
  18 | 00000036                       # offsetToPaletteTypeArray=54
  22 | 00000042                       # offsetToPaletteLabelArray=66
  26 | 00000048                       # offsetToPaletteEntryLabelArray=72
  30 | 000000ff ffcc66ff 000000ff     # colorRecord #0, #1, #2 (BGRA)
  42 | 66ccffff 000000ff 000080ff     # colorRecord #3, #4, #5 (BGRA)
  54 | 00000002 00000001 00000000     # paletteFlags=[2, 1, 0]
  66 | 0101 ffff 0102                 # paletteName=[257, 0xffff, 258]
  72 | ffff 0100                      # paletteEntryLabel=[0xffff, 256]
*/
static hb_face_t *cpal_v1 = NULL;

static hb_face_t *cpal = NULL;
static hb_face_t *cbdt = NULL;
static hb_face_t *sbix = NULL;
static hb_face_t *svg = NULL;
static hb_face_t *empty = NULL;
static hb_face_t *colrv1 = NULL;

#define assert_color_rgba(colors, i, r, g, b, a) G_STMT_START {	\
  const hb_color_t *_colors = (colors); \
  const size_t _i = (i); \
  const uint8_t red = (r), green = (g), blue = (b), alpha = (a); \
  if (hb_color_get_red (_colors[_i]) != red) { \
    g_assertion_message_cmpnum (G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, \
				"colors[" #i "]", _colors[_i], "==", red, 'x'); \
  } \
  if (hb_color_get_green (_colors[_i]) != green) { \
    g_assertion_message_cmpnum (G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, \
				"colors[" #i "]", _colors[_i], "==", green, 'x'); \
  } \
  if (hb_color_get_blue (_colors[_i]) != blue) { \
    g_assertion_message_cmpnum (G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, \
				"colors[" #i "]", colors[_i], "==", blue, 'x'); \
  } \
  if (hb_color_get_alpha (_colors[_i]) != alpha) { \
    g_assertion_message_cmpnum (G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, \
				"colors[" #i "]", _colors[_i], "==", alpha, 'x'); \
  } \
} G_STMT_END


static void
test_hb_ot_color_palette_get_count (void)
{
  g_assert_cmpint (hb_ot_color_palette_get_count (hb_face_get_empty()), ==, 0);
  g_assert_cmpint (hb_ot_color_palette_get_count (cpal_v0), ==, 2);
  g_assert_cmpint (hb_ot_color_palette_get_count (cpal_v1), ==, 3);
}


static void
test_hb_ot_color_palette_get_name_id_empty (void)
{
  /* numPalettes=0, so all calls are for out-of-bounds palette indices */
  g_assert_cmpint (hb_ot_color_palette_get_name_id (hb_face_get_empty(), 0), ==, HB_OT_NAME_ID_INVALID);
  g_assert_cmpint (hb_ot_color_palette_get_name_id (hb_face_get_empty(), 1), ==, HB_OT_NAME_ID_INVALID);
}


static void
test_hb_ot_color_palette_get_name_id_v0 (void)
{
  g_assert_cmpint (hb_ot_color_palette_get_name_id (cpal_v0, 0), ==, HB_OT_NAME_ID_INVALID);
  g_assert_cmpint (hb_ot_color_palette_get_name_id (cpal_v0, 1), ==, HB_OT_NAME_ID_INVALID);

  /* numPalettes=2, so palette #2 is out of bounds */
  g_assert_cmpint (hb_ot_color_palette_get_name_id (cpal_v0, 2), ==, HB_OT_NAME_ID_INVALID);
}


static void
test_hb_ot_color_palette_get_name_id_v1 (void)
{
  g_assert_cmpint (hb_ot_color_palette_get_name_id (cpal_v1, 0), ==, 257);
  g_assert_cmpint (hb_ot_color_palette_get_name_id (cpal_v1, 1), ==, HB_OT_NAME_ID_INVALID);
  g_assert_cmpint (hb_ot_color_palette_get_name_id (cpal_v1, 2), ==, 258);

  /* numPalettes=3, so palette #3 is out of bounds */
  g_assert_cmpint (hb_ot_color_palette_get_name_id (cpal_v1, 3), ==, HB_OT_NAME_ID_INVALID);
}


static void
test_hb_ot_color_palette_get_flags_empty (void)
{
  /* numPalettes=0, so all calls are for out-of-bounds palette indices */
  g_assert_cmpint (hb_ot_color_palette_get_flags (hb_face_get_empty(), 0), ==, HB_OT_COLOR_PALETTE_FLAG_DEFAULT);
  g_assert_cmpint (hb_ot_color_palette_get_flags (hb_face_get_empty(), 1), ==, HB_OT_COLOR_PALETTE_FLAG_DEFAULT);
}


static void
test_hb_ot_color_palette_get_flags_v0 (void)
{
  g_assert_cmpint (hb_ot_color_palette_get_flags (cpal_v0, 0), ==, HB_OT_COLOR_PALETTE_FLAG_DEFAULT);
  g_assert_cmpint (hb_ot_color_palette_get_flags (cpal_v0, 1), ==, HB_OT_COLOR_PALETTE_FLAG_DEFAULT);

  /* numPalettes=2, so palette #2 is out of bounds */
  g_assert_cmpint (hb_ot_color_palette_get_flags (cpal_v0, 2), ==, HB_OT_COLOR_PALETTE_FLAG_DEFAULT);
}


static void
test_hb_ot_color_palette_get_flags_v1 (void)
{
  g_assert_cmpint (hb_ot_color_palette_get_flags (cpal_v1, 0), ==, HB_OT_COLOR_PALETTE_FLAG_USABLE_WITH_DARK_BACKGROUND);
  g_assert_cmpint (hb_ot_color_palette_get_flags (cpal_v1, 1), ==, HB_OT_COLOR_PALETTE_FLAG_USABLE_WITH_LIGHT_BACKGROUND);
  g_assert_cmpint (hb_ot_color_palette_get_flags (cpal_v0, 2), ==, HB_OT_COLOR_PALETTE_FLAG_DEFAULT);

  /* numPalettes=3, so palette #3 is out of bounds */
  g_assert_cmpint (hb_ot_color_palette_get_flags (cpal_v0, 3), ==, HB_OT_COLOR_PALETTE_FLAG_DEFAULT);
}


static void
test_hb_ot_color_palette_get_colors_empty (void)
{
  g_assert_cmpint (hb_ot_color_palette_get_colors (empty, 0, 0, NULL, NULL), ==, 0);
}


static void
test_hb_ot_color_palette_get_colors_v0 (void)
{
  unsigned int num_colors = hb_ot_color_palette_get_colors (cpal_v0, 0, 0, NULL, NULL);
  hb_color_t *colors = (hb_color_t*) malloc (num_colors * sizeof (hb_color_t));
  size_t colors_size = num_colors * sizeof(*colors);
  g_assert_cmpint (num_colors, ==, 2);

  /* Palette #0, start_index=0 */
  g_assert_cmpint (hb_ot_color_palette_get_colors (cpal_v0, 0, 0, &num_colors, colors), ==, 2);
  g_assert_cmpint (num_colors, ==, 2);
  assert_color_rgba (colors, 0, 0x00, 0x00, 0x00, 0xff);
  assert_color_rgba (colors, 1, 0x66, 0xcc, 0xff, 0xff);

  /* Palette #1, start_index=0 */
  g_assert_cmpint (hb_ot_color_palette_get_colors (cpal_v0, 1, 0, &num_colors, colors), ==, 2);
  g_assert_cmpint (num_colors, ==, 2);
  assert_color_rgba (colors, 0, 0x00, 0x00, 0x00, 0xff);
  assert_color_rgba (colors, 1, 0x80, 0x00, 0x00, 0xff);

  /* Palette #2 (there are only #0 and #1 in the font, so this is out of bounds) */
  g_assert_cmpint (hb_ot_color_palette_get_colors (cpal_v0, 2, 0, &num_colors, colors), ==, 0);

  /* Palette #0, start_index=1 */
  memset(colors, 0x33, colors_size);
  num_colors = 2;
  g_assert_cmpint (hb_ot_color_palette_get_colors (cpal_v0, 0, 1, &num_colors, colors), ==, 2);
  g_assert_cmpint (num_colors, ==, 1);
  assert_color_rgba (colors, 0, 0x66, 0xcc, 0xff, 0xff);
  assert_color_rgba (colors, 1, 0x33, 0x33, 0x33, 0x33);  /* untouched */

  /* Palette #0, start_index=0, pretend that we have only allocated space for 1 color */
  memset(colors, 0x44, colors_size);
  num_colors = 1;
  g_assert_cmpint (hb_ot_color_palette_get_colors (cpal_v0, 0, 0, &num_colors, colors), ==, 2);
  g_assert_cmpint (num_colors, ==, 1);
  assert_color_rgba (colors, 0, 0x00, 0x00, 0x00, 0xff);
  assert_color_rgba (colors, 1, 0x44, 0x44, 0x44, 0x44);  /* untouched */

  /* start_index > numPaletteEntries */
  memset (colors, 0x44, colors_size);
  num_colors = 2;
  g_assert_cmpint (hb_ot_color_palette_get_colors (cpal_v0, 0, 9876, &num_colors, colors), ==, 2);
  g_assert_cmpint (num_colors, ==, 0);
  assert_color_rgba (colors, 0, 0x44, 0x44, 0x44, 0x44);  /* untouched */
  assert_color_rgba (colors, 1, 0x44, 0x44, 0x44, 0x44);  /* untouched */
	
  free (colors);
}


static void
test_hb_ot_color_palette_get_colors_v1 (void)
{
  hb_color_t colors[3];
  unsigned int num_colors = hb_ot_color_palette_get_colors (cpal_v1, 0, 0, NULL, NULL);
  size_t colors_size = 3 * sizeof (hb_color_t);
  g_assert_cmpint (num_colors, ==, 2);

  /* Palette #0, start_index=0 */
  memset (colors, 0x77, colors_size);
  g_assert_cmpint (hb_ot_color_palette_get_colors (cpal_v1, 0, 0, &num_colors, colors), ==, 2);
  g_assert_cmpint (num_colors, ==, 2);
  assert_color_rgba (colors, 0, 0x00, 0x00, 0x00, 0xff);
  assert_color_rgba (colors, 1, 0x66, 0xcc, 0xff, 0xff);
  assert_color_rgba (colors, 2, 0x77, 0x77, 0x77, 0x77);  /* untouched */

  /* Palette #1, start_index=0 */
  memset (colors, 0x77, colors_size);
  g_assert_cmpint (hb_ot_color_palette_get_colors (cpal_v1, 1, 0, &num_colors, colors), ==, 2);
  g_assert_cmpint (num_colors, ==, 2);
  assert_color_rgba (colors, 0, 0x00, 0x00, 0x00, 0xff);
  assert_color_rgba (colors, 1, 0xff, 0xcc, 0x66, 0xff);
  assert_color_rgba (colors, 2, 0x77, 0x77, 0x77, 0x77);  /* untouched */

  /* Palette #2, start_index=0 */
  memset (colors, 0x77, colors_size);
  g_assert_cmpint (hb_ot_color_palette_get_colors (cpal_v1, 2, 0, &num_colors, colors), ==, 2);
  g_assert_cmpint (num_colors, ==, 2);
  assert_color_rgba (colors, 0, 0x00, 0x00, 0x00, 0xff);
  assert_color_rgba (colors, 1, 0x80, 0x00, 0x00, 0xff);
  assert_color_rgba (colors, 2, 0x77, 0x77, 0x77, 0x77);  /* untouched */

  /* Palette #3 (out of bounds), start_index=0 */
  memset (colors, 0x77, colors_size);
  g_assert_cmpint (hb_ot_color_palette_get_colors (cpal_v1, 3, 0, &num_colors, colors), ==, 0);
  g_assert_cmpint (num_colors, ==, 0);
  assert_color_rgba (colors, 0, 0x77, 0x77, 0x77, 0x77);  /* untouched */
  assert_color_rgba (colors, 1, 0x77, 0x77, 0x77, 0x77);  /* untouched */
  assert_color_rgba (colors, 2, 0x77, 0x77, 0x77, 0x77);  /* untouched */
}


static void
test_hb_ot_color_palette_color_get_name_id (void)
{
  g_assert_cmpuint (hb_ot_color_palette_color_get_name_id (empty, 0), ==, HB_OT_NAME_ID_INVALID);
  g_assert_cmpuint (hb_ot_color_palette_color_get_name_id (empty, 1), ==, HB_OT_NAME_ID_INVALID);
  g_assert_cmpuint (hb_ot_color_palette_color_get_name_id (empty, 2), ==, HB_OT_NAME_ID_INVALID);
  g_assert_cmpuint (hb_ot_color_palette_color_get_name_id (cpal_v0, 0), ==, HB_OT_NAME_ID_INVALID);
  g_assert_cmpuint (hb_ot_color_palette_color_get_name_id (cpal_v0, 1), ==, HB_OT_NAME_ID_INVALID);
  g_assert_cmpuint (hb_ot_color_palette_color_get_name_id (cpal_v0, 2), ==, HB_OT_NAME_ID_INVALID);
  g_assert_cmpuint (hb_ot_color_palette_color_get_name_id (cpal_v1, 0), ==, HB_OT_NAME_ID_INVALID);
  g_assert_cmpuint (hb_ot_color_palette_color_get_name_id (cpal_v1, 1), ==, 256);
  g_assert_cmpuint (hb_ot_color_palette_color_get_name_id (cpal_v1, 2), ==, HB_OT_NAME_ID_INVALID);
}


static void
test_hb_ot_color_glyph_get_layers (void)
{
  hb_ot_color_layer_t layers[1];
  unsigned int count = 1;
  unsigned int num_layers;

  g_assert_cmpuint (hb_ot_color_glyph_get_layers (cpal_v1, 0, 0,
						  NULL, NULL), ==, 0);
  g_assert_cmpuint (hb_ot_color_glyph_get_layers (cpal_v1, 1, 0,
						  NULL, NULL), ==, 0);
  g_assert_cmpuint (hb_ot_color_glyph_get_layers (cpal_v1, 2, 0,
						  NULL, NULL), ==, 2);

  num_layers = hb_ot_color_glyph_get_layers (cpal_v1, 2, 0, &count, layers);

  g_assert_cmpuint (num_layers, ==, 2);
  g_assert_cmpuint (count, ==, 1);
  g_assert_cmpuint (layers[0].glyph, ==, 3);
  g_assert_cmpuint (layers[0].color_index, ==, 1);

  count = 1;
  hb_ot_color_glyph_get_layers (cpal_v1, 2, 1, &count, layers);

  g_assert_cmpuint (num_layers, ==, 2);
  g_assert_cmpuint (count, ==, 1);
  g_assert_cmpuint (layers[0].glyph, ==, 4);
  g_assert_cmpuint (layers[0].color_index, ==, 0);
}

static void
test_hb_ot_color_has_data (void)
{
  g_assert (hb_ot_color_has_layers (empty) == FALSE);
  g_assert (hb_ot_color_has_layers (cpal_v0) == TRUE);
  g_assert (hb_ot_color_has_layers (cpal_v1) == TRUE);
  g_assert (hb_ot_color_has_layers (cpal) == TRUE);
  g_assert (hb_ot_color_has_layers (cbdt) == FALSE);
  g_assert (hb_ot_color_has_layers (sbix) == FALSE);
  g_assert (hb_ot_color_has_layers (svg) == FALSE);
  g_assert (hb_ot_color_has_layers (colrv1) == FALSE);

  g_assert (hb_ot_color_has_palettes (empty) == FALSE);
  g_assert (hb_ot_color_has_palettes (cpal_v0) == TRUE);
  g_assert (hb_ot_color_has_palettes (cpal_v1) == TRUE);
  g_assert (hb_ot_color_has_palettes (cpal) == TRUE);
  g_assert (hb_ot_color_has_palettes (cbdt) == FALSE);
  g_assert (hb_ot_color_has_palettes (sbix) == FALSE);
  g_assert (hb_ot_color_has_palettes (svg) == FALSE);
  g_assert (hb_ot_color_has_palettes (colrv1) == TRUE);

  g_assert (hb_ot_color_has_svg (empty) == FALSE);
  g_assert (hb_ot_color_has_svg (cpal_v0) == FALSE);
  g_assert (hb_ot_color_has_svg (cpal_v1) == FALSE);
  g_assert (hb_ot_color_has_svg (cpal) == FALSE);
  g_assert (hb_ot_color_has_svg (cbdt) == FALSE);
  g_assert (hb_ot_color_has_svg (sbix) == FALSE);
  g_assert (hb_ot_color_has_svg (svg) == TRUE);
  g_assert (hb_ot_color_has_svg (colrv1) == FALSE);

  g_assert (hb_ot_color_has_png (empty) == FALSE);
  g_assert (hb_ot_color_has_png (cpal_v0) == FALSE);
  g_assert (hb_ot_color_has_png (cpal_v1) == FALSE);
  g_assert (hb_ot_color_has_png (cpal) == FALSE);
  g_assert (hb_ot_color_has_png (cbdt) == TRUE);
  g_assert (hb_ot_color_has_png (sbix) == TRUE);
  g_assert (hb_ot_color_has_png (svg) == FALSE);
  g_assert (hb_ot_color_has_png (colrv1) == FALSE);

  g_assert (hb_ot_color_has_paint (empty) == FALSE);
  g_assert (hb_ot_color_has_paint (cpal_v0) == FALSE);
  g_assert (hb_ot_color_has_paint (cpal_v1) == FALSE);
  g_assert (hb_ot_color_has_paint (cpal) == FALSE);
  g_assert (hb_ot_color_has_paint (cbdt) == FALSE);
  g_assert (hb_ot_color_has_paint (sbix) == FALSE);
  g_assert (hb_ot_color_has_paint (svg) == FALSE);
  g_assert (hb_ot_color_has_paint (colrv1) == TRUE);
}

static void
test_hb_ot_color_glyph_has_paint (void)
{
  g_assert (hb_ot_color_has_paint (colrv1));
  g_assert (hb_ot_color_glyph_has_paint (colrv1, 10));
  g_assert (!hb_ot_color_glyph_has_paint (colrv1, 20));
}

static void
test_hb_ot_color_svg (void)
{
  hb_blob_t *blob;
  unsigned int length;
  const char *data;

  blob = hb_ot_color_glyph_reference_svg (svg, 0);
  g_assert (hb_blob_get_length (blob) == 0);

  blob = hb_ot_color_glyph_reference_svg (svg, 1);
  data = hb_blob_get_data (blob, &length);
  g_assert_cmpuint (length, ==, 146);
  g_assert (strncmp (data, "<?xml", 4) == 0);
  g_assert (strncmp (data + 140, "</svg>", 5) == 0);
  hb_blob_destroy (blob);

  blob = hb_ot_color_glyph_reference_svg (empty, 0);
  g_assert (hb_blob_get_length (blob) == 0);
}


static void
test_hb_ot_color_png (void)
{
  hb_blob_t *blob;
  unsigned int length;
  const char *data;
  hb_glyph_extents_t extents;
  hb_font_t *cbdt_font;

  /* sbix */
  hb_font_t *sbix_font;
  sbix_font = hb_font_create (sbix);
  blob = hb_ot_color_glyph_reference_png (sbix_font, 0);
  hb_font_get_glyph_extents (sbix_font, 0, &extents);
  g_assert_cmpint (extents.x_bearing, ==, 0);
  g_assert_cmpint (extents.y_bearing, ==, 0);
  g_assert_cmpint (extents.width, ==, 0);
  g_assert_cmpint (extents.height, ==, 0);
  g_assert (hb_blob_get_length (blob) == 0);

  blob = hb_ot_color_glyph_reference_png (sbix_font, 1);
  data = hb_blob_get_data (blob, &length);
  g_assert_cmpuint (length, ==, 224);
  g_assert (strncmp (data + 1, "PNG", 3) == 0);
  hb_font_get_glyph_extents (sbix_font, 1, &extents);
  g_assert_cmpint (extents.x_bearing, ==, 0);
  g_assert_cmpint (extents.y_bearing, ==, 800);
  g_assert_cmpint (extents.width, ==, 800);
  g_assert_cmpint (extents.height, ==, -800);
  hb_blob_destroy (blob);
  hb_font_destroy (sbix_font);

  /* cbdt */
  cbdt_font = hb_font_create (cbdt);
  blob = hb_ot_color_glyph_reference_png (cbdt_font, 0);
  g_assert (hb_blob_get_length (blob) == 0);

  blob = hb_ot_color_glyph_reference_png (cbdt_font, 1);
  data = hb_blob_get_data (blob, &length);
  g_assert_cmpuint (length, ==, 88);
  g_assert (strncmp (data + 1, "PNG", 3) == 0);
  hb_font_get_glyph_extents (cbdt_font, 1, &extents);
  g_assert_cmpint (extents.x_bearing, ==, 0);
  g_assert_cmpint (extents.y_bearing, ==, 1024);
  g_assert_cmpint (extents.width, ==, 1024);
  g_assert_cmpint (extents.height, ==, -1024);
  hb_blob_destroy (blob);
  hb_font_destroy (cbdt_font);
}

int
main (int argc, char **argv)
{
  int status = 0;

  hb_test_init (&argc, &argv);
  cpal_v0 = hb_test_open_font_file ("fonts/cpal-v0.ttf");
  cpal_v1 = hb_test_open_font_file ("fonts/cpal-v1.ttf");
  cpal = hb_test_open_font_file ("fonts/chromacheck-colr.ttf");
  cbdt = hb_test_open_font_file ("fonts/chromacheck-cbdt.ttf");
  sbix = hb_test_open_font_file ("fonts/chromacheck-sbix.ttf");
  svg = hb_test_open_font_file ("fonts/chromacheck-svg.ttf");
  colrv1 = hb_test_open_font_file ("fonts/noto_handwriting-cff2_colr_1.otf");
  empty = hb_face_get_empty ();
  hb_test_add (test_hb_ot_color_palette_get_count);
  hb_test_add (test_hb_ot_color_palette_get_name_id_empty);
  hb_test_add (test_hb_ot_color_palette_get_name_id_v0);
  hb_test_add (test_hb_ot_color_palette_get_name_id_v1);
  hb_test_add (test_hb_ot_color_palette_get_flags_empty);
  hb_test_add (test_hb_ot_color_palette_get_flags_v0);
  hb_test_add (test_hb_ot_color_palette_get_flags_v1);
  hb_test_add (test_hb_ot_color_palette_get_colors_empty);
  hb_test_add (test_hb_ot_color_palette_get_colors_v0);
  hb_test_add (test_hb_ot_color_palette_get_colors_v1);
  hb_test_add (test_hb_ot_color_palette_color_get_name_id);
  hb_test_add (test_hb_ot_color_glyph_get_layers);
  hb_test_add (test_hb_ot_color_has_data);
  hb_test_add (test_hb_ot_color_png);
  hb_test_add (test_hb_ot_color_svg);
  hb_test_add (test_hb_ot_color_glyph_has_paint);

  status = hb_test_run();
  hb_face_destroy (cpal_v0);
  hb_face_destroy (cpal_v1);
  hb_face_destroy (cpal);
  hb_face_destroy (cbdt);
  hb_face_destroy (sbix);
  hb_face_destroy (svg);
  hb_face_destroy (colrv1);
  return status;
}
