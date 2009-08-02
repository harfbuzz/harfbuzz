/*
 * Copyright (C) 2009  Red Hat, Inc.
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

#include "hb-font-private.h"

#include "hb-blob.h"


/*
 * hb_font_callbacks_t
 */

static hb_font_callbacks_t _hb_font_callbacks_nil = {
  HB_REFERENCE_COUNT_INVALID /* ref_count */
  /*
  hb_font_get_glyph_func_t glyph_func;
  hb_font_get_contour_point_func_t contour_point_func;
  hb_font_get_glyph_metrics_func_t glyph_metrics_func;
  hb_font_get_kerning_func_t kerning_func;
  */
};

hb_font_callbacks_t *
hb_font_callbacks_create (void)
{
  hb_font_callbacks_t *fcallbacks;

  if (!HB_OBJECT_DO_CREATE (fcallbacks))
    return &_hb_font_callbacks_nil;

  return fcallbacks;
}

hb_font_callbacks_t *
hb_font_callbacks_reference (hb_font_callbacks_t *fcallbacks)
{
  HB_OBJECT_DO_REFERENCE (fcallbacks);
}

unsigned int
hb_font_callbacks_get_reference_count (hb_font_callbacks_t *fcallbacks)
{
  HB_OBJECT_DO_GET_REFERENCE_COUNT (fcallbacks);
}

void
hb_font_callbacks_destroy (hb_font_callbacks_t *fcallbacks)
{
  HB_OBJECT_DO_DESTROY (fcallbacks);

  free (fcallbacks);
}

hb_font_callbacks_t *
hb_font_callbacks_duplicate (hb_font_callbacks_t *other_fcallbacks)
{
  hb_font_callbacks_t *fcallbacks;

  if (!HB_OBJECT_DO_CREATE (fcallbacks))
    return &_hb_font_callbacks_nil;

  *fcallbacks = *other_fcallbacks;

  /* re-init refcount */
  HB_OBJECT_DO_INIT (fcallbacks);

  return fcallbacks;
}



/*
 * hb_unicode_callbacks_t
 */

static hb_unicode_callbacks_t _hb_unicode_callbacks_nil = {
  HB_REFERENCE_COUNT_INVALID /* ref_count */
  /*
  hb_unicode_get_general_category_func_t general_category_func);
  hb_unicode_get_combining_class_func_t combining_class_func);
  hb_unicode_get_mirroring_char_func_t mirroring_char_func);
  hb_unicode_get_script_func_t script_func);
  hb_unicode_get_eastasian_width_func_t eastasian_width_func);
  */
};

hb_unicode_callbacks_t *
hb_unicode_callbacks_create (void)
{
  hb_unicode_callbacks_t *ucallbacks;

  if (!HB_OBJECT_DO_CREATE (ucallbacks))
    return &_hb_unicode_callbacks_nil;

  return ucallbacks;
}

hb_unicode_callbacks_t *
hb_unicode_callbacks_reference (hb_unicode_callbacks_t *ucallbacks)
{
  HB_OBJECT_DO_REFERENCE (ucallbacks);
}

unsigned int
hb_unicode_callbacks_get_reference_count (hb_unicode_callbacks_t *ucallbacks)
{
  HB_OBJECT_DO_GET_REFERENCE_COUNT (ucallbacks);
}

void
hb_unicode_callbacks_destroy (hb_unicode_callbacks_t *ucallbacks)
{
  HB_OBJECT_DO_DESTROY (ucallbacks);

  free (ucallbacks);
}

hb_unicode_callbacks_t *
hb_unicode_callbacks_duplicate (hb_unicode_callbacks_t *other_ucallbacks)
{
  hb_unicode_callbacks_t *ucallbacks;

  if (!HB_OBJECT_DO_CREATE (ucallbacks))
    return &_hb_unicode_callbacks_nil;

  *ucallbacks = *other_ucallbacks;
  HB_OBJECT_DO_INIT (ucallbacks);

  return ucallbacks;
}



/*
 * hb_face_t
 */

static hb_face_t _hb_face_nil = {
  HB_REFERENCE_COUNT_INVALID, /* ref_count */

  NULL, /* blob */
  0, /* index */

  NULL, /* get_table */
  NULL, /* destroy */
  NULL, /* user_data */

  NULL, /* fcallbacks */
  NULL  /* ucallbacks */
};

hb_face_t *
hb_face_create_for_data (hb_blob_t    *blob,
			 unsigned int  index)
{
  hb_face_t *face;

  if (!HB_OBJECT_DO_CREATE (face))
    return &_hb_face_nil;

  face->blob = hb_blob_reference (blob);
  face->index = index;

  return face;
}

hb_face_t *
hb_face_create_for_tables (hb_get_table_func_t  get_table,
			   hb_destroy_func_t    destroy,
			   void                *user_data)
{
  hb_face_t *face;

  if (!HB_OBJECT_DO_CREATE (face)) {
    if (destroy)
      destroy (user_data);
    return &_hb_face_nil;
  }

  face->get_table = get_table;
  face->destroy = destroy;
  face->user_data = user_data;

  return face;
}

hb_face_t *
hb_face_reference (hb_face_t *face)
{
  HB_OBJECT_DO_REFERENCE (face);
}

unsigned int
hb_face_get_reference_count (hb_face_t *face)
{
  HB_OBJECT_DO_GET_REFERENCE_COUNT (face);
}

void
hb_face_destroy (hb_face_t *face)
{
  HB_OBJECT_DO_DESTROY (face);

  hb_blob_destroy (face->blob);

  if (face->destroy)
    face->destroy (face->user_data);

  hb_font_callbacks_destroy (face->fcallbacks);
  hb_unicode_callbacks_destroy (face->ucallbacks);

  free (face);
}

void
hb_face_set_font_callbacks (hb_face_t *face,
			    hb_font_callbacks_t *fcallbacks)
{
  if (HB_OBJECT_IS_INERT (face))
    return;

  hb_font_callbacks_reference (fcallbacks);
  hb_font_callbacks_destroy (face->fcallbacks);
  face->fcallbacks = fcallbacks;
}

void
hb_face_set_unicode_callbacks (hb_face_t *face,
			       hb_unicode_callbacks_t *ucallbacks)
{
  if (HB_OBJECT_IS_INERT (face))
    return;

  hb_unicode_callbacks_reference (ucallbacks);
  hb_unicode_callbacks_destroy (face->ucallbacks);
  face->ucallbacks = ucallbacks;
}



/*
 * hb_font_t
 */

static hb_font_t _hb_font_nil = {
  HB_REFERENCE_COUNT_INVALID, /* ref_count */

  NULL, /* face */

  0, /* x_scale */
  0, /* y_scale */

  0, /* x_ppem */
  0  /* y_ppem */
};

hb_font_t *
hb_font_create (hb_face_t *face)
{
  hb_font_t *font;

  if (!HB_OBJECT_DO_CREATE (font))
    return &_hb_font_nil;

  font->face = hb_face_reference (face);

  return font;
}

hb_font_t *
hb_font_reference (hb_font_t *font)
{
  HB_OBJECT_DO_REFERENCE (font);
}

unsigned int
hb_font_get_reference_count (hb_font_t *font)
{
  HB_OBJECT_DO_GET_REFERENCE_COUNT (font);
}

void
hb_font_destroy (hb_font_t *font)
{
  HB_OBJECT_DO_DESTROY (font);

  hb_face_destroy (font->face);

  free (font);
}

void
hb_font_set_scale (hb_font_t *font,
		   hb_16dot16_t x_scale,
		   hb_16dot16_t y_scale)
{
  if (HB_OBJECT_IS_INERT (font))
    return;

  font->x_scale = x_scale;
  font->y_scale = y_scale;
}

void
hb_font_set_ppem (hb_font_t *font,
		  unsigned int x_ppem,
		  unsigned int y_ppem)
{
  if (HB_OBJECT_IS_INERT (font))
    return;

  font->x_ppem = x_ppem;
  font->y_ppem = y_ppem;
}

