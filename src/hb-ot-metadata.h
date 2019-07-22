/*
 * Copyright © 2019  Ebrahim Byagowi
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

#ifndef HB_OT_H_IN
#error "Include <hb-ot.h> instead."
#endif

#ifndef HB_OT_METADATA_H
#define HB_OT_METADATA_H

#include "hb.h"

HB_BEGIN_DECLS

/**
 * hb_ot_metadata_t:
 *
 * From https://docs.microsoft.com/en-us/typography/opentype/spec/meta
 *
 * Since: REPLACEME
 **/
typedef enum {
/*
   HB_OT_METADATA_APPL			= HB_TAG ('a','p','p','l'),
   HB_OT_METADATA_BILD			= HB_TAG ('b','i','l','d'),
*/
  HB_OT_METADATA_DESIGN_LANGUAGES	= HB_TAG ('d','l','n','g'),
  HB_OT_METADATA_SUPPORTED_LANGUAGES	= HB_TAG ('s','l','n','g')
} hb_ot_metadata_t;

HB_EXTERN hb_blob_t *
hb_ot_metadata_reference_entry (hb_face_t *face, hb_ot_metadata_t tag);

HB_END_DECLS

#endif /* HB_OT_METADATA_H */
