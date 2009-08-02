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

#ifndef HB_BLOB_H
#define HB_BLOB_H

#include "hb-common.h"

HB_BEGIN_DECLS

typedef enum {
  HB_MEMORY_MODE_DUPLICATE,
  HB_MEMORY_MODE_READONLY,
  HB_MEMORY_MODE_WRITEABLE,
  HB_MEMORY_MODE_READONLY_NEVER_DUPLICATE,
  HB_MEMORY_MODE_READONLY_MAY_MAKE_WRITEABLE,
} hb_memory_mode_t;

typedef struct _hb_blob_t hb_blob_t;

hb_blob_t *
hb_blob_create (const char        *data,
		unsigned int       len,
		hb_memory_mode_t   mode,
		hb_destroy_func_t  destroy,
		void              *user_data);

hb_blob_t *
hb_blob_reference (hb_blob_t *blob);

unsigned int
hb_blob_get_reference_count (hb_blob_t *blob);

void
hb_blob_destroy (hb_blob_t *blob);

const char *
hb_blob_get_data (hb_blob_t    *blob,
		  unsigned int *len);

hb_bool_t
hb_blob_is_writeable (hb_blob_t *blob);

hb_bool_t
hb_blob_try_writeable_inplace (hb_blob_t *blob);

/* DANGER: May rebase or nullify */
void
hb_blob_make_writeable (hb_blob_t *blob);

HB_END_DECLS

#endif /* HB_BLOB_H */
