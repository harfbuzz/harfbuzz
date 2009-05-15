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

#ifndef HB_OT_LAYOUT_PRIVATE_H
#define HB_OT_LAYOUT_PRIVATE_H

#include "hb-private.h"
#include "hb-ot-layout.h"

/* XXX */
#include "harfbuzz-buffer.h"


typedef unsigned int hb_ot_layout_class_t;

struct _hb_ot_layout_t {
  const struct GDEF *gdef;
  const struct GSUB *gsub;
  const struct /*XXX*/GSUBGPOS *gpos;

  struct {
    unsigned char *klasses;
    unsigned int len;
  } new_gdef;

  /* TODO add max-nesting-level here? */
};


/* XXX #define HB_OT_LAYOUT_INTERNAL static */
#define HB_OT_LAYOUT_INTERNAL

HB_BEGIN_DECLS();

/*
 * GDEF
 */

HB_OT_LAYOUT_INTERNAL hb_bool_t
_hb_ot_layout_has_new_glyph_classes (hb_ot_layout_t *layout);

HB_OT_LAYOUT_INTERNAL unsigned int
_hb_ot_layout_get_glyph_property (hb_ot_layout_t *layout,
				  hb_codepoint_t  glyph);

HB_OT_LAYOUT_INTERNAL void
_hb_ot_layout_set_glyph_property (hb_ot_layout_t *layout,
				  hb_codepoint_t  glyph,
				  unsigned int    property);

HB_OT_LAYOUT_INTERNAL hb_bool_t
_hb_ot_layout_check_glyph_property (hb_ot_layout_t *layout,
				    HB_GlyphItem    gitem,
				    unsigned int    lookup_flags,
				    unsigned int   *property);

/* XXX */
HB_Error
hb_buffer_ensure( HB_Buffer buffer,
		   HB_UInt   size );

HB_END_DECLS();

#endif /* HB_OT_LAYOUT_PRIVATE_H */
