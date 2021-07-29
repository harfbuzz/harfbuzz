/*
 * Copyright © 2018  Google, Inc.
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
 * Google Author(s): Rod Sheeter
 */

#ifndef HB_SUBSET_H
#define HB_SUBSET_H

#include "hb.h"

HB_BEGIN_DECLS

/**
 * hb_subset_input_t:
 *
 * Things that change based on the input. Characters to keep, etc.
 */

typedef struct hb_subset_input_t hb_subset_input_t;

/**
 * TODO(garretrieger): update to match added values, and no hinting change.
 * hb_subset_flags_t:
 * @HB_SUBSET_FLAGS_NONE: bit set with no flags set.
 * @HB_SUBSET_FLAGS_NO_HINTING: If set hinting instructions will be dropped in
 * the produced subset. Otherwise hinting instructions will be retained.
 * Defaults to true.
 * @HB_SUBSET_FLAGS_RETAIN_GIDS: If set glyph indices will not be modified in
 * the produced subset. If glyphs are dropped their indices will be retained
 * as an empty glyph. Defaults to false.
 * @HB_SUBSET_FLAGS_DESUBROUTINIZE: If set and subsetting a CFF font the
 * subsetter will attempt to remove subroutines from the CFF glyphs.
 * Defaults to false.
 * @HB_SUBSET_FLAGS_NAME_LEGACY: If set non-unicode name records will be
 * retained in the subset. Defaults to false.
 * @HB_SUBSET_FLAGS_SET_OVERLAPS_FLAG:  If set the subsetter will set the
 * OVERLAP_SIMPLE flag on each simple glyph. Defaults to false.
 * @HB_SUBSET_FLAGS_PASSTHROUGH_UNRECOGNIZED: If set the subsetter will not
 * drop unrecognized tables and instead pass them through untouched.
 * Defaults to false.
 * @HB_SUBSET_FLAGS_NOTDEF_OUTLINE: If set the notdef glyph outline will be
 * retained in the final subset. Defaults to false.
 * @HB_SUBSET_FLAGS_GLYPH_NAMES: If set the PS glyph names will be retained
 * in the final subset. Defaults to false.
 * @HB_SUBSET_FLAGS_NO_PRUNE_UNICODE_RANGES: If set then the unicode ranges in
 * OS/2 will not be recalculated.
 * @HB_SUBSET_FLAGS_RETAIN_ALL_FEATURES: If set all layout features will be
 * retained. If unset then the set accessed by
 * hb_subset_input_layout_features_set() will be used to determine the features
 * to be retained.
 * HB_SUBSET_FLAGS_ALL: bit set with all flags set.
 *
 * List of boolean properties that can be configured on the subset input.
 *
 * Since: REPLACE
 **/
typedef enum
{
  HB_SUBSET_FLAGS_NONE = 0,
  HB_SUBSET_FLAGS_NO_HINTING = 1,
  HB_SUBSET_FLAGS_RETAIN_GIDS = 1 << 1,
  HB_SUBSET_FLAGS_DESUBROUTINIZE = 1 << 2,
  HB_SUBSET_FLAGS_NAME_LEGACY = 1 << 3,
  HB_SUBSET_FLAGS_SET_OVERLAPS_FLAG = 1 << 4,
  HB_SUBSET_FLAGS_PASSTHROUGH_UNRECOGNIZED = 1 << 5,
  HB_SUBSET_FLAGS_NOTDEF_OUTLINE = 1 << 6,
  HB_SUBSET_FLAGS_GLYPH_NAMES = 1 << 7,
  HB_SUBSET_FLAGS_NO_PRUNE_UNICODE_RANGES = 1 << 8,
  HB_SUBSET_FLAGS_RETAIN_ALL_FEATURES = 1 << 9,
  HB_SUBSET_FLAGS_ALL = -1,
} hb_subset_flags_t;

HB_EXTERN hb_subset_input_t *
hb_subset_input_create_or_fail (void);

HB_EXTERN hb_subset_input_t *
hb_subset_input_reference (hb_subset_input_t *input);

HB_EXTERN void
hb_subset_input_destroy (hb_subset_input_t *input);

HB_EXTERN hb_bool_t
hb_subset_input_set_user_data (hb_subset_input_t  *input,
			       hb_user_data_key_t *key,
			       void *		   data,
			       hb_destroy_func_t   destroy,
			       hb_bool_t	   replace);

HB_EXTERN void *
hb_subset_input_get_user_data (const hb_subset_input_t *input,
			       hb_user_data_key_t	   *key);

HB_EXTERN hb_set_t *
hb_subset_input_unicode_set (hb_subset_input_t *input);

HB_EXTERN hb_set_t *
hb_subset_input_glyph_set (hb_subset_input_t *input);

HB_EXTERN hb_set_t *
hb_subset_input_nameid_set (hb_subset_input_t *input);

HB_EXTERN hb_set_t *
hb_subset_input_namelangid_set (hb_subset_input_t *input);

HB_EXTERN hb_set_t *
hb_subset_input_layout_features_set (hb_subset_input_t *input);

HB_EXTERN hb_set_t *
hb_subset_input_no_subset_tables_set (hb_subset_input_t *input);

HB_EXTERN hb_set_t *
hb_subset_input_drop_tables_set (hb_subset_input_t *input);

HB_EXTERN hb_subset_flags_t
hb_subset_input_get_flags (hb_subset_input_t *input);

HB_EXTERN void
hb_subset_input_set_flags (hb_subset_input_t *input,
                           hb_subset_flags_t mask,
                           hb_subset_flags_t value);

HB_EXTERN hb_face_t *
hb_subset_or_fail (hb_face_t *source, const hb_subset_input_t *input);

HB_END_DECLS

#endif /* HB_SUBSET_H */
