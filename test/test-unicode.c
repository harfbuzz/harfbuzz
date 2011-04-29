/*
 * Copyright © 2011  Codethink Limited
 * Copyright © 2011  Google, Inc.
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
 * Google Author(s): Behdad Esfahbod
 */

#include "hb-test.h"

/* Unit tests for hb-unicode.h */


static void
test_unicode_nil (void)
{
  hb_unicode_funcs_t *uf = hb_unicode_funcs_create (NULL);

  g_assert (!hb_unicode_funcs_is_immutable (uf));

  g_assert_cmpint (hb_unicode_get_script (uf, 'd'), ==, HB_SCRIPT_UNKNOWN);

  hb_unicode_funcs_destroy (uf);
}

static void
test_unicode_glib (void)
{
  hb_unicode_funcs_t *uf = hb_glib_get_unicode_funcs ();

  g_assert (hb_unicode_funcs_is_immutable (uf));

  g_assert_cmpint (hb_unicode_get_script (uf, 'd'), ==, HB_SCRIPT_LATIN);
}

static void
test_unicode_default (void)
{
  hb_unicode_funcs_t *uf = hb_unicode_funcs_get_default ();

  g_assert (hb_unicode_funcs_is_immutable (uf));

  g_assert_cmpint (hb_unicode_get_script (uf, 'd'), ==, HB_SCRIPT_LATIN);
}


#define MAGIC0 0x12345678
#define MAGIC1 0x76543210

typedef struct {
  int value;
  gboolean freed;
} data_t;

typedef struct {
  data_t data[2];
} data_fixture_t;
static void
data_fixture_init (data_fixture_t *f, gconstpointer user_data)
{
  f->data[0].value = MAGIC0;
  f->data[1].value = MAGIC1;
}
static void
data_fixture_finish (data_fixture_t *f, gconstpointer user_data)
{
}

static void free_up (void *p)
{
  data_t *data = (data_t *) p;

  g_assert (data->value == MAGIC0 || data->value == MAGIC1);
  g_assert (data->freed == FALSE);
  data->freed = TRUE;
}

static hb_script_t
simple_get_script (hb_unicode_funcs_t *ufuncs,
                   hb_codepoint_t      codepoint,
                   void               *user_data)
{
  data_t *data = (data_t *) user_data;

  g_assert (hb_unicode_funcs_get_parent (ufuncs) == NULL);
  g_assert (data->value == MAGIC0);
  g_assert (data->freed == FALSE);

  if ('a' <= codepoint && codepoint <= 'z')
    return HB_SCRIPT_LATIN;
  else
    return HB_SCRIPT_UNKNOWN;
}

static hb_script_t
a_is_for_arabic_get_script (hb_unicode_funcs_t *ufuncs,
                            hb_codepoint_t      codepoint,
                            void               *user_data)
{
  data_t *data = (data_t *) user_data;

  g_assert (hb_unicode_funcs_get_parent (ufuncs) != NULL);
  g_assert (data->value == MAGIC1);
  g_assert (data->freed == FALSE);

  if (codepoint == 'a') {
    return HB_SCRIPT_ARABIC;
  } else {
    hb_unicode_funcs_t *parent = hb_unicode_funcs_get_parent (ufuncs);

    return hb_unicode_get_script (parent, codepoint);
  }
}

static void
test_unicode_custom (data_fixture_t *f, gconstpointer user_data)
{
  hb_unicode_funcs_t *uf = hb_unicode_funcs_create (NULL);

  hb_unicode_funcs_set_script_func (uf, simple_get_script,
                                    &f->data[0], free_up);

  g_assert_cmpint (hb_unicode_get_script (uf, 'a'), ==, HB_SCRIPT_LATIN);
  g_assert_cmpint (hb_unicode_get_script (uf, '0'), ==, HB_SCRIPT_UNKNOWN);

  g_assert (!hb_unicode_funcs_is_immutable (uf));
  hb_unicode_funcs_make_immutable (uf);
  g_assert (hb_unicode_funcs_is_immutable (uf));

  /* Since uf is immutable now, the following setter should do nothing. */
  hb_unicode_funcs_set_script_func (uf, a_is_for_arabic_get_script,
                                    &f->data[1], free_up);

  g_assert (!f->data[0].freed && !f->data[1].freed);
  hb_unicode_funcs_destroy (uf);
  g_assert (f->data[0].freed && !f->data[1].freed);
}

static void
test_unicode_subclassing_nil (data_fixture_t *f, gconstpointer user_data)
{
  hb_unicode_funcs_t *uf, *aa;

  uf = hb_unicode_funcs_create (NULL);

  aa = hb_unicode_funcs_create (uf);

  hb_unicode_funcs_destroy (uf);

  hb_unicode_funcs_set_script_func (aa, a_is_for_arabic_get_script,
                                    &f->data[1], free_up);

  g_assert_cmpint (hb_unicode_get_script (aa, 'a'), ==, HB_SCRIPT_ARABIC);
  g_assert_cmpint (hb_unicode_get_script (aa, 'b'), ==, HB_SCRIPT_UNKNOWN);

  g_assert (!f->data[0].freed && !f->data[1].freed);
  hb_unicode_funcs_destroy (aa);
  g_assert (!f->data[0].freed && f->data[1].freed);
}

static void
test_unicode_subclassing_default (data_fixture_t *f, gconstpointer user_data)
{
  hb_unicode_funcs_t *uf, *aa;

  uf = hb_unicode_funcs_get_default ();
  aa = hb_unicode_funcs_create (uf);

  hb_unicode_funcs_set_script_func (aa, a_is_for_arabic_get_script,
                                    &f->data[1], free_up);

  g_assert_cmpint (hb_unicode_get_script (aa, 'a'), ==, HB_SCRIPT_ARABIC);
  g_assert_cmpint (hb_unicode_get_script (aa, 'b'), ==, HB_SCRIPT_LATIN);

  g_assert (!f->data[0].freed && !f->data[1].freed);
  hb_unicode_funcs_destroy (aa);
  g_assert (!f->data[0].freed && f->data[1].freed);
}

static void
test_unicode_subclassing_deep (data_fixture_t *f, gconstpointer user_data)
{
  hb_unicode_funcs_t *uf, *aa;

  uf = hb_unicode_funcs_create (NULL);

  hb_unicode_funcs_set_script_func (uf, simple_get_script,
                                    &f->data[0], free_up);

  aa = hb_unicode_funcs_create (uf);

  hb_unicode_funcs_destroy (uf);

  /* make sure the 'uf' didn't get freed, since 'aa' holds a ref */
  g_assert (!f->data[0].freed);

  hb_unicode_funcs_set_script_func (aa, a_is_for_arabic_get_script,
                                    &f->data[1], free_up);

  g_assert_cmpint (hb_unicode_get_script (aa, 'a'), ==, HB_SCRIPT_ARABIC);
  g_assert_cmpint (hb_unicode_get_script (aa, 'b'), ==, HB_SCRIPT_LATIN);
  g_assert_cmpint (hb_unicode_get_script (aa, '0'), ==, HB_SCRIPT_UNKNOWN);

  g_assert (!f->data[0].freed && !f->data[1].freed);
  hb_unicode_funcs_destroy (aa);
  g_assert (f->data[0].freed && f->data[1].freed);
}


int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_unicode_nil);
  hb_test_add (test_unicode_glib);
  hb_test_add (test_unicode_default);

  hb_test_add_fixture (data_fixture, NULL, test_unicode_custom);
  hb_test_add_fixture (data_fixture, NULL, test_unicode_subclassing_nil);
  hb_test_add_fixture (data_fixture, NULL, test_unicode_subclassing_default);
  hb_test_add_fixture (data_fixture, NULL, test_unicode_subclassing_deep);

  /* XXX test all methods for their defaults and various (glib, icu, default) implementations. */
  /* XXX test glib & icu two-way script conversion */

  return hb_test_run ();
}
