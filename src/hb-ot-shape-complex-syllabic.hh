/*
 * Copyright © 2020  Google, Inc.
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
 * Author(s): Simon Cozens
 */

#ifndef HB_OT_SHAPE_COMPLEX_SYLLABIC_HH
#define HB_OT_SHAPE_COMPLEX_SYLLABIC_HH

#include "hb.hh"

#include "hb-ot-shape-complex.hh"

enum syllable_type_t {
  broken_cluster,
  alien_cluster,
  consonant_syllable,
  punctuation_cluster,
  indic_vowel_syllable,
  indic_standalone_cluster,
  indic_symbol_cluster,
  use_independent_cluster,
  use_virama_terminated_cluster,
  use_sakot_terminated_cluster,
  use_standard_cluster,
  use_number_joiner_terminated_cluster,
  use_numeral_cluster,
  use_symbol_cluster,
  use_hieroglyph_cluster
};

void
hb_syllabic_insert_dotted_circles (hb_buffer_t *buffer, hb_font_t *font,
			       int repha_category,
			       void (*set_properties)(hb_glyph_info_t &info));

#endif /* HB_OT_SHAPE_COMPLEX_SYLLABIC_HH */
