/*
 * Copyright (C) 2009  Red Hat, Inc.
 *
 *  This is part of HarfBuzz, an OpenType Layout engine library.
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
 * Red Hat Author(s): Behdad Esfahbod
 */

#include "hb-private.h"

#include "hb-blob.h"
#include "hb-refcount-private.h"

struct _hb_blob_t {
  hb_reference_count_t ref_count;

  const char *data;
  unsigned int len;
  hb_memory_mode_t mode;

  hb_destroy_func_t destroy;
  void *user_data;
};
static hb_blob_t _hb_blob_nil = {
  HB_REFERENCE_COUNT_INVALID, /* ref_count */

  NULL, /* data */
  0, /* len */
  HB_MEMORY_MODE_READONLY, /* mode */

  NULL, /* destroy */
  NULL /* user_data */
};

static void
_hb_blob_destroy_user_data (hb_blob_t *blob)
{
  if (blob->destroy) {
    blob->destroy (blob->user_data);
    blob->destroy = NULL;
    blob->user_data = NULL;
  }
}

hb_blob_t *
hb_blob_create (const char        *data,
		unsigned int       len,
		hb_memory_mode_t   mode,
		hb_destroy_func_t  destroy,
		void              *user_data)
{
  hb_blob_t *blob;

  blob = calloc (1, sizeof (hb_blob_t));
  if (!blob) {
    if (destroy)
      destroy (user_data);
    return &_hb_blob_nil;
  }

  HB_OBJECT_DO_CREATE (blob);

  blob->data = data;
  blob->len = len;
  blob->mode = mode;

  blob->destroy = destroy;
  blob->user_data = user_data;

  if (blob->mode == HB_MEMORY_MODE_DUPLICATE) {
    blob->mode = HB_MEMORY_MODE_READONLY;
    hb_blob_make_writeable (blob);
  }

  return blob;
}

hb_blob_t *
hb_blob_reference (hb_blob_t *blob)
{
  HB_OBJECT_DO_REFERENCE (blob);
}

void
hb_blob_destroy (hb_blob_t *blob)
{
  HB_OBJECT_DO_DESTROY (blob);

  _hb_blob_destroy_user_data (blob);

  free (blob);
}

const char *
hb_blob_get_data (hb_blob_t    *blob,
		  unsigned int *len)
{
  if (len)
    *len = blob->len;

  return blob->data;
}

hb_bool_t
hb_blob_is_writeable (hb_blob_t *blob)
{
  return blob->mode == HB_MEMORY_MODE_WRITEABLE;
}

hb_bool_t
hb_blob_try_writeable_inplace (hb_blob_t *blob)
{
  if (blob->mode == HB_MEMORY_MODE_READONLY_MAY_MAKE_WRITEABLE) {
    /* XXX
     * mprotect
    blob->mode == HB_MEMORY_MODE_WRITEABLE;
    */
  }

  return blob->mode == HB_MEMORY_MODE_WRITEABLE;
}

/* DANGER: May rebase or nullify */
void
hb_blob_make_writeable (hb_blob_t *blob)
{
  if (blob->mode == HB_MEMORY_MODE_READONLY_NEVER_DUPLICATE)
  {
    _hb_blob_destroy_user_data (blob);
    blob->data = NULL;
    blob->len = 0;
  }
  else if (blob->mode == HB_MEMORY_MODE_READONLY)
  {
    char *new_data;

    new_data = malloc (blob->len);
    if (new_data)
      memcpy (new_data, blob->data, blob->len);

    _hb_blob_destroy_user_data (blob);

    if (!new_data) {
      blob->data = NULL;
      blob->len = 0;
    } else
      blob->data = new_data;

    blob->mode = HB_MEMORY_MODE_WRITEABLE;
  }
  else
    hb_blob_try_writeable_inplace (blob);
}
