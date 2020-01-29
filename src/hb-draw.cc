/*
 * Copyright © 2019-2020  Ebrahim Byagowi
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

#include "hb.hh"

#ifndef HB_NO_DRAW

#include "hb-draw.hh"
#include "hb-ot.h"
#include "hb-ot-glyf-table.hh"
#include "hb-ot-cff1-table.hh"
#include "hb-ot-cff2-table.hh"

/**
 * hb_draw_pen_set_move_to_func:
 * @pen: decompose functions object
 * @move_to: move-to callback
 *
 * Sets move-to callback to the decompose functions object.
 *
 * Since: REPLACEME
 **/
void
hb_draw_pen_set_move_to_func (hb_draw_pen_t              *pen,
				hb_draw_pen_move_to_func_t  move_to)
{
  if (unlikely (hb_object_is_immutable (pen))) return;
  pen->_move_to = move_to;
}

/**
 * hb_draw_pen_set_line_to_func:
 * @pen: decompose functions object
 * @line_to: line-to callback
 *
 * Sets line-to callback to the decompose functions object.
 *
 * Since: REPLACEME
 **/
void
hb_draw_pen_set_line_to_func (hb_draw_pen_t              *pen,
			      hb_draw_pen_line_to_func_t  line_to)
{
  if (unlikely (hb_object_is_immutable (pen))) return;
  pen->_line_to = line_to;
}

/**
 * hb_draw_pen_set_quadratic_to_func:
 * @funcs: decompose functions object
 * @move_to: quadratic-to callback
 *
 * Sets quadratic-to callback to the decompose functions object.
 *
 * Since: REPLACEME
 **/
void
hb_draw_pen_set_quadratic_to_func (hb_draw_pen_t                   *pen,
				   hb_draw_pen_quadratic_to_func_t  quadratic_to)
{
  if (unlikely (hb_object_is_immutable (pen))) return;
  pen->_quadratic_to = quadratic_to;
}

/**
 * hb_draw_pen_set_cubic_to_func:
 * @funcs: decompose functions
 * @cubic_to: cubic-to callback
 *
 * Sets cubic-to callback to the decompose functions object.
 *
 * Since: REPLACEME
 **/
void
hb_draw_pen_set_cubic_to_func (hb_draw_pen_t               *pen,
			       hb_draw_pen_cubic_to_func_t  cubic_to)
{
  if (unlikely (hb_object_is_immutable (pen))) return;
  pen->_cubic_to = cubic_to;
}

/**
 * hb_draw_pen_set_close_path_func:
 * @pen: decompose functions object
 * @close_path: close-path callback
 *
 * Sets close-path callback to the decompose functions object.
 *
 * Since: REPLACEME
 **/
void
hb_draw_pen_set_close_path_func (hb_draw_pen_t                 *pen,
				 hb_draw_pen_close_path_func_t  close_path)
{
  if (unlikely (hb_object_is_immutable (pen))) return;
  pen->_close_path = close_path;
}

/**
 * hb_draw_pen_set_user_data:
 *
 * Since: REPLACEME
 **/
void
hb_draw_pen_set_user_data (hb_draw_pen_t *pen, void *user_data)
{
  pen->user_data = user_data;
}

/**
 * hb_draw_pen_get_user_data:
 *
 * Since: REPLACEME
 **/
void *
hb_draw_pen_get_user_data (hb_draw_pen_t *pen)
{
  return pen->user_data;
}

/**
 * hb_draw_pen_move_to:
 *
 * Since: REPLACEME
 **/
void
hb_draw_pen_move_to (hb_draw_pen_t *pen, hb_position_t x, hb_position_t y)
{
  pen->move_to (x, y);
}

/**
 * hb_draw_pen_line_to:
 *
 * Since: REPLACEME
 **/
void
hb_draw_pen_line_to (hb_draw_pen_t *pen, hb_position_t x, hb_position_t y)
{
  pen->line_to (x, y);
}

/**
 * hb_draw_pen_quadratic_to:
 *
 * Since: REPLACEME
 **/
void
hb_draw_pen_quadratic_to (hb_draw_pen_t *pen,
			  hb_position_t control_x, hb_position_t control_y,
			  hb_position_t to_x, hb_position_t to_y)
{
  pen->quadratic_to (control_x, control_y, to_x, to_y);
}

/**
 * hb_draw_pen_cubic_to:
 *
 * Since: REPLACEME
 **/
void
hb_draw_pen_cubic_to (hb_draw_pen_t *pen,
		      hb_position_t control1_x, hb_position_t control1_y,
		      hb_position_t control2_x, hb_position_t control2_y,
		      hb_position_t to_x, hb_position_t to_y)
{
  pen->cubic_to (control1_x, control1_y, control2_x, control2_y, to_x, to_y);
}

/**
 * hb_draw_pen_close_path:
 *
 * Since: REPLACEME
 **/
void
hb_draw_pen_close_path (hb_draw_pen_t *pen)
{
  pen->close_path ();
}

static void
_move_to_nil (hb_position_t to_x HB_UNUSED, hb_position_t to_y HB_UNUSED, void *user_data HB_UNUSED) {}

static void
_line_to_nil (hb_position_t to_x HB_UNUSED, hb_position_t to_y HB_UNUSED, void *user_data HB_UNUSED) {}

static void
_cubic_to_nil (hb_position_t control1_x HB_UNUSED, hb_position_t control1_y HB_UNUSED,
	       hb_position_t control2_x HB_UNUSED, hb_position_t control2_y HB_UNUSED,
	       hb_position_t to_x HB_UNUSED, hb_position_t to_y HB_UNUSED,
	       void *user_data HB_UNUSED) {}

static void
_close_path_nil (void *user_data HB_UNUSED) {}

/**
 * hb_draw_pen_create:
 *
 * Creates a new decompose callbacks object.
 *
 * Since: REPLACEME
 **/
hb_draw_pen_t *
hb_draw_pen_create ()
{
  hb_draw_pen_t *pen;
  if (unlikely (!(pen = hb_object_create<hb_draw_pen_t> ())))
    return const_cast<hb_draw_pen_t *> (&Null (hb_draw_pen_t));

  pen->_move_to = (hb_draw_pen_move_to_func_t) _move_to_nil;
  pen->_line_to = (hb_draw_pen_line_to_func_t) _line_to_nil;
  pen->_quadratic_to = nullptr;
  pen->_cubic_to = (hb_draw_pen_cubic_to_func_t) _cubic_to_nil;
  pen->_close_path = (hb_draw_pen_close_path_func_t) _close_path_nil;
  pen->curr_x = 0;
  pen->curr_y = 0;
  pen->user_data = nullptr;
  return pen;
}

/**
 * hb_draw_pen_reference:
 * @funcs: decompose functions
 *
 * Add to callbacks object refcount.
 *
 * Returns: The same object.
 * Since: REPLACEME
 **/
hb_draw_pen_t *
hb_draw_pen_reference (hb_draw_pen_t *pen)
{
  return hb_object_reference (pen);
}

/**
 * hb_draw_pen_destroy:
 * @pen: decompose functions
 *
 * Decreases refcount of callbacks object and deletes the object if it reaches
 * to zero.
 *
 * Since: REPLACEME
 **/
void
hb_draw_pen_destroy (hb_draw_pen_t *pen)
{
  if (!hb_object_destroy (pen)) return;

  free (pen);
}

/**
 * hb_font_draw_glyph:
 * @font: a font object
 * @glyph: a glyph id
 * @funcs: decompose callbacks object
 * @user_data: parameter you like be passed to the callbacks when are called
 *
 * Decomposes a glyph.
 *
 * Returns: Whether the font had the glyph and the operation completed successfully.
 * Since: REPLACEME
 **/
hb_bool_t
hb_font_draw_glyph (hb_font_t *font, hb_codepoint_t glyph, hb_draw_pen_t *pen)
{
  if (unlikely (pen == &Null (hb_draw_pen_t) ||
		glyph >= font->face->get_num_glyphs ()))
    return false;

  if (font->face->table.glyf->get_path (font, glyph, pen)) return true;
#ifndef HB_NO_CFF
  if (font->face->table.cff1->get_path (font, glyph, pen)) return true;
  if (font->face->table.cff2->get_path (font, glyph, pen)) return true;
#endif

  return false;
}

#endif
