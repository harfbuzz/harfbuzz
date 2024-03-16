#ifndef OT_VAR_VARC_VARC_HH
#define OT_VAR_VARC_VARC_HH

#include "../../../hb-ot-layout-common.hh"

namespace OT {

//namespace Var {

/*
 * VARC -- Variable Composites
 * https://github.com/harfbuzz/boring-expansion-spec/blob/main/VARC.md
 */

#ifndef HB_NO_VAR

struct VarComponent
{
  HB_INTERNAL hb_ubytes_t
  get_path_at (hb_font_t *font, hb_codepoint_t parent_gid, hb_draw_session_t &draw_session,
	       hb_array_t<const int> coords,
	       hb_ubytes_t record) const;
};

struct VarCompositeGlyph
{
  void
  get_path_at (hb_font_t *font, hb_codepoint_t glyph, hb_draw_session_t &draw_session,
	       hb_array_t<const int> coords,
	       hb_ubytes_t record) const
  {
    while (record)
    {
      const VarComponent &comp = * (const VarComponent *) (record.arrayZ);
      record = comp.get_path_at (font, glyph, draw_session, coords, record);
    }
  }
};

struct VARC
{
  static constexpr hb_tag_t tableTag = HB_TAG ('V', 'A', 'R', 'C');

  bool
  get_path_at (hb_font_t *font, hb_codepoint_t glyph, hb_draw_session_t &draw_session,
	       hb_array_t<const int> coords,
	       hb_codepoint_t parent_glyph = HB_CODEPOINT_INVALID) const
  {
    // Don't recurse on the same glyph.
    unsigned idx = glyph == parent_glyph ?
		   NOT_COVERED :
		   (this+coverage).get_coverage (glyph);
    if (idx == NOT_COVERED)
    {
      if (!font->face->table.glyf->get_path_at (font, glyph, draw_session, coords))
#ifndef HB_NO_CFF
      if (!font->face->table.cff2->get_path_at (font, glyph, draw_session, coords))
      if (!font->face->table.cff1->get_path (font, glyph, draw_session)) // Doesn't have variations
#endif
	return false;
      return true;
    }

    hb_ubytes_t record = (this+glyphRecords)[idx];

    const VarCompositeGlyph &composite = * (const VarCompositeGlyph *) (record.arrayZ);
    composite.get_path_at (font, glyph, draw_session, coords, record);

    return true;
  }

  bool
  get_path (hb_font_t *font, hb_codepoint_t gid, hb_draw_session_t &draw_session) const
  { return get_path_at (font, gid, draw_session, hb_array (font->coords, font->num_coords)); }

  bool paint_glyph (hb_font_t *font, hb_codepoint_t gid, hb_paint_funcs_t *funcs, void *data, hb_color_t foreground) const
  {
    funcs->push_clip_glyph (data, gid, font);
    funcs->color (data, true, foreground);
    funcs->pop_clip (data);

    return true;
  }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (version.sanitize (c) &&
		  hb_barrier () &&
		  version.major == 1 &&
		  coverage.sanitize (c, this) &&
		  varStore.sanitize (c, this) &&
		  axisIndicesList.sanitize (c, this) &&
		  glyphRecords.sanitize (c, this));
  }

  protected:
  FixedVersion<> version; /* Version identifier */
  Offset32To<Coverage> coverage;
  Offset32To<MultiItemVariationStore> varStore;
  Offset32To<CFF2Index/*Of<TupleValues>*/> axisIndicesList;
  Offset32To<CFF2Index/*Of<VarCompositeGlyph>*/> glyphRecords;
  public:
  DEFINE_SIZE_STATIC (20);
};


hb_ubytes_t
VarComponent::get_path_at (hb_font_t *font, hb_codepoint_t parent_gid, hb_draw_session_t &draw_session,
			   hb_array_t<const int> coords,
			   hb_ubytes_t record) const
{
  hb_codepoint_t gid = 0;



  font->face->table.VARC->get_path_at (font, gid, draw_session, coords, parent_gid);
  record += 1;//get_size ();
  return record;
}

#endif

//}

}

#endif  /* OT_VAR_VARC_VARC_HH */
