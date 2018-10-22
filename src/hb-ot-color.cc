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
 * Google Author(s): Sascha Brawer, Behdad Esfahbod
 */

#include "hb-open-type.hh"
#include "hb-ot-color-cbdt-table.hh"
#include "hb-ot-color-colr-table.hh"
#include "hb-ot-color-cpal-table.hh"
#include "hb-ot-color-sbix-table.hh"
#include "hb-ot-color-svg-table.hh"
#include "hb-ot-face.hh"
#include "hb-ot.h"

#include <stdlib.h>
#include <string.h>

#include "hb-ot-layout.hh"


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

#if 0
static inline const OT::CBDT_accelerator_t&
_get_cbdt (hb_face_t *face)
{
  if (unlikely (!hb_ot_shaper_face_data_ensure (face))) return Null(OT::CBDT_accelerator_t);
  return *(hb_ot_face_data (face)->CBDT.get ());
}

static inline const OT::sbix&
_get_sbix (hb_face_t *face)
{
  if (unlikely (!hb_ot_shaper_face_data_ensure (face))) return Null(OT::sbix);
  return *(hb_ot_face_data (face)->sbix.get ());
}

static inline const OT::SVG&
_get_svg (hb_face_t *face)
{
  if (unlikely (!hb_ot_shaper_face_data_ensure (face))) return Null(OT::SVG);
  return *(hb_ot_face_data (face)->SVG.get ());
}
#endif


/*
 * CPAL
 */


/**
 * hb_ot_color_has_palettes:
 * @face: a font face.
 *
 * Returns: whether CPAL table is available.
 *
 * Since: REPLACEME
 */
hb_bool_t
hb_ot_color_has_palettes (hb_face_t *face)
{
  return _get_cpal (face).has_data ();
}

/**
 * hb_ot_color_palette_get_count:
 * @face: a font face.
 *
 * Returns: the number of color palettes in @face, or zero if @face has
 * no colors.
 *
 * Since: REPLACEME
 */
unsigned int
hb_ot_color_palette_get_count (hb_face_t *face)
{
  return _get_cpal (face).get_palette_count ();
}

/**
 * hb_ot_color_palette_get_name_id:
 * @face:    a font face.
 * @palette: the index of the color palette whose name is being requested.
 *
 * Retrieves the name id of a color palette. For example, a color font can
 * have themed palettes like "Spring", "Summer", "Fall", and "Winter".
 *
 * Returns: an identifier within @face's `name` table.
 * If the requested palette has no name the result is #HB_NAME_ID_INVALID.
 *
 * Since: REPLACEME
 */
hb_name_id_t
hb_ot_color_palette_get_name_id (hb_face_t *face,
				 unsigned int palette_index)
{
  return _get_cpal (face).get_palette_name_id (palette_index);
}

/**
 * hb_ot_color_palette_color_get_name_id:
 * @face: a font face.
 * @color_index:
 *
 * Returns: Name ID associated with a palette entry, e.g. eye color
 *
 * Since: REPLACEME
 */
hb_name_id_t
hb_ot_color_palette_color_get_name_id (hb_face_t *face,
				       unsigned int color_index)
{
  return _get_cpal (face).get_color_name_id (color_index);
}

/**
 * hb_ot_color_palette_get_flags:
 * @face:    a font face
 * @palette_index: the index of the color palette whose flags are being requested
 *
 * Returns: the flags for the requested color palette.
 *
 * Since: REPLACEME
 */
hb_ot_color_palette_flags_t
hb_ot_color_palette_get_flags (hb_face_t *face,
			       unsigned int palette_index)
{
  const OT::CPAL& cpal = _get_cpal(face);
  return cpal.get_palette_flags (palette_index);
}

/**
 * hb_ot_color_palette_get_colors:
 * @face:         a font face.
 * @palette_index:the index of the color palette whose colors
 *                are being requested.
 * @start_offset: the index of the first color being requested.
 * @color_count:  (inout) (optional): on input, how many colors
 *                can be maximally stored into the @colors array;
 *                on output, how many colors were actually stored.
 * @colors: (array length=color_count) (out) (optional):
 *                an array of #hb_color_t records. After calling
 *                this function, @colors will be filled with
 *                the palette colors. If @colors is NULL, the function
 *                will just return the number of total colors
 *                without storing any actual colors; this can be used
 *                for allocating a buffer of suitable size before calling
 *                hb_ot_color_palette_get_colors() a second time.
 *
 * Retrieves the colors in a color palette.
 *
 * Returns: the total number of colors in the palette.
 *
 * Since: REPLACEME
 */
unsigned int
hb_ot_color_palette_get_colors (hb_face_t      *face,
				unsigned int    palette_index,      /* default=0 */
				unsigned int    start_offset,
				unsigned int   *colors_count  /* IN/OUT.  May be NULL. */,
				hb_color_t     *colors        /* OUT.     May be NULL. */)
{
  const OT::CPAL& cpal = _get_cpal(face);
  if (unlikely (palette_index >= cpal.get_palette_count ()))
  {
    if (colors_count) *colors_count = 0;
    return 0;
  }

  unsigned int num_results = 0;
  if (colors_count)
  {
    unsigned int platte_count;
    platte_count = MIN<unsigned int>(*colors_count,
				     cpal.get_color_count () - start_offset);
    for (unsigned int i = 0; i < platte_count; i++)
    {
      if (cpal.get_color_record_argb(start_offset + i, palette_index, &colors[num_results]))
	++num_results;
    }
  }

  if (likely (colors_count)) *colors_count = num_results;
  return cpal.get_color_count ();
}


/*
 * COLR
 */

/**
 * hb_ot_color_has_layers:
 * @face: a font face.
 *
 * Returns: whether COLR table is available.
 *
 * Since: REPLACEME
 */
hb_bool_t
hb_ot_color_has_layers (hb_face_t *face)
{
  return _get_colr (face).has_data ();
}

/**
 * hb_ot_color_glyph_get_layers:
 * @face: a font face.
 * @glyph:
 * @start_offset:
 * @count:  (inout) (optional):
 * @layers: (array length=count) (out) (optional):
 *
 * Returns:
 *
 * Since: REPLACEME
 */
unsigned int
hb_ot_color_glyph_get_layers (hb_face_t           *face,
			      hb_codepoint_t       glyph,
			      unsigned int         start_offset,
			      unsigned int        *count, /* IN/OUT.  May be NULL. */
			      hb_ot_color_layer_t *layers /* OUT.     May be NULL. */)
{
  const OT::COLR& colr = _get_colr (face);
  return colr.get_glyph_layers (glyph, start_offset, count, layers);
}
