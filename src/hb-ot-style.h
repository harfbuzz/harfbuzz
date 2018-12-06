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

#ifndef HB_OT_H_IN
#error "Include <hb-ot.h> instead."
#endif

#ifndef HB_OT_STYLE_H
#define HB_OT_STYLE_H

#include "hb.h"

HB_BEGIN_DECLS

/**
 * hb_ot_style_flag_t:
 * @HB_OT_STYLE_FLAG_OLDER_SIBLING_FONT_ATTRIBUTE: If set, this axis value
 * table provides axis value information that is applicable to other fonts
 * within the same font family. This is used if the other fonts were
 * released earlier and did not include information about values for some
 * axis. If newer versions of the other fonts include the information
 * themselves and are present, then this record is ignored.
 * @ELIDABLE_AXIS_VALUE_NAME: If set, it indicates that the axis value
 * represents the “normal” value for the axis and may be omitted when
 * composing name strings.
 *
 * Since: REPLACEME
 */
typedef enum { /*< flags >*/
  HB_OT_STYLE_FLAG_OLDER_SIBLING_FONT_ATTRIBUTE	= 0x00000001u,
  HB_OT_STYLE_FLAG_ELIDABLE_AXIS_VALUE_NAME	= 0x00000002u,

  _HB_OT_STYLE_FLAG_MAX_VALUE			= 0x7FFFFFFFu, /*< skip >*/
} hb_ot_style_flag_t;

/**
 * hb_ot_style_info_t
 *
 * Since: REPLACEME
 */
typedef struct
{
  hb_tag_t		tag;
  hb_ot_name_id_t	axis_name_id;
  unsigned int		axis_ordering;

  unsigned int		axis_index;
  unsigned int		value_index;
  hb_ot_style_flag_t	flags;
  hb_ot_name_id_t	value_name_id;
  float			value;
  float			min_value;
  float			max_value;
  float			linked_value;

  /*< private >*/
  unsigned int		reserved;
} hb_ot_style_info_t;

HB_EXTERN const hb_ot_style_info_t *
hb_ot_style_get (hb_face_t    *face,
		 unsigned int *num_entries /* OUT */);

HB_END_DECLS

#endif /* HB_OT_STYLE_H */
