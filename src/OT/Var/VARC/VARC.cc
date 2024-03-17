#include "VARC.hh"

#ifndef HB_NO_VAR_COMPOSITES

#include "../../../hb-draw.hh"
#include "../../../hb-geometry.hh"

namespace OT {

//namespace Var {


struct hb_transforming_pen_context_t
{
  hb_transform_t transform;
  hb_draw_funcs_t *dfuncs;
  void *data;
  hb_draw_state_t *st;
};

static void
hb_transforming_pen_move_to (hb_draw_funcs_t *dfuncs HB_UNUSED,
			     void *data,
			     hb_draw_state_t *st,
			     float to_x, float to_y,
			     void *user_data HB_UNUSED)
{
  hb_transforming_pen_context_t *c = (hb_transforming_pen_context_t *) data;

  c->transform.transform_point (to_x, to_y);

  c->dfuncs->move_to (c->data, *c->st, to_x, to_y);
}

static void
hb_transforming_pen_line_to (hb_draw_funcs_t *dfuncs HB_UNUSED,
			     void *data,
			     hb_draw_state_t *st,
			     float to_x, float to_y,
			     void *user_data HB_UNUSED)
{
  hb_transforming_pen_context_t *c = (hb_transforming_pen_context_t *) data;

  c->transform.transform_point (to_x, to_y);

  c->dfuncs->line_to (c->data, *c->st, to_x, to_y);
}

static void
hb_transforming_pen_quadratic_to (hb_draw_funcs_t *dfuncs HB_UNUSED,
				  void *data,
				  hb_draw_state_t *st,
				  float control_x, float control_y,
				  float to_x, float to_y,
				  void *user_data HB_UNUSED)
{
  hb_transforming_pen_context_t *c = (hb_transforming_pen_context_t *) data;

  c->transform.transform_point (control_x, control_y);
  c->transform.transform_point (to_x, to_y);

  c->dfuncs->quadratic_to (c->data, *c->st, control_x, control_y, to_x, to_y);
}

static void
hb_transforming_pen_cubic_to (hb_draw_funcs_t *dfuncs HB_UNUSED,
			      void *data,
			      hb_draw_state_t *st,
			      float control1_x, float control1_y,
			      float control2_x, float control2_y,
			      float to_x, float to_y,
			      void *user_data HB_UNUSED)
{
  hb_transforming_pen_context_t *c = (hb_transforming_pen_context_t *) data;

  c->transform.transform_point (control1_x, control1_y);
  c->transform.transform_point (control2_x, control2_y);
  c->transform.transform_point (to_x, to_y);

  c->dfuncs->cubic_to (c->data, *c->st, control1_x, control1_y, control2_x, control2_y, to_x, to_y);
}

static void
hb_transforming_pen_close_path (hb_draw_funcs_t *dfuncs HB_UNUSED,
				void *data,
				hb_draw_state_t *st,
				void *user_data HB_UNUSED)
{
  hb_transforming_pen_context_t *c = (hb_transforming_pen_context_t *) data;

  c->dfuncs->close_path (c->data, *c->st);
}

static inline void free_static_transforming_pen_funcs ();

static struct hb_transforming_pen_funcs_lazy_loader_t : hb_draw_funcs_lazy_loader_t<hb_transforming_pen_funcs_lazy_loader_t>
{
  static hb_draw_funcs_t *create ()
  {
    hb_draw_funcs_t *funcs = hb_draw_funcs_create ();

    hb_draw_funcs_set_move_to_func (funcs, hb_transforming_pen_move_to, nullptr, nullptr);
    hb_draw_funcs_set_line_to_func (funcs, hb_transforming_pen_line_to, nullptr, nullptr);
    hb_draw_funcs_set_quadratic_to_func (funcs, hb_transforming_pen_quadratic_to, nullptr, nullptr);
    hb_draw_funcs_set_cubic_to_func (funcs, hb_transforming_pen_cubic_to, nullptr, nullptr);
    hb_draw_funcs_set_close_path_func (funcs, hb_transforming_pen_close_path, nullptr, nullptr);

    hb_draw_funcs_make_immutable (funcs);

    hb_atexit (free_static_transforming_pen_funcs);

    return funcs;
  }
} static_transforming_pen_funcs;

static inline
void free_static_transforming_pen_funcs ()
{
  static_transforming_pen_funcs.free_instance ();
}

static hb_draw_funcs_t *
hb_transforming_pen_get_funcs ()
{
  return static_transforming_pen_funcs.get_unconst ();
}


hb_ubytes_t
VarComponent::get_path_at (hb_font_t *font, hb_codepoint_t parent_gid, hb_draw_session_t &draw_session,
			   hb_array_t<const int> coords,
			   hb_ubytes_t record,
			   hb_set_t *visited,
			   signed *edges_left,
			   signed depth_left) const
{
  auto &VARC = *font->face->table.VARC;
  const unsigned char *end = record.arrayZ + record.length;

#define READ_UINT32VAR(name) \
  HB_STMT_START { \
    if (unlikely (record.length < HBUINT32VAR::min_size)) return hb_ubytes_t (); \
    hb_barrier (); \
    auto &varint = * (const HBUINT32VAR *) record.arrayZ; \
    unsigned size = varint.get_size (); \
    if (unlikely (record.length < size)) return hb_ubytes_t (); \
    name = (uint32_t) varint; \
    record += size; \
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
      return hb_ubytes_t ();
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

  // Apply variations if any
  if (flags & (unsigned) flags_t::AXIS_VALUES_HAVE_VARIATION)
  {
    uint32_t axisValuesVarIdx;
    READ_UINT32VAR (axisValuesVarIdx);
    if (coords)
      (&VARC+VARC.varStore).get_delta (axisValuesVarIdx, coords, axisValues.as_array ());
  }

  auto component_coords = coords;
  /* Copying coords is expensive; so we have put an arbitrary
   * limit on the max number of coords for now. */
  if ((flags & (unsigned) flags_t::RESET_UNSPECIFIED_AXES) ||
      coords.length > HB_VAR_COMPOSITE_MAX_AXES)
    component_coords = hb_array<int> ();

  coord_setter_t coord_setter (component_coords);
  for (unsigned i = 0; i < axisIndices.length; i++)
    coord_setter[axisIndices[i]] = axisValues[i];

  component_coords = coord_setter.get_coords ();

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
	  static_assert (type::static_size == HBINT16::static_size, ""); \
	  if (unlikely (record.length < HBINT16::static_size)) \
	    return hb_ubytes_t (); \
	  hb_barrier (); \
	  transform.name = * (const HBINT16 *) record.arrayZ; \
	  record += HBINT16::static_size; \
	}
  PROCESS_TRANSFORM_COMPONENTS;
#undef PROCESS_TRANSFORM_COMPONENT

  // Apply variations if any
  if (transformVarIdx != VarIdx::NO_VARIATION && coords)
  {
    hb_vector_t<float> transformValuesVector;
#define PROCESS_TRANSFORM_COMPONENT(type, flag, name) \
	if (flags & (unsigned) flags_t::flag) \
	  transformValuesVector.push (transform.name);
    PROCESS_TRANSFORM_COMPONENTS;
#undef PROCESS_TRANSFORM_COMPONENT
    auto transformValues = transformValuesVector.as_array ();
    (&VARC+VARC.varStore).get_delta (transformVarIdx, coords, transformValues);
#define PROCESS_TRANSFORM_COMPONENT(type, flag, name) \
	if (flags & (unsigned) flags_t::flag) \
	  transform.name = *transformValues++;
    PROCESS_TRANSFORM_COMPONENTS;
#undef PROCESS_TRANSFORM_COMPONENT
  }

  // Divide them by their divisors
#define PROCESS_TRANSFORM_COMPONENT(type, flag, name) \
	if (flags & (unsigned) flags_t::flag) \
	{ \
	  HBINT16 int_v; \
	  int_v = roundf (transform.name); \
	  type typed_v = * (const type *) &int_v; \
	  float float_v = (float) typed_v; \
	  transform.name = float_v; \
	}
  PROCESS_TRANSFORM_COMPONENTS;
#undef PROCESS_TRANSFORM_COMPONENT

  if (!(flags & (unsigned) flags_t::HAVE_SCALE_Y))
    transform.scaleY = transform.scaleX;

  // Scale the transform by the font's scale
  float x_scale = font->x_multf;
  float y_scale = font->y_multf;
  transform.translateX *= x_scale;
  transform.translateY *= y_scale;
  transform.tCenterX *= x_scale;
  transform.tCenterY *= y_scale;

  // Build a transforming pen to apply the transform.
  hb_draw_funcs_t *transformer_funcs = hb_transforming_pen_get_funcs ();
  hb_transforming_pen_context_t context {transform.to_transform (),
					 draw_session.funcs,
					 draw_session.draw_data,
					 &draw_session.st};
  hb_draw_session_t transformer_session {transformer_funcs, &context};

  VARC.get_path_at (font, gid, transformer_session, component_coords, parent_gid, visited, edges_left, depth_left - 1);

#undef PROCESS_TRANSFORM_COMPONENTS
#undef READ_UINT32VAR

  return record;
}

//} // namespace Var
} // namespace OT

#endif
