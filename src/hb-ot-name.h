/*
 * Copyright © 2018  Ebrahim Byagowi.
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

#ifndef HB_OT_NAME_H
#define HB_OT_NAME_H

#include "hb.h"

HB_BEGIN_DECLS


/**
 * hb_name_id_t:
 *
 * Since: 2.0.0
 */
typedef unsigned int hb_name_id_t;

/**
 * HB_NAME_ID_INVALID
 *
 * Since: 2.0.0
 **/
#define HB_NAME_ID_INVALID 0xFFFF


#if 0
HB_EXTERN hb_bool_t
Xhb_ot_name_get_utf8 (hb_face_t     *face,
		     hb_name_id_t   name_id,
		     hb_language_t  language,
		     unsigned int  *text_size /* IN/OUT */,
		     char          *text      /* OUT */);
#endif

HB_EXTERN hb_bool_t
hb_ot_name_get_utf16 (hb_face_t     *face,
		      hb_name_id_t   name_id,
		      hb_language_t  language,
		      unsigned int  *text_size /* IN/OUT */,
		      uint16_t      *text      /* OUT */);

#if 0
HB_EXTERN hb_bool_t
Xhb_ot_name_get_utf32 (hb_face_t     *face,
		      hb_name_id_t   name_id,
		      hb_language_t  language,
		      unsigned int  *text_size /* IN/OUT */,
		      uint32_t      *text      /* OUT */);
#endif


typedef struct hb_ot_name_entry_t
{
  hb_name_id_t  name_id;
  /*< private >*/
  hb_var_int_t  var;
  /*< public >*/
  hb_language_t language;
} hb_ot_name_entry_t;

HB_EXTERN unsigned int
hb_ot_name_get_names (hb_face_t                 *face,
		      const hb_ot_name_entry_t **entries /* OUT */);


HB_END_DECLS

#endif /* HB_OT_NAME_H */
