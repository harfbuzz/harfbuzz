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

/* Unit tests for hb-ot-layout.h - OpenType MATH table  */

static FT_Library ft_library;
static FT_Face ft_face;
static hb_font_t *hb_font;
static hb_face_t *hb_face;

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
  FT_Error ft_error;
  if ((ft_error = FT_New_Face (ft_library, fontFile, 0, &ft_face)))
    abort();
  unsigned int fontSize = 1000;
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
test_has_math_data (void)
{
  initFreeType();

  openFont("fonts/MathTestFontNone.otf");
  g_assert(!hb_ot_layout_has_math_data (hb_face)); // MATH table not available
  closeFont();

  openFont("fonts/MathTestFontEmpty.otf");
  g_assert(hb_ot_layout_has_math_data (hb_face)); // MATH table available
  closeFont();

  cleanupFreeType();
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_has_math_data);

  return hb_test_run();
}
