/*
 * Copyright © 2022  Behdad Esfahbod
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

#ifndef HB_FT_COLR_HH
#define HB_FT_COLR_HH

#include "hb.hh"


#ifndef HB_NO_PAINT
static bool
hb_ft_paint_glyph_colr (hb_font_t *font,
			void *font_data,
			hb_codepoint_t gid,
			hb_paint_funcs_t *paint_funcs, void *paint_data,
			unsigned int palette_index,
			hb_color_t foreground,
			void *user_data)
{
  const hb_ft_font_t *ft_font = (const hb_ft_font_t *) font_data;
  FT_Face ft_face = ft_font->ft_face;

  /* Face is locked. */

  /* COLRv0 */
  FT_Error error;
  FT_Color*         palette;
  FT_LayerIterator  iterator;

  FT_Bool  have_layers;
  FT_UInt  layer_glyph_index;
  FT_UInt  layer_color_index;

  error = FT_Palette_Select(ft_face, palette_index, &palette);
  if ( error )
    palette = NULL;

  iterator.p  = NULL;
  have_layers = FT_Get_Color_Glyph_Layer(ft_face,
					 gid,
					 &layer_glyph_index,
					 &layer_color_index,
					 &iterator);

  if (palette && have_layers)
  {
    do
    {
      hb_bool_t is_foreground = true;
      hb_color_t color = foreground;

      if ( layer_color_index != 0xFFFF )
      {
	FT_Color layer_color = palette[layer_color_index];
	color = HB_COLOR (layer_color.blue,
			  layer_color.green,
			  layer_color.red,
			  layer_color.alpha);
	is_foreground = false;
      }

      ft_font->lock.unlock ();
      paint_funcs->push_clip_glyph (paint_data, layer_glyph_index, font);
      paint_funcs->color (paint_data, is_foreground, color);
      paint_funcs->pop_clip (paint_data);
      ft_font->lock.lock ();

    } while (FT_Get_Color_Glyph_Layer(ft_face,
				      gid,
				      &layer_glyph_index,
				      &layer_color_index,
				      &iterator));
    return true;
  }

  return false;
}
#endif


#endif /* HB_FT_COLR_HH */
