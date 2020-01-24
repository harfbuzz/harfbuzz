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

#ifndef HB_NO_GLYPH

#include "hb-ot.h"
#include "hb-ot-glyf-table.hh"
#include "hb-ot-cff1-table.hh"
#include "hb-ot-cff2-table.hh"
#include "hb-ot-glyph.hh"

/**
 * hb_ot_glyph_decompose_funcs_set_move_to_func:
 * @funcs: decompose functions object
 * @move_to: move-to callback
 *
 * Sets move-to callback to the decompose functions object.
 *
 * Since: REPLACEME
 **/
void
hb_ot_glyph_decompose_funcs_set_move_to_func (hb_ot_glyph_decompose_funcs_t        *funcs,
					      hb_ot_glyph_decompose_move_to_func_t  move_to)
{
  if (unlikely (funcs == &Null (hb_ot_glyph_decompose_funcs_t))) return;
  funcs->move_to = move_to;
}

/**
 * hb_ot_glyph_decompose_funcs_set_line_to_func:
 * @funcs: decompose functions object
 * @line_to: line-to callback
 *
 * Sets line-to callback to the decompose functions object.
 *
 * Since: REPLACEME
 **/
void
hb_ot_glyph_decompose_funcs_set_line_to_func (hb_ot_glyph_decompose_funcs_t        *funcs,
					      hb_ot_glyph_decompose_line_to_func_t  line_to)
{
  if (unlikely (funcs == &Null (hb_ot_glyph_decompose_funcs_t))) return;
  funcs->line_to = line_to;
}

/**
 * hb_ot_glyph_decompose_funcs_set_conic_to_func:
 * @funcs: decompose functions object
 * @move_to: conic-to callback
 *
 * Sets conic-to callback to the decompose functions object.
 *
 * Since: REPLACEME
 **/
void
hb_ot_glyph_decompose_funcs_set_conic_to_func (hb_ot_glyph_decompose_funcs_t         *funcs,
					       hb_ot_glyph_decompose_conic_to_func_t  conic_to)
{
  if (unlikely (funcs == &Null (hb_ot_glyph_decompose_funcs_t))) return;
  funcs->conic_to = conic_to;
}

/**
 * hb_ot_glyph_decompose_funcs_set_cubic_to_func:
 * @funcs: decompose functions
 * @cubic_to: cubic-to callback
 *
 * Sets cubic-to callback to the decompose functions object.
 *
 * Since: REPLACEME
 **/
void
hb_ot_glyph_decompose_funcs_set_cubic_to_func (hb_ot_glyph_decompose_funcs_t         *funcs,
					       hb_ot_glyph_decompose_cubic_to_func_t  cubic_to)
{
  if (unlikely (funcs == &Null (hb_ot_glyph_decompose_funcs_t))) return;
  funcs->cubic_to = cubic_to;
}

/**
 * hb_ot_glyph_decompose_funcs_set_close_path_func:
 * @funcs: decompose functions object
 * @close_path: close-path callback
 *
 * Sets close-path callback to the decompose functions object.
 *
 * Since: REPLACEME
 **/
void
hb_ot_glyph_decompose_funcs_set_close_path_func (hb_ot_glyph_decompose_funcs_t           *funcs,
						 hb_ot_glyph_decompose_close_path_func_t  close_path)
{
  if (unlikely (funcs == &Null (hb_ot_glyph_decompose_funcs_t))) return;
  funcs->close_path = close_path;
}

static void
_move_to_nil (hb_position_t to_x HB_UNUSED, hb_position_t to_y HB_UNUSED, void *user_data HB_UNUSED) {}

static void
_line_to_nil (hb_position_t to_x HB_UNUSED, hb_position_t to_y HB_UNUSED, void *user_data HB_UNUSED) {}

static void
_conic_to_nil (hb_position_t control_x HB_UNUSED, hb_position_t control_y HB_UNUSED,
		hb_position_t to_x HB_UNUSED, hb_position_t to_y HB_UNUSED,
		void *user_data HB_UNUSED) {}
static void
_cubic_to_nil (hb_position_t control1_x HB_UNUSED, hb_position_t control1_y HB_UNUSED,
		hb_position_t control2_x HB_UNUSED, hb_position_t control2_y HB_UNUSED,
		hb_position_t to_x HB_UNUSED, hb_position_t to_y HB_UNUSED,
		void *user_data HB_UNUSED) {}

static void
_close_path_nil (void *user_data HB_UNUSED) {}

/**
 * hb_ot_glyph_decompose_funcs_create:
 *
 * Creates a new decompose callbacks object.
 *
 * Since: REPLACEME
 **/
hb_ot_glyph_decompose_funcs_t *
hb_ot_glyph_decompose_funcs_create ()
{
  hb_ot_glyph_decompose_funcs_t *funcs;
  if (unlikely (!(funcs = hb_object_create<hb_ot_glyph_decompose_funcs_t> ())))
    return const_cast<hb_ot_glyph_decompose_funcs_t *> (&Null (hb_ot_glyph_decompose_funcs_t));

  funcs->move_to = (hb_ot_glyph_decompose_move_to_func_t) _move_to_nil;
  funcs->line_to = (hb_ot_glyph_decompose_line_to_func_t) _line_to_nil;
  funcs->conic_to = (hb_ot_glyph_decompose_conic_to_func_t) _conic_to_nil;
  funcs->cubic_to = (hb_ot_glyph_decompose_cubic_to_func_t) _cubic_to_nil;
  funcs->close_path = (hb_ot_glyph_decompose_close_path_func_t) _close_path_nil;
  return funcs;
}

/**
 * hb_ot_glyph_decompose_funcs_reference:
 * @funcs: decompose functions
 *
 * Add to callbacks object refcount.
 *
 * Returns: The same object.
 * Since: REPLACEME
 **/
hb_ot_glyph_decompose_funcs_t *
hb_ot_glyph_decompose_funcs_reference (hb_ot_glyph_decompose_funcs_t *funcs)
{
  return hb_object_reference (funcs);
}

/**
 * hb_ot_glyph_decompose_funcs_destroy:
 * @funcs: decompose functions
 *
 * Decreases refcount of callbacks object and deletes the object if it reaches
 * to zero.
 *
 * Since: REPLACEME
 **/
void
hb_ot_glyph_decompose_funcs_destroy (hb_ot_glyph_decompose_funcs_t *funcs)
{
  if (!hb_object_destroy (funcs)) return;

  free (funcs);
}

/**
 * hb_ot_glyph_decompose:
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
hb_ot_glyph_decompose (hb_font_t *font, hb_codepoint_t glyph,
		       const hb_ot_glyph_decompose_funcs_t *funcs,
		       void *user_data)
{
  if (unlikely (!funcs || funcs == &Null (hb_ot_glyph_decompose_funcs_t) ||
		glyph >= font->face->get_num_glyphs ()))
    return false;

  if (font->face->table.glyf->get_path (font, glyph, funcs, user_data)) return true;
#ifndef HB_NO_CFF
  if (font->face->table.cff1->get_path (font, glyph, funcs, user_data)) return true;
  if (font->face->table.cff2->get_path (font, glyph, funcs, user_data)) return true;
#endif

  return false;
}

#endif
