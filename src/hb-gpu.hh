/*
 * Copyright (C) 2026  Behdad Esfahbod
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
 * Author(s): Behdad Esfahbod
 */

#ifndef HB_GPU_HH
#define HB_GPU_HH

#include "hb.hh"

/* Internal blob-recycling helpers shared between the GPU encoders.
 * Both hb_gpu_draw_encode() and hb_gpu_paint_encode() build a buffer
 * of the encoded bytes and wrap it as an hb_blob_t.  The caller can
 * later hand the blob back via hb_gpu_*_recycle_blob(); on the next
 * encode the same buffer is reused (possibly realloc'd) and the same
 * blob handle is returned, avoiding malloc/free and blob-allocation
 * traffic across glyph-by-glyph encoding loops.
 *
 * Both encoders identify "their own" recycled blobs by the address
 * of _hb_gpu_blob_data_destroy below, so the bookkeeping struct and
 * destroy callback live in one place.
 */

struct hb_gpu_blob_data_t
{
  char *buf;
  unsigned capacity;
};

HB_INTERNAL void
_hb_gpu_blob_data_destroy (void *user_data);

/* Acquire a buffer of at least @needed bytes.  If @recycled is one
 * of our blobs, reuse its buffer (or realloc it).  *@out_capacity
 * receives the actual capacity (>= @needed).  *@out_replaced_buf is
 * set to the recycled buf when realloc fails and a fresh buffer was
 * allocated instead -- the caller must hb_free() that buf after
 * _hb_gpu_blob_finalize() runs.  Returns nullptr on allocation
 * failure. */
HB_INTERNAL char *
_hb_gpu_blob_acquire (hb_blob_t *recycled,
		      unsigned   needed,
		      unsigned  *out_capacity,
		      char     **out_replaced_buf);

/* Wrap @buf (of @capacity, with @length used) into an hb_blob_t.
 * If @recycled is one of our blobs, update and return it (cheap);
 * otherwise create a new blob.  Pass @replaced_recycled_buf from
 * _hb_gpu_blob_acquire(). */
HB_INTERNAL hb_blob_t *
_hb_gpu_blob_finalize (char      *buf,
		       unsigned   capacity,
		       unsigned   length,
		       hb_blob_t *recycled,
		       char      *replaced_recycled_buf);

/* Discard @buf returned by _hb_gpu_blob_acquire without committing
 * to a blob.  Frees @buf if it was a fresh allocation; leaves any
 * recycled buffer untouched. */
HB_INTERNAL void
_hb_gpu_blob_abort (char *buf, hb_blob_t *recycled);

/* Stash @blob as the encoder's recycled output for the next encode.
 * Destroys any previously stashed blob.  Safe to call with @blob =
 * nullptr or the empty-singleton blob (treated as "drop"). */
HB_INTERNAL void
_hb_gpu_blob_recycle (hb_blob_t **slot, hb_blob_t *blob);

#endif /* HB_GPU_HH */
