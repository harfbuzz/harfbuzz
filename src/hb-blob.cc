/*
 * Copyright © 2009  Red Hat, Inc.
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
 * Red Hat Author(s): Behdad Esfahbod
 */

#include "hb-private.hh"

#include "hb-blob-private.hh"

#ifdef HAVE_SYS_MMAN_H
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <sys/mman.h>
#endif /* HAVE_SYS_MMAN_H */

#include <stdio.h>
#include <errno.h>

HB_BEGIN_DECLS


#ifndef HB_DEBUG_BLOB
#define HB_DEBUG_BLOB (HB_DEBUG+0)
#endif

hb_blob_t _hb_blob_nil = {
  HB_OBJECT_HEADER_STATIC,

  0, /* length */

  HB_MUTEX_INIT, /* lock */

  0, /* lock_count */
  HB_MEMORY_MODE_READONLY, /* mode */

  NULL, /* data */

  NULL, /* user_data */
  NULL  /* destroy */
};

static void
_hb_blob_destroy_user_data (hb_blob_t *blob)
{
  if (blob->destroy) {
    blob->destroy (blob->user_data);
    blob->user_data = NULL;
    blob->destroy = NULL;
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
		void              *user_data,
		hb_destroy_func_t  destroy)
{
  hb_blob_t *blob;

  if (!length || !(blob = hb_object_create<hb_blob_t> ())) {
    if (destroy)
      destroy (user_data);
    return &_hb_blob_nil;
  }

  hb_mutex_init (blob->lock);
  blob->lock_count = 0;

  blob->data = data;
  blob->length = length;
  blob->mode = mode;

  blob->user_data = user_data;
  blob->destroy = destroy;

  if (blob->mode == HB_MEMORY_MODE_DUPLICATE) {
    blob->mode = HB_MEMORY_MODE_READONLY;
    if (!hb_blob_try_writable (blob)) {
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

  if (!length || offset >= parent->length || !(blob = hb_object_create<hb_blob_t> ()))
    return &_hb_blob_nil;

  pdata = hb_blob_lock (parent);

  hb_mutex_lock (parent->lock);
  blob = hb_blob_create (pdata + offset,
			 MIN (length, parent->length - offset),
			 parent->mode,
			 hb_blob_reference (parent),
			 (hb_destroy_func_t) _hb_blob_unlock_and_destroy);
  hb_mutex_unlock (parent->lock);

  return blob;
}

hb_blob_t *
hb_blob_get_empty (void)
{
  return &_hb_blob_nil;
}

hb_blob_t *
hb_blob_reference (hb_blob_t *blob)
{
  return hb_object_reference (blob);
}

void
hb_blob_destroy (hb_blob_t *blob)
{
  if (!hb_object_destroy (blob)) return;

  _hb_blob_destroy_user_data (blob);
  hb_mutex_free (blob->lock);

  free (blob);
}

hb_bool_t
hb_blob_set_user_data (hb_blob_t          *blob,
		       hb_user_data_key_t *key,
		       void *              data,
		       hb_destroy_func_t   destroy)
{
  return hb_object_set_user_data (blob, key, data, destroy);
}

void *
hb_blob_get_user_data (hb_blob_t          *blob,
		       hb_user_data_key_t *key)
{
  return hb_object_get_user_data (blob, key);
}


unsigned int
hb_blob_get_length (hb_blob_t *blob)
{
  return blob->length;
}

const char *
hb_blob_lock (hb_blob_t *blob)
{
  if (hb_object_is_inert (blob))
    return NULL;

  hb_mutex_lock (blob->lock);

  (void) (HB_DEBUG_BLOB &&
    fprintf (stderr, "%p %s (%d) -> %p\n", blob, HB_FUNC,
	     blob->lock_count, blob->data));

  blob->lock_count++;

  hb_mutex_unlock (blob->lock);

  return blob->data;
}

void
hb_blob_unlock (hb_blob_t *blob)
{
  if (hb_object_is_inert (blob))
    return;

  hb_mutex_lock (blob->lock);

  (void) (HB_DEBUG_BLOB &&
    fprintf (stderr, "%p %s (%d) -> %p\n", blob, HB_FUNC,
	     blob->lock_count, blob->data));

  assert (blob->lock_count > 0);
  blob->lock_count--;

  hb_mutex_unlock (blob->lock);
}

hb_bool_t
hb_blob_is_writable (hb_blob_t *blob)
{
  hb_memory_mode_t mode;

  if (hb_object_is_inert (blob))
    return FALSE;

  hb_mutex_lock (blob->lock);

  mode = blob->mode;

  hb_mutex_unlock (blob->lock);

  return mode == HB_MEMORY_MODE_WRITABLE;
}


static hb_bool_t
_try_make_writable_inplace_unix_locked (hb_blob_t *blob)
{
#if defined(HAVE_SYS_MMAN_H) && defined(HAVE_MPROTECT)
  uintptr_t pagesize = -1, mask, length;
  const char *addr;

#if defined(HAVE_SYSCONF) && defined(_SC_PAGE_SIZE)
  pagesize = (uintptr_t) sysconf (_SC_PAGE_SIZE);
#elif defined(HAVE_SYSCONF) && defined(_SC_PAGESIZE)
  pagesize = (uintptr_t) sysconf (_SC_PAGESIZE);
#elif defined(HAVE_GETPAGESIZE)
  pagesize = (uintptr_t) getpagesize ();
#endif

  if ((uintptr_t) -1L == pagesize) {
    (void) (HB_DEBUG_BLOB &&
      fprintf (stderr, "%p %s: failed to get pagesize: %s\n", blob, HB_FUNC, strerror (errno)));
    return FALSE;
  }
  (void) (HB_DEBUG_BLOB &&
    fprintf (stderr, "%p %s: pagesize is %lu\n", blob, HB_FUNC, (unsigned long) pagesize));

  mask = ~(pagesize-1);
  addr = (const char *) (((uintptr_t) blob->data) & mask);
  length = (const char *) (((uintptr_t) blob->data + blob->length + pagesize-1) & mask)  - addr;
  (void) (HB_DEBUG_BLOB &&
    fprintf (stderr, "%p %s: calling mprotect on [%p..%p] (%lu bytes)\n",
	     blob, HB_FUNC,
	     addr, addr+length, (unsigned long) length));
  if (-1 == mprotect ((void *) addr, length, PROT_READ | PROT_WRITE)) {
    (void) (HB_DEBUG_BLOB &&
      fprintf (stderr, "%p %s: %s\n", blob, HB_FUNC, strerror (errno)));
    return FALSE;
  }

  (void) (HB_DEBUG_BLOB &&
    fprintf (stderr, "%p %s: successfully made [%p..%p] (%lu bytes) writable\n",
	     blob, HB_FUNC,
	     addr, addr+length, (unsigned long) length));
  return TRUE;
#else
  return FALSE;
#endif
}

static void
try_writable_inplace_locked (hb_blob_t *blob)
{
  (void) (HB_DEBUG_BLOB &&
    fprintf (stderr, "%p %s: making writable\n", blob, HB_FUNC));

  if (_try_make_writable_inplace_unix_locked (blob)) {
    (void) (HB_DEBUG_BLOB &&
      fprintf (stderr, "%p %s: making writable -> succeeded\n", blob, HB_FUNC));
    blob->mode = HB_MEMORY_MODE_WRITABLE;
  } else {
    (void) (HB_DEBUG_BLOB &&
      fprintf (stderr, "%p %s: making writable -> FAILED\n", blob, HB_FUNC));
    /* Failed to make writable inplace, mark that */
    blob->mode = HB_MEMORY_MODE_READONLY;
  }
}

hb_bool_t
hb_blob_try_writable_inplace (hb_blob_t *blob)
{
  hb_memory_mode_t mode;

  if (hb_object_is_inert (blob))
    return FALSE;

  hb_mutex_lock (blob->lock);

  if (blob->mode == HB_MEMORY_MODE_READONLY_MAY_MAKE_WRITABLE)
    try_writable_inplace_locked (blob);

  mode = blob->mode;

  hb_mutex_unlock (blob->lock);

  return mode == HB_MEMORY_MODE_WRITABLE;
}

hb_bool_t
hb_blob_try_writable (hb_blob_t *blob)
{
  hb_memory_mode_t mode;

  if (hb_object_is_inert (blob))
    return FALSE;

  hb_mutex_lock (blob->lock);

  if (blob->mode == HB_MEMORY_MODE_READONLY_MAY_MAKE_WRITABLE)
    try_writable_inplace_locked (blob);

  if (blob->mode == HB_MEMORY_MODE_READONLY)
  {
    char *new_data;

    (void) (HB_DEBUG_BLOB &&
      fprintf (stderr, "%p %s (%d) -> %p\n", blob, HB_FUNC,
	       blob->lock_count, blob->data));

    if (blob->lock_count)
      goto done;

    new_data = (char *) malloc (blob->length);
    if (new_data) {
      (void) (HB_DEBUG_BLOB &&
	fprintf (stderr, "%p %s: dupped successfully -> %p\n", blob, HB_FUNC, blob->data));
      memcpy (new_data, blob->data, blob->length);
      _hb_blob_destroy_user_data (blob);
      blob->mode = HB_MEMORY_MODE_WRITABLE;
      blob->data = new_data;
      blob->user_data = new_data;
      blob->destroy = free;
    }
  }

done:
  mode = blob->mode;

  hb_mutex_unlock (blob->lock);

  return mode == HB_MEMORY_MODE_WRITABLE;
}


HB_END_DECLS
