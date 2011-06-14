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

#include "hb-blob.h"
#include "hb-object-private.hh"

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


struct _hb_blob_t {
  hb_object_header_t header;

  bool immutable;

  const char *data;
  unsigned int length;
  hb_memory_mode_t mode;

  void *user_data;
  hb_destroy_func_t destroy;
};

static hb_blob_t _hb_blob_nil = {
  HB_OBJECT_HEADER_STATIC,

  TRUE, /* immutable */

  0, /* length */
  NULL, /* data */
  HB_MEMORY_MODE_READONLY, /* mode */

  NULL, /* user_data */
  NULL  /* destroy */
};


static bool _try_writable (hb_blob_t *blob);

static void
_hb_blob_destroy_user_data (hb_blob_t *blob)
{
  if (blob->destroy) {
    blob->destroy (blob->user_data);
    blob->user_data = NULL;
    blob->destroy = NULL;
  }
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

  blob->data = data;
  blob->length = length;
  blob->mode = mode;

  blob->user_data = user_data;
  blob->destroy = destroy;

  if (blob->mode == HB_MEMORY_MODE_DUPLICATE) {
    blob->mode = HB_MEMORY_MODE_READONLY;
    if (!_try_writable (blob)) {
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

  if (!length || offset >= parent->length)
    return &_hb_blob_nil;

  hb_blob_make_immutable (parent);

  blob = hb_blob_create (parent->data + offset,
			 MIN (length, parent->length - offset),
			 parent->mode,
			 hb_blob_reference (parent),
			 (hb_destroy_func_t) hb_blob_destroy);

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


void
hb_blob_make_immutable (hb_blob_t *blob)
{
  if (hb_object_is_inert (blob))
    return;

  blob->immutable = TRUE;
}

hb_bool_t
hb_blob_is_immutable (hb_blob_t *blob)
{
  return blob->immutable;
}


unsigned int
hb_blob_get_length (hb_blob_t *blob)
{
  return blob->length;
}

const char *
hb_blob_get_data (hb_blob_t *blob, unsigned int *length)
{
  if (length)
    *length = blob->length;

  return blob->data;
}

char *
hb_blob_get_data_writable (hb_blob_t *blob, unsigned int *length)
{
  if (!_try_writable (blob)) {
    if (length)
      *length = 0;

    return NULL;
  }

  if (length)
    *length = blob->length;

  return const_cast<char *> (blob->data);
}


static hb_bool_t
_try_make_writable_inplace_unix (hb_blob_t *blob)
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
      fprintf (stderr, "%p %s: failed to get pagesize: %s\n", (void *) blob, HB_FUNC, strerror (errno)));
    return FALSE;
  }
  (void) (HB_DEBUG_BLOB &&
    fprintf (stderr, "%p %s: pagesize is %lu\n", (void *) blob, HB_FUNC, (unsigned long) pagesize));

  mask = ~(pagesize-1);
  addr = (const char *) (((uintptr_t) blob->data) & mask);
  length = (const char *) (((uintptr_t) blob->data + blob->length + pagesize-1) & mask)  - addr;
  (void) (HB_DEBUG_BLOB &&
    fprintf (stderr, "%p %s: calling mprotect on [%p..%p] (%lu bytes)\n",
	     (void *) blob, HB_FUNC,
	     addr, addr+length, (unsigned long) length));
  if (-1 == mprotect ((void *) addr, length, PROT_READ | PROT_WRITE)) {
    (void) (HB_DEBUG_BLOB &&
      fprintf (stderr, "%p %s: %s\n", (void *) blob, HB_FUNC, strerror (errno)));
    return FALSE;
  }

  blob->mode = HB_MEMORY_MODE_WRITABLE;

  (void) (HB_DEBUG_BLOB &&
    fprintf (stderr, "%p %s: successfully made [%p..%p] (%lu bytes) writable\n",
	     (void *) blob, HB_FUNC,
	     addr, addr+length, (unsigned long) length));
  return TRUE;
#else
  return FALSE;
#endif
}

static bool
_try_writable_inplace (hb_blob_t *blob)
{
  (void) (HB_DEBUG_BLOB &&
    fprintf (stderr, "%p %s: making writable inplace\n", (void *) blob, HB_FUNC));

  if (_try_make_writable_inplace_unix (blob))
    return TRUE;

  (void) (HB_DEBUG_BLOB &&
    fprintf (stderr, "%p %s: making writable -> FAILED\n", (void *) blob, HB_FUNC));

  /* Failed to make writable inplace, mark that */
  blob->mode = HB_MEMORY_MODE_READONLY;
  return FALSE;
}

static bool
_try_writable (hb_blob_t *blob)
{
  if (blob->immutable)
    return FALSE;

  if (blob->mode == HB_MEMORY_MODE_WRITABLE)
    return TRUE;

  if (blob->mode == HB_MEMORY_MODE_READONLY_MAY_MAKE_WRITABLE && _try_writable_inplace (blob))
    return TRUE;

  if (blob->mode == HB_MEMORY_MODE_WRITABLE)
    return TRUE;


  (void) (HB_DEBUG_BLOB &&
    fprintf (stderr, "%p %s -> %p\n", (void *) blob, HB_FUNC, blob->data));

  char *new_data;

  new_data = (char *) malloc (blob->length);
  if (unlikely (!new_data))
    return FALSE;

  (void) (HB_DEBUG_BLOB &&
    fprintf (stderr, "%p %s: dupped successfully -> %p\n", (void *) blob, HB_FUNC, blob->data));

  memcpy (new_data, blob->data, blob->length);
  _hb_blob_destroy_user_data (blob);
  blob->mode = HB_MEMORY_MODE_WRITABLE;
  blob->data = new_data;
  blob->user_data = new_data;
  blob->destroy = free;

  return TRUE;
}


HB_END_DECLS
