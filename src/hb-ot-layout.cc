/*
 * Copyright (C) 2007,2008  Red Hat, Inc.
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

#define HB_OT_LAYOUT_CC

#include "hb-ot-layout.h"
#include "hb-ot-layout-private.h"

#include "hb-ot-layout-open-private.h"
#include "hb-ot-layout-gdef-private.h"
#include "hb-ot-layout-gsub-private.h"

#include <stdlib.h>

HB_OT_Layout *
hb_ot_layout_create (const char *font_data,
                     int         face_index)
{
  HB_OT_Layout *layout = (HB_OT_Layout *) calloc (1, sizeof (HB_OT_Layout));

  const OpenTypeFontFile &ot = OpenTypeFontFile::get_for_data (font_data);
  const OpenTypeFontFace &font = ot[face_index];

  layout->gdef = font.find_table (GDEF::Tag);
  layout->gsub = font.find_table (GSUB::Tag);
  layout->gpos = font.find_table (GPOS::Tag);

  return layout;
}

void
hb_ot_layout_destroy (HB_OT_Layout *layout)
{
  free (layout);
}

hb_ot_layout_glyph_properties_t
hb_ot_layout_get_glyph_properties (HB_OT_Layout                    *layout,
				   hb_ot_layout_glyph_t             glyph);

void
hb_ot_layout_set_glyph_properties (HB_OT_Layout                    *layout,
				   hb_ot_layout_glyph_t             glyph,
				   hb_ot_layout_glyph_properties_t  properties);
