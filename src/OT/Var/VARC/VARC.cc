#include "VARC.hh"

#ifndef HB_NO_VAR

namespace OT {

//namespace Var {


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

  auto component_coords = coords;
  /* Copying coords is expensive; so we have put an arbitrary
   * limit on the max number of coords for now. */
  if ((flags & (unsigned) flags_t::RESET_UNSPECIFIED_AXES) ||
      coords.length > HB_GLYF_VAR_COMPOSITE_MAX_AXES)
    component_coords = hb_array<int> ();

  coord_setter_t coord_setter (component_coords);
  for (unsigned i = 0; i < axisIndices.length; i++)
    coord_setter[axisIndices[i]] = axisValues[i];

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
  hb_vector_t<float> transformValuesVector;

  // Read transform components
#define PROCESS_TRANSFORM_COMPONENT(type, flag, name) \
	if (flags & (unsigned) flags_t::flag) \
	{ \
	  if (unlikely (record.length < type::static_size)) \
	    return hb_ubytes_t (); \
	  const type &var = * (const type *) record.arrayZ; \
	  transform.name = float (var); \
	  transformValuesVector.push (transform.name); \
	  record += type::static_size; \
	}
  PROCESS_TRANSFORM_COMPONENTS;
#undef PROCESS_TRANSFORM_COMPONENT

  if (transformVarIdx != VarIdx::NO_VARIATION)
  {
    auto transformValues = transformValuesVector.as_array ();
    (&VARC+VARC.varStore).get_delta (transformVarIdx, coords, transformValues);
#define PROCESS_TRANSFORM_COMPONENT(type, flag, name) \
	if (flags & (unsigned) flags_t::flag) \
	  transform.name = *transformValues++; \
    PROCESS_TRANSFORM_COMPONENTS;
#undef PROCESS_TRANSFORM_COMPONENT
  }

  if (!(flags & (unsigned) flags_t::HAVE_SCALE_Y))
    transform.scaleY = transform.scaleX;

  //hb_draw_funcs_t *transformer_funcs = _get_
  //hb_transformer_context_t context {transform.to_transform (), draw_session};

  VARC.get_path_at (font, gid, draw_session, coord_setter.get_coords (), parent_gid);

  return record;
}

//} // namespace Var
} // namespace OT

#endif
