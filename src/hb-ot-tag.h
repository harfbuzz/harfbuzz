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

#ifndef HB_OT_H_IN
#error "Include <hb-ot.h> instead."
#endif

#ifndef HB_OT_TAG_H
#define HB_OT_TAG_H

#include "hb.h"

HB_BEGIN_DECLS


#define HB_OT_TAG_DEFAULT_SCRIPT	HB_TAG ('D', 'F', 'L', 'T')
#define HB_OT_TAG_DEFAULT_LANGUAGE	HB_TAG ('d', 'f', 'l', 't')

/**
 * HB_OT_MAX_TAGS_PER_SCRIPT:
 *
 * Since: 2.0.0
 **/
#define HB_OT_MAX_TAGS_PER_SCRIPT	3u
/**
 * HB_OT_MAX_TAGS_PER_LANGUAGE:
 *
 * Since: 2.0.0
 **/
#define HB_OT_MAX_TAGS_PER_LANGUAGE	3u

HB_EXTERN void
hb_ot_tags_from_script_and_language (hb_script_t   script,
				     hb_language_t language,
				     unsigned int *script_count /* IN/OUT */,
				     hb_tag_t     *script_tags /* OUT */,
				     unsigned int *language_count /* IN/OUT */,
				     hb_tag_t     *language_tags /* OUT */);

HB_EXTERN hb_script_t
hb_ot_tag_to_script (hb_tag_t tag);

HB_EXTERN hb_language_t
hb_ot_tag_to_language (hb_tag_t tag);

HB_EXTERN void
hb_ot_tags_to_script_and_language (hb_tag_t       script_tag,
				   hb_tag_t       language_tag,
				   hb_script_t   *script /* OUT */,
				   hb_language_t *language /* OUT */);


HB_END_DECLS

#endif /* HB_OT_TAG_H */
