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

#ifndef HB_OT_LAYOUT_CC
#error "This file should only be included from hb-ot-layout.c"
#endif

#include "hb-private.h"
#include "hb-ot-layout.h"

typedef uint16_t hb_ot_layout_class_t;
typedef uint16_t hb_ot_layout_glyph_properties_t;
typedef int hb_ot_layout_coverage_t;	/* -1 is not covered, >= 0 otherwise */

struct GDEF;
struct GSUB;
struct GPOS;

struct _HB_OT_Layout {
  const GDEF *gdef;
  const GSUB *gsub;
  const GPOS *gpos;

  struct {
    uint8_t *klasses;
    unsigned int len;
  } new_gdef;

};


HB_BEGIN_DECLS();

static hb_ot_layout_glyph_properties_t
_hb_ot_layout_get_glyph_properties (HB_OT_Layout         *layout,
				    hb_ot_layout_glyph_t  glyph);

HB_END_DECLS();

#endif /* HB_OT_LAYOUT_PRIVATE_H */
