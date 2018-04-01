/*
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
 */

#include "hb-private.hh"

#include "hb-aat-layout-ankr-table.hh"
#include "hb-aat-layout-bsln-table.hh"
#include "hb-aat-layout-feat-table.hh"
#include "hb-aat-layout-kerx-table.hh"
#include "hb-aat-layout-morx-table.hh"
#include "hb-aat-layout-trak-table.hh"
#include "hb-aat-fmtx-table.hh"
#include "hb-aat-gcid-table.hh"
#include "hb-aat-ltag-table.hh"
#include "hb-ot-layout-base-table.hh"
#include "hb-ot-layout-gdef-table.hh"
#include "hb-ot-layout-gpos-table.hh"
#include "hb-ot-layout-gsub-table.hh"
#include "hb-ot-layout-jstf-table.hh"
#include "hb-ot-color-cbdt-table.hh"
#include "hb-ot-color-colr-table.hh"
#include "hb-ot-color-cpal-table.hh"
#include "hb-ot-color-sbix-table.hh"
#include "hb-ot-color-svg-table.hh"
#include "hb-ot-math-table.hh"
#include "hb-ot-var-avar-table.hh"
#include "hb-ot-var-fvar-table.hh"
#include "hb-ot-var-hvar-table.hh"
#include "hb-ot-var-mvar-table.hh"
#include "hb-ot-cmap-table.hh"
#include "hb-ot-glyf-table.hh"
#include "hb-ot-hdmx-table.hh"
#include "hb-ot-head-table.hh"
#include "hb-ot-hhea-table.hh"
#include "hb-ot-hmtx-table.hh"
#include "hb-ot-kern-table.hh"
#include "hb-ot-maxp-table.hh"
#include "hb-ot-name-table.hh"
#include "hb-ot-os2-table.hh"
#include "hb-ot-post-table.hh"


/**
 * hb_sanitize: Sanitizes a table specified by a tag
 * @face:
 * @tag:
 *
 *
 *
 * Return value: Sanitizer result, nullptr if we don't support that table at all
 * Since: REPLACEME
 **/
hb_blob_t *
hb_sanitize (hb_face_t *face, hb_tag_t tag)
{
  unsigned int num_glyphs = hb_face_get_glyph_count (face);
  hb_blob_t *blob = hb_face_reference_table (face, tag);
  hb_blob_t *result = nullptr;
#define DEFINE_SANITIZER(table) \
  case table::tableTag: { \
    OT::Sanitizer<table> sanitizer; \
    sanitizer.set_num_glyphs (num_glyphs); \
    result = sanitizer.sanitize (blob); \
    break; \
  }
  switch (tag)
  {
  DEFINE_SANITIZER(AAT::ankr);
  DEFINE_SANITIZER(AAT::bsln);
  DEFINE_SANITIZER(AAT::feat);
  DEFINE_SANITIZER(AAT::kerx);
  DEFINE_SANITIZER(AAT::morx);
  DEFINE_SANITIZER(AAT::trak);
  DEFINE_SANITIZER(AAT::fmtx);
  DEFINE_SANITIZER(AAT::gcid);
  DEFINE_SANITIZER(AAT::ltag);
  DEFINE_SANITIZER(OT::BASE );
  DEFINE_SANITIZER(OT::GDEF );
  DEFINE_SANITIZER(OT::GPOS );
  DEFINE_SANITIZER(OT::GSUB );
  DEFINE_SANITIZER(OT::JSTF );
  DEFINE_SANITIZER(OT::CBDT );
  DEFINE_SANITIZER(OT::CBLC );
  DEFINE_SANITIZER(OT::COLR );
  DEFINE_SANITIZER(OT::CPAL );
  DEFINE_SANITIZER(OT::sbix );
  DEFINE_SANITIZER(OT::SVG  );
  DEFINE_SANITIZER(OT::MATH );
  DEFINE_SANITIZER(OT::avar );
  DEFINE_SANITIZER(OT::fvar );
  DEFINE_SANITIZER(OT::HVAR );
  DEFINE_SANITIZER(OT::VVAR );
  DEFINE_SANITIZER(OT::MVAR );
  DEFINE_SANITIZER(OT::cmap );
  DEFINE_SANITIZER(OT::glyf );
  DEFINE_SANITIZER(OT::loca );
  DEFINE_SANITIZER(OT::hdmx );
  DEFINE_SANITIZER(OT::head );
  DEFINE_SANITIZER(OT::hhea );
  DEFINE_SANITIZER(OT::vhea );
  DEFINE_SANITIZER(OT::hmtx );
  DEFINE_SANITIZER(OT::vmtx );
  DEFINE_SANITIZER(OT::kern );
  DEFINE_SANITIZER(OT::maxp );
  DEFINE_SANITIZER(OT::name );
  DEFINE_SANITIZER(OT::os2  );
  DEFINE_SANITIZER(OT::post );
  }
#undef DEFINE_SANITIZER
  return result;
}
