/*
 * Copyright (C) 2009  Red Hat, Inc.
 * Copyright (C) 2009  Keith Stribley <devel@thanlwinsoft.org>
 *
 *  This is part of HarfBuzz, an OpenType Layout engine library.
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
 * Red Hat Author(s): Behdad Esfahbod
 */

#include "hb-private.h"

#include "hb-ft.h"

#include "hb-font-private.h"

#include FT_TRUETYPE_TABLES_H

#if 0
extern hb_codepoint_t hb_GetGlyph(hb_font_t *font, hb_face_t *face, const void *user_data,
                            hb_codepoint_t unicode, hb_codepoint_t variant_selector)
{
    FT_Face fcFace = (FT_Face)user_data;
    return FT_Get_Char_Index(fcFace, unicode);
}

extern hb_bool_t hb_GetContourPoint(hb_font_t *font, hb_face_t *face, const void *user_data,
                               hb_codepoint_t glyph, hb_position_t *x, hb_position_t *y)
{
    unsigned int point = 0; /* TODO this should be in API */
    int load_flags = FT_LOAD_DEFAULT;

    FT_Face fcFace = (FT_Face)user_data;
    if (FT_Load_Glyph(fcFace, glyph, load_flags))
        return 0;

    if (fcFace->glyph->format != ft_glyph_format_outline)
        return 0;

    unsigned int nPoints = fcFace->glyph->outline.n_points;
    if (!(nPoints))
        return 0;

    if (point > nPoints)
        return 0;

    *x = fcFace->glyph->outline.points[point].x;
    *y = fcFace->glyph->outline.points[point].y;

    return 1;
}

extern void hb_GetGlyphMetrics(hb_font_t *font, hb_face_t *face, const void *user_data,
                          hb_codepoint_t glyph, hb_glyph_metrics_t *metrics)
{
    int load_flags = FT_LOAD_DEFAULT;
    FT_Face fcFace = (FT_Face)user_data;

    metrics->x = metrics->y = 0;
	metrics->x_offset = metrics->y_offset = 0;
    if (FT_Load_Glyph(fcFace, glyph, load_flags))
    {
        metrics->width = metrics->height = 0;
    }
    else
    {
        metrics->width = fcFace->glyph->metrics.width;
        metrics->height = fcFace->glyph->metrics.height;
		metrics->x_offset = fcFace->glyph->advance.x;
		metrics->y_offset = fcFace->glyph->advance.y;
    }
}

extern hb_position_t hb_GetKerning(hb_font_t *font, hb_face_t *face, const void *user_data,
                             hb_codepoint_t first_glyph, hb_codepoint_t second_glyph)
{
    FT_Face fcFace = (FT_Face)user_data;
    FT_Vector aKerning;
    if (FT_Get_Kerning(fcFace, first_glyph, second_glyph, FT_KERNING_DEFAULT,
                       &aKerning))
    {
        return 0;
    }
    return aKerning.x;
}
#endif

static hb_font_funcs_t ft_ffuncs = {
  HB_REFERENCE_COUNT_INVALID, /* ref_count */

  TRUE, /* immutable */

#if 0
  hb_ft_get_glyph,
  hb_ft_get_contour_point,
  hb_ft_get_glyph_metrics,
  hb_ft_get_kerning
#endif
};

hb_font_funcs_t *
hb_ft_get_font_funcs (void)
{
  return &ft_ffuncs;
}


static hb_blob_t *
_get_table  (hb_tag_t tag, void *user_data)
{
  FT_Face ft_face = (FT_Face) user_data;
  FT_Byte *buffer;
  FT_ULong  length = 0;
  FT_Error error;

  error = FT_Load_Sfnt_Table (ft_face, tag, 0, NULL, &length);
  if (error)
    return hb_blob_create_empty ();

  buffer = malloc (length);
  if (buffer == NULL)
    return hb_blob_create_empty ();

  error = FT_Load_Sfnt_Table (ft_face, tag, 0, buffer, &length);
  if (error)
    return hb_blob_create_empty ();

  return hb_blob_create ((const char *) buffer, length,
			 HB_MEMORY_MODE_WRITABLE,
			 free, buffer);
}


hb_face_t *
hb_ft_face_create (FT_Face           ft_face,
		   hb_destroy_func_t destroy)
{
  hb_face_t *face;

  if (ft_face->stream->base != NULL) {
    hb_blob_t *blob;

    blob = hb_blob_create ((const char *) ft_face->stream->base,
			   (unsigned int) ft_face->stream->size,
			   HB_MEMORY_MODE_READONLY_MAY_MAKE_WRITABLE,
			   destroy, ft_face);
    face = hb_face_create_for_data (blob, ft_face->face_index);
    hb_blob_destroy (blob);
  } else {
    face = hb_face_create_for_tables (_get_table, destroy, ft_face);
  }

  return face;
}


hb_font_t *
hb_ft_font_create (FT_Face           ft_face,
		   hb_destroy_func_t destroy)
{
  hb_font_t *font;

  font = hb_font_create ();
  hb_font_set_funcs (font,
		     hb_ft_get_font_funcs (),
		     destroy, ft_face);
  hb_font_set_scale (font,
		     ft_face->size->metrics.x_scale,
		     ft_face->size->metrics.y_scale);
  hb_font_set_ppem (font,
		    ft_face->size->metrics.x_ppem,
		    ft_face->size->metrics.y_ppem);

  return font;
}
