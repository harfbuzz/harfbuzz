/*
 * Copyright © 2011  Google, Inc.
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

#include "hb-ot-shape-private.hh"
#include "hb-ot-shape-complex-private.hh"

HB_BEGIN_DECLS

/*
 * HIGHLEVEL DESIGN:
 *
 * This file exports one main function: _hb_ot_shape_normalize().
 *
 * This function closely reflects the Unicode Normalization Algorithm,
 * yet it's different.  The shaper an either prefer decomposed (NFD) or
 * composed (NFC).
 *
 * In general what happens is that: each grapheme is decomposed in a chain
 * of 1:2 decompositions, marks reordered, and then recomposed if desires,
 * so far it's like Unicode Normalization.  However, the decomposition and
 * recomposition only happens if the font supports the resulting characters.
 *
 * The goals are:
 *
 *   - Try to render all canonically equivalent strings similarly.  To really
 *     achieve this we have to always do the full decomposition and then
 *     selectively recompose from there.  It's kinda too expensive though, so
 *     we skip some cases.  For example, if composed is desired, we simply
 *     don't touch 1-character clusters that are supported by the font, even
 *     though their NFC may be different.
 *
 *   - When a font has a precomposed character for a sequence but the 'ccmp'
 *     feature in the font is not adequate, form use the precomposed character
 *     which typically has better mark positioning.
 *
 *   - When a font does not support a character but supports its decomposition,
 *     well, use the decomposition.
 *
 *   - The Indic shaper requests decomposed output.  This will handle splitting
 *     matra for the Indic shaper.
 */


static bool
decompose (hb_ot_shape_context_t *c,
	   bool recompose,
	   hb_codepoint_t ab)
{
  hb_codepoint_t a, b, glyph;
  bool has_this = hb_font_get_glyph (c->font, ab, 0, &glyph);

  /* If recomposing and the single char is supported by the font, we're good. */
  if (recompose && has_this)
    return TRUE;

  if (hb_unicode_decompose (c->buffer->unicode, ab, &a, &b) &&
      hb_font_get_glyph (c->font, b, 0, &glyph) &&
      decompose (c, recompose, a))
  {
    /* Successfully decomposed. */

    if (recompose) {
      /* Try composing b with base if not blocked */

    }

    return TRUE;
  }

  return has_this;
}

static bool
decompose_single_char_cluster (hb_ot_shape_context_t *c,
			       bool recompose,
			       unsigned int i)
{
//  c->buffer->copy ();
//  bool ret = decompose (c, recompose, c->buffer->info[i].codepoint);
//  c->buffer->skip ();
//  return ret;
  return FALSE;
}

static void
handle_single_char_cluster (hb_ot_shape_context_t *c,
			    bool recompose,
			    unsigned int i)
{
  /* Decompose */
  decompose_single_char_cluster (c, recompose, i);
}

static void
handle_multi_char_cluster (hb_ot_shape_context_t *c,
			   bool recompose,
			   unsigned int start,
			   unsigned int end)
{
  /* TODO Currently if there's a variation-selector we give-up, it's just too hard. */
  for (unsigned int i = start; i < end; i++)
    if (unlikely (is_variation_selector (c->buffer->info[i].codepoint)))
      return;

}

void
_hb_ot_shape_normalize (hb_ot_shape_context_t *c)
{
  hb_buffer_t *buffer = c->buffer;
  bool recompose = !hb_ot_shape_complex_prefer_decomposed (c->plan->shaper);

  buffer->clear_output ();

  unsigned int count = buffer->len;
  for (buffer->idx = 0; buffer->idx < count;)
  {

    unsigned int end;
    for (end = buffer->idx + 1; end < count; end++)
      if (buffer->info[buffer->idx].cluster != buffer->info[end].cluster)
        break;

    if (buffer->idx + 1 == end)
      handle_single_char_cluster (c, recompose, buffer->idx);
    else
      handle_multi_char_cluster (c, recompose, buffer->idx, end);
    while (buffer->idx < end)
      c->buffer->next_glyph ();
  }

  buffer->swap_buffers ();
}

HB_END_DECLS
