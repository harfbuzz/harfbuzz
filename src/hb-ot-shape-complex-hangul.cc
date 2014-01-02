/*
 * Copyright © 2013  Google, Inc.
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


/* Hangul shaper */


static const hb_tag_t hangul_features[] =
{
  HB_TAG('l','j','m','o'),
  HB_TAG('v','j','m','o'),
  HB_TAG('t','j','m','o'),
  HB_TAG_NONE
};

static void
collect_features_hangul (hb_ot_shape_planner_t *plan)
{
  for (const hb_tag_t *script_features = hangul_features; script_features && *script_features; script_features++)
    plan->map.add_global_bool_feature (*script_features);
}

#define LBase 0x1100
#define VBase 0x1161
#define TBase 0x11A7
#define LCount 19
#define VCount 21
#define TCount 28
#define SBase 0xAC00
#define NCount (VCount * TCount)
#define SCount (LCount * NCount)

#define isCombiningL(u) (hb_in_range<hb_codepoint_t> ((u), LBase, LBase+LCount-1))
#define isCombiningV(u) (hb_in_range<hb_codepoint_t> ((u), VBase, VBase+VCount-1))
#define isCombiningT(u) (hb_in_range<hb_codepoint_t> ((u), TBase+1, TBase+TCount-1))
#define isCombinedS(u) (hb_in_range<hb_codepoint_t> ((u), SBase, SBase+SCount-1))

#define isT(u) (hb_in_ranges<hb_codepoint_t> ((u),  0x11A8, 0x11FF, 0xD7CB, 0xD7FB))

static void
preprocess_text_hangul (const hb_ot_shape_plan_t *plan,
			hb_buffer_t              *buffer,
			hb_font_t                *font)
{
  /* Hangul syllables come in two shapes: LV, and LVT.  Of those:
   *
   *   - LV can be precomposed, or decomposed.  Lets call those
   *     <LV> and <L,V>,
   *   - LVT can be fully precomposed, partically precomposed, or
   *     fully decomposed.  Ie. <LVT>, <LV,T>, or <L,V,T>.
   *
   * The composition / decomposition is mechanical.  However, not
   * all <L,V> sequences compose, and not all <LV,T> sequences
   * compose.
   *
   * Here are the specifics:
   *
   *   - <L>: U+1100..115F, U+A960..A97F
   *   - <V>: U+1160..11A7, U+D7B0..D7C7
   *   - <T>: U+11A8..11FF, U+D7CB..D7FB
   *
   *   - Only the <L,V> sequences for the 11xx ranges combine.
   *   - Only <LV,T> sequences for T in U+11A8..11C3 combine.
   *
   * Here is what we want to accomplish in this shaper:
   *
   *   - If the whole syllable can be precomposed, do that,
   *   - Otherwise, fully decompose.
   *
   * That is, of the different possible syllables:
   *
   *   <L>
   *   <L,V>
   *   <L,V,T>
   *   <LV>
   *   <LVT>
   *   <LV, T>
   *
   * - <L> needs no work.
   *
   * - <LV> and <LVT> can stay the way they are if the font supports them, otherwise we
   *   should fully decompose them if font supports.
   *
   * - <L,V> and <L,V,T> we should compose if the whole thing can be composed.
   *
   * - <LV,T> we should compose if the whole thing can be composed, otherwise we should
   *   decompose.
   */

  buffer->clear_output ();
  unsigned int count = buffer->len;
  for (buffer->idx = 0; buffer->idx < count;)
  {
    hb_codepoint_t u = buffer->cur().codepoint;

    if (isCombiningL(u) && buffer->idx + 1 < count)
    {
      hb_codepoint_t l = u;
      hb_codepoint_t v = buffer->cur(+1).codepoint;
      if (isCombiningV(v))
      {
        /* Have <L,V> or <L,V,T>. */
        unsigned int len = 2;
	unsigned int tindex = 0;
	if (buffer->idx + 2 < count)
	{
	  hb_codepoint_t t = buffer->cur(+2).codepoint;
	  if (isCombiningT(t))
	  {
	    len = 3;
	    tindex = t - TBase;
	  }
	  else if (isT (t))
	  {
	    /* Old T jamo.  Doesn't combine.  Don't combine *anything*. */
	   len = 0;
	  }
	}

	if (len)
	{
	  hb_codepoint_t s = SBase + (l - LBase) * NCount + (v - VBase) * TCount + tindex;
	  if (font->has_glyph (s))
	  {
	    buffer->replace_glyphs (len, 1, &s);
	    if (unlikely (buffer->in_error))
	      return;
	    continue;
	  }
	}
      }
    }

    else if (isCombinedS(u))
    {
       /* Have <LV>, <LVT>, or <LV,T> */
      hb_codepoint_t s = u;
      bool has_glyph = font->has_glyph (s);
      unsigned int lindex = (s - SBase) / NCount;
      unsigned int nindex = (s - SBase) % NCount;
      unsigned int vindex = nindex / TCount;
      unsigned int tindex = nindex % TCount;

      if (!tindex &&
	  buffer->idx + 1 < count &&
	  isCombiningT (buffer->cur(+1).codepoint))
      {
	/* <LV,T>, try to combine. */
	unsigned int new_tindex = buffer->cur(+1).codepoint - TBase;
	hb_codepoint_t new_s = s + new_tindex;
        if (font->has_glyph (new_s))
	{
	  buffer->replace_glyphs (2, 1, &new_s);
	  if (unlikely (buffer->in_error))
	    return;
	  continue;
	}
      }

      /* Otherwise, decompose if font doesn't support <LV> or <LVT>,
       * or if having non-combining <LV,T>.  Note that we already handled
       * combining <LV,T> above. */
      if (!has_glyph ||
	  (!tindex &&
	   buffer->idx + 1 < count &&
	   isT (buffer->cur(+1).codepoint)))
      {
	hb_codepoint_t decomposed[3] = {LBase + lindex,
					VBase + vindex,
					TBase + tindex};
        if (font->has_glyph (decomposed[0]) &&
	    font->has_glyph (decomposed[1]) &&
	    (!tindex || font->has_glyph (decomposed[2])))
	{
	  buffer->replace_glyphs (1, tindex ? 3 : 2, decomposed);
	  if (unlikely (buffer->in_error))
	    return;
	  continue;
	}
      }
    }

    buffer->next_glyph ();
  }
  buffer->swap_buffers ();
}

const hb_ot_complex_shaper_t _hb_ot_complex_shaper_hangul =
{
  "hangul",
  collect_features_hangul,
  NULL, /* override_features */
  NULL, /* data_create */
  NULL, /* data_destroy */
  preprocess_text_hangul,
  HB_OT_SHAPE_NORMALIZATION_MODE_NONE,
  NULL, /* decompose */
  NULL, /* compose */
  NULL, /* setup_masks */
  HB_OT_SHAPE_ZERO_WIDTH_MARKS_DEFAULT,
  false, /* fallback_position */
};
