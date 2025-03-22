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

#include "hb-directwrite.h"

/* Declare object creator for dynamic support of DWRITE */
typedef HRESULT (WINAPI *t_DWriteCreateFactory)(
  DWRITE_FACTORY_TYPE factoryType,
  REFIID              iid,
  IUnknown            **factory
);

IDWriteFontFace *
get_dwfontface (const wchar_t *family_name)
{
  HRESULT hr;
  t_DWriteCreateFactory CreateFactory;
  HMODULE dwrite_dll;
  IDWriteFactory *factory;
  IDWriteFactory7 *factory7;
  IDWriteFontCollection3 *collection;
  UINT32 count;
  IDWriteFontFamily2 *family;
  IDWriteFont *font;
  UINT32 index = 0;

  dwrite_dll = LoadLibrary (TEXT ("DWRITE"));
  g_assert_nonnull (dwrite_dll);

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif

  CreateFactory = (t_DWriteCreateFactory) GetProcAddress (dwrite_dll, "DWriteCreateFactory");
  g_assert_nonnull (CreateFactory);

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

  hr = CreateFactory (DWRITE_FACTORY_TYPE_SHARED, __uuidof (IDWriteFactory), (IUnknown**) &factory);
  g_assert_true (SUCCEEDED (hr));

  hr = factory->QueryInterface (__uuidof (IDWriteFactory7), (void**) &factory7);
  g_assert_true (SUCCEEDED (hr));

  hr = factory7->GetSystemFontCollection (FALSE, DWRITE_FONT_FAMILY_MODEL_TYPOGRAPHIC, &collection);
  g_assert_true (SUCCEEDED (hr));

  count = collection->GetFontFamilyCount ();
  g_assert_cmpuint (count, >, 0);

  if (family_name)
  {
    BOOL exists;
    hr = collection->FindFamilyName (family_name, &index, &exists);
    g_assert_true (SUCCEEDED (hr));
    g_assert_true (exists);
  }

  hr = collection->GetFontFamily (index, &family);
  g_assert_true (SUCCEEDED (hr));

  hr = family->GetFirstMatchingFont (DWRITE_FONT_WEIGHT_NORMAL,
                                      DWRITE_FONT_STRETCH_NORMAL,
                                      DWRITE_FONT_STYLE_NORMAL,
                                      &font);
  g_assert_true (SUCCEEDED (hr));

  factory->Release ();

  IDWriteFontFace *dw_face = nullptr;
  hr = font->CreateFontFace (&dw_face);
  g_assert_true (SUCCEEDED (hr));

  font->Release ();

  return dw_face;
}

static void
test_native_directwrite_basic (void)
{
  IDWriteFontFace *dwfontface;
  hb_font_t *font;
  IDWriteFontFace *dwfontface2;

  dwfontface = get_dwfontface (nullptr);
  g_assert_nonnull (dwfontface);

  font = hb_directwrite_font_create (dwfontface);

  dwfontface2 = hb_directwrite_font_get_dw_font_face (font);

  g_assert_true (dwfontface2 == dwfontface);

  hb_font_destroy (font);

  dwfontface->Release ();
}


static void
test_native_directwrite_variations (void)
{
  IDWriteFontFace *dwfontface;
  hb_font_t *font;
  unsigned int length;

  dwfontface = get_dwfontface (L"Bahnschrift");
  g_assert_nonnull (dwfontface);

  font = hb_directwrite_font_create (dwfontface);
  hb_font_get_var_coords_normalized(font, &length);
  g_assert_cmpuint (length, !=, 0);

  hb_font_destroy (font);

  dwfontface->Release ();
}


int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_native_directwrite_basic);
  hb_test_add (test_native_directwrite_variations);

  return hb_test_run ();
}
