/*
 * Copyright © 2023 Matthias Clasen
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
 */

#if !defined(HB_H_IN) && !defined(HB_NO_SINGLE_HEADER_ERROR)
#error "Include <hb.h> instead."
#endif

#ifndef HB_FACE_COLLECTION_H
#define HB_FACE_COLLECTION_H

#include "hb-common.h"

HB_BEGIN_DECLS

/* hb_face_descriptor_t */

typedef struct hb_face_descriptor_t hb_face_collection_t;

hb_face_descriptor_t *
hb_face_descriptor_create ();

void
hb_face_descriptor_destroy (hb_face_descriptor_t *desc);

const char *
hb_face_descriptor_get_name (hb_face_descriptor_t *desc,
                             hb_ot_name_id_t       name);

void
hb_face_descriptor_add_name (hb_face_descriptor_t *desc,
                             hb_ot_name_id_t       name,
                             const char           *value);

float
hb_face_descriptor_get_style (hb_face_descriptor_t *desc,
                              hb_tag_t              style);

void
hb_face_descriptor_add_style (hb_face_descriptor_t *desc,
                              hb_tag_t              style,
                              float                 value);

void
hb_face_descriptor_add_script (hb_face_descriptor_t *desc,
                               hb_script_t           script);

hb_face_descriptor_t *
hb_face_describe (hb_face_t *face);

/* hb_face_collection_t */

typedef struct hb_face_collection_t hb_face_collection_t;

hb_face_collection_t *
hb_face_collection_create ();

void
hb_face_collection_add_from_path (hb_face_collection_t *collection,
                                  const char           *path);

void
hb_face_collection_add_face (hb_face_collection_t *collection,
                             hb_face_t            *face);

unsigned int
hb_face_collection_match (hb_face_collection_t *collection,
                          hb_face_descriptor_t *desc,
                          unsigned int         *len,
                          hb_face_t            *faces);

HB_END_DECLS

#endif  /* HB_FACE_COLLECTION_H */
