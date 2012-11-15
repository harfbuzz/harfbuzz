/*
 * Copyright © 2010,2012  Google, Inc.
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

#include "hb-ot-shape-complex-private.hh"


/* TODO Add kana, and other small shapers here */


/* The default shaper *only* adds additional per-script features.*/

static const hb_tag_t hangul_features[] =
{
  HB_TAG('l','j','m','o'),
  HB_TAG('v','j','m','o'),
  HB_TAG('t','j','m','o'),
  HB_TAG_NONE
};

static const hb_tag_t tibetan_features[] =
{
  HB_TAG('a','b','v','s'),
  HB_TAG('b','l','w','s'),
  HB_TAG('a','b','v','m'),
  HB_TAG('b','l','w','m'),
  HB_TAG_NONE
};

static void
collect_features_default (hb_ot_shape_planner_t *plan)
{
  const hb_tag_t *script_features = NULL;

  switch ((hb_tag_t) plan->props.script)
  {
    /* Unicode-1.1 additions */
    case HB_SCRIPT_HANGUL:
      script_features = hangul_features;
      break;

    /* Unicode-2.0 additions */
    case HB_SCRIPT_TIBETAN:
      script_features = tibetan_features;
      break;
  }

  for (; script_features && *script_features; script_features++)
    plan->map.add_bool_feature (*script_features);
}

static hb_ot_shape_normalization_mode_t
normalization_preference_default (const hb_segment_properties_t *props)
{
  switch ((hb_tag_t) props->script)
  {
    /* Unicode-1.1 additions */
    case HB_SCRIPT_HANGUL:
      return HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_FULL;
  }
  return HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_DIACRITICS;
}

static hb_bool_t
compose_default (hb_unicode_funcs_t *unicode,
		 hb_codepoint_t  a,
		 hb_codepoint_t  b,
		 hb_codepoint_t *ab)
{
  /* Hebrew presentation-form shaping.
   * https://bugzilla.mozilla.org/show_bug.cgi?id=728866 */
  // Hebrew presentation forms with dagesh, for characters 0x05D0..0x05EA;
  // note that some letters do not have a dagesh presForm encoded
  static const hb_codepoint_t sDageshForms[0x05EA - 0x05D0 + 1] = {
    0xFB30, // ALEF
    0xFB31, // BET
    0xFB32, // GIMEL
    0xFB33, // DALET
    0xFB34, // HE
    0xFB35, // VAV
    0xFB36, // ZAYIN
    0, // HET
    0xFB38, // TET
    0xFB39, // YOD
    0xFB3A, // FINAL KAF
    0xFB3B, // KAF
    0xFB3C, // LAMED
    0, // FINAL MEM
    0xFB3E, // MEM
    0, // FINAL NUN
    0xFB40, // NUN
    0xFB41, // SAMEKH
    0, // AYIN
    0xFB43, // FINAL PE
    0xFB44, // PE
    0, // FINAL TSADI
    0xFB46, // TSADI
    0xFB47, // QOF
    0xFB48, // RESH
    0xFB49, // SHIN
    0xFB4A // TAV
  };

  hb_bool_t found = unicode->compose (a, b, ab);

  if (!found && (b & ~0x7F) == 0x0580) {
      // special-case Hebrew presentation forms that are excluded from
      // standard normalization, but wanted for old fonts
      switch (b) {
      case 0x05B4: // HIRIQ
	  if (a == 0x05D9) { // YOD
	      *ab = 0xFB1D;
	      found = true;
	  }
	  break;
      case 0x05B7: // patah
	  if (a == 0x05F2) { // YIDDISH YOD YOD
	      *ab = 0xFB1F;
	      found = true;
	  } else if (a == 0x05D0) { // ALEF
	      *ab = 0xFB2E;
	      found = true;
	  }
	  break;
      case 0x05B8: // QAMATS
	  if (a == 0x05D0) { // ALEF
	      *ab = 0xFB2F;
	      found = true;
	  }
	  break;
      case 0x05B9: // HOLAM
	  if (a == 0x05D5) { // VAV
	      *ab = 0xFB4B;
	      found = true;
	  }
	  break;
      case 0x05BC: // DAGESH
	  if (a >= 0x05D0 && a <= 0x05EA) {
	      *ab = sDageshForms[a - 0x05D0];
	      found = (*ab != 0);
	  } else if (a == 0xFB2A) { // SHIN WITH SHIN DOT
	      *ab = 0xFB2C;
	      found = true;
	  } else if (a == 0xFB2B) { // SHIN WITH SIN DOT
	      *ab = 0xFB2D;
	      found = true;
	  }
	  break;
      case 0x05BF: // RAFE
	  switch (a) {
	  case 0x05D1: // BET
	      *ab = 0xFB4C;
	      found = true;
	      break;
	  case 0x05DB: // KAF
	      *ab = 0xFB4D;
	      found = true;
	      break;
	  case 0x05E4: // PE
	      *ab = 0xFB4E;
	      found = true;
	      break;
	  }
	  break;
      case 0x05C1: // SHIN DOT
	  if (a == 0x05E9) { // SHIN
	      *ab = 0xFB2A;
	      found = true;
	  } else if (a == 0xFB49) { // SHIN WITH DAGESH
	      *ab = 0xFB2C;
	      found = true;
	  }
	  break;
      case 0x05C2: // SIN DOT
	  if (a == 0x05E9) { // SHIN
	      *ab = 0xFB2B;
	      found = true;
	  } else if (a == 0xFB49) { // SHIN WITH DAGESH
	      *ab = 0xFB2D;
	      found = true;
	  }
	  break;
      }
  }

  return found;
}

const hb_ot_complex_shaper_t _hb_ot_complex_shaper_default =
{
  "default",
  collect_features_default,
  NULL, /* override_features */
  NULL, /* data_create */
  NULL, /* data_destroy */
  NULL, /* preprocess_text */
  normalization_preference_default,
  NULL, /* decompose */
  compose_default,
  NULL, /* setup_masks */
  true, /* zero_width_attached_marks */
};


/* Thai / Lao shaper */

static void
preprocess_text_thai (const hb_ot_shape_plan_t *plan HB_UNUSED,
		      hb_buffer_t              *buffer,
		      hb_font_t                *font HB_UNUSED)
{
  /* The following is NOT specified in the MS OT Thai spec, however, it seems
   * to be what Uniscribe and other engines implement.  According to Eric Muller:
   *
   * When you have a SARA AM, decompose it in NIKHAHIT + SARA AA, *and* move the
   * NIKHAHIT backwards over any tone mark (0E48-0E4B).
   *
   * <0E14, 0E4B, 0E33> -> <0E14, 0E4D, 0E4B, 0E32>
   *
   * This reordering is legit only when the NIKHAHIT comes from a SARA AM, not
   * when it's there to start with. The string <0E14, 0E4B, 0E4D> is probably
   * not what a user wanted, but the rendering is nevertheless nikhahit above
   * chattawa.
   *
   * Same for Lao.
   *
   * Note:
   *
   * Uniscribe also does so below-marks reordering.  Namely, it positions U+0E3A
   * after U+0E38 and U+0E39.  We do that by modifying the ccc for U+0E3A.
   * See unicode->modified_combining_class ().  Lao does NOT have a U+0E3A
   * equivalent.
   */


  /*
   * Here are the characters of significance:
   *
   *			Thai	Lao
   * SARA AM:		U+0E33	U+0EB3
   * SARA AA:		U+0E32	U+0EB2
   * Nikhahit:		U+0E4D	U+0ECD
   *
   * Testing shows that Uniscribe reorder the following marks:
   * Thai:	<0E31,0E34..0E37,0E47..0E4E>
   * Lao:	<0EB1,0EB4..0EB7,0EC7..0ECE>
   *
   * Note how the Lao versions are the same as Thai + 0x80.
   */

  /* We only get one script at a time, so a script-agnostic implementation
   * is adequate here. */
#define IS_SARA_AM(x) (((x) & ~0x0080) == 0x0E33)
#define NIKHAHIT_FROM_SARA_AM(x) ((x) - 0xE33 + 0xE4D)
#define SARA_AA_FROM_SARA_AM(x) ((x) - 1)
#define IS_TONE_MARK(x) (hb_in_ranges<hb_codepoint_t> ((x) & ~0x0080, 0x0E34, 0x0E37, 0x0E47, 0x0E4E, 0x0E31, 0x0E31))

  buffer->clear_output ();
  unsigned int count = buffer->len;
  for (buffer->idx = 0; buffer->idx < count;)
  {
    hb_codepoint_t u = buffer->cur().codepoint;
    if (likely (!IS_SARA_AM (u))) {
      buffer->next_glyph ();
      continue;
    }

    /* Is SARA AM. Decompose and reorder. */
    hb_codepoint_t decomposed[2] = {hb_codepoint_t (NIKHAHIT_FROM_SARA_AM (u)),
				    hb_codepoint_t (SARA_AA_FROM_SARA_AM (u))};
    buffer->replace_glyphs (1, 2, decomposed);
    if (unlikely (buffer->in_error))
      return;

    /* Ok, let's see... */
    unsigned int end = buffer->out_len;
    unsigned int start = end - 2;
    while (start > 0 && IS_TONE_MARK (buffer->out_info[start - 1].codepoint))
      start--;

    if (start + 2 < end)
    {
      /* Move Nikhahit (end-2) to the beginning */
      buffer->merge_out_clusters (start, end);
      hb_glyph_info_t t = buffer->out_info[end - 2];
      memmove (buffer->out_info + start + 1,
	       buffer->out_info + start,
	       sizeof (buffer->out_info[0]) * (end - start - 2));
      buffer->out_info[start] = t;
    }
    else
    {
      /* Since we decomposed, and NIKHAHIT is combining, merge clusters with the
       * previous cluster. */
      if (start)
	buffer->merge_out_clusters (start - 1, end);
    }
  }
  buffer->swap_buffers ();
}

const hb_ot_complex_shaper_t _hb_ot_complex_shaper_thai =
{
  "thai",
  NULL, /* collect_features */
  NULL, /* override_features */
  NULL, /* data_create */
  NULL, /* data_destroy */
  preprocess_text_thai,
  NULL, /* normalization_preference */
  NULL, /* decompose */
  NULL, /* compose */
  NULL, /* setup_masks */
  true, /* zero_width_attached_marks */
};
