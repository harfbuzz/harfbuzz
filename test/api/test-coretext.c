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

CTFontRef
get_ctfont (void)
{
  CTFontCollectionRef collection;
  CFArrayRef ctfaces;
  CTFontDescriptorRef desc;
  CTFontRef ctfont;

  collection = CTFontCollectionCreateFromAvailableFonts (0);
  ctfaces = CTFontCollectionCreateMatchingFontDescriptors (collection);

  desc = CFArrayGetValueAtIndex (ctfaces, 0);
  ctfont = CTFontCreateWithFontDescriptor (desc, 0.0, NULL);

  return ctfont;
}

static void
test_native_coretext_basic (void)
{
  CTFontRef ctfont;
  hb_font_t *font;
  CTFontRef ctfont2;

  ctfont = get_ctfont ();

  g_assert_nonnull (ctfont);

  font = hb_coretext_font_create (ctfont);

  ctfont2 = hb_coretext_font_get_ct_font (font);

  g_assert_true (ctfont2 == ctfont);

  hb_font_destroy (font);

  CFRelease (ctfont);
}

static void
test_native_coretext_variations (void)
{
  // Test that we copy variations from CTFont when creating hb_font_t
  CTFontRef ctfont;
  CFDictionaryRef variations;
  CFIndex count;
  hb_font_t *font;
  unsigned int length;

  // System UI font is a variable font
#if !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE) && MAC_OS_X_VERSION_MIN_REQUIRED < 1080
# define kCTFontUIFontSystem kCTFontSystemFontType
# define kCTFontUIFontEmphasizedSystem kCTFontEmphasizedSystemFontType
#endif
  ctfont = CTFontCreateUIFontForLanguage (kCTFontUIFontSystem, 12.0, CFSTR ("en-US"));
  g_assert_nonnull (ctfont);

  variations = CTFontCopyVariation (ctfont);
  g_assert_nonnull (variations);

  count = CFDictionaryGetCount (variations);
  g_assert_cmpint (count, >, 0);

  font = hb_coretext_font_create (ctfont);
  hb_font_get_var_coords_normalized(font, &length);
  g_assert_cmpuint (length, !=, 0);

  hb_font_destroy (font);

  CFRelease (ctfont);
}

static void
check_coretext_collection (hb_bool_t collection)
{
  const char *filename = collection ? "coretext-collection.ttc" : "coretext-variable.ttf";
  char *path = g_test_build_filename (G_TEST_DIST, "fonts", filename, NULL);
  hb_blob_t *blob = hb_blob_create_from_file_or_fail (path);
  g_assert_nonnull (blob);

  static const char *names[] = {
    "HBCoreTextFirst-Regular", "HBCoreTextSecond-Regular", "HBCoreTextThird-Regular",
  };
  unsigned loaders = 1;
#if (defined(__IPHONE_OS_VERSION_MIN_REQUIRED) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000) || \
    (defined(__MAC_OS_X_VERSION_MIN_REQUIRED) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101500) || \
    (defined(__TV_OS_VERSION_MIN_REQUIRED) && __TV_OS_VERSION_MIN_REQUIRED >= 110000) || \
    (defined(__WATCH_OS_VERSION_MIN_REQUIRED) && __WATCH_OS_VERSION_MIN_REQUIRED >= 40000) || \
    (defined(__MACCATALYST_VERSION_MIN_REQUIRED) && __MACCATALYST_VERSION_MIN_REQUIRED >= 130100) || \
    (defined(__VISION_OS_VERSION_MIN_REQUIRED) && __VISION_OS_VERSION_MIN_REQUIRED >= 10000)
  loaders = 2;
#endif
  for (unsigned loader = 0; loader < loaders; loader++)
    for (unsigned member = 0; member < (collection ? G_N_ELEMENTS (names) : 1); member++)
    {
      unsigned instances = 1;
#ifndef HB_NO_VAR
      if (member < 2) instances = 4;
#endif
      for (unsigned instance = 0; instance < instances; instance++)
      {
        unsigned index = member | (instance << 16);
        hb_face_t *face = loader ? hb_coretext_face_create_from_blob_or_fail (blob, index) :
                                  hb_coretext_face_create_from_file_or_fail (path, index);
        g_assert_nonnull (face);
        g_assert_cmpuint (hb_face_get_index (face), ==, index);
        CGFontRef cg_font = hb_coretext_face_get_cg_font (face);
        CFStringRef name = CGFontCopyPostScriptName (cg_font);
        char actual[64];
        g_assert_true (CFStringGetCString (name, actual, sizeof (actual), kCFStringEncodingUTF8));
        g_assert_cmpstr (actual, ==, names[member]);
        CFRelease (name);

        hb_font_t *font = hb_font_create (face);
        hb_codepoint_t glyph;
        g_assert_true (hb_font_get_nominal_glyph (font, 'A', &glyph));
        int delta = instance == 1 ? 100 : instance == 2 ? -50 : 0;
        int expected = 500 + member * 200 + delta;
        g_assert_cmpint (hb_font_get_glyph_h_advance (font, glyph), ==, expected);
        hb_coretext_font_set_funcs (font);
        g_assert_cmpint (hb_font_get_glyph_h_advance (font, glyph), ==, expected);

        static const char *shapers[] = {"coretext", NULL};
        hb_buffer_t *buffer = hb_buffer_create ();
        hb_buffer_add_utf8 (buffer, "A", -1, 0, -1);
        hb_buffer_guess_segment_properties (buffer);
        g_assert_true (hb_shape_full (font, buffer, NULL, 0, shapers));
        g_assert_cmpuint (hb_buffer_get_length (buffer), ==, 1);
        g_assert_cmpint (hb_buffer_get_glyph_positions (buffer, NULL)[0].x_advance, ==, expected);
        hb_buffer_destroy (buffer);
        hb_font_destroy (font);
        hb_face_destroy (face);
      }
    }

  unsigned invalid_index = collection ? 3 : 1;
  g_assert_null (hb_coretext_face_create_from_file_or_fail (path, invalid_index));
  g_assert_null (hb_coretext_face_create_from_blob_or_fail (blob, invalid_index));
  hb_blob_destroy (blob);
  g_free (path);
}

static void
test_coretext_collection (void)
{
  check_coretext_collection (true);
}

static void
test_coretext_named_instances (void)
{
  check_coretext_collection (false);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_native_coretext_basic);
  hb_test_add (test_native_coretext_variations);
  hb_test_add (test_coretext_collection);
  hb_test_add (test_coretext_named_instances);

  return hb_test_run ();
}
