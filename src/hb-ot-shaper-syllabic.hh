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

#ifndef HB_OT_SHAPER_SYLLABIC_HH
#define HB_OT_SHAPER_SYLLABIC_HH

#include "hb.hh"

#include "hb-ot-shaper.hh"


HB_INTERNAL bool
hb_syllabic_insert_dotted_circles (hb_font_t *font,
				   hb_buffer_t *buffer,
				   unsigned int broken_syllable_type,
				   unsigned int dottedcircle_category,
				   int repha_category = -1,
				   int dottedcircle_position = -1);

HB_INTERNAL void
hb_syllabic_record_cut_slow (hb_buffer_t *buffer,
			     unsigned int start,
			     unsigned int end);

/* Call for each piece [start, end) that foreach_syllable() yields.
 * foreach_syllable() cuts syllables longer than HB_MAX_SYLLABLE_LENGTH;
 * this records such cuts so later walks see the same pieces.  Only does
 * work for pieces of HB_MAX_SYLLABLE_LENGTH glyphs. */
static inline void
hb_syllabic_record_cut (hb_buffer_t *buffer,
			unsigned int start,
			unsigned int end)
{
  if (unlikely (end - start >= HB_MAX_SYLLABLE_LENGTH))
    hb_syllabic_record_cut_slow (buffer, start, end);
}

HB_INTERNAL bool
hb_syllabic_clear_var (const hb_ot_shape_plan_t *plan,
		       hb_font_t *font,
		       hb_buffer_t *buffer);


#endif /* HB_OT_SHAPER_SYLLABIC_HH */
