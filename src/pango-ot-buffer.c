/* Pango
 * pango-ot-buffer.c: Buffer of glyphs for shaping/positioning
 *
 * Copyright (C) 2004 Red Hat Software
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "pango-ot-private.h"

#define PANGO_SCALE_26_6 (PANGO_SCALE / (1<<6))
#define PANGO_UNITS_26_6(d) (PANGO_SCALE_26_6 * (d))

PangoOTBuffer *
pango_ot_buffer_new (PangoFcFont *font)
{
  /* We lock the font here immediately for the silly reason
   * of getting the FT_Memory; otherwise we'd have to
   * add a new operation to PangoFcFontmap; callers will
   * probably already have the font locked, however,
   * so there is little performance penalty.
   */
  PangoOTBuffer *buffer = g_new (PangoOTBuffer, 1);
  FT_Face face = pango_fc_font_lock_face (font);

  if (otl_buffer_new (face->memory, &buffer->buffer) != FT_Err_Ok)
    g_error ("Allocation of OTLBuffer failed");

  buffer->font = g_object_ref (font);
  buffer->applied_gpos = FALSE;
  buffer->rtl = FALSE;
  buffer->zero_width_marks = FALSE;

  pango_fc_font_unlock_face (font);

  return buffer;
}

void
pango_ot_buffer_destroy (PangoOTBuffer *buffer)
{
  otl_buffer_free (buffer->buffer);
  g_object_unref (buffer->font);
  g_free (buffer);
}

void
pango_ot_buffer_clear (PangoOTBuffer *buffer)
{
  otl_buffer_clear (buffer->buffer);
  buffer->applied_gpos = FALSE;
}

void
pango_ot_buffer_add_glyph (PangoOTBuffer *buffer,
			   guint          glyph_index,
			   guint          properties,
			   guint          cluster)
{
  otl_buffer_add_glyph (buffer->buffer,
			glyph_index, properties, cluster);

}

void
pango_ot_buffer_set_rtl (PangoOTBuffer *buffer,
			 gboolean       rtl)
{
  rtl = rtl != FALSE;
  buffer->rtl = rtl;
}

/**
 * pango_ot_buffer_set_zero_width_marks:
 * @buffer: a #PangoOTBuffer
 * @zero_width_marks: %TRUE if characters with a mark class should
 *  be forced to zero width.
 * 
 * Sets whether characters with a mark class should be forced to zero width.
 * This setting is needed for proper positioning of Arabic accents,
 * but will produce incorrect results with standard OpenType indic
 * fonts.
 **/
void
pango_ot_buffer_set_zero_width_marks (PangoOTBuffer     *buffer,
				      gboolean           zero_width_marks)
{
  buffer->zero_width_marks = zero_width_marks != FALSE;
}

void
pango_ot_buffer_get_glyphs (PangoOTBuffer  *buffer,
			    PangoOTGlyph  **glyphs,
			    int            *n_glyphs)
{
  if (glyphs)
    *glyphs = (PangoOTGlyph *)buffer->buffer->in_string;

  if (n_glyphs)
    *n_glyphs = buffer->buffer->in_length;
}

static void
swap_range (PangoGlyphString *glyphs, int start, int end)
{
  int i, j;
  
  for (i = start, j = end - 1; i < j; i++, j--)
    {
      PangoGlyphInfo glyph_info;
      gint log_cluster;
      
      glyph_info = glyphs->glyphs[i];
      glyphs->glyphs[i] = glyphs->glyphs[j];
      glyphs->glyphs[j] = glyph_info;
      
      log_cluster = glyphs->log_clusters[i];
      glyphs->log_clusters[i] = glyphs->log_clusters[j];
      glyphs->log_clusters[j] = log_cluster;
    }
}

static void
apply_gpos_ltr (PangoGlyphString *glyphs,
		OTL_Position      positions)
{
  int i;
	    
  for (i = 0; i < glyphs->num_glyphs; i++)
    {
      FT_Pos x_pos = positions[i].x_pos;
      FT_Pos y_pos = positions[i].y_pos;
      int back = i;
      int j;
      
      while (positions[back].back != 0)
	{
	  back  -= positions[back].back;
	  x_pos += positions[back].x_pos;
	  y_pos += positions[back].y_pos;
	}
      
      for (j = back; j < i; j++)
	glyphs->glyphs[i].geometry.x_offset -= glyphs->glyphs[j].geometry.width;
      
      glyphs->glyphs[i].geometry.x_offset += PANGO_UNITS_26_6(x_pos);
      glyphs->glyphs[i].geometry.y_offset -= PANGO_UNITS_26_6(y_pos);
      
      if (positions[i].new_advance)
	glyphs->glyphs[i].geometry.width  = PANGO_UNITS_26_6(positions[i].x_advance);
      else
	glyphs->glyphs[i].geometry.width += PANGO_UNITS_26_6(positions[i].x_advance);
    }
}

static void
apply_gpos_rtl (PangoGlyphString *glyphs,
		OTL_Position      positions)
{
  int i;
  
  for (i = 0; i < glyphs->num_glyphs; i++)
    {
      int i_rev = glyphs->num_glyphs - i - 1;
      int back_rev = i_rev;
      int back;
      FT_Pos x_pos = positions[i_rev].x_pos;
      FT_Pos y_pos = positions[i_rev].y_pos;
      int j;

      while (positions[back_rev].back != 0)
	{
	  back_rev -= positions[back_rev].back;
	  x_pos += positions[back_rev].x_pos;
	  y_pos += positions[back_rev].y_pos;
	}

      back = glyphs->num_glyphs - back_rev - 1;
      
      for (j = i; j < back; j++)
	glyphs->glyphs[i].geometry.x_offset += glyphs->glyphs[j].geometry.width;
      
      glyphs->glyphs[i].geometry.x_offset += PANGO_UNITS_26_6(x_pos);
      glyphs->glyphs[i].geometry.y_offset -= PANGO_UNITS_26_6(y_pos);
      
      if (positions[i_rev].new_advance)
	glyphs->glyphs[i].geometry.width  = PANGO_UNITS_26_6(positions[i_rev].x_advance);
      else
	glyphs->glyphs[i].geometry.width += PANGO_UNITS_26_6(positions[i_rev].x_advance);
    }
}

void
pango_ot_buffer_output (PangoOTBuffer    *buffer,
			PangoGlyphString *glyphs)
{
  FT_Face face;
  PangoOTInfo *info;
  TTO_GDEF gdef = NULL;
  int i;
  int last_cluster;

  face = pango_fc_font_lock_face (buffer->font);
  g_assert (face);
  
  /* Copy glyphs into output glyph string */
  pango_glyph_string_set_size (glyphs, buffer->buffer->in_length);

  last_cluster = -1;
  for (i = 0; i < buffer->buffer->in_length; i++)
    {
      OTL_GlyphItem item = &buffer->buffer->in_string[i];
      
      glyphs->glyphs[i].glyph = item->gindex;

      glyphs->log_clusters[i] = item->cluster;
      if (glyphs->log_clusters[i] != last_cluster)
	glyphs->glyphs[i].attr.is_cluster_start = 1;
      else
	glyphs->glyphs[i].attr.is_cluster_start = 0;

      last_cluster = glyphs->log_clusters[i];
    }

  info = pango_ot_info_get (face);
  gdef = pango_ot_info_get_gdef (info);
  
  /* Apply default positioning */
  for (i = 0; i < glyphs->num_glyphs; i++)
    {
      if (glyphs->glyphs[i].glyph)
	{
	  PangoRectangle logical_rect;
	  
	  FT_UShort property;

	  if (buffer->zero_width_marks &&
	      gdef &&
	      TT_GDEF_Get_Glyph_Property (gdef, glyphs->glyphs[i].glyph, &property) == FT_Err_Ok &&
	      (property == TTO_MARK || (property & IGNORE_SPECIAL_MARKS) != 0))
	    {
	      glyphs->glyphs[i].geometry.width = 0;
	    }
	  else
	    {
	      pango_font_get_glyph_extents ((PangoFont *)buffer->font, glyphs->glyphs[i].glyph, NULL, &logical_rect);
	      glyphs->glyphs[i].geometry.width = logical_rect.width;
	    }
	}
      else
	glyphs->glyphs[i].geometry.width = 0;
  
      glyphs->glyphs[i].geometry.x_offset = 0;
      glyphs->glyphs[i].geometry.y_offset = 0;
    }

  if (buffer->rtl)
    {
      /* Swap all glyphs */
      swap_range (glyphs, 0, glyphs->num_glyphs);
    }

  if (buffer->applied_gpos)
    {
      if (buffer->rtl)
	apply_gpos_rtl (glyphs, buffer->buffer->positions);
      else
	apply_gpos_ltr (glyphs, buffer->buffer->positions);
    }

  pango_fc_font_unlock_face (buffer->font);
}
