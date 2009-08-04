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

#include <unistd.h>
#include <sys/mman.h>

/* XXX Not thread-safe */

struct _hb_blob_t {
  hb_reference_count_t ref_count;

  hb_reference_count_t lock;

  const char *data;
  unsigned int length;
  hb_memory_mode_t mode;

  hb_destroy_func_t destroy;
  void *user_data;
};
static hb_blob_t _hb_blob_nil = {
  HB_REFERENCE_COUNT_INVALID, /* ref_count */

  HB_REFERENCE_COUNT_INVALID, /* lock */

  NULL, /* data */
  0, /* length */
  HB_MEMORY_MODE_READONLY_NEVER_DUPLICATE, /* mode */

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

static void
_hb_blob_unlock_and_destroy (hb_blob_t *blob)
{
  hb_blob_unlock (blob);
  hb_blob_destroy (blob);
}

hb_blob_t *
hb_blob_create (const char        *data,
		unsigned int       length,
		hb_memory_mode_t   mode,
		hb_destroy_func_t  destroy,
		void              *user_data)
{
  hb_blob_t *blob;

  if (!length || !HB_OBJECT_DO_CREATE (hb_blob_t, blob)) {
    if (destroy)
      destroy (user_data);
    return &_hb_blob_nil;
  }

  HB_REFERENCE_COUNT_INIT (blob->lock, 0);

  blob->data = data;
  blob->length = length;
  blob->mode = mode;

  blob->destroy = destroy;
  blob->user_data = user_data;

  if (blob->mode == HB_MEMORY_MODE_DUPLICATE) {
    blob->mode = HB_MEMORY_MODE_READONLY;
    if (!hb_blob_try_writeable (blob)) {
      hb_blob_destroy (blob);
      return &_hb_blob_nil;
    }
  }

  return blob;
}

hb_blob_t *
hb_blob_create_sub_blob (hb_blob_t    *parent,
			 unsigned int  offset,
			 unsigned int  length)
{
  hb_blob_t *blob;
  const char *pdata;

  if (!length || offset >= parent->length || !HB_OBJECT_DO_CREATE (hb_blob_t, blob))
    return &_hb_blob_nil;

  pdata = hb_blob_lock (parent);

  blob->data = pdata + offset;
  blob->length = MIN (length, parent->length - offset);
  blob->mode = parent->mode;

  blob->destroy = (hb_destroy_func_t) _hb_blob_unlock_and_destroy;
  blob->user_data = hb_blob_reference (parent);

  return blob;
}

hb_blob_t *
hb_blob_create_empty (void)
{
  return &_hb_blob_nil;
}

hb_blob_t *
hb_blob_reference (hb_blob_t *blob)
{
  HB_OBJECT_DO_REFERENCE (blob);
}

unsigned int
hb_blob_get_reference_count (hb_blob_t *blob)
{
  HB_OBJECT_DO_GET_REFERENCE_COUNT (blob);
}

void
hb_blob_destroy (hb_blob_t *blob)
{
  HB_OBJECT_DO_DESTROY (blob);

  _hb_blob_destroy_user_data (blob);

  free (blob);
}

unsigned int
hb_blob_get_length (hb_blob_t *blob)
{
  return blob->length;
}

const char *
hb_blob_lock (hb_blob_t *blob)
{
  if (!HB_OBJECT_IS_INERT (blob))
    (void) _hb_reference_count_inc (blob->lock);

  return blob->data;
}

void
hb_blob_unlock (hb_blob_t *blob)
{
  if (!HB_OBJECT_IS_INERT (blob)) {
    int old_lock = _hb_reference_count_inc (blob->lock);
    assert (old_lock > 0);
  }
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
    int pagesize;
    unsigned int length;
    const char *addr;

    pagesize = sysconf(_SC_PAGE_SIZE);
    if (-1 == pagesize)
      return FALSE;

    addr = (const char *) (((size_t) blob->data) & pagesize);
    length = (const char *) (((size_t) blob->data + blob->length + pagesize-1) & pagesize)  - addr;
    if (-1 == mprotect ((void *) addr, length, PROT_READ | PROT_WRITE))
      return FALSE;

    blob->mode = HB_MEMORY_MODE_WRITEABLE;
  }

  return blob->mode == HB_MEMORY_MODE_WRITEABLE;
}

hb_bool_t
hb_blob_try_writeable (hb_blob_t *blob)
{
  if (blob->mode == HB_MEMORY_MODE_READONLY_NEVER_DUPLICATE)
    return FALSE;

  if (blob->mode == HB_MEMORY_MODE_READONLY)
  {
    char *new_data;

    if (HB_REFERENCE_COUNT_HAS_REFERENCE (blob->lock))
      return FALSE;

    new_data = malloc (blob->length);
    if (new_data) {
      memcpy (new_data, blob->data, blob->length);
      blob->data = new_data;
      blob->mode = HB_MEMORY_MODE_WRITEABLE;
      _hb_blob_destroy_user_data (blob);
      return TRUE;
    } else
      return FALSE;
  }

  return
    hb_blob_try_writeable_inplace (blob);
}
