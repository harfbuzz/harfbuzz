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

struct _hb_blob_t {
  hb_reference_count_t ref_count;

  hb_blob_t *parent;

  const char *data;
  unsigned int offset;
  unsigned int length;
  hb_memory_mode_t mode;

  hb_destroy_func_t destroy;
  void *user_data;
};
static hb_blob_t _hb_blob_nil = {
  HB_REFERENCE_COUNT_INVALID, /* ref_count */

  NULL, /* parent */

  NULL, /* data */
  0, /* offset */
  0, /* length */
  HB_MEMORY_MODE_READONLY, /* mode */

  NULL, /* destroy */
  NULL /* user_data */
};

static void
_hb_blob_destroy_user_data (hb_blob_t *blob)
{
  if (blob->destroy) {
    if (blob->parent == blob->user_data)
      blob->parent = NULL;
    blob->destroy (blob->user_data);
    blob->destroy = NULL;
    blob->user_data = NULL;
  }
}

static void
_hb_blob_nullify (hb_blob_t *blob)
{
  _hb_blob_destroy_user_data (blob);
  blob->data = NULL;
  blob->offset = 0;
  blob->length = 0;
}

static void
_hb_blob_sync_parent_mode (hb_blob_t *blob)
{
  if (blob->parent) {
    if (blob->mode != HB_MEMORY_MODE_WRITEABLE && hb_blob_is_writeable (blob->parent))
      blob->mode = HB_MEMORY_MODE_WRITEABLE;
  }
}

static void
_hb_blob_sync_parent_data (hb_blob_t *blob)
{
  if (blob->parent) {
    const char *pdata;
    unsigned int plength;

    pdata = hb_blob_get_data (blob->parent, &plength);

    if (pdata != blob->data) {
      if (blob->offset >= plength) {
        /* nothing left */
	_hb_blob_nullify (blob);
      } else {
	blob->data = pdata;
	blob->length = MIN (blob->length, plength - blob->offset);
      }
    }
  }
}

hb_blob_t *
hb_blob_create (const char        *data,
		unsigned int       length,
		hb_memory_mode_t   mode,
		hb_destroy_func_t  destroy,
		void              *user_data)
{
  hb_blob_t *blob;

  if (!length || !HB_OBJECT_DO_CREATE (blob)) {
    if (destroy)
      destroy (user_data);
    return &_hb_blob_nil;
  }

  blob->data = data;
  blob->offset = 0;
  blob->length = length;
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
hb_blob_create_sub_blob (hb_blob_t    *parent,
			 unsigned int  offset,
			 unsigned int  length)
{
  hb_blob_t *blob;

  if (!length || !HB_OBJECT_DO_CREATE (blob))
    return &_hb_blob_nil;

  blob->parent = parent; /* we keep the ref in user_data */

  blob->data = parent->data + 1; /* make sure they're not equal */
  blob->offset = offset;
  blob->length = length;
  blob->mode = parent->mode;

  blob->destroy = (hb_destroy_func_t) hb_blob_destroy;
  blob->user_data = hb_blob_reference (parent);

  _hb_blob_sync_parent_data (blob);

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

const char *
hb_blob_get_data (hb_blob_t    *blob,
		  unsigned int *length)
{
  _hb_blob_sync_parent_data (blob);

  if (length)
    *length = blob->length;

  return blob->data + blob->offset;
}

hb_bool_t
hb_blob_is_writeable (hb_blob_t *blob)
{
  _hb_blob_sync_parent_mode (blob);

  return blob->mode == HB_MEMORY_MODE_WRITEABLE;
}

hb_bool_t
hb_blob_try_writeable_inplace (hb_blob_t *blob)
{
  if (HB_OBJECT_IS_INERT (blob))
    return FALSE;

  _hb_blob_sync_parent_mode (blob);

  if (blob->mode == HB_MEMORY_MODE_READONLY_MAY_MAKE_WRITEABLE) {
    _hb_blob_sync_parent_data (blob);

    if (blob->length) {
      int pagesize;
      unsigned int length;
      const char *addr;

      pagesize = sysconf(_SC_PAGE_SIZE);
      if (-1 == pagesize)
	return FALSE;

      addr = (const char *) (((size_t) blob->data + blob->offset) & pagesize);
      length = (const char *) (((size_t) blob->data + blob->offset + blob->length + pagesize-1) & pagesize)  - addr;
      if (-1 == mprotect ((void *) addr, length, PROT_READ | PROT_WRITE))
        return FALSE;
    }
    blob->mode = HB_MEMORY_MODE_WRITEABLE;
  }

  return blob->mode == HB_MEMORY_MODE_WRITEABLE;
}

/* DANGER: May rebase or nullify */
void
hb_blob_make_writeable (hb_blob_t *blob)
{
  if (HB_OBJECT_IS_INERT (blob))
    return;

  _hb_blob_sync_parent_mode (blob);

  if (blob->mode == HB_MEMORY_MODE_READONLY_NEVER_DUPLICATE)
  {
    _hb_blob_nullify (blob);
  }
  else if (blob->mode == HB_MEMORY_MODE_READONLY)
  {
    char *new_data;

    _hb_blob_sync_parent_data (blob);

    if (blob->length) {
      new_data = malloc (blob->length);
      if (new_data)
	memcpy (new_data, blob->data + blob->offset, blob->length);

      _hb_blob_destroy_user_data (blob);

      if (!new_data) {
	_hb_blob_nullify (blob);
      } else {
	blob->data = new_data;
	blob->offset = 0;
      }
    }

    blob->mode = HB_MEMORY_MODE_WRITEABLE;
  }
  else
    hb_blob_try_writeable_inplace (blob);
}
