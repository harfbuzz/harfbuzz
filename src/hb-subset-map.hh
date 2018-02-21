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
 * Google Author(s): Roderick Sheeter
 */

#ifndef HB_OT_SUBSET_MAP_HH
#define HB_OT_SUBSET_MAP_HH

typedef struct hb_map_t hb_map_t;

HB_INTERNAL hb_map_t *
hb_map_create_or_fail ();

HB_INTERNAL void
hb_map_put (hb_map_t *map, hb_codepoint_t key, hb_codepoint_t value);

HB_INTERNAL hb_bool_t
hb_map_get (hb_map_t *map, hb_codepoint_t key, hb_codepoint_t *value /* OUT */);

HB_INTERNAL void
hb_map_destroy (hb_map_t *map);

#endif /* HB_OT_SUBSET_MAP_HH */