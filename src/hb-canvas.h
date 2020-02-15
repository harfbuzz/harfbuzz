/*
 * Copyright © 2020  Ebrahim Byagowi
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

#ifndef HB_H_IN
#error "Include <hb.h> instead."
#endif

#ifndef HB_CANVAS_H
#define HB_CANVAS_H

#include "hb.h"

HB_BEGIN_DECLS

/**
 * hb_canvas_t:
 *
 * Since: REPLACEME
 **/
typedef struct hb_canvas_t hb_canvas_t;

HB_EXTERN hb_canvas_t *
hb_canvas_create (unsigned int w, unsigned h);

HB_EXTERN void
hb_canvas_write_bitmap (hb_canvas_t *canvas, uint8_t *bitmap);

HB_EXTERN void
hb_canvas_destroy (hb_canvas_t *canvas);

HB_EXTERN void
hb_canvas_move_to (hb_canvas_t *canvas, float x, float y);

HB_EXTERN void
hb_canvas_line_to (hb_canvas_t *canvas, float x, float y);

HB_EXTERN void
hb_canvas_quadratic_to (hb_canvas_t *canvas, float x1, float y1, float x2, float y2);

HB_EXTERN void
hb_canvas_cubic_to (hb_canvas_t *canvas, float x1, float y1, float x2, float y2, float x3, float y3);

HB_EXTERN void
hb_canvas_close_path (hb_canvas_t *canvas);

HB_EXTERN void
hb_canvas_clear (hb_canvas_t *canvas);

HB_END_DECLS

#endif /* HB_CANVAS_H */
