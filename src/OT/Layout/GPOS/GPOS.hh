#ifndef OT_LAYOUT_GPOS_GPOS_HH
#define OT_LAYOUT_GPOS_GPOS_HH

// TODO(garretrieger): move to new layout.
#include "../../../hb-ot-layout-common.hh"
#include "../../../hb-ot-layout-gsubgpos.hh"
#include "PosLookup.hh"

namespace OT {
namespace Layout {
namespace GPOS {

/*
 * GPOS -- Glyph Positioning
 * https://docs.microsoft.com/en-us/typography/opentype/spec/gpos
 */

struct GPOS : GSUBGPOS
{
  static constexpr hb_tag_t tableTag = HB_OT_TAG_GPOS;

  using Lookup = PosLookup;

  const PosLookup& get_lookup (unsigned int i) const
  { return static_cast<const PosLookup &> (GSUBGPOS::get_lookup (i)); }

  static inline void position_start (hb_font_t *font, hb_buffer_t *buffer);
  static inline void position_finish_advances (hb_font_t *font, hb_buffer_t *buffer);
  static inline void position_finish_offsets (hb_font_t *font, hb_buffer_t *buffer);

  bool subset (hb_subset_context_t *c) const
  {
    hb_subset_layout_context_t l (c, tableTag, c->plan->gpos_lookups, c->plan->gpos_langsys, c->plan->gpos_features);
    return GSUBGPOS::subset<PosLookup> (&l);
  }

  bool sanitize (hb_sanitize_context_t *c) const
  { return GSUBGPOS::sanitize<PosLookup> (c); }

  HB_INTERNAL bool is_blocklisted (hb_blob_t *blob,
                                   hb_face_t *face) const;

  void collect_variation_indices (hb_collect_variation_indices_context_t *c) const
  {
    for (unsigned i = 0; i < GSUBGPOS::get_lookup_count (); i++)
    {
      if (!c->gpos_lookups->has (i)) continue;
      const PosLookup &l = get_lookup (i);
      l.dispatch (c);
    }
  }

  void closure_lookups (hb_face_t      *face,
                        const hb_set_t *glyphs,
                        hb_set_t       *lookup_indexes /* IN/OUT */) const
  { GSUBGPOS::closure_lookups<PosLookup> (face, glyphs, lookup_indexes); }

  typedef GSUBGPOS::accelerator_t<GPOS> accelerator_t;
};

}
}

struct GPOS_accelerator_t : Layout::GPOS::GPOS::accelerator_t {
  GPOS_accelerator_t (hb_face_t *face) : Layout::GPOS::GPOS::accelerator_t (face) {}
};
}

#endif  /* OT_LAYOUT_GSUB_GSUB_HH */
