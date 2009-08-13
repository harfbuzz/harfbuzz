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

#ifdef HAVE_SYS_MMAN_H
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <sys/mman.h>
#endif /* HAVE_SYS_MMAN_H */

struct _hb_blob_t {
  hb_reference_count_t ref_count;

  unsigned int length;

  hb_mutex_t lock;
  /* the rest are protected by lock */

  unsigned int lock_count;
  hb_memory_mode_t mode;

  const char *data;

  hb_destroy_func_t destroy;
  void *user_data;
};
static hb_blob_t _hb_blob_nil = {
  HB_REFERENCE_COUNT_INVALID, /* ref_count */

  0, /* length */

  HB_MUTEX_INIT, /* lock */

  0, /* lock_count */
  HB_MEMORY_MODE_READONLY_NEVER_DUPLICATE, /* mode */

  NULL, /* data */

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

  hb_mutex_init (blob->lock);
  blob->lock_count = 0;

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

  hb_mutex_lock (parent->lock);
  blob->mode = parent->mode;
  hb_mutex_unlock (parent->lock);

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
  if (HB_OBJECT_IS_INERT (blob))
    return NULL;

  hb_mutex_lock (blob->lock);

  blob->lock_count++;
#if HB_DEBUG
  fprintf (stderr, "%p %s (%d) -> %p\n", blob, __FUNCTION__,
	   blob->lock_count, blob->data);
#endif

  hb_mutex_unlock (blob->lock);

  return blob->data;
}

void
hb_blob_unlock (hb_blob_t *blob)
{
  if (HB_OBJECT_IS_INERT (blob))
    return;

  hb_mutex_lock (blob->lock);

  assert (blob->lock_count > 0);
  blob->lock_count--;
#if HB_DEBUG
  fprintf (stderr, "%p %s (%d) -> %p\n", blob, __FUNCTION__,
	   hb_atomic_int_get (blob->lock_count), blob->data);
#endif

  hb_mutex_unlock (blob->lock);
}

hb_bool_t
hb_blob_is_writeable (hb_blob_t *blob)
{
  hb_memory_mode_t mode;

  if (HB_OBJECT_IS_INERT (blob))
    return FALSE;

  hb_mutex_lock (blob->lock);

  mode = blob->mode;

  hb_mutex_unlock (blob->lock);

  return mode == HB_MEMORY_MODE_WRITEABLE;
}

hb_bool_t
hb_blob_try_writeable_inplace (hb_blob_t *blob)
{
  hb_memory_mode_t mode;

  if (HB_OBJECT_IS_INERT (blob))
    return FALSE;

  hb_mutex_lock (blob->lock);

#ifdef HAVE_SYS_MMAN_H
  if (blob->mode == HB_MEMORY_MODE_READONLY_MAY_MAKE_WRITEABLE) {
    unsigned int pagesize, mask, length;
    const char *addr;

#if HB_DEBUG
    fprintf (stderr, "%p %s: making writeable\n", blob, __FUNCTION__);
#endif
    pagesize = (unsigned int) sysconf(_SC_PAGE_SIZE);
    if ((unsigned int) -1 == pagesize) {
#if HB_DEBUG
      fprintf (stderr, "%p %s: %s\n", blob, __FUNCTION__, strerror (errno));
#endif
      goto done;
    }
#if HB_DEBUG
    fprintf (stderr, "%p %s: pagesize is %u\n", blob, __FUNCTION__, pagesize);
#endif

    mask = ~(pagesize-1);
    addr = (const char *) (((size_t) blob->data) & mask);
    length = (const char *) (((size_t) blob->data + blob->length + pagesize-1) & mask)  - addr;
#if HB_DEBUG
    fprintf (stderr, "%p %s: calling mprotect on [%p..%p] (%d bytes)\n",
	     blob, __FUNCTION__,
	     addr, addr+length, length);
#endif
    if (-1 == mprotect ((void *) addr, length, PROT_READ | PROT_WRITE)) {
#if HB_DEBUG
      fprintf (stderr, "%p %s: %s\n", blob, __FUNCTION__, strerror (errno));
#endif
      goto done;
    }

    blob->mode = HB_MEMORY_MODE_WRITEABLE;

#if HB_DEBUG
    fprintf (stderr, "%p %s: successfully made [%p..%p] (%d bytes) writeable\n",
	     blob, __FUNCTION__,
	     addr, addr+length, length);
#endif
  }
#else /* !HAVE_SYS_MMAN_H */
#warning "No way to make readonly memory writeable.  This is suboptimal."
#endif

done:
  mode = blob->mode;

  hb_mutex_unlock (blob->lock);

  return mode == HB_MEMORY_MODE_WRITEABLE;
}

hb_bool_t
hb_blob_try_writeable (hb_blob_t *blob)
{
  hb_memory_mode_t mode;

  if (HB_OBJECT_IS_INERT (blob))
    return FALSE;

  hb_mutex_lock (blob->lock);

  if (blob->mode == HB_MEMORY_MODE_READONLY_NEVER_DUPLICATE)
    goto done;

  if (blob->mode == HB_MEMORY_MODE_READONLY)
  {
    char *new_data;

#if HB_DEBUG
    fprintf (stderr, "%p %s (%d) -> %p\n", blob, __FUNCTION__,
	     blob->lock_count, blob->data);
#endif

    if (blob->lock_count)
      goto done;

    new_data = malloc (blob->length);
    if (new_data) {
#if HB_DEBUG
      fprintf (stderr, "%p %s: dupped successfully -> %p\n", blob, __FUNCTION__, blob->data);
#endif
      memcpy (new_data, blob->data, blob->length);
      blob->data = new_data;
      blob->mode = HB_MEMORY_MODE_WRITEABLE;
      _hb_blob_destroy_user_data (blob);
    }
  }

done:
  mode = blob->mode;

  hb_mutex_unlock (blob->lock);

  if (blob->mode == HB_MEMORY_MODE_READONLY_MAY_MAKE_WRITEABLE)
    return hb_blob_try_writeable_inplace (blob);

  return mode == HB_MEMORY_MODE_WRITEABLE;
}
