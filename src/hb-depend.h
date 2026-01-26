/*
 * Copyright © 2024  Adobe, Inc.
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
 * Adobe Author(s): Skef Iterum
 */

#if !defined(HB_H_IN) && !defined(HB_NO_SINGLE_HEADER_ERROR)
#error "Include <hb.h> instead."
#endif

#ifndef HB_DEPEND_H
#define HB_DEPEND_H


HB_BEGIN_DECLS

#ifdef HB_DEPEND_API

/**
 * hb_depend_t:
 *
 * Data type for holding glyph dependency graphs.
 *
 * <warning>Highly experimental API. Subject to change.</warning>
 *
 * Since: REPLACEME
 */

typedef struct hb_depend_t hb_depend_t;

HB_EXTERN hb_depend_t *
hb_depend_from_face (hb_face_t *face);

HB_EXTERN hb_bool_t
hb_depend_get_glyph_entry(hb_depend_t *depend,
                          hb_codepoint_t gid,
                          hb_codepoint_t index,
                          hb_tag_t *table_tag, /* OUT */
                          hb_codepoint_t *dependent, /* OUT */
                          hb_tag_t *layout_tag, /* OUT */
                          hb_codepoint_t *ligature_set, /* OUT */
                          hb_codepoint_t *context_set /* OUT */);

HB_EXTERN hb_bool_t
hb_depend_get_set_from_index(hb_depend_t *depend, hb_codepoint_t index,
                             hb_set_t *out);

HB_EXTERN void
hb_depend_print (hb_depend_t *depend);

HB_EXTERN void
hb_depend_destroy(hb_depend_t *depend);

#endif

HB_END_DECLS

#endif /* HB_DEPEND_H */
