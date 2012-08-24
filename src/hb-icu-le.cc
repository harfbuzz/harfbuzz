/*
 * Copyright © 2012  Google, Inc.
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
 * Google Author(s): Behdad Esfahbod
 */

#define HB_SHAPER icu_le
#include "hb-shaper-impl-private.hh"

#include "hb-icu-le/PortableFontInstance.h"

#include "layout/loengine.h"

#include <harfbuzz.h>


#ifndef HB_DEBUG_ICU_LE
#define HB_DEBUG_ICU_LE (HB_DEBUG+0)
#endif


/*
 * shaper face data
 */

struct hb_icu_le_shaper_face_data_t {};

hb_icu_le_shaper_face_data_t *
_hb_icu_le_shaper_face_data_create (hb_face_t *face)
{
  return (hb_icu_le_shaper_face_data_t *) HB_SHAPER_DATA_SUCCEEDED;
}

void
_hb_icu_le_shaper_face_data_destroy (hb_icu_le_shaper_face_data_t *data)
{
}


/*
 * shaper font data
 */

struct hb_icu_le_shaper_font_data_t {};

hb_icu_le_shaper_font_data_t *
_hb_icu_le_shaper_font_data_create (hb_font_t *font)
{
  return (hb_icu_le_shaper_font_data_t *) HB_SHAPER_DATA_SUCCEEDED;
}

void
_hb_icu_le_shaper_font_data_destroy (hb_icu_le_shaper_font_data_t *data)
{
  free (data);
}


/*
 * shaper shape_plan data
 */

struct hb_icu_le_shaper_shape_plan_data_t {};

hb_icu_le_shaper_shape_plan_data_t *
_hb_icu_le_shaper_shape_plan_data_create (hb_shape_plan_t    *shape_plan,
				       const hb_feature_t *user_features,
				       unsigned int        num_user_features)
{
  return (hb_icu_le_shaper_shape_plan_data_t *) HB_SHAPER_DATA_SUCCEEDED;
}

void
_hb_icu_le_shaper_shape_plan_data_destroy (hb_icu_le_shaper_shape_plan_data_t *data)
{
}


/*
 * shaper
 */

hb_bool_t
_hb_icu_le_shape (hb_shape_plan_t    *shape_plan,
		  hb_font_t          *font,
		  hb_buffer_t        *buffer,
		  const hb_feature_t *features,
		  unsigned int        num_features)
{
  return false;
}
