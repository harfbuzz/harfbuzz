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

HB_BEGIN_DECLS

static bool
get_glyph (hb_ot_shape_context_t *c, unsigned int i)
{
  hb_buffer_t *b = c->buffer;

  return hb_font_get_glyph (c->font, b->info[i].codepoint, 0, &b->info[i].intermittent_glyph());
}

static bool
handle_single_char_cluster (hb_ot_shape_context_t *c,
			    unsigned int i)
{
  if (get_glyph (c, i))
    return FALSE;

  /* Decompose */

  return FALSE;
}

static bool
handle_multi_char_cluster (hb_ot_shape_context_t *c,
			   unsigned int i,
			   unsigned int end)
{
  /* If there's a variation-selector, give-up, it's just too hard. */
  return FALSE;
}

bool
_hb_normalize (hb_ot_shape_context_t *c)
{
  hb_buffer_t *b = c->buffer;
  bool changed = FALSE;

  unsigned int count = b->len;
  for (unsigned int i = 0; i < count;) {
    unsigned int end;
    for (end = i + 1; end < count; end++)
      if (b->info[i].cluster != b->info[end].cluster)
        break;
    if (i + 1 == end)
      changed |= handle_single_char_cluster (c, i);
    else
      changed |= handle_multi_char_cluster (c, i, end);
    i = end;
  }

  return changed;
}

HB_END_DECLS
