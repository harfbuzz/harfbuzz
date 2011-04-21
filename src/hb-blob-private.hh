/*
 * Copyright © 2010  Red Hat, Inc.
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

#ifndef HB_BLOB_PRIVATE_HH
#define HB_BLOB_PRIVATE_HH

#include "hb-private.hh"

#include "hb-blob.h"
#include "hb-object-private.hh"

HB_BEGIN_DECLS


struct _hb_blob_t {
  hb_object_header_t header;

  unsigned int length;

  hb_mutex_t lock;
  /* the rest are protected by lock */

  unsigned int lock_count;
  hb_memory_mode_t mode;

  const char *data;

  void *user_data;
  hb_destroy_func_t destroy;
};

extern HB_INTERNAL hb_blob_t _hb_blob_nil;


HB_END_DECLS

#endif /* HB_BLOB_PRIVATE_HH */
