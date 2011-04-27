/*
 * Copyright © 2011 Codethink Limited
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
 * Codethink Author(s): Ryan Lortie
 */

#include "hb-test.h"

/* This file tests the unicode virtual functions interface */

static void
test_nil (void)
{
  hb_unicode_funcs_t *uf = hb_unicode_funcs_create (NULL);

  g_assert_cmpint (hb_unicode_get_script (uf, 'd'), ==, HB_SCRIPT_UNKNOWN);

  hb_unicode_funcs_destroy (uf);
}

static void
test_glib (void)
{
  hb_unicode_funcs_t *uf = hb_glib_get_unicode_funcs ();

  g_assert_cmpint (hb_unicode_get_script (uf, 'd'), ==, HB_SCRIPT_LATIN);
}

static void
test_default (void)
{
  hb_unicode_funcs_t *uf = hb_unicode_funcs_get_default ();

  g_assert_cmpint (hb_unicode_get_script (uf, 'd'), ==, HB_SCRIPT_LATIN);
}

static gboolean freed0, freed1;
static int unique_pointer0[1];
static int unique_pointer1[1];
static void free_up (void *up)
{
  if (up == unique_pointer0) {
    g_assert (!freed0);
    freed0 = TRUE;
  } else if (up == unique_pointer1) {
    g_assert (!freed1);
    freed1 = TRUE;
  } else {
    g_assert_not_reached ();
  }
}

static hb_script_t
simple_get_script (hb_unicode_funcs_t *ufuncs,
                   hb_codepoint_t      codepoint,
                   void               *user_data)
{
  g_assert (hb_unicode_funcs_get_parent (ufuncs) == NULL);
  g_assert (unique_pointer0 == user_data);

  if ('a' <= codepoint && codepoint <= 'z')
    return HB_SCRIPT_LATIN;
  else
    return HB_SCRIPT_UNKNOWN;
}

static void
test_custom (void)
{
  hb_unicode_funcs_t *uf = hb_unicode_funcs_create (NULL);

  hb_unicode_funcs_set_script_func (uf, simple_get_script,
                                    unique_pointer0, free_up);

  g_assert_cmpint (hb_unicode_get_script (uf, 'a'), ==, HB_SCRIPT_LATIN);
  g_assert_cmpint (hb_unicode_get_script (uf, '0'), ==, HB_SCRIPT_UNKNOWN);

  g_assert (!freed0 && !freed1);
  hb_unicode_funcs_destroy (uf);
  g_assert (freed0 && !freed1);
  freed0 = FALSE;
}


static hb_script_t
a_is_for_arabic_get_script (hb_unicode_funcs_t *ufuncs,
                            hb_codepoint_t      codepoint,
                            void               *user_data)
{
  g_assert (hb_unicode_funcs_get_parent (ufuncs) != NULL);
  g_assert (user_data == unique_pointer1);

  if (codepoint == 'a') {
    return HB_SCRIPT_ARABIC;
  } else {
    hb_unicode_funcs_t *parent = hb_unicode_funcs_get_parent (ufuncs);

    return hb_unicode_get_script (parent, codepoint);
  }
}

static void
test_subclassing_nil (void)
{
  hb_unicode_funcs_t *uf, *aa;

  uf = hb_unicode_funcs_create (NULL);

  aa = hb_unicode_funcs_create (uf);

  hb_unicode_funcs_destroy (uf);

  hb_unicode_funcs_set_script_func (aa, a_is_for_arabic_get_script,
                                    unique_pointer1, free_up);

  g_assert_cmpint (hb_unicode_get_script (aa, 'a'), ==, HB_SCRIPT_ARABIC);
  g_assert_cmpint (hb_unicode_get_script (aa, 'b'), ==, HB_SCRIPT_UNKNOWN);

  g_assert (!freed0 && !freed1);
  hb_unicode_funcs_destroy (aa);
  g_assert (!freed0 && freed1);
  freed1 = FALSE;
}

static void
test_subclassing_glib (void)
{
  hb_unicode_funcs_t *uf, *aa;

  uf = hb_glib_get_unicode_funcs ();
  aa = hb_unicode_funcs_create (uf);

  hb_unicode_funcs_set_script_func (aa, a_is_for_arabic_get_script,
                                    unique_pointer1, free_up);

  g_assert_cmpint (hb_unicode_get_script (aa, 'a'), ==, HB_SCRIPT_ARABIC);
  g_assert_cmpint (hb_unicode_get_script (aa, 'b'), ==, HB_SCRIPT_LATIN);

  g_assert (!freed0 && !freed1);
  hb_unicode_funcs_destroy (aa);
  g_assert (!freed0 && freed1);
  freed1 = FALSE;
}

static void
test_subclassing_deep (void)
{
  hb_unicode_funcs_t *uf, *aa;

  uf = hb_unicode_funcs_create (NULL);

  hb_unicode_funcs_set_script_func (uf, simple_get_script,
                                    unique_pointer0, free_up);

  aa = hb_unicode_funcs_create (uf);

  hb_unicode_funcs_destroy (uf);

  /* make sure the 'uf' didn't get freed, since 'aa' holds a ref */
  g_assert (!freed0);

  hb_unicode_funcs_set_script_func (aa, a_is_for_arabic_get_script,
                                    unique_pointer1, free_up);

  g_assert_cmpint (hb_unicode_get_script (aa, 'a'), ==, HB_SCRIPT_ARABIC);
  g_assert_cmpint (hb_unicode_get_script (aa, 'b'), ==, HB_SCRIPT_LATIN);
  g_assert_cmpint (hb_unicode_get_script (aa, '0'), ==, HB_SCRIPT_UNKNOWN);

  g_assert (!freed0 && !freed1);
  hb_unicode_funcs_destroy (aa);
  g_assert (freed0 && freed1);
  freed0 = freed1 = FALSE;
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/unicode/nil", test_nil);
  g_test_add_func ("/unicode/glib", test_glib);
  g_test_add_func ("/unicode/default", test_default);
  g_test_add_func ("/unicode/custom", test_custom);
  g_test_add_func ("/unicode/subclassing/nil", test_subclassing_nil);
  g_test_add_func ("/unicode/subclassing/glib", test_subclassing_glib);
  g_test_add_func ("/unicode/subclassing/deep", test_subclassing_deep);

  /* XXX test all methods for their defaults and various (glib, icu, default) implementations. */
  /* XXX test glib & icu two-way script conversion */

  return g_test_run ();
}
