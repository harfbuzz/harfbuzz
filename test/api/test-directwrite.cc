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
 * Author: Chun-wei Fan
 */


#include "hb-test.h"

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <initguid.h>
#include <dwrite.h>
#include "hb-directwrite.h"

#ifdef _MSC_VER
# define UUID_OF_IDWriteFactory __uuidof (IDWriteFactory)
#else
# define UUID_OF_IDWriteFactory IID_IDWriteFactory
#endif

static hb_face_t *
get_directwrite_hb_face (void)
{
  HRESULT hr;
  IDWriteFactory *factory = NULL;
  IDWriteFontCollection *collection = NULL;
  UINT32 count = 0;
  IDWriteFontFamily *family = NULL;
  IDWriteFont *font = NULL;
  IDWriteFontFace *face = NULL;
  hb_face_t *hb_face;
  bool success = TRUE;

  hr = DWriteCreateFactory (DWRITE_FACTORY_TYPE_SHARED,
                            UUID_OF_IDWriteFactory,
                            reinterpret_cast<IUnknown**> (&factory));

  if (FAILED (hr) || !factory)
    {
      g_error ("DWriteCreateFactory failed with error code %x", (unsigned)hr);
      success = FALSE;
    }

  if (success)
    factory->GetSystemFontCollection (&collection, FALSE);
  if (success && (FAILED (hr) || collection == NULL))
    {
      g_error ("IDWriteFactory::GetSystemFontCollection failed with error code %x\n", (unsigned)hr);
      success = FALSE;
    }

  /* get the first family, then the first font (and so face) from the first family */
  if (success)
    hr = collection->GetFontFamily (0, &family);
  if (success && (FAILED (hr) || family == NULL))
    {
      g_error ("IDWriteFontCollection::GetFontFamily failed with error code %x\n", (unsigned)hr);
      success = FALSE;
    }

  if (success)
    hr = family->GetFont (0, &font);
  if (success && (FAILED (hr) || font == NULL))
    {
      g_error ("IDWriteFontFamily::GetFont failed with error code %x\n", (unsigned)hr);
      success = FALSE;
    }

  if (success)
    hr = font->CreateFontFace (&face);
  if (success && (FAILED (hr) || face == NULL))
    {
      g_error ("IDWriteFont::CreateFontFace failed with error code %x\n", (unsigned)hr);
      success = FALSE;
    }

  g_assert_true (success);
  g_assert_nonnull (face);

  if (success)
    hb_face = hb_directwrite_face_create (face);

  if (face != NULL)
    face->Release ();
  if (font != NULL)
    font->Release ();
  if (collection != NULL)
    collection->Release ();
  if (factory != NULL)
    factory->Release ();

  return hb_face;
}

static void
test_native_directwrite_basic (void)
{
  IDWriteFontFace *result = NULL;
  hb_face_t *face;

  face = get_directwrite_hb_face ();

  g_assert_nonnull (face);
  g_assert_true (face != hb_face_get_empty ());

  result = hb_directwrite_face_get_font_face (face);

  g_assert_nonnull (result);

  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_native_directwrite_basic);

  return hb_test_run ();
}
