#include "VARC.hh"

#ifndef HB_NO_VAR_COMPOSITES

#include "../../../hb-draw.hh"
#include "../../../hb-ot-layout-common.hh"
#include "../../../hb-ot-layout-gdef-table.hh"

namespace OT {

//namespace Var {

static constexpr uint32_t DEBUG_VARC_TRACE_VAR_IDX = 22478849;

static void
debug_trace_coords (hb_codepoint_t parent_gid,
                    hb_codepoint_t gid,
                    uint32_t var_idx,
                    hb_array_t<const float> coords_f32)
{
  if (var_idx != DEBUG_VARC_TRACE_VAR_IDX)
    return;

  fprintf (stderr,
           "VARC_COORDS parent_gid=%u gid=%u var_idx=%u len=%u",
           (unsigned) parent_gid,
           (unsigned) gid,
           (unsigned) var_idx,
           (unsigned) coords_f32.length);
  for (unsigned i = 0; i < coords_f32.length; i++)
    fprintf (stderr, " %u:%.16f/%d", i, (double) coords_f32[i], (int) roundf (coords_f32[i]));
  fprintf (stderr, "\n");
}


#ifndef HB_NO_DRAW

struct hb_transforming_pen_context_t
{
  hb_transform_t<> transform;
  hb_draw_funcs_t *dfuncs;
  void *data;
  hb_draw_state_t *st;
  float draw_x_scale;
  float draw_y_scale;
  bool trace_commands;
  hb_codepoint_t parent_gid;
  hb_codepoint_t gid;
  uint32_t var_idx;
  unsigned seq;
};

static void
debug_trace_cmd_point (const hb_transforming_pen_context_t *c,
                       const char *phase,
                       unsigned seq,
                       const char *op,
                       float x,
                       float y)
{
  if (!c->trace_commands)
    return;

  fprintf (stderr,
           "VARC_CMD phase=%s parent_gid=%u gid=%u var_idx=%u seq=%u op=%s x_raw_26_6=%.16f y_raw_26_6=%.16f x_norm=%.16f y_norm=%.16f x_cmp=%.16f y_cmp=%.16f x_after_draw=%.16f y_after_draw=%.16f x_unscaled=%.16f y_unscaled=%.16f\n",
           phase,
           (unsigned) c->parent_gid,
           (unsigned) c->gid,
           (unsigned) c->var_idx,
           seq,
           op,
           (double) (x * 64.0f),
           (double) (y * 64.0f),
           (double) x,
           (double) y,
           (double) x,
           (double) y,
           (double) (x * c->draw_x_scale),
           (double) (y * c->draw_y_scale),
           (double) (c->draw_x_scale ? (x / c->draw_x_scale) : 0.f),
           (double) (c->draw_y_scale ? (y / c->draw_y_scale) : 0.f));
}

static void
debug_trace_cmd_quad (const hb_transforming_pen_context_t *c,
                      const char *phase,
                      unsigned seq,
                      const char *op,
                      float cx0,
                      float cy0,
                      float x,
                      float y)
{
  if (!c->trace_commands)
    return;

  fprintf (stderr,
           "VARC_CMD phase=%s parent_gid=%u gid=%u var_idx=%u seq=%u op=%s cx0_raw_26_6=%.16f cy0_raw_26_6=%.16f x_raw_26_6=%.16f y_raw_26_6=%.16f cx0_norm=%.16f cy0_norm=%.16f x_norm=%.16f y_norm=%.16f cx0_cmp=%.16f cy0_cmp=%.16f x_cmp=%.16f y_cmp=%.16f cx0_after_draw=%.16f cy0_after_draw=%.16f x_after_draw=%.16f y_after_draw=%.16f cx0_unscaled=%.16f cy0_unscaled=%.16f x_unscaled=%.16f y_unscaled=%.16f\n",
           phase,
           (unsigned) c->parent_gid,
           (unsigned) c->gid,
           (unsigned) c->var_idx,
           seq,
           op,
           (double) (cx0 * 64.0f),
           (double) (cy0 * 64.0f),
           (double) (x * 64.0f),
           (double) (y * 64.0f),
           (double) cx0,
           (double) cy0,
           (double) x,
           (double) y,
           (double) cx0,
           (double) cy0,
           (double) x,
           (double) y,
           (double) (cx0 * c->draw_x_scale),
           (double) (cy0 * c->draw_y_scale),
           (double) (x * c->draw_x_scale),
           (double) (y * c->draw_y_scale),
           (double) (c->draw_x_scale ? (cx0 / c->draw_x_scale) : 0.f),
           (double) (c->draw_y_scale ? (cy0 / c->draw_y_scale) : 0.f),
           (double) (c->draw_x_scale ? (x / c->draw_x_scale) : 0.f),
           (double) (c->draw_y_scale ? (y / c->draw_y_scale) : 0.f));
}

static void
debug_trace_cmd_cubic (const hb_transforming_pen_context_t *c,
                       const char *phase,
                       unsigned seq,
                       const char *op,
                       float cx0,
                       float cy0,
                       float cx1,
                       float cy1,
                       float x,
                       float y)
{
  if (!c->trace_commands)
    return;

  fprintf (stderr,
           "VARC_CMD phase=%s parent_gid=%u gid=%u var_idx=%u seq=%u op=%s cx0_raw_26_6=%.16f cy0_raw_26_6=%.16f cx1_raw_26_6=%.16f cy1_raw_26_6=%.16f x_raw_26_6=%.16f y_raw_26_6=%.16f cx0_norm=%.16f cy0_norm=%.16f cx1_norm=%.16f cy1_norm=%.16f x_norm=%.16f y_norm=%.16f cx0_cmp=%.16f cy0_cmp=%.16f cx1_cmp=%.16f cy1_cmp=%.16f x_cmp=%.16f y_cmp=%.16f cx0_after_draw=%.16f cy0_after_draw=%.16f cx1_after_draw=%.16f cy1_after_draw=%.16f x_after_draw=%.16f y_after_draw=%.16f cx0_unscaled=%.16f cy0_unscaled=%.16f cx1_unscaled=%.16f cy1_unscaled=%.16f x_unscaled=%.16f y_unscaled=%.16f\n",
           phase,
           (unsigned) c->parent_gid,
           (unsigned) c->gid,
           (unsigned) c->var_idx,
           seq,
           op,
           (double) (cx0 * 64.0f),
           (double) (cy0 * 64.0f),
           (double) (cx1 * 64.0f),
           (double) (cy1 * 64.0f),
           (double) (x * 64.0f),
           (double) (y * 64.0f),
           (double) cx0,
           (double) cy0,
           (double) cx1,
           (double) cy1,
           (double) x,
           (double) y,
           (double) cx0,
           (double) cy0,
           (double) cx1,
           (double) cy1,
           (double) x,
           (double) y,
           (double) (cx0 * c->draw_x_scale),
           (double) (cy0 * c->draw_y_scale),
           (double) (cx1 * c->draw_x_scale),
           (double) (cy1 * c->draw_y_scale),
           (double) (x * c->draw_x_scale),
           (double) (y * c->draw_y_scale),
           (double) (c->draw_x_scale ? (cx0 / c->draw_x_scale) : 0.f),
           (double) (c->draw_y_scale ? (cy0 / c->draw_y_scale) : 0.f),
           (double) (c->draw_x_scale ? (cx1 / c->draw_x_scale) : 0.f),
           (double) (c->draw_y_scale ? (cy1 / c->draw_y_scale) : 0.f),
           (double) (c->draw_x_scale ? (x / c->draw_x_scale) : 0.f),
           (double) (c->draw_y_scale ? (y / c->draw_y_scale) : 0.f));
}

static void
debug_trace_cmd_close (const hb_transforming_pen_context_t *c,
                       const char *phase,
                       unsigned seq,
                       const char *op)
{
  if (!c->trace_commands)
    return;

  fprintf (stderr,
           "VARC_CMD phase=%s parent_gid=%u gid=%u var_idx=%u seq=%u op=%s\n",
           phase,
           (unsigned) c->parent_gid,
           (unsigned) c->gid,
           (unsigned) c->var_idx,
           seq,
           op);
}

static void
hb_transforming_pen_move_to (hb_draw_funcs_t *dfuncs HB_UNUSED,
			     void *data,
			     hb_draw_state_t *st,
			     float to_x, float to_y,
			     void *user_data HB_UNUSED)
{
  hb_transforming_pen_context_t *c = (hb_transforming_pen_context_t *) data;
  unsigned seq = c->seq++;
  float pre_x = to_x;
  float pre_y = to_y;
  debug_trace_cmd_point (c, "pre", seq, "M", pre_x, pre_y);

  c->transform.transform_point (to_x, to_y);
  debug_trace_cmd_point (c, "post", seq, "M", to_x, to_y);

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
  unsigned seq = c->seq++;
  float pre_x = to_x;
  float pre_y = to_y;
  debug_trace_cmd_point (c, "pre", seq, "L", pre_x, pre_y);

  c->transform.transform_point (to_x, to_y);
  debug_trace_cmd_point (c, "post", seq, "L", to_x, to_y);

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
  unsigned seq = c->seq++;
  float pre_control_x = control_x;
  float pre_control_y = control_y;
  float pre_to_x = to_x;
  float pre_to_y = to_y;
  debug_trace_cmd_quad (c, "pre", seq, "Q", pre_control_x, pre_control_y, pre_to_x, pre_to_y);

  c->transform.transform_point (control_x, control_y);
  c->transform.transform_point (to_x, to_y);
  debug_trace_cmd_quad (c, "post", seq, "Q", control_x, control_y, to_x, to_y);

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
  unsigned seq = c->seq++;
  float pre_control1_x = control1_x;
  float pre_control1_y = control1_y;
  float pre_control2_x = control2_x;
  float pre_control2_y = control2_y;
  float pre_to_x = to_x;
  float pre_to_y = to_y;
  debug_trace_cmd_cubic (c, "pre", seq, "C",
                         pre_control1_x, pre_control1_y,
                         pre_control2_x, pre_control2_y,
                         pre_to_x, pre_to_y);

  c->transform.transform_point (control1_x, control1_y);
  c->transform.transform_point (control2_x, control2_y);
  c->transform.transform_point (to_x, to_y);
  debug_trace_cmd_cubic (c, "post", seq, "C",
                         control1_x, control1_y,
                         control2_x, control2_y,
                         to_x, to_y);

  c->dfuncs->cubic_to (c->data, *c->st, control1_x, control1_y, control2_x, control2_y, to_x, to_y);
}

static void
hb_transforming_pen_close_path (hb_draw_funcs_t *dfuncs HB_UNUSED,
				void *data,
				hb_draw_state_t *st,
				void *user_data HB_UNUSED)
{
  hb_transforming_pen_context_t *c = (hb_transforming_pen_context_t *) data;
  unsigned seq = c->seq++;
  debug_trace_cmd_close (c, "pre", seq, "Z");

  c->dfuncs->close_path (c->data, *c->st);
  debug_trace_cmd_close (c, "post", seq, "Z");
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
VarComponent::get_path_at (const hb_varc_context_t &c,
			   hb_codepoint_t parent_gid,
			   hb_array_t<const float> coords_f32,
			   hb_transform_t<> total_transform,
			   hb_ubytes_t total_record,
			   hb_scalar_cache_t *cache,
			   uint32_t trace_var_idx) const
{
  const unsigned char *end = total_record.arrayZ + total_record.length;
  const unsigned char *record = total_record.arrayZ;

  auto &VARC = *c.font->face->table.VARC->table;
  auto &varStore = &VARC+VARC.varStore;

#define READ_UINT32VAR(name) \
  HB_STMT_START { \
    if (unlikely (unsigned (end - record) < HBUINT32VAR::min_size)) return hb_ubytes_t (); \
    hb_barrier (); \
    auto &varint = * (const HBUINT32VAR *) record; \
    unsigned size = varint.get_size (); \
    if (unlikely (unsigned (end - record) < size)) return hb_ubytes_t (); \
    name = (uint32_t) varint; \
    record += size; \
  } HB_STMT_END

  uint32_t flags;
  READ_UINT32VAR (flags);

  // gid

  hb_codepoint_t gid = 0;
  if (flags & (unsigned) flags_t::GID_IS_24BIT)
  {
    if (unlikely (unsigned (end - record) < HBGlyphID24::static_size))
      return hb_ubytes_t ();
    hb_barrier ();
    gid = * (const HBGlyphID24 *) record;
    record += HBGlyphID24::static_size;
  }
  else
  {
    if (unlikely (unsigned (end - record) < HBGlyphID16::static_size))
      return hb_ubytes_t ();
    hb_barrier ();
    gid = * (const HBGlyphID16 *) record;
    record += HBGlyphID16::static_size;
  }

  // Condition
  bool show = true;
  if (flags & (unsigned) flags_t::HAVE_CONDITION)
  {
    hb_vector_t<int> condition_coords_storage;
    if (coords_f32.length && unlikely (!condition_coords_storage.resize (coords_f32.length)))
      return hb_ubytes_t ();
    for (unsigned i = 0; i < coords_f32.length; i++)
      condition_coords_storage[i] = (int) roundf (coords_f32[i]);
    hb_array_t<const int> condition_coords = condition_coords_storage.as_array ();

    unsigned conditionIndex;
    READ_UINT32VAR (conditionIndex);
    const auto &condition = (&VARC+VARC.conditionList)[conditionIndex];
    auto instancer = MultiItemVarStoreInstancer(&varStore, nullptr, condition_coords, coords_f32, cache);
    show = condition.evaluate (condition_coords.arrayZ, condition_coords.length, &instancer);
  }

  // Axis values

  auto &axisIndices = c.scratch.axisIndices;
  axisIndices.clear ();
  auto &axisValues = c.scratch.axisValues;
  axisValues.clear ();
  if (flags & (unsigned) flags_t::HAVE_AXES)
  {
    unsigned axisIndicesIndex;
    READ_UINT32VAR (axisIndicesIndex);
    axisIndices.extend ((&VARC+VARC.axisIndicesList)[axisIndicesIndex]);
    axisValues.resize (axisIndices.length);
    const HBUINT8 *p = (const HBUINT8 *) record;
    TupleValues::decompile (p, axisValues, (const HBUINT8 *) end);
    record = (const unsigned char *) p;
  }

  // Apply variations if any
  if (flags & (unsigned) flags_t::AXIS_VALUES_HAVE_VARIATION)
  {
    uint32_t axisValuesVarIdx;
    READ_UINT32VAR (axisValuesVarIdx);
    if (show && coords_f32 && !axisValues.in_error ())
      varStore.get_delta (axisValuesVarIdx, coords_f32, axisValues.as_array (), cache);
  }

  hb_vector_t<float> component_coords_f32_storage;
  hb_array_t<const float> component_coords_f32 = coords_f32;

  /* Copying coords is expensive; so we have put an arbitrary
   * limit on the max number of coords for now. */
  if ((flags & (unsigned) flags_t::RESET_UNSPECIFIED_AXES) ||
      component_coords_f32.length > HB_VAR_COMPOSITE_MAX_AXES)
  {
    if (unlikely (!component_coords_f32_storage.resize (c.font->num_coords)))
      return hb_ubytes_t ();
    for (unsigned i = 0; i < c.font->num_coords; i++)
      component_coords_f32_storage[i] = (float) c.font->coords[i];
    component_coords_f32 = component_coords_f32_storage.as_array ();
  }

  // Transform

  uint32_t transformVarIdx = VarIdx::NO_VARIATION;
  if (flags & (unsigned) flags_t::TRANSFORM_HAS_VARIATION)
    READ_UINT32VAR (transformVarIdx);

#define PROCESS_TRANSFORM_COMPONENTS \
	HB_STMT_START { \
	PROCESS_TRANSFORM_COMPONENT ( 0, FWORD, HAVE_TRANSLATE_X, translateX); \
	PROCESS_TRANSFORM_COMPONENT ( 0, FWORD, HAVE_TRANSLATE_Y, translateY); \
	PROCESS_TRANSFORM_COMPONENT (12, F4DOT12, HAVE_ROTATION, rotation); \
	PROCESS_TRANSFORM_COMPONENT (10, F6DOT10, HAVE_SCALE_X, scaleX); \
	PROCESS_TRANSFORM_COMPONENT (10, F6DOT10, HAVE_SCALE_Y, scaleY); \
	PROCESS_TRANSFORM_COMPONENT (12, F4DOT12, HAVE_SKEW_X, skewX); \
	PROCESS_TRANSFORM_COMPONENT (12, F4DOT12, HAVE_SKEW_Y, skewY); \
	PROCESS_TRANSFORM_COMPONENT ( 0, FWORD, HAVE_TCENTER_X, tCenterX); \
	PROCESS_TRANSFORM_COMPONENT ( 0, FWORD, HAVE_TCENTER_Y, tCenterY); \
	} HB_STMT_END

  hb_transform_decomposed_t<> transform;

  // Read transform components
#define PROCESS_TRANSFORM_COMPONENT(shift, type, flag, name) \
	if (flags & (unsigned) flags_t::flag) \
	{ \
	  static_assert (type::static_size == HBINT16::static_size, ""); \
	  if (unlikely (unsigned (end - record) < HBINT16::static_size)) \
	    return hb_ubytes_t (); \
	  hb_barrier (); \
	  transform.name = * (const HBINT16 *) record; \
	  record += HBINT16::static_size; \
	}
  PROCESS_TRANSFORM_COMPONENTS;
#undef PROCESS_TRANSFORM_COMPONENT

  // Read reserved records
  unsigned i = flags & (unsigned) flags_t::RESERVED_MASK;
  while (i)
  {
    HB_UNUSED uint32_t discard;
    READ_UINT32VAR (discard);
    i &= i - 1;
  }

  /* Parsing is over now. */

  if (show)
  {
    // Apply axis overrides in float coordinate space.
    if (axisIndices)
    {
      if (component_coords_f32.arrayZ != component_coords_f32_storage.arrayZ)
      {
        component_coords_f32_storage.clear ();
        component_coords_f32_storage.extend (component_coords_f32);
        if (unlikely (component_coords_f32_storage.in_error ()))
          return hb_ubytes_t ();
        component_coords_f32 = component_coords_f32_storage.as_array ();
      }

      for (unsigned i = 0; i < axisIndices.length; i++)
      {
        unsigned axis = axisIndices[i];
        if (axis >= HB_VAR_COMPOSITE_MAX_AXES)
          continue;
        if (unlikely (component_coords_f32_storage.length <= axis &&
                      !component_coords_f32_storage.resize (axis + 1)))
          return hb_ubytes_t ();
        component_coords_f32_storage[axis] = axisValues[i];
      }
      component_coords_f32 = component_coords_f32_storage.as_array ();
    }

    // Apply transform variations if any
    if (transformVarIdx != VarIdx::NO_VARIATION && coords_f32)
    {
      debug_trace_coords (parent_gid, gid, transformVarIdx, coords_f32);

      float rotation_pre_raw = transform.rotation;
      float transformValues[9];
      unsigned rotationIndex = (unsigned) -1;
      if (flags & (unsigned) flags_t::HAVE_ROTATION)
      {
        rotationIndex = 0;
        if (flags & (unsigned) flags_t::HAVE_TRANSLATE_X) rotationIndex++;
        if (flags & (unsigned) flags_t::HAVE_TRANSLATE_Y) rotationIndex++;
      }
      unsigned numTransformValues = 0;
#define PROCESS_TRANSFORM_COMPONENT(shift, type, flag, name) \
	  if (flags & (unsigned) flags_t::flag) \
	    transformValues[numTransformValues++] = transform.name;
      PROCESS_TRANSFORM_COMPONENTS;
#undef PROCESS_TRANSFORM_COMPONENT
      varStore.get_delta (transformVarIdx, coords_f32, hb_array (transformValues, numTransformValues), cache);
      if (rotationIndex < numTransformValues)
        varStore.debug_rotation_region_terms (transformVarIdx,
                                              coords_f32.arrayZ, coords_f32.length,
                                              numTransformValues,
                                              rotationIndex,
                                              parent_gid,
                                              gid,
                                              cache);
      numTransformValues = 0;
#define PROCESS_TRANSFORM_COMPONENT(shift, type, flag, name) \
	  if (flags & (unsigned) flags_t::flag) \
	    transform.name = transformValues[numTransformValues++];
      PROCESS_TRANSFORM_COMPONENTS;
#undef PROCESS_TRANSFORM_COMPONENT
      if (rotationIndex != (unsigned) -1)
        fprintf (stderr,
                 "VARC_ROT parent_gid=%u gid=%u var_idx=%u pre_raw=%.16f pre=%.16f delta_raw=%.16f delta=%.16f post_raw=%.16f post=%.16f\n",
                 (unsigned) parent_gid,
                 (unsigned) gid,
                 (unsigned) transformVarIdx,
                 (double) rotation_pre_raw,
                 (double) (rotation_pre_raw * (1.f / (1 << 12))),
                 (double) (transform.rotation - rotation_pre_raw),
                 (double) ((transform.rotation - rotation_pre_raw) * (1.f / (1 << 12))),
                 (double) transform.rotation,
                 (double) (transform.rotation * (1.f / (1 << 12))));

      if (transformVarIdx == DEBUG_VARC_TRACE_VAR_IDX)
        fprintf (stderr,
                 "VARC_XFORM parent_gid=%u gid=%u var_idx=%u tx=%.16f ty=%.16f rot_raw=%.16f sx_raw=%.16f sy_raw=%.16f skx_raw=%.16f sky_raw=%.16f cx=%.16f cy=%.16f\n",
                 (unsigned) parent_gid,
                 (unsigned) gid,
                 (unsigned) transformVarIdx,
                 (double) transform.translateX,
                 (double) transform.translateY,
                 (double) transform.rotation,
                 (double) transform.scaleX,
                 (double) transform.scaleY,
                 (double) transform.skewX,
                 (double) transform.skewY,
                 (double) transform.tCenterX,
                 (double) transform.tCenterY);
    }

    // Divide them by their divisors
#define PROCESS_TRANSFORM_COMPONENT(shift, type, flag, name) \
	  if (shift && (flags & (unsigned) flags_t::flag)) \
	     transform.name *= 1.f / (1 << shift);
    PROCESS_TRANSFORM_COMPONENTS;
#undef PROCESS_TRANSFORM_COMPONENT

    if (!(flags & (unsigned) flags_t::HAVE_SCALE_Y))
      transform.scaleY = transform.scaleX;

    transform.rotation *= HB_PI;
    transform.skewX *= HB_PI;
    transform.skewY *= HB_PI;

    if (transform.rotation != 0)
    {
      float s, c;
      hb_sincos (transform.rotation, s, c);
      fprintf (stderr,
               "VARC_TRIG kind=rotate in=%.16f sin=%.16f cos=%.16f\n",
               (double) transform.rotation,
               (double) s,
               (double) c);
    }
    if (transform.skewX != 0 || transform.skewY != 0)
    {
      float skew_x = -transform.skewX;
      float skew_y = transform.skewY;
      float tan_x = skew_x ? tanf (skew_x) : 0;
      float tan_y = skew_y ? tanf (skew_y) : 0;
      fprintf (stderr,
               "VARC_TRIG kind=skew in_x=%.16f in_y=%.16f tan_x=%.16f tan_y=%.16f\n",
               (double) skew_x,
               (double) skew_y,
               (double) tan_x,
               (double) tan_y);
    }

    hb_transform_t<> local_transform = transform.to_transform ();
    hb_transform_t<> combined_transform = total_transform;
    combined_transform.transform (local_transform);
    if (transformVarIdx == DEBUG_VARC_TRACE_VAR_IDX)
      fprintf (stderr,
               "VARC_AFFINE parent_gid=%u gid=%u var_idx=%u xx=%.16f yx=%.16f xy=%.16f yy=%.16f x0=%.16f y0=%.16f\n",
               (unsigned) parent_gid,
               (unsigned) gid,
               (unsigned) transformVarIdx,
               (double) combined_transform.xx,
               (double) combined_transform.yx,
               (double) combined_transform.xy,
               (double) combined_transform.yy,
               (double) combined_transform.x0,
               (double) combined_transform.y0);
    total_transform = combined_transform;

    uint32_t child_trace_var_idx = trace_var_idx;
    if (transformVarIdx == DEBUG_VARC_TRACE_VAR_IDX)
      child_trace_var_idx = transformVarIdx;

    bool same_coords = component_coords_f32.length == coords_f32.length &&
		       component_coords_f32.arrayZ == coords_f32.arrayZ;

    c.depth_left--;
    VARC.get_path_at (c, gid,
		      component_coords_f32,
		      total_transform,
		      parent_gid,
		      same_coords ? cache : nullptr,
                      child_trace_var_idx);
    c.depth_left++;
  }

#undef PROCESS_TRANSFORM_COMPONENTS
#undef READ_UINT32VAR

  return hb_ubytes_t (record, end - record);
}

bool
VARC::get_path_at (const hb_varc_context_t &c,
		   hb_codepoint_t glyph,
		   hb_array_t<const float> coords_f32,
		   hb_transform_t<> transform,
		   hb_codepoint_t parent_glyph,
		   hb_scalar_cache_t *parent_cache,
                   uint32_t trace_var_idx) const
{
  // Don't recurse on the same glyph.
  unsigned idx = glyph == parent_glyph ?
		 NOT_COVERED :
		 (this+coverage).get_coverage (glyph);
  if (idx == NOT_COVERED)
  {
    hb_vector_t<int> coords_storage;
    if (coords_f32.length && unlikely (!coords_storage.resize (coords_f32.length)))
      return false;
    for (unsigned i = 0; i < coords_f32.length; i++)
      coords_storage[i] = (int) roundf (coords_f32[i]);
    hb_array_t<const int> coords = coords_storage.as_array ();

    if (c.draw_session)
    {
      hb_transform_t<> leaf_transform = transform;
      leaf_transform.x0 *= c.font->x_multf;
      leaf_transform.y0 *= c.font->y_multf;
      bool trace_commands = trace_var_idx == DEBUG_VARC_TRACE_VAR_IDX;
      float draw_x_scale = 1.f;
      float draw_y_scale = 1.f;
      if (c.font && c.font->parent)
      {
        if (c.font->parent->x_scale)
          draw_x_scale = (float) c.font->x_scale / (float) c.font->parent->x_scale;
        if (c.font->parent->y_scale)
          draw_y_scale = (float) c.font->y_scale / (float) c.font->parent->y_scale;
      }
      if (trace_commands)
      {
        fprintf (stderr,
                 "VARC_CMD_CTX parent_gid=%u gid=%u var_idx=%u draw_x_scale=%.16f draw_y_scale=%.16f font_x_scale=%d font_y_scale=%d parent_x_scale=%d parent_y_scale=%d\n",
                 (unsigned) parent_glyph,
                 (unsigned) glyph,
                 (unsigned) trace_var_idx,
                 (double) draw_x_scale,
                 (double) draw_y_scale,
                 c.font ? c.font->x_scale : 0,
                 c.font ? c.font->y_scale : 0,
                 (c.font && c.font->parent) ? c.font->parent->x_scale : 0,
                 (c.font && c.font->parent) ? c.font->parent->y_scale : 0);
      }

      // Build a transforming pen to apply the transform.
      hb_draw_funcs_t *transformer_funcs = hb_transforming_pen_get_funcs ();
      hb_transforming_pen_context_t context {leaf_transform,
					     c.draw_session->funcs,
					     c.draw_session->draw_data,
					     &c.draw_session->st,
                                             draw_x_scale,
                                             draw_y_scale,
                                             trace_commands,
                                             parent_glyph,
                                             glyph,
                                             trace_var_idx,
                                             0};
      hb_draw_session_t transformer_session {transformer_funcs, &context};
      hb_draw_session_t &shape_draw_session = (!trace_commands && leaf_transform.is_identity ())
                                              ? *c.draw_session
                                              : transformer_session;

      if (c.font->face->table.glyf->get_path_at (c.font, glyph, shape_draw_session, coords, c.scratch.glyf_scratch)) return true;
#ifndef HB_NO_CFF
      if (c.font->face->table.cff2->get_path_at (c.font, glyph, shape_draw_session, coords)) return true;
      if (c.font->face->table.cff1->get_path (c.font, glyph, shape_draw_session)) return true; // Doesn't have variations
#endif
      return false;
    }
    else if (c.extents)
    {
      hb_glyph_extents_t glyph_extents;
      if (!c.font->face->table.glyf->get_extents_at (c.font, glyph, &glyph_extents, coords))
#ifndef HB_NO_CFF
      if (!c.font->face->table.cff2->get_extents_at (c.font, glyph, &glyph_extents, coords))
      if (!c.font->face->table.cff1->get_extents (c.font, glyph, &glyph_extents)) // Doesn't have variations
#endif
	return false;

      hb_extents_t<> comp_extents (glyph_extents);
      hb_transform_t<> leaf_transform = transform;
      leaf_transform.x0 *= c.font->x_multf;
      leaf_transform.y0 *= c.font->y_multf;
      leaf_transform.transform_extents (comp_extents);
      c.extents->union_ (comp_extents);
    }
    return true;
  }

  if (c.depth_left <= 0)
    return true;

  if (c.edges_left <= 0)
    return true;
  (c.edges_left)--;

  hb_decycler_node_t node (c.decycler);
  if (unlikely (!node.visit (glyph)))
    return true;

  hb_ubytes_t record = (this+glyphRecords)[idx];

  hb_scalar_cache_t static_cache;
  hb_scalar_cache_t *cache = parent_cache ?
                                  parent_cache :
                                  (this+varStore).create_cache (&static_cache);

  VarCompositeGlyph::get_path_at (c,
                                  glyph,
                                  coords_f32,
                                  transform,
                                  record,
                                  cache,
                                  trace_var_idx);

  if (cache != parent_cache)
    (this+varStore).destroy_cache (cache, &static_cache);

  return true;
}

#endif

//} // namespace Var
} // namespace OT

#endif
