/*
 * Copyright (C) 2009  Red Hat, Inc.
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
 * Red Hat Author(s): Behdad Esfahbod
 */

#include "hb-private.h"

#include "hb-glib.h"

#include "hb-unicode-private.hh"

#include <glib.h>

HB_BEGIN_DECLS


static unsigned int
hb_glib_get_combining_class (hb_unicode_funcs_t *ufuncs,
                             hb_codepoint_t      unicode,
                             void               *user_data)

{
  return g_unichar_combining_class (unicode);
}

static unsigned int
hb_glib_get_eastasian_width (hb_unicode_funcs_t *ufuncs,
                             hb_codepoint_t      unicode,
                             void               *user_data)
{
  return g_unichar_iswide (unicode) ? 2 : 1;
}

static hb_unicode_general_category_t
hb_glib_get_general_category (hb_unicode_funcs_t *ufuncs,
                              hb_codepoint_t      unicode,
                              void               *user_data)

{
  /* hb_unicode_general_category_t and GUnicodeType are identical */
  return (hb_unicode_general_category_t) g_unichar_type (unicode);
}

static hb_codepoint_t
hb_glib_get_mirroring (hb_unicode_funcs_t *ufuncs,
                       hb_codepoint_t      unicode,
                       void               *user_data)
{
  g_unichar_get_mirror_char (unicode, &unicode);
  return unicode;
}

static hb_script_t
hb_glib_get_script (hb_unicode_funcs_t *ufuncs,
                    hb_codepoint_t      unicode,
                    void               *user_data)
{
  GUnicodeScript script = g_unichar_get_script (unicode);
  switch (script)
  {
#define MATCH_SCRIPT(C) case G_UNICODE_SCRIPT_##C: return HB_SCRIPT_##C
#define MATCH_SCRIPT2(C1, C2) case G_UNICODE_SCRIPT_##C1: return HB_SCRIPT_##C2

  MATCH_SCRIPT2(INVALID_CODE, INVALID);

  MATCH_SCRIPT (COMMON);
  MATCH_SCRIPT (INHERITED);
  MATCH_SCRIPT (ARABIC);
  MATCH_SCRIPT (ARMENIAN);
  MATCH_SCRIPT (BENGALI);
  MATCH_SCRIPT (BOPOMOFO);
  MATCH_SCRIPT (CHEROKEE);
  MATCH_SCRIPT (COPTIC);
  MATCH_SCRIPT (CYRILLIC);
  MATCH_SCRIPT (DESERET);
  MATCH_SCRIPT (DEVANAGARI);
  MATCH_SCRIPT (ETHIOPIC);
  MATCH_SCRIPT (GEORGIAN);
  MATCH_SCRIPT (GOTHIC);
  MATCH_SCRIPT (GREEK);
  MATCH_SCRIPT (GUJARATI);
  MATCH_SCRIPT (GURMUKHI);
  MATCH_SCRIPT (HAN);
  MATCH_SCRIPT (HANGUL);
  MATCH_SCRIPT (HEBREW);
  MATCH_SCRIPT (HIRAGANA);
  MATCH_SCRIPT (KANNADA);
  MATCH_SCRIPT (KATAKANA);
  MATCH_SCRIPT (KHMER);
  MATCH_SCRIPT (LAO);
  MATCH_SCRIPT (LATIN);
  MATCH_SCRIPT (MALAYALAM);
  MATCH_SCRIPT (MONGOLIAN);
  MATCH_SCRIPT (MYANMAR);
  MATCH_SCRIPT (OGHAM);
  MATCH_SCRIPT (OLD_ITALIC);
  MATCH_SCRIPT (ORIYA);
  MATCH_SCRIPT (RUNIC);
  MATCH_SCRIPT (SINHALA);
  MATCH_SCRIPT (SYRIAC);
  MATCH_SCRIPT (TAMIL);
  MATCH_SCRIPT (TELUGU);
  MATCH_SCRIPT (THAANA);
  MATCH_SCRIPT (THAI);
  MATCH_SCRIPT (TIBETAN);
  MATCH_SCRIPT (CANADIAN_ABORIGINAL);
  MATCH_SCRIPT (YI);
  MATCH_SCRIPT (TAGALOG);
  MATCH_SCRIPT (HANUNOO);
  MATCH_SCRIPT (BUHID);
  MATCH_SCRIPT (TAGBANWA);

  /* Unicode-4.0 additions */
  MATCH_SCRIPT (BRAILLE);
  MATCH_SCRIPT (CYPRIOT);
  MATCH_SCRIPT (LIMBU);
  MATCH_SCRIPT (OSMANYA);
  MATCH_SCRIPT (SHAVIAN);
  MATCH_SCRIPT (LINEAR_B);
  MATCH_SCRIPT (TAI_LE);
  MATCH_SCRIPT (UGARITIC);

  /* Unicode-4.1 additions */
  MATCH_SCRIPT (NEW_TAI_LUE);
  MATCH_SCRIPT (BUGINESE);
  MATCH_SCRIPT (GLAGOLITIC);
  MATCH_SCRIPT (TIFINAGH);
  MATCH_SCRIPT (SYLOTI_NAGRI);
  MATCH_SCRIPT (OLD_PERSIAN);
  MATCH_SCRIPT (KHAROSHTHI);

  /* Unicode-5.0 additions */
  MATCH_SCRIPT (UNKNOWN);
  MATCH_SCRIPT (BALINESE);
  MATCH_SCRIPT (CUNEIFORM);
  MATCH_SCRIPT (PHOENICIAN);
  MATCH_SCRIPT (PHAGS_PA);
  MATCH_SCRIPT (NKO);

  /* Unicode-5.1 additions */
  MATCH_SCRIPT (KAYAH_LI);
  MATCH_SCRIPT (LEPCHA);
  MATCH_SCRIPT (REJANG);
  MATCH_SCRIPT (SUNDANESE);
  MATCH_SCRIPT (SAURASHTRA);
  MATCH_SCRIPT (CHAM);
  MATCH_SCRIPT (OL_CHIKI);
  MATCH_SCRIPT (VAI);
  MATCH_SCRIPT (CARIAN);
  MATCH_SCRIPT (LYCIAN);
  MATCH_SCRIPT (LYDIAN);

  /* Unicode-5.2 additions */
#if GLIB_CHECK_VERSION(2,26,0)
  MATCH_SCRIPT (AVESTAN);
  MATCH_SCRIPT (BAMUM);
  MATCH_SCRIPT (EGYPTIAN_HIEROGLYPHS);
  MATCH_SCRIPT (IMPERIAL_ARAMAIC);
  MATCH_SCRIPT (INSCRIPTIONAL_PAHLAVI);
  MATCH_SCRIPT (INSCRIPTIONAL_PARTHIAN);
  MATCH_SCRIPT (JAVANESE);
  MATCH_SCRIPT (KAITHI);
  MATCH_SCRIPT (TAI_THAM);
  MATCH_SCRIPT (LISU);
  MATCH_SCRIPT (MEETEI_MAYEK);
  MATCH_SCRIPT (OLD_SOUTH_ARABIAN);
#if GLIB_CHECK_VERSION(2,27,92)
  MATCH_SCRIPT (OLD_TURKIC);
#else
  MATCH_SCRIPT2(OLD_TURKISH, OLD_TURKIC);
#endif
  MATCH_SCRIPT (SAMARITAN);
  MATCH_SCRIPT (TAI_VIET);
#endif

  /* Unicode-6.0 additions */
#if GLIB_CHECK_VERSION(2,28,0)
  MATCH_SCRIPT (BATAK);
  MATCH_SCRIPT (BRAHMI);
  MATCH_SCRIPT (MANDAIC);
#endif

#undef MATCH_SCRIPT
#undef MATCH_SCRIPT2
  }

  return HB_SCRIPT_UNKNOWN;
}

static hb_unicode_funcs_t glib_ufuncs = {
  HB_REFERENCE_COUNT_INVALID, /* ref_count */
  NULL, /* parent */
  TRUE, /* immutable */
  {
    hb_glib_get_combining_class,
    hb_glib_get_eastasian_width,
    hb_glib_get_general_category,
    hb_glib_get_mirroring,
    hb_glib_get_script
  }
};

hb_unicode_funcs_t *
hb_glib_get_unicode_funcs (void)
{
  return &glib_ufuncs;
}


HB_END_DECLS
