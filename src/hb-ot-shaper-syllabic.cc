/*
 * Copyright © 2021  Behdad Esfahbod.
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

#ifndef HB_NO_OT_SHAPE

#include "hb-ot-shaper-syllabic.hh"


bool
hb_syllabic_insert_dotted_circles (hb_font_t *font,
				   hb_buffer_t *buffer,
				   unsigned int broken_syllable_type,
				   unsigned int dottedcircle_category,
				   int repha_category,
				   int dottedcircle_position)
{
  if (unlikely (buffer->flags & HB_BUFFER_FLAG_DO_NOT_INSERT_DOTTED_CIRCLE))
    return false;
  if (likely (!(buffer->scratch_flags & HB_BUFFER_SCRATCH_FLAG_HAS_BROKEN_SYLLABLE)))
  {
    if (buffer->messaging ())
      (void) buffer->message (font, "skipped inserting dotted-circles because there is no broken syllables");
    return false;
  }

  if (buffer->messaging () &&
      !buffer->message (font, "start inserting dotted-circles"))
    return false;

  hb_codepoint_t dottedcircle_glyph;
  if (!font->get_nominal_glyph (0x25CCu, &dottedcircle_glyph))
    return false;

  hb_glyph_info_t dottedcircle = {0};
  dottedcircle.codepoint = 0x25CCu;
  dottedcircle.ot_shaper_var_u8_category() = dottedcircle_category;
  if (dottedcircle_position != -1)
    dottedcircle.ot_shaper_var_u8_auxiliary() = dottedcircle_position;
  dottedcircle.codepoint = dottedcircle_glyph;

  buffer->clear_output ();

  buffer->idx = 0;
  unsigned int last_syllable = 0;
  while (buffer->idx < buffer->len && buffer->successful)
  {
    unsigned int syllable = buffer->cur().syllable();
    if (unlikely (last_syllable != syllable && (syllable & 0x0F) == broken_syllable_type))
    {
      last_syllable = syllable;

      hb_glyph_info_t ginfo = dottedcircle;
      ginfo.cluster = buffer->cur().cluster;
      ginfo.mask = buffer->cur().mask;
      ginfo.syllable() = buffer->cur().syllable();

      /* Insert dottedcircle after possible Repha. */
      if (repha_category != -1)
      {
	while (buffer->idx < buffer->len && buffer->successful &&
	       last_syllable == buffer->cur().syllable() &&
	       buffer->cur().ot_shaper_var_u8_category() == (unsigned) repha_category)
	  (void) buffer->next_glyph ();
      }

      (void) buffer->output_info (ginfo);
    }
    else
      (void) buffer->next_glyph ();
  }
  buffer->sync ();

  if (buffer->messaging ())
    (void) buffer->message (font, "end inserting dotted-circles");

  return true;
}

/* foreach_syllable() cuts syllables longer than HB_MAX_SYLLABLE_LENGTH,
 * but the cut is computed from the current glyph positions and is not
 * recorded, so walks before and after substitutions see different pieces.
 * Record the cut in syllable(), which substituted glyphs inherit, by giving
 * each piece its own serial.  The pieces are not syllables on their own and
 * text must not be broken between them, so keep them in one cluster. */
HB_INTERNAL void
hb_syllabic_limit_syllable_length (hb_buffer_t *buffer)
{
  hb_glyph_info_t *info = buffer->info;
  unsigned int syllable_serial = 1;
  unsigned int first_piece = 0;
  foreach_syllable (buffer, start, end)
  {
    unsigned int syllable = info[start].syllable();
    if (likely (end == buffer->len || info[end].syllable() != syllable))
    {
      /* Last piece of this syllable. */
      if (unlikely (first_piece < start))
	buffer->merge_clusters (first_piece, end);
      first_piece = end;
    }
    for (unsigned int i = start; i < end; i++)
      info[i].syllable() = (syllable_serial << 4) | (syllable & 0x0F);
    syllable_serial++;
    if (syllable_serial == 16) syllable_serial = 1;
  }
}

HB_INTERNAL bool
hb_syllabic_clear_var (const hb_ot_shape_plan_t *plan,
		       hb_font_t *font,
		       hb_buffer_t *buffer)
{
  HB_BUFFER_DEALLOCATE_VAR (buffer, syllable);
  return false;
}


#endif
