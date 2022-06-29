/*
 * Copyright © 2022 Red Hat, Inc.
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
 * Author: Matthias Clasen
 */


#include "hb-test.h"

#include "hb-coretext.h"

CGFontRef
get_cgfont (void)
{
  CTFontCollectionRef collection;
  CFArrayRef ctfaces;
  CTFontDescriptorRef desc;
  CTFontRef ctfont;
  CGFontRef cgfont;

  collection = CTFontCollectionCreateFromAvailableFonts (0);
  ctfaces = CTFontCollectionCreateMatchingFontDescriptors (collection);

  desc = CFArrayGetValueAtIndex (ctfaces, 0);
  ctfont = CTFontCreateWithFontDescriptor (desc, 0.0, NULL);
  cgfont = CTFontCopyGraphicsFont (ctfont, NULL);

  CFRelease (ctfont);

  return cgfont;
}

static void
test_native_coretext_basic (void)
{
  CGFontRef cgfont;
  hb_face_t *face;
  hb_font_t *font;
  CTFontRef ctfont;

  cgfont = get_cgfont ();

  g_assert_nonnull (cgfont);

  face = hb_coretext_face_create (cgfont);
  font = hb_font_create (face);

  ctfont = hb_coretext_font_get_ct_font (font);

  /* Test whatever relationship between cgfont and ctfont
   * we want to guarantee.
   */
  g_assert_nonnull (ctfont);

  hb_font_destroy (font);
  hb_face_destroy (face);

  CFRelease (cgfont);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_native_coretext_basic);

  return hb_test_run ();
}
