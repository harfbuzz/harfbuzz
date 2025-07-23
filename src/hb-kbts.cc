/*
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
 * Author(s): Khaled Hosny
 */

#include "hb.hh"

#if HAVE_KBTS

#include "hb-shaper-impl.hh"

#define KB_TEXT_SHAPE_IMPLEMENTATION
#define KB_TEXT_SHAPE_STATIC

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wunused-function"
#include "kb_text_shape.h"
#pragma GCC diagnostic pop


hb_kbts_face_data_t *
_hb_kbts_shaper_face_data_create (hb_face_t *face)
{
  return nullptr;
}

void
_hb_kbts_shaper_face_data_destroy (hb_kbts_face_data_t *data)
{
}

hb_kbts_font_data_t *
_hb_kbts_shaper_font_data_create (hb_font_t *font)
{
  return nullptr;
}

void
_hb_kbts_shaper_font_data_destroy (hb_kbts_font_data_t *data)
{
}

hb_bool_t
_hb_kbts_shape (hb_shape_plan_t    *shape_plan,
		hb_font_t          *font,
		hb_buffer_t        *buffer,
		const hb_feature_t *features,
		unsigned int        num_features)
{
  return false;
}

#endif