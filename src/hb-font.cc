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
#include "hb-unicode-private.h"
#include "hb-open-file-private.hh"
#include "hb-blob.h"

#include "hb-ot-layout-private.h"

/*
 * hb_font_funcs_t
 */

hb_font_funcs_t _hb_font_funcs_nil = {
  HB_REFERENCE_COUNT_INVALID, /* ref_count */

  TRUE,  /* immutable */

  NULL, /* glyph_func */
  NULL, /* contour_point_func */
  NULL, /* glyph_metrics_func */
  NULL  /* kerning_func */
};

hb_font_funcs_t *
hb_font_funcs_create (void)
{
  hb_font_funcs_t *ffuncs;

  if (!HB_OBJECT_DO_CREATE (hb_font_funcs_t, ffuncs))
    return &_hb_font_funcs_nil;

  return ffuncs;
}

hb_font_funcs_t *
hb_font_funcs_reference (hb_font_funcs_t *ffuncs)
{
  HB_OBJECT_DO_REFERENCE (ffuncs);
}

unsigned int
hb_font_funcs_get_reference_count (hb_font_funcs_t *ffuncs)
{
  HB_OBJECT_DO_GET_REFERENCE_COUNT (ffuncs);
}

void
hb_font_funcs_destroy (hb_font_funcs_t *ffuncs)
{
  HB_OBJECT_DO_DESTROY (ffuncs);

  free (ffuncs);
}

hb_font_funcs_t *
hb_font_funcs_copy (hb_font_funcs_t *other_ffuncs)
{
  hb_font_funcs_t *ffuncs;

  if (!HB_OBJECT_DO_CREATE (hb_font_funcs_t, ffuncs))
    return &_hb_font_funcs_nil;

  *ffuncs = *other_ffuncs;

  /* re-init refcount */
  HB_OBJECT_DO_INIT (ffuncs);
  ffuncs->immutable = FALSE;

  return ffuncs;
}

void
hb_font_funcs_make_immutable (hb_font_funcs_t *ffuncs)
{
  if (HB_OBJECT_IS_INERT (ffuncs))
    return;

  ffuncs->immutable = TRUE;
}



/*
 * hb_face_t
 */

static hb_blob_t *
_hb_face_get_table_from_blob (hb_tag_t tag, void *user_data)
{
  hb_face_t *face = (hb_face_t *) user_data;

  const OpenTypeFontFile &ot_file = Sanitizer<OpenTypeFontFile>::lock_instance (face->blob);
  const OpenTypeFontFace &ot_face = ot_file.get_face (face->index);

  const OpenTypeTable &table = ot_face.get_table_by_tag (tag);

  hb_blob_t *blob = hb_blob_create_sub_blob (face->blob, table.offset, table.length);

  hb_blob_unlock (face->blob);

  return blob;
}

static hb_face_t _hb_face_nil = {
  HB_REFERENCE_COUNT_INVALID, /* ref_count */

  NULL, /* blob */
  0, /* index */

  NULL, /* get_table */
  NULL, /* destroy */
  NULL, /* user_data */

  &_hb_unicode_funcs_nil,  /* unicode */

  {} /* ot_layout */
};

hb_face_t *
hb_face_create_for_tables (hb_get_table_func_t  get_table,
			   hb_destroy_func_t    destroy,
			   void                *user_data)
{
  hb_face_t *face;

  if (!HB_OBJECT_DO_CREATE (hb_face_t, face)) {
    if (destroy)
      destroy (user_data);
    return &_hb_face_nil;
  }

  face->get_table = get_table;
  face->destroy = destroy;
  face->user_data = user_data;

  _hb_ot_layout_init (face);

  return face;
}

hb_face_t *
hb_face_create_for_data (hb_blob_t    *blob,
			 unsigned int  index)
{
  hb_face_t *face;

  if (!HB_OBJECT_DO_CREATE (hb_face_t, face))
    return &_hb_face_nil;

  face->blob = Sanitizer<OpenTypeFontFile>::sanitize (hb_blob_reference (blob));
  face->index = index;
  face->get_table = _hb_face_get_table_from_blob;
  face->user_data = face;

  _hb_ot_layout_init (face);

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

  _hb_ot_layout_fini (face);

  hb_blob_destroy (face->blob);

  if (face->destroy)
    face->destroy (face->user_data);

  hb_unicode_funcs_destroy (face->unicode);

  free (face);
}

void
hb_face_set_unicode_funcs (hb_face_t *face,
			       hb_unicode_funcs_t *unicode)
{
  if (HB_OBJECT_IS_INERT (face))
    return;

  hb_unicode_funcs_reference (unicode);
  hb_unicode_funcs_destroy (face->unicode);
  face->unicode = unicode;
}

hb_blob_t *
hb_face_get_table (hb_face_t *face,
		   hb_tag_t   tag)
{
  if (HB_UNLIKELY (!face || !face->get_table))
    return hb_blob_create_empty ();

  return face->get_table (tag, face->user_data);
}


/*
 * hb_font_t
 */

static hb_font_t _hb_font_nil = {
  HB_REFERENCE_COUNT_INVALID, /* ref_count */

  0, /* x_scale */
  0, /* y_scale */

  0, /* x_ppem */
  0, /* y_ppem */

  NULL /* klass */
};

hb_font_t *
hb_font_create (void)
{
  hb_font_t *font;

  if (!HB_OBJECT_DO_CREATE (hb_font_t, font))
    return &_hb_font_nil;

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

  hb_font_funcs_destroy (font->klass);

  free (font);
}

void
hb_font_set_funcs (hb_font_t       *font,
		   hb_font_funcs_t *klass)
{
  if (HB_OBJECT_IS_INERT (font))
    return;

  hb_font_funcs_reference (klass);
  hb_font_funcs_destroy (font->klass);
  font->klass = klass;
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

