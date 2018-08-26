/*
 * Copyright © 2007,2008,2009  Red Hat, Inc.
 * Copyright © 2012,2013  Google, Inc.
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
 * Red Hat Author(s): Behdad Esfahbod
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_OT_FACE_HH
#define HB_OT_FACE_HH

#include "hb.hh"

#include "hb-machinery.hh"
#include "hb-set-digest.hh"

#include "hb-ot-cmap-table.hh"
#include "hb-ot-glyf-table.hh"
#include "hb-ot-hmtx-table.hh"
#include "hb-ot-kern-table.hh"
#include "hb-ot-post-table.hh"
#include "hb-ot-color-cbdt-table.hh"


/*
 * hb_ot_face_data_t
 */

struct hb_ot_layout_lookup_accelerator_t
{
  template <typename TLookup>
  inline void init (const TLookup &lookup)
  {
    digest.init ();
    lookup.add_coverage (&digest);
  }

  inline void fini (void)
  {
  }

  inline bool may_have (hb_codepoint_t g) const {
    return digest.may_have (g);
  }

  private:
  hb_set_digest_t digest;
};

/* Most of these tables are NOT needed for shaping.  But we need to hook them *somewhere*.
 * This is as good as any place. */
#define HB_OT_LAYOUT_TABLES \
    /* OpenType shaping. */ \
    HB_OT_LAYOUT_TABLE(OT, GDEF) \
    HB_OT_LAYOUT_TABLE(OT, GSUB) \
    HB_OT_LAYOUT_TABLE(OT, GPOS) \
    HB_OT_LAYOUT_TABLE(OT, JSTF) \
    HB_OT_LAYOUT_TABLE(OT, BASE) \
    /* AAT shaping. */ \
    HB_OT_LAYOUT_TABLE(AAT, morx) \
    HB_OT_LAYOUT_TABLE(AAT, kerx) \
    HB_OT_LAYOUT_TABLE(AAT, ankr) \
    HB_OT_LAYOUT_TABLE(AAT, trak) \
    /* OpenType variations. */ \
    HB_OT_LAYOUT_TABLE(OT, fvar) \
    HB_OT_LAYOUT_TABLE(OT, avar) \
    HB_OT_LAYOUT_TABLE(OT, MVAR) \
    /* OpenType color. */ \
    HB_OT_LAYOUT_TABLE(OT, COLR) \
    HB_OT_LAYOUT_TABLE(OT, CPAL) \
    HB_OT_LAYOUT_TABLE(OT, CBDT) \
    HB_OT_LAYOUT_TABLE(OT, CBLC) \
    HB_OT_LAYOUT_TABLE(OT, sbix) \
    HB_OT_LAYOUT_TABLE(OT, svg) \
    /* OpenType math. */ \
    HB_OT_LAYOUT_TABLE(OT, MATH) \
    /* OpenType fundamentals. */ \
    HB_OT_LAYOUT_TABLE(OT, post) \
    /* */

/* Declare tables. */
#define HB_OT_LAYOUT_TABLE(Namespace, Type) namespace Namespace { struct Type; }
HB_OT_LAYOUT_TABLES
#undef HB_OT_LAYOUT_TABLE

struct hb_ot_face_data_t
{
  /* All the president's tables. */
  struct tables_t
  {
    HB_INTERNAL void init0 (hb_face_t *face);
    HB_INTERNAL void fini (void);

#define HB_OT_LAYOUT_TABLE_ORDER(Namespace, Type) \
      HB_PASTE (ORDER_, HB_PASTE (Namespace, HB_PASTE (_, Type)))
    enum order_t
    {
      ORDER_ZERO,
#define HB_OT_LAYOUT_TABLE(Namespace, Type) \
	HB_OT_LAYOUT_TABLE_ORDER (Namespace, Type),
      HB_OT_LAYOUT_TABLES
#undef HB_OT_LAYOUT_TABLE
    };

    hb_face_t *face; /* MUST be JUST before the lazy loaders. */
#define HB_OT_LAYOUT_TABLE(Namespace, Type) \
    hb_table_lazy_loader_t<struct Namespace::Type, HB_OT_LAYOUT_TABLE_ORDER (Namespace, Type)> Type;
    HB_OT_LAYOUT_TABLES
#undef HB_OT_LAYOUT_TABLE
  } table;

  struct accelerator_t
  {
    inline void init0 (hb_face_t *face)
    {
      this->face = face;
      cmap.init0 ();
      h_metrics.init0 ();
      v_metrics.init0 ();
      glyf.init0 ();
      cbdt.init0 ();
      post.init0 ();
      kern.init0 ();
    }
    inline void fini (void)
    {
      cmap.fini ();
      h_metrics.fini ();
      v_metrics.fini ();
      glyf.fini ();
      cbdt.fini ();
      post.fini ();
      kern.fini ();
    }

    hb_face_t *face; /* MUST be JUST before the lazy loaders. */
    hb_face_lazy_loader_t<1, OT::cmap::accelerator_t> cmap;
    hb_face_lazy_loader_t<2, OT::hmtx::accelerator_t> h_metrics;
    hb_face_lazy_loader_t<3, OT::vmtx::accelerator_t> v_metrics;
    hb_face_lazy_loader_t<4, OT::glyf::accelerator_t> glyf;
    hb_face_lazy_loader_t<5, OT::CBDT::accelerator_t> cbdt;
    hb_face_lazy_loader_t<6, OT::post::accelerator_t> post;
    hb_face_lazy_loader_t<7, OT::kern::accelerator_t> kern;
  } accel;

  /* More accelerators.  Merge into previous. */
  unsigned int gsub_lookup_count;
  unsigned int gpos_lookup_count;
  hb_ot_layout_lookup_accelerator_t *gsub_accels;
  hb_ot_layout_lookup_accelerator_t *gpos_accels;
};


HB_INTERNAL hb_ot_face_data_t *
_hb_ot_face_data_create (hb_face_t *face);

HB_INTERNAL void
_hb_ot_face_data_destroy (hb_ot_face_data_t *data);


#define hb_ot_face_data(face) ((hb_ot_face_data_t *) face->shaper_data.ot.get_relaxed ())



#endif /* HB_OT_FACE_HH */
