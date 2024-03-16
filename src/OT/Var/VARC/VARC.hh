#ifndef OT_VAR_VARC_VARC_HH
#define OT_VAR_VARC_VARC_HH

#include "../../../hb-ot-layout-common.hh"
#include "../../../hb-geometry.hh"

namespace OT {

//namespace Var {

/*
 * VARC -- Variable Composites
 * https://github.com/harfbuzz/boring-expansion-spec/blob/main/VARC.md
 */

#ifndef HB_NO_VAR

struct VarComponent
{
  enum class flags_t : uint32_t
  {
    RESET_UNSPECIFIED_AXES	= 1u << 0,
    HAVE_AXES			= 1u << 1,
    AXIS_VALUES_HAVE_VARIATION	= 1u << 2,
    TRANSFORM_HAS_VARIATION	= 1u << 3,
    HAVE_TRANSLATE_X		= 1u << 4,
    HAVE_TRANSLATE_Y		= 1u << 5,
    HAVE_ROTATION		= 1u << 6,
    USE_MY_METRICS		= 1u << 7,
    HAVE_SCALE_X		= 1u << 8,
    HAVE_SCALE_Y		= 1u << 9,
    HAVE_TCENTER_X		= 1u << 10,
    HAVE_TCENTER_Y		= 1u << 11,
    GID_IS_24BIT		= 1u << 12,
    HAVE_SKEW_X			= 1u << 13,
    HAVE_SKEW_Y			= 1u << 14,
    RESERVED			= ~((1u << 15) - 1),
  };

  HB_INTERNAL hb_ubytes_t
  get_path_at (hb_font_t *font, hb_codepoint_t parent_gid, hb_draw_session_t &draw_session,
	       hb_array_t<const int> coords,
	       hb_ubytes_t record) const;
};

struct VarCompositeGlyph
{
  static void
  get_path_at (hb_font_t *font, hb_codepoint_t glyph, hb_draw_session_t &draw_session,
	       hb_array_t<const int> coords,
	       hb_ubytes_t record)
  {
    while (record)
    {
      const VarComponent &comp = * (const VarComponent *) (record.arrayZ);
      record = comp.get_path_at (font, glyph, draw_session, coords, record);
    }
  }
};

HB_MARK_AS_FLAG_T (VarComponent::flags_t);

struct VARC
{
  friend struct VarComponent;

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

    VarCompositeGlyph::get_path_at (font, glyph, draw_session, coords, record);

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
  Offset32To<TupleList> axisIndicesList;
  Offset32To<CFF2Index/*Of<VarCompositeGlyph>*/> glyphRecords;
  public:
  DEFINE_SIZE_STATIC (20);
};

hb_ubytes_t
VarComponent::get_path_at (hb_font_t *font, hb_codepoint_t parent_gid, hb_draw_session_t &draw_session,
			   hb_array_t<const int> coords,
			   hb_ubytes_t record) const
{
  auto &VARC = *font->face->table.VARC;
  const unsigned char *end = record.arrayZ + record.length;

#define READ_UINT32VAR(name) \
  HB_STMT_START { \
    if (unlikely (record.length < HBUINT32VAR::min_size || \
		  (!hb_barrier ()) || \
		  record.length < ((const HBUINT32VAR *) record.arrayZ)->get_size ())) \
      return hb_ubytes_t (); \
    name = (uint32_t) (* (const HBUINT32VAR *) record.arrayZ); \
    record += ((const HBUINT32VAR *) record.arrayZ)->get_size (); \
  } HB_STMT_END

  uint32_t flags;
  READ_UINT32VAR (flags);

  // gid
  hb_codepoint_t gid = 0;
  if (flags & (unsigned) flags_t::GID_IS_24BIT)
  {
    if (unlikely (record.length < HBGlyphID24::static_size))
      return hb_ubytes_t ();
    hb_barrier ();
    gid = (* (const HBGlyphID24 *) record.arrayZ);
    record += HBGlyphID24::static_size;
  }
  else
  {
    if (unlikely (record.length < HBGlyphID16::static_size))
     {
      return hb_ubytes_t ();
     }
    hb_barrier ();
    gid = (* (const HBGlyphID16 *) record.arrayZ);
    record += HBGlyphID16::static_size;
  }

  // Axis values

  hb_vector_t<unsigned> axisIndices;
  hb_vector_t<signed> axisValuesInt;
  if (flags & (unsigned) flags_t::HAVE_AXES)
  {
    unsigned axisIndicesIndex;
    READ_UINT32VAR (axisIndicesIndex);
    axisIndices = (&VARC+VARC.axisIndicesList)[axisIndicesIndex];
    axisValuesInt.resize (axisIndices.length);
    const HBUINT8 *p = (const HBUINT8 *) record.arrayZ;
    TupleValues::decompile (p, axisValuesInt, (const HBUINT8 *) end);
    record += (const uint8_t *) p - record.arrayZ;
  }
  hb_vector_t<float> axisValues (axisValuesInt);

  if (flags & (unsigned) flags_t::AXIS_VALUES_HAVE_VARIATION)
  {
    uint32_t axisValuesVarIdx;
    READ_UINT32VAR (axisValuesVarIdx);
    (&VARC+VARC.varStore).get_delta (axisValuesVarIdx, coords, axisValues.as_array ());
  }

  for (unsigned i = 0; i < axisValues.length; i++)
    axisValues[i] /= 16384.0f;

  // Transform

  uint32_t transformVarIdx = VarIdx::NO_VARIATION;
  if (flags & (unsigned) flags_t::TRANSFORM_HAS_VARIATION)
    READ_UINT32VAR (transformVarIdx);

#define PROCESS_TRANSFORM_COMPONENTS \
	HB_STMT_START { \
	PROCESS_TRANSFORM_COMPONENT (FWORD, HAVE_TRANSLATE_X, translateX); \
	PROCESS_TRANSFORM_COMPONENT (FWORD, HAVE_TRANSLATE_Y, translateY); \
	PROCESS_TRANSFORM_COMPONENT (F4DOT12, HAVE_ROTATION, rotation); \
	PROCESS_TRANSFORM_COMPONENT (F6DOT10, HAVE_SCALE_X, scaleX); \
	PROCESS_TRANSFORM_COMPONENT (F6DOT10, HAVE_SCALE_Y, scaleY); \
	PROCESS_TRANSFORM_COMPONENT (F4DOT12, HAVE_SKEW_X, skewX); \
	PROCESS_TRANSFORM_COMPONENT (F4DOT12, HAVE_SKEW_Y, skewY); \
	PROCESS_TRANSFORM_COMPONENT (FWORD, HAVE_TCENTER_X, tCenterX); \
	PROCESS_TRANSFORM_COMPONENT (FWORD, HAVE_TCENTER_Y, tCenterY); \
	} HB_STMT_END

  hb_transform_decomposed_t transform;

  // Read transform components
#define PROCESS_TRANSFORM_COMPONENT(type, flag, name) \
  if (flags & (unsigned) flags_t::flag) \
  { \
    if (unlikely (record.length < type::static_size)) \
      return hb_ubytes_t (); \
    const type &var = * (const type *) record.arrayZ; \
    transform.name = float (var); \
    record += type::static_size; \
  }
  PROCESS_TRANSFORM_COMPONENTS;
#undef PROCESS_TRANSFORM_COMPONENT

  VARC.get_path_at (font, gid, draw_session, coords, parent_gid);

  return record;
}

#endif

//}

}

#endif  /* OT_VAR_VARC_VARC_HH */
