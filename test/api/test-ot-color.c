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
  g_assert_true (hb_ot_color_has_layers (empty) == FALSE);
  g_assert_true (hb_ot_color_has_layers (cpal_v0) == TRUE);
  g_assert_true (hb_ot_color_has_layers (cpal_v1) == TRUE);
  g_assert_true (hb_ot_color_has_layers (cpal) == TRUE);
  g_assert_true (hb_ot_color_has_layers (cbdt) == FALSE);
  g_assert_true (hb_ot_color_has_layers (sbix) == FALSE);
  g_assert_true (hb_ot_color_has_layers (svg) == FALSE);
  g_assert_true (hb_ot_color_has_layers (colrv1) == FALSE);

  g_assert_true (hb_ot_color_has_palettes (empty) == FALSE);
  g_assert_true (hb_ot_color_has_palettes (cpal_v0) == TRUE);
  g_assert_true (hb_ot_color_has_palettes (cpal_v1) == TRUE);
  g_assert_true (hb_ot_color_has_palettes (cpal) == TRUE);
  g_assert_true (hb_ot_color_has_palettes (cbdt) == FALSE);
  g_assert_true (hb_ot_color_has_palettes (sbix) == FALSE);
  g_assert_true (hb_ot_color_has_palettes (svg) == FALSE);
  g_assert_true (hb_ot_color_has_palettes (colrv1) == TRUE);

#ifndef HB_NO_SVG
  g_assert_true (hb_ot_color_has_svg (empty) == FALSE);
  g_assert_true (hb_ot_color_has_svg (cpal_v0) == FALSE);
  g_assert_true (hb_ot_color_has_svg (cpal_v1) == FALSE);
  g_assert_true (hb_ot_color_has_svg (cpal) == FALSE);
  g_assert_true (hb_ot_color_has_svg (cbdt) == FALSE);
  g_assert_true (hb_ot_color_has_svg (sbix) == FALSE);
  g_assert_true (hb_ot_color_has_svg (svg) == TRUE);
  g_assert_true (hb_ot_color_has_svg (colrv1) == FALSE);
#endif

  g_assert_true (hb_ot_color_has_png (empty) == FALSE);
  g_assert_true (hb_ot_color_has_png (cpal_v0) == FALSE);
  g_assert_true (hb_ot_color_has_png (cpal_v1) == FALSE);
  g_assert_true (hb_ot_color_has_png (cpal) == FALSE);
  g_assert_true (hb_ot_color_has_png (cbdt) == TRUE);
  g_assert_true (hb_ot_color_has_png (sbix) == TRUE);
  g_assert_true (hb_ot_color_has_png (svg) == FALSE);
  g_assert_true (hb_ot_color_has_png (colrv1) == FALSE);

  g_assert_true (hb_ot_color_has_paint (empty) == FALSE);
  g_assert_true (hb_ot_color_has_paint (cpal_v0) == FALSE);
  g_assert_true (hb_ot_color_has_paint (cpal_v1) == FALSE);
  g_assert_true (hb_ot_color_has_paint (cpal) == FALSE);
  g_assert_true (hb_ot_color_has_paint (cbdt) == FALSE);
  g_assert_true (hb_ot_color_has_paint (sbix) == FALSE);
  g_assert_true (hb_ot_color_has_paint (svg) == FALSE);
  g_assert_true (hb_ot_color_has_paint (colrv1) == TRUE);
}

static void
test_hb_ot_color_glyph_has_paint (void)
{
  g_assert_true (hb_ot_color_has_paint (colrv1));
  g_assert_true (hb_ot_color_glyph_has_paint (colrv1, 10));
  g_assert_true (!hb_ot_color_glyph_has_paint (colrv1, 20));
}

static void
test_hb_ot_color_svg (void)
{
#ifndef HB_NO_SVG
  hb_blob_t *blob;
  unsigned int length;
  const char *data;

  blob = hb_ot_color_glyph_reference_svg (svg, 0);
  g_assert_true (hb_blob_get_length (blob) == 0);

  blob = hb_ot_color_glyph_reference_svg (svg, 1);
  data = hb_blob_get_data (blob, &length);
  g_assert_cmpuint (length, ==, 146);
  g_assert_true (strncmp (data, "<?xml", 4) == 0);
  g_assert_true (strncmp (data + 140, "</svg>", 5) == 0);
  hb_blob_destroy (blob);

  blob = hb_ot_color_glyph_reference_svg (empty, 0);
  g_assert_true (hb_blob_get_length (blob) == 0);
#endif
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
  g_assert_true (hb_blob_get_length (blob) == 0);

  blob = hb_ot_color_glyph_reference_png (sbix_font, 1);
  data = hb_blob_get_data (blob, &length);
  g_assert_cmpuint (length, ==, 224);
  g_assert_true (strncmp (data + 1, "PNG", 3) == 0);
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
  g_assert_true (hb_blob_get_length (blob) == 0);

  blob = hb_ot_color_glyph_reference_png (cbdt_font, 1);
  data = hb_blob_get_data (blob, &length);
  g_assert_cmpuint (length, ==, 88);
  g_assert_true (strncmp (data + 1, "PNG", 3) == 0);
  hb_font_get_glyph_extents (cbdt_font, 1, &extents);
  g_assert_cmpint (extents.x_bearing, ==, 0);
  g_assert_cmpint (extents.y_bearing, ==, 1024);
  g_assert_cmpint (extents.width, ==, 1024);
  g_assert_cmpint (extents.height, ==, -1024);
  hb_blob_destroy (blob);
  hb_font_destroy (cbdt_font);
}

static void
test_hb_ot_color_png_cbdt_records (void)
{
  /* Index subtable records that are out of order and overlap.  A glyph
   * belongs to the first one that covers it.  Images of the Nth record
   * are N bytes long.  This pins the accelerator's record index against
   * the first-covering-record rule of the old linear walk. */
  static const unsigned int lengths[] = {3, 2, 2, 3, 1, 1, 3, 3, 0};
  static const uint8_t cblc[] = {
    0, 3, 0, 0, /* version */
    0, 0, 0, 1, /* numSizes */
    /* BitmapSize */
    0, 0, 0, 8 + 48, /* indexSubtableArrayOffset */
    0, 0, 0, 78, /* indexTablesSize */
    0, 0, 0, 3, /* numberOfIndexSubtables */
    0, 0, 0, 0, /* colorRef */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* hori */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* vert */
    0, 0, 0, 7, /* startGlyphIndex, endGlyphIndex */
    32, 32, 32, 1, /* ppemX, ppemY, bitDepth, flags */
    /* IndexSubtableRecords: firstGlyphIndex, lastGlyphIndex, offsetToSubtable */
    0, 4, 0, 5, 0, 0, 0, 24,
    0, 1, 0, 2, 0, 0, 0, 38,
    0, 0, 0, 7, 0, 0, 0, 52,
    /* IndexSubtableFormat3s: indexFormat, imageFormat, imageDataOffset, offsets */
    0, 3, 0, 19, 0, 0, 0, 4, 0, 0, 0, 8, 0, 16,
    0, 3, 0, 19, 0, 0, 0, 4 + 64, 0, 0, 0, 8, 0, 16,
    0, 3, 0, 19, 0, 0, 0, 4 + 128, 0, 0, 0, 8, 0, 16, 0, 24, 0, 32, 0, 40, 0, 48, 0, 56, 0, 64,
  };
  uint8_t cbdt_data[4 + 3 * 64] = {0, 3, 0, 0};
  for (unsigned int i = 0; i < 3 * 8; i++)
    cbdt_data[4 + 8 * i + 3] = i / 8 + 1; /* GlyphBitmapDataFormat19 dataLen */

  hb_face_t *face = hb_face_builder_create ();
  hb_blob_t *blob = hb_blob_create ((const char *) cblc, sizeof (cblc),
				    HB_MEMORY_MODE_READONLY, NULL, NULL);
  hb_face_builder_add_table (face, HB_TAG ('C','B','L','C'), blob);
  hb_blob_destroy (blob);
  blob = hb_blob_create ((const char *) cbdt_data, sizeof (cbdt_data),
			 HB_MEMORY_MODE_READONLY, NULL, NULL);
  hb_face_builder_add_table (face, HB_TAG ('C','B','D','T'), blob);
  hb_blob_destroy (blob);

  hb_font_t *font = hb_font_create (face);
  for (unsigned int gid = 0; gid < G_N_ELEMENTS (lengths); gid++)
  {
    blob = hb_ot_color_glyph_reference_png (font, gid);
    g_assert_cmpuint (hb_blob_get_length (blob), ==, lengths[gid]);
    hb_blob_destroy (blob);
  }
  hb_font_destroy (font);
  hb_face_destroy (face);
}

#if !defined(HB_NO_VAR) && !defined(HB_NO_PAINT)
static void
test_hb_ot_color_clip_extents_overflow (void)
{
  /* Glyph 1 has a ClipBoxFormat2 whose deltas saturate every edge to
   * INT32_MAX at wght=1.  Scaling those extents must clamp them instead
   * of converting an out-of-range float to int. */
  hb_face_t *face = hb_test_open_font_file ("fonts/colr-var-overflow.ttf");
  hb_font_t *font = hb_font_create (face);
  hb_variation_t var = {HB_TAG ('w','g','h','t'), 1.f};
  hb_glyph_extents_t extents;
  const struct {
    int scale;
    hb_position_t bearing;
    hb_position_t size;
  } tests[] = {
    /* float(INT32_MAX) rounds to 2^31, one past the clamped bearing. */
    { 1, G_MAXINT32, 1},
    { 2, G_MAXINT32, G_MAXINT32},
    {-1, G_MININT32, 0},
    {-2, G_MININT32, G_MININT32},
  };

  hb_font_set_variations (font, &var, 1);
  int upem = hb_face_get_upem (face);
  for (unsigned i = 0; i < G_N_ELEMENTS (tests); i++)
  {
    hb_font_set_scale (font, tests[i].scale * upem, tests[i].scale * upem);
    g_assert_true (hb_font_get_glyph_extents (font, 1, &extents));
    g_assert_cmpint (extents.x_bearing, ==, tests[i].bearing);
    g_assert_cmpint (extents.y_bearing, ==, tests[i].bearing);
    g_assert_cmpint (extents.width, ==, tests[i].size);
    g_assert_cmpint (extents.height, ==, tests[i].size);
  }

  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_hb_ot_color_varidx_add_overflow (void)
{
  /* Mutate Glyph 1's ClipBoxFormat2 varIdxBase in colr-var-overflow.ttf to
   * 0xFFFFFFFE.  Offsets 0..3 must not wrap 32-bit uint32_t to varIdx 0..1
   * (whose wght=1 deltas are INT32_MAX) or hit NO_VARIATION unchecked. */
  hb_face_t *orig = hb_test_open_font_file ("fonts/colr-var-overflow.ttf");
  hb_blob_t *colr_blob = hb_face_reference_table (orig, HB_TAG ('C','O','L','R'));
  unsigned colr_len = 0;
  const uint8_t *colr_src = (const uint8_t *) hb_blob_get_data (colr_blob, &colr_len);
  g_assert_cmpuint (colr_len, >, 34);

  uint8_t *colr_mut = (uint8_t *) g_malloc (colr_len);
  memcpy (colr_mut, colr_src, colr_len);
  hb_blob_destroy (colr_blob);

  uint32_t off_clip = ((uint32_t) colr_mut[22] << 24) |
                      ((uint32_t) colr_mut[23] << 16) |
                      ((uint32_t) colr_mut[24] << 8)  |
                      (uint32_t) colr_mut[25];
  g_assert_cmpuint (off_clip + 12, <=, colr_len);
  uint32_t box_off = ((uint32_t) colr_mut[off_clip + 9] << 16) |
                     ((uint32_t) colr_mut[off_clip + 10] << 8) |
                     (uint32_t) colr_mut[off_clip + 11];
  uint32_t abs_box = off_clip + box_off;
  g_assert_cmpuint (abs_box + 13, <=, colr_len);
  g_assert_cmpuint (colr_mut[abs_box], ==, 2); /* ClipBoxFormat2 */

  /* Set varIdxBase = 0xFFFFFFFE (base box is 300, 300, 300, 300). */
  colr_mut[abs_box + 9]  = 0xFF;
  colr_mut[abs_box + 10] = 0xFF;
  colr_mut[abs_box + 11] = 0xFF;
  colr_mut[abs_box + 12] = 0xFE;

  hb_face_t *face = hb_face_builder_create ();
  hb_tag_t tags[32];
  unsigned tag_count = G_N_ELEMENTS (tags);
  hb_face_get_table_tags (orig, 0, &tag_count, tags);
  for (unsigned i = 0; i < tag_count; i++)
  {
    if (tags[i] == HB_TAG ('C','O','L','R'))
      continue;
    hb_blob_t *tb = hb_face_reference_table (orig, tags[i]);
    hb_face_builder_add_table (face, tags[i], tb);
    hb_blob_destroy (tb);
  }
  hb_blob_t *mut_blob = hb_blob_create ((const char *) colr_mut, colr_len,
                                        HB_MEMORY_MODE_READONLY, colr_mut, g_free);
  hb_face_builder_add_table (face, HB_TAG ('C','O','L','R'), mut_blob);
  hb_blob_destroy (mut_blob);
  hb_face_destroy (orig);

  hb_font_t *font = hb_font_create (face);
  hb_variation_t var = {HB_TAG ('w','g','h','t'), 1.f};
  hb_font_set_variations (font, &var, 1);
  int upem = hb_face_get_upem (face);
  hb_font_set_scale (font, upem, upem);

  hb_glyph_extents_t extents;
  g_assert_true (hb_font_get_glyph_extents (font, 1, &extents));
  g_assert_cmpint (extents.x_bearing, ==, 300);
  g_assert_cmpint (extents.y_bearing, ==, 300);
  g_assert_cmpint (extents.width, ==, 0);
  g_assert_cmpint (extents.height, ==, 0);

  hb_font_destroy (font);
  hb_face_destroy (face);
}
#endif

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
  hb_test_add (test_hb_ot_color_png_cbdt_records);
#if !defined(HB_NO_VAR) && !defined(HB_NO_PAINT)
  hb_test_add (test_hb_ot_color_clip_extents_overflow);
#endif
#ifndef HB_NO_SVG
  hb_test_add (test_hb_ot_color_svg);
#endif
  hb_test_add (test_hb_ot_color_glyph_has_paint);
#if !defined(HB_NO_VAR) && !defined(HB_NO_PAINT)
  hb_test_add (test_hb_ot_color_varidx_add_overflow);
#endif

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
