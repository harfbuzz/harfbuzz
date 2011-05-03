/*
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
 * Google Author(s): Behdad Esfahbod
 */

#include "hb-test.h"

/* Unit tests for hb-object-private.h */


#ifdef HAVE_FREETYPE
#include <hb-ft.h>
#endif


static void *
create_blob (void)
{
  static char data[] = "test data";
  return hb_blob_create (data, sizeof (data), HB_MEMORY_MODE_READONLY, NULL, NULL);
}
static void *
create_blob_inert (void)
{
  return hb_blob_get_empty ();
}

static void *
create_buffer (void)
{
  return hb_buffer_create (0);
}
static void *
create_buffer_inert (void)
{
  return hb_buffer_create (-1);
}

static void *
create_face (void)
{
  hb_blob_t *blob = (hb_blob_t *) create_blob ();
  hb_face_t *face = hb_face_create_for_data (blob, 0);
  hb_blob_destroy (blob);
  return face;
}
static void *
create_face_inert (void)
{
  return hb_face_create_for_data ((hb_blob_t *) create_blob_inert (), 0);
}

static void *
create_font (void)
{
  hb_face_t *face = (hb_face_t *) create_face ();
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);
  return font;
}
static void *
create_font_inert (void)
{
  return hb_font_create (create_face_inert ());
}

static void *
create_font_funcs (void)
{
  return hb_font_funcs_create ();
}
static void *
create_font_funcs_inert (void)
{
#ifdef HAVE_FREETYPE
  return hb_ft_get_font_funcs ();
#else
  return NULL;
#endif
}

static void *
create_unicode_funcs (void)
{
  return hb_unicode_funcs_create (NULL);
}
static void *
create_unicode_funcs_inert (void)
{
  return hb_unicode_funcs_get_default ();
}



typedef void     *(*create_func_t)         (void);
typedef void     *(*reference_func_t)      (void *obj);
typedef void      (*destroy_func_t)        (void *obj);
typedef hb_bool_t (*set_user_data_func_t)  (void *obj, hb_user_data_key_t *key, void *data, hb_destroy_func_t destroy);
typedef void *    (*get_user_data_func_t)  (void *obj, hb_user_data_key_t *key);
typedef void      (*make_immutable_func_t) (void *obj);
typedef hb_bool_t (*is_immutable_func_t)   (void *obj);

typedef struct {
  create_func_t          create;
  create_func_t          create_inert;
  reference_func_t       reference;
  destroy_func_t         destroy;
  set_user_data_func_t   set_user_data;
  get_user_data_func_t   get_user_data;
  make_immutable_func_t  make_immutable;
  is_immutable_func_t    is_immutable;
  const char            *name;
} object_t;

#define OBJECT_WITHOUT_IMMUTABILITY(name) \
  { \
    (create_func_t)         create_##name, \
    (create_func_t)         create_##name##_inert, \
    (reference_func_t)      hb_##name##_reference, \
    (destroy_func_t)        hb_##name##_destroy, \
    (set_user_data_func_t)  hb_##name##_set_user_data, \
    (get_user_data_func_t)  hb_##name##_get_user_data, \
    (make_immutable_func_t) NULL, \
    (is_immutable_func_t)   NULL, \
    #name, \
  }
#define OBJECT_WITH_IMMUTABILITY(name) \
  { \
    (create_func_t)         create_##name, \
    (create_func_t)         create_##name##_inert, \
    (reference_func_t)      hb_##name##_reference, \
    (destroy_func_t)        hb_##name##_destroy, \
    (set_user_data_func_t)  hb_##name##_set_user_data, \
    (get_user_data_func_t)  hb_##name##_get_user_data, \
    (make_immutable_func_t) hb_##name##_make_immutable, \
    (is_immutable_func_t)   hb_##name##_is_immutable, \
    #name, \
  }
static const object_t objects[] =
{
  OBJECT_WITHOUT_IMMUTABILITY (blob),
  OBJECT_WITHOUT_IMMUTABILITY (buffer),
  OBJECT_WITHOUT_IMMUTABILITY (face),
  OBJECT_WITHOUT_IMMUTABILITY (font),
  OBJECT_WITH_IMMUTABILITY (font_funcs),
  OBJECT_WITH_IMMUTABILITY (unicode_funcs)
};
#undef OBJECT


#define MAGIC0 0x12345678
#define MAGIC1 0x76543210

typedef struct {
  int value;
  gboolean freed;
} data_t;

static int global_data;

static void global_free_up (void *p G_GNUC_UNUSED)
{
  global_data++;
}

static void free_up0 (void *p)
{
  data_t *data = (data_t *) p;

  g_assert_cmphex (data->value, ==, MAGIC0);
  g_assert (!data->freed);
  data->freed = TRUE;
}

static void free_up1 (void *p)
{
  data_t *data = (data_t *) p;

  g_assert_cmphex (data->value, ==, MAGIC1);
  g_assert (!data->freed);
  data->freed = TRUE;
}

static void
test_object (void)
{
  unsigned int i;

  for (i = 0; i < G_N_ELEMENTS (objects); i++) {
    const object_t *o = &objects[i];
    void *obj;
    hb_user_data_key_t key[2];

    {
      unsigned int i;
      data_t data[2] = {{MAGIC0, FALSE}, {MAGIC1, FALSE}};

      g_test_message ("Testing object %s", o->name);

      g_test_message ("->create()");
      obj = o->create ();
      g_assert (obj);

      g_assert (obj == o->reference (obj));
      o->destroy (obj);

      if (o->is_immutable)
	g_assert (!o->is_immutable (obj));

      g_assert (o->set_user_data (obj, &key[0], &data[0], free_up0));
      g_assert (o->get_user_data (obj, &key[0]) == &data[0]);

      if (o->is_immutable) {
	o->make_immutable (obj);
	g_assert (o->is_immutable (obj));
      }

      /* Should still work even if object is made immutable */
      g_assert (o->set_user_data (obj, &key[1], &data[1], free_up1));
      g_assert (o->get_user_data (obj, &key[1]) == &data[1]);

      g_assert (!o->set_user_data (obj, NULL, &data[0], free_up0));
      g_assert (o->get_user_data (obj, &key[0]) == &data[0]);
      g_assert (o->set_user_data (obj, &key[0], &data[1], NULL));
      g_assert (data[0].freed);
      g_assert (o->get_user_data (obj, &key[0]) == &data[1]);
      g_assert (!data[1].freed);

      data[0].freed = FALSE;
      g_assert (o->set_user_data (obj, &key[0], &data[0], free_up0));
      g_assert (!data[0].freed);
      g_assert (o->set_user_data (obj, &key[0], NULL, NULL));
      g_assert (data[0].freed);

      data[0].freed = FALSE;
      global_data = 0;
      g_assert (o->set_user_data (obj, &key[0], &data[0], free_up0));
      g_assert_cmpuint (global_data, ==, 0);
      g_assert (o->set_user_data (obj, &key[0], NULL, global_free_up));
      g_assert_cmpuint (global_data, ==, 0);
      g_assert (o->set_user_data (obj, &key[0], NULL, NULL));
      g_assert_cmpuint (global_data, ==, 1);

      global_data = 0;
      for (i = 2; i < 1000; i++)
	g_assert (o->set_user_data (obj, &key[i], &data[i], global_free_up));
      for (i = 2; i < 1000; i++)
	g_assert (o->get_user_data (obj, &key[i]) == &data[i]);
      for (i = 100; i < 1000; i++)
	g_assert (o->set_user_data (obj, &key[i], NULL, NULL));
      g_assert_cmpuint (global_data, ==, 900);


      g_assert (!data[1].freed);
      o->destroy (obj);
      g_assert (data[0].freed);
      g_assert (data[1].freed);
      g_assert_cmpuint (global_data, ==, 1000-2);
    }

    {
      data_t data[2] = {{MAGIC0, FALSE}, {MAGIC1, FALSE}};

      g_test_message ("->create_inert()");
      obj = o->create_inert ();
      if (!obj)
	continue;

      g_assert (obj == o->reference (obj));
      o->destroy (obj);

      if (o->is_immutable)
	g_assert (o->is_immutable (obj));

      g_assert (!o->set_user_data (obj, &key[0], &data[0], free_up0));
      g_assert (!o->get_user_data (obj, &key[0]));

      o->destroy (obj);
      o->destroy (obj);
      o->destroy (obj);
      o->destroy (obj);
      o->destroy (obj);

      g_assert (!data[0].freed);
    }
  }
}


int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_object);

  return hb_test_run ();
}
