/*
 * Copyright © 2017 Elie Roux<elie.roux@telecom-bretagne.eu>
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
 */

#ifndef HB_OT_H_IN
#error "Include <hb-ot.h> instead."
#endif

#ifndef HB_OT_BASE_H
#define HB_OT_BASE_H

#include "hb.h"

HB_BEGIN_DECLS

#define HB_OT_TAG_BASE      HB_TAG('B','A','S','E')

// https://www.microsoft.com/typography/otspec/baselinetags.htm

#define HB_OT_TAG_BASE_HANG HB_TAG('h','a','n','g')
#define HB_OT_TAG_BASE_ICFB HB_TAG('i','c','f','b')
#define HB_OT_TAG_BASE_ICFT HB_TAG('i','c','f','t')
#define HB_OT_TAG_BASE_IDEO HB_TAG('i','d','e','o')
#define HB_OT_TAG_BASE_IDTB HB_TAG('i','d','t','b')
#define HB_OT_TAG_BASE_MATH HB_TAG('m','a','t','h')
#define HB_OT_TAG_BASE_ROMN HB_TAG('r','o','m','n')

/* Methods */

// HB_EXTERN hb_bool_t
// hb_ot_base_has_data (hb_face_t *face);

HB_END_DECLS

#endif /* HB_OT_BASE_H */
