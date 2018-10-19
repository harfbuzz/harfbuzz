/*
 * Copyright © 2016  Google, Inc.
 * Copyright © 2018  Ebrahim Byagowi
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
 * Google Author(s): Sascha Brawer
 */

#include "hb.hh"
#include "hb-open-type.hh"
#include "hb-ot-color-colr-table.hh"
#include "hb-ot-color-cpal-table.hh"
#include "hb-ot-face.hh"
#include "hb-ot.h"

#include <stdlib.h>
#include <string.h>

#include "hb-ot-layout.hh"
#include "hb-shaper.hh"

#if 0
HB_MARK_AS_FLAG_T (hb_ot_color_palette_flags_t)
//HB_SHAPER_DATA_ENSURE_DECLARE(ot, face) Hmm?
#endif


static inline const OT::COLR&
_get_colr (hb_face_t *face)
{
  if (unlikely (!hb_ot_shaper_face_data_ensure (face))) return Null(OT::COLR);
  return *(hb_ot_face_data (face)->COLR.get ());
}

static inline const OT::CPAL&
_get_cpal (hb_face_t *face)
{
  if (unlikely (!hb_ot_shaper_face_data_ensure (face))) return Null(OT::CPAL);
  return *(hb_ot_face_data (face)->CPAL.get ());
}

HB_EXTERN hb_bool_t
hb_ot_color_has_cpal_data (hb_face_t *face)
{
  return &_get_cpal (face) != &Null(OT::CPAL);
}

HB_EXTERN hb_bool_t
hb_ot_color_has_colr_data (hb_face_t *face)
{
  return &_get_colr (face) != &Null(OT::COLR);
}

/**
 * hb_ot_color_get_palette_count:
 * @face: a font face.
 *
 * Returns: the number of color palettes in @face, or zero if @face has
 * no colors.
 *
 * Since: REPLACEME
 */
unsigned int
hb_ot_color_get_palette_count (hb_face_t *face)
{
  const OT::CPAL& cpal = _get_cpal (face);
  return cpal.get_palette_count ();
}

#if 0
/**
 * hb_ot_color_get_palette_name_id:
 * @face: a font face.
 * @palette: the index of the color palette whose name is being requested.
 *
 * Retrieves the name id of a color palette. For example, a color font can
 * have themed palettes like "Spring", "Summer", "Fall", and "Winter".
 *
 * Returns: an identifier within @face's `name` table.
 * If the requested palette has no name, or if @face has no colors,
 * or if @palette is not between 0 and hb_ot_color_get_palette_count(),
 * the result is 0xFFFF. The implementation does not check whether
 * the returned palette name id is actually in @face's `name` table.
 *
 * Since: REPLACEME
 */
unsigned int
hb_ot_color_get_palette_name_id (hb_face_t *face, unsigned int palette)
{
  const OT::CPAL& cpal = _get_cpal (face);
  return cpal.get_palette_name_id (palette);
}


/**
 * hb_ot_color_get_palette_flags:
 * @face: a font face
 * @palette: the index of the color palette whose flags are being requested
 *
 * Returns: the flags for the requested color palette.  If @face has no colors,
 * or if @palette is not between 0 and hb_ot_color_get_palette_count(),
 * the result is #HB_OT_COLOR_PALETTE_FLAG_DEFAULT.
 *
 * Since: REPLACEME
 */
hb_ot_color_palette_flags_t
hb_ot_color_get_palette_flags (hb_face_t *face, unsigned int palette)
{
  const OT::CPAL& cpal = _get_cpal(face);
  return cpal.get_palette_flags (palette);
}
#endif


/**
 * hb_ot_color_get_palette_colors:
 * @face:         a font face.
 * @palette:      the index of the color palette whose colors
 *                are being requested.
 * @start_offset: the index of the first color being requested.
 * @color_count:  (inout) (optional): on input, how many colors
 *                can be maximally stored into the @colors array;
 *                on output, how many colors were actually stored.
 * @colors: (array length=color_count) (optional):
 *                an array of #hb_ot_color_t records. After calling
 *                this function, @colors will be filled with
 *                the palette colors. If @colors is NULL, the function
 *                will just return the number of total colors
 *                without storing any actual colors; this can be used
 *                for allocating a buffer of suitable size before calling
 *                hb_ot_color_get_palette_colors() a second time.
 *
 * Retrieves the colors in a color palette.
 *
 * Returns: the total number of colors in the palette. All palettes in
 * a font have the same number of colors. If @face has no colors, or if
 * @palette is not between 0 and hb_ot_color_get_palette_count(),
 * the result is zero.
 *
 * Since: REPLACEME
 */
unsigned int
hb_ot_color_get_palette_colors (hb_face_t      *face,
				unsigned int    palette, /* default=0 */
				unsigned int    start_offset,
				unsigned int   *count /* IN/OUT */,
				hb_ot_color_t  *colors /* OUT */)
{
  const OT::CPAL& cpal = _get_cpal(face);
  if (unlikely (palette >= cpal.get_palette_count ()))
  {
    if (count) *count = 0;
    return 0;
  }

  unsigned int num_results = 0;
  if (count)
  {
    unsigned int platte_count = MIN<unsigned int>(*count, cpal.get_palette_entries_count () - start_offset);
    for (unsigned int i = 0; i < platte_count; i++)
    {
      if (cpal.get_color_record_argb(start_offset + i, palette, &colors[num_results]))
        ++num_results;
    }
  }

  if (likely (count)) *count = num_results;
  return cpal.get_palette_entries_count ();
}

unsigned int
hb_ot_color_get_color_layers (hb_face_t        *face,
			      hb_codepoint_t    gid,
			      unsigned int      start_offset,
			      unsigned int     *count,        /* IN/OUT.  May be NULL. */
			      hb_codepoint_t   *gids,         /* OUT.     May be NULL. */
			      unsigned int     *color_indices /* OUT.     May be NULL. */)
{
  const OT::COLR& colr = _get_colr (face);
  unsigned int num_results = 0;
  unsigned int start_layer_index, num_layers = 0;
  if (colr.get_base_glyph_record (gid, &start_layer_index, &num_layers))
  {
    if (count)
    {
      unsigned int layer_count = MIN<unsigned int>(*count, num_layers - start_offset);
      for (unsigned int i = 0; i < layer_count; i++)
      {
	if (colr.get_layer_record (start_layer_index + start_offset + i,
				   &gids[num_results], &color_indices[num_results]))
	  ++num_results;
      }
    }
  }

  if (likely (count)) *count = num_results;
  return num_layers;
}
