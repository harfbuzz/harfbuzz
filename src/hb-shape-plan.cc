/*
 * Copyright © 2012  Google, Inc.
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
 * Google Author(s): Behdad Esfahbod
 */

#include "hb-private.hh"

#include "hb-shape-plan-private.hh"
#include "hb-shaper-private.hh"
#include "hb-font-private.hh"

#define HB_SHAPER_DATA_ENSURE_DECLARE(shaper, object) \
static inline bool \
hb_##shaper##_##object##_data_ensure (hb_##object##_t *object) \
{\
  retry: \
  HB_SHAPER_DATA_TYPE (shaper, object) *data = (HB_SHAPER_DATA_TYPE (shaper, object) *) hb_atomic_ptr_get (&HB_SHAPER_DATA (shaper, object)); \
  if (unlikely (!data)) { \
    data = HB_SHAPER_DATA_CREATE_FUNC (shaper, object) (object); \
    if (unlikely (!data)) \
      data = (HB_SHAPER_DATA_TYPE (shaper, object) *) HB_SHAPER_DATA_INVALID; \
    if (!hb_atomic_ptr_cmpexch (&HB_SHAPER_DATA (shaper, object), NULL, data)) { \
      HB_SHAPER_DATA_DESTROY_FUNC (shaper, object) (data); \
      goto retry; \
    } \
  } \
  return data != NULL && !HB_SHAPER_DATA_IS_INVALID (data); \
}

#define HB_SHAPER_IMPLEMENT(shaper) HB_SHAPER_DATA_ENSURE_DECLARE(shaper, face)
#include "hb-shaper-list.hh"
#undef HB_SHAPER_IMPLEMENT


void
hb_shape_plan_plan (hb_shape_plan_t    *shape_plan,
		    const hb_feature_t *user_features,
		    unsigned int        num_user_features,
		    const char * const *shaper_list)
{
  const hb_shaper_pair_t *shapers = _hb_shapers_get ();
  unsigned num_shapers = 0;

#define HB_SHAPER_PLAN(shaper) \
	HB_STMT_START { \
	  if (hb_##shaper##_face_data_ensure (shape_plan->face)) { \
	    /* TODO Ensure face shaper data. */ \
	    HB_SHAPER_DATA_TYPE (shaper, shape_plan) *data= \
	      HB_SHAPER_DATA_CREATE_FUNC (shaper, shape_plan) (shape_plan, user_features, num_user_features); \
	    if (data) { \
	      HB_SHAPER_DATA (shaper, shape_plan) = data; \
	      shape_plan->shapers[num_shapers++] = _hb_##shaper##_shape; \
	    } \
	    if (false /* shaper never fails */) \
	      return; \
	  } \
	} HB_STMT_END

  if (likely (!shaper_list)) {
    for (unsigned int i = 0; i < HB_SHAPERS_COUNT; i++)
      if (0)
	;
#define HB_SHAPER_IMPLEMENT(shaper) \
      else if (shapers[i].func == _hb_##shaper##_shape) \
	HB_SHAPER_PLAN (shaper);
#include "hb-shaper-list.hh"
#undef HB_SHAPER_IMPLEMENT
  } else {
    for (; *shaper_list; shaper_list++)
      if (0)
	;
#define HB_SHAPER_IMPLEMENT(shaper) \
      else if (0 == strcmp (*shaper_list, #shaper)) \
	HB_SHAPER_PLAN (shaper);
#include "hb-shaper-list.hh"
#undef HB_SHAPER_IMPLEMENT
  }

#undef HB_SHAPER_PLAN
}


/*
 * hb_shape_plan_t
 */

hb_shape_plan_t *
hb_shape_plan_create (hb_face_t                     *face,
		      const hb_segment_properties_t *props,
		      const hb_feature_t            *user_features,
		      unsigned int                   num_user_features,
		      const char * const            *shaper_list)
{
  hb_shape_plan_t *shape_plan;

  if (unlikely (!face))
    face = hb_face_get_empty ();
  if (unlikely (!props || hb_object_is_inert (face)))
    return hb_shape_plan_get_empty ();
  if (!(shape_plan = hb_object_create<hb_shape_plan_t> ()))
    return hb_shape_plan_get_empty ();

  hb_face_make_immutable (face);
  shape_plan->face = hb_face_reference (face);
  shape_plan->props = *props;

  hb_shape_plan_plan (shape_plan, user_features, num_user_features, shaper_list);

  return shape_plan;
}

hb_shape_plan_t *
hb_shape_plan_get_empty (void)
{
  static const hb_shape_plan_t _hb_shape_plan_nil = {
    HB_OBJECT_HEADER_STATIC,
  };

  return const_cast<hb_shape_plan_t *> (&_hb_shape_plan_nil);
}

hb_shape_plan_t *
hb_shape_plan_reference (hb_shape_plan_t *shape_plan)
{
  return hb_object_reference (shape_plan);
}

void
hb_shape_plan_destroy (hb_shape_plan_t *shape_plan)
{
  if (!hb_object_destroy (shape_plan)) return;

#define HB_SHAPER_IMPLEMENT(shaper) HB_SHAPER_DATA_DESTROY(shaper, shape_plan);
#include "hb-shaper-list.hh"
#undef HB_SHAPER_IMPLEMENT

  free (shape_plan);
}
