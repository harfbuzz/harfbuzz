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
 * Author(s): Behdad Esfahbod
 */

#include "hb.hh"

#ifdef HAVE_HARFRUZZ

#include "hb-shaper-impl.hh"


/*
 * shaper face data
 */

extern "C" void *
_hb_harfruzz_shaper_face_data_create_rs (hb_face_t *face);

hb_harfruzz_face_data_t *
_hb_harfruzz_shaper_face_data_create (hb_face_t *face)
{
  return (hb_harfruzz_face_data_t *) _hb_harfruzz_shaper_face_data_create_rs (face);
}

extern "C" void
_hb_harfruzz_shaper_face_data_destroy_rs (void *data);

void
_hb_harfruzz_shaper_face_data_destroy (hb_harfruzz_face_data_t *data)
{
  _hb_harfruzz_shaper_face_data_destroy_rs (data);
}


/*
 * shaper font data
 */

struct hb_harfruzz_font_data_t {};

hb_harfruzz_font_data_t *
_hb_harfruzz_shaper_font_data_create (hb_font_t *font HB_UNUSED)
{
  return (hb_harfruzz_font_data_t *) HB_SHAPER_DATA_SUCCEEDED;
}

void
_hb_harfruzz_shaper_font_data_destroy (hb_harfruzz_font_data_t *data HB_UNUSED)
{
}


/*
 * shaper
 */

extern "C" hb_bool_t
_hb_harfruzz_shape_rs (const void         *face_data,
		       hb_font_t          *font,
		       hb_buffer_t        *buffer,
		       const hb_feature_t *features,
		       unsigned int        num_features);

hb_bool_t
_hb_harfruzz_shape (hb_shape_plan_t    *shape_plan HB_UNUSED,
		    hb_font_t          *font,
		    hb_buffer_t        *buffer,
		    const hb_feature_t *features,
		    unsigned int        num_features)
{
  hb_face_t *face = font->face;
  const hb_harfruzz_face_data_t *data = face->data.harfruzz;

  return _hb_harfruzz_shape_rs (data,
				font,
				buffer,
				features,
				num_features);
}


#endif
