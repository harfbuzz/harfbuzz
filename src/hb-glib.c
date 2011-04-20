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

#include "hb-unicode-private.h"

#include <glib.h>

HB_BEGIN_DECLS


static hb_codepoint_t
hb_glib_get_mirroring (hb_unicode_funcs_t *ufuncs,
                       hb_codepoint_t      unicode,
                       void               *user_data)
{
  g_unichar_get_mirror_char (unicode, &unicode);
  return unicode;
}

static hb_unicode_general_category_t
hb_glib_get_general_category (hb_unicode_funcs_t *ufuncs,
                              hb_codepoint_t      unicode,
                              void               *user_data)

{
  return g_unichar_type (unicode);
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

  MATCH_SCRIPT (COMMON);             /* Zyyy */
  MATCH_SCRIPT (INHERITED);          /* Qaai */
  MATCH_SCRIPT (ARABIC);             /* Arab */
  MATCH_SCRIPT (ARMENIAN);           /* Armn */
  MATCH_SCRIPT (BENGALI);            /* Beng */
  MATCH_SCRIPT (BOPOMOFO);           /* Bopo */
  MATCH_SCRIPT (CHEROKEE);           /* Cher */
  MATCH_SCRIPT (COPTIC);             /* Qaac */
  MATCH_SCRIPT (CYRILLIC);           /* Cyrl (Cyrs) */
  MATCH_SCRIPT (DESERET);            /* Dsrt */
  MATCH_SCRIPT (DEVANAGARI);         /* Deva */
  MATCH_SCRIPT (ETHIOPIC);           /* Ethi */
  MATCH_SCRIPT (GEORGIAN);           /* Geor (Geon); Geoa) */
  MATCH_SCRIPT (GOTHIC);             /* Goth */
  MATCH_SCRIPT (GREEK);              /* Grek */
  MATCH_SCRIPT (GUJARATI);           /* Gujr */
  MATCH_SCRIPT (GURMUKHI);           /* Guru */
  MATCH_SCRIPT (HAN);                /* Hani */
  MATCH_SCRIPT (HANGUL);             /* Hang */
  MATCH_SCRIPT (HEBREW);             /* Hebr */
  MATCH_SCRIPT (HIRAGANA);           /* Hira */
  MATCH_SCRIPT (KANNADA);            /* Knda */
  MATCH_SCRIPT (KATAKANA);           /* Kana */
  MATCH_SCRIPT (KHMER);              /* Khmr */
  MATCH_SCRIPT (LAO);                /* Laoo */
  MATCH_SCRIPT (LATIN);              /* Latn (Latf); Latg) */
  MATCH_SCRIPT (MALAYALAM);          /* Mlym */
  MATCH_SCRIPT (MONGOLIAN);          /* Mong */
  MATCH_SCRIPT (MYANMAR);            /* Mymr */
  MATCH_SCRIPT (OGHAM);              /* Ogam */
  MATCH_SCRIPT (OLD_ITALIC);         /* Ital */
  MATCH_SCRIPT (ORIYA);              /* Orya */
  MATCH_SCRIPT (RUNIC);              /* Runr */
  MATCH_SCRIPT (SINHALA);            /* Sinh */
  MATCH_SCRIPT (SYRIAC);             /* Syrc (Syrj, Syrn); Syre) */
  MATCH_SCRIPT (TAMIL);              /* Taml */
  MATCH_SCRIPT (TELUGU);             /* Telu */
  MATCH_SCRIPT (THAANA);             /* Thaa */
  MATCH_SCRIPT (THAI);               /* Thai */
  MATCH_SCRIPT (TIBETAN);            /* Tibt */
  MATCH_SCRIPT (CANADIAN_ABORIGINAL);/* Cans */
  MATCH_SCRIPT (YI);                 /* Yiii */
  MATCH_SCRIPT (TAGALOG);            /* Tglg */
  MATCH_SCRIPT (HANUNOO);            /* Hano */
  MATCH_SCRIPT (BUHID);              /* Buhd */
  MATCH_SCRIPT (TAGBANWA);           /* Tagb */

  /* Unicode-4.0 additions */
  MATCH_SCRIPT (BRAILLE);            /* Brai */
  MATCH_SCRIPT (CYPRIOT);            /* Cprt */
  MATCH_SCRIPT (LIMBU);              /* Limb */
  MATCH_SCRIPT (OSMANYA);            /* Osma */
  MATCH_SCRIPT (SHAVIAN);            /* Shaw */
  MATCH_SCRIPT (LINEAR_B);           /* Linb */
  MATCH_SCRIPT (TAI_LE);             /* Tale */
  MATCH_SCRIPT (UGARITIC);           /* Ugar */

  /* Unicode-4.1 additions */
  MATCH_SCRIPT (NEW_TAI_LUE);        /* Talu */
  MATCH_SCRIPT (BUGINESE);           /* Bugi */
  MATCH_SCRIPT (GLAGOLITIC);         /* Glag */
  MATCH_SCRIPT (TIFINAGH);           /* Tfng */
  MATCH_SCRIPT (SYLOTI_NAGRI);       /* Sylo */
  MATCH_SCRIPT (OLD_PERSIAN);        /* Xpeo */
  MATCH_SCRIPT (KHAROSHTHI);         /* Khar */

  /* Unicode-5.0 additions */
  MATCH_SCRIPT (UNKNOWN);            /* Zzzz */
  MATCH_SCRIPT (BALINESE);           /* Bali */
  MATCH_SCRIPT (CUNEIFORM);          /* Xsux */
  MATCH_SCRIPT (PHOENICIAN);         /* Phnx */
  MATCH_SCRIPT (PHAGS_PA);           /* Phag */
  MATCH_SCRIPT (NKO);                /* Nkoo */

  /* Unicode-5.1 additions */
  MATCH_SCRIPT (KAYAH_LI);           /* Kali */
  MATCH_SCRIPT (LEPCHA);             /* Lepc */
  MATCH_SCRIPT (REJANG);             /* Rjng */
  MATCH_SCRIPT (SUNDANESE);          /* Sund */
  MATCH_SCRIPT (SAURASHTRA);         /* Saur */
  MATCH_SCRIPT (CHAM);               /* Cham */
  MATCH_SCRIPT (OL_CHIKI);           /* Olck */
  MATCH_SCRIPT (VAI);                /* Vaii */
  MATCH_SCRIPT (CARIAN);             /* Cari */
  MATCH_SCRIPT (LYCIAN);             /* Lyci */
  MATCH_SCRIPT (LYDIAN);             /* Lydi */

  /* Unicode-5.2 additions */
#if GLIB_CHECK_VERSION(2,26,0)
  MATCH_SCRIPT (AVESTAN);                /* Avst */
  MATCH_SCRIPT (BAMUM);                  /* Bamu */
  MATCH_SCRIPT (EGYPTIAN_HIEROGLYPHS);   /* Egyp */
  MATCH_SCRIPT (IMPERIAL_ARAMAIC);       /* Armi */
  MATCH_SCRIPT (INSCRIPTIONAL_PAHLAVI);  /* Phli */
  MATCH_SCRIPT (INSCRIPTIONAL_PARTHIAN); /* Prti */
  MATCH_SCRIPT (JAVANESE);               /* Java */
  MATCH_SCRIPT (KAITHI);                 /* Kthi */
  MATCH_SCRIPT (TAI_THAM);               /* Lana */
  MATCH_SCRIPT (LISU);                   /* Lisu */
  MATCH_SCRIPT (MEETEI_MAYEK);           /* Mtei */
  MATCH_SCRIPT (OLD_SOUTH_ARABIAN);      /* Sarb */
#if GLIB_CHECK_VERSION(2,28,0)
  MATCH_SCRIPT (OLD_TURKIC);             /* Orkh */
#else
  MATCH_SCRIPT2(OLD_TURKISH, OLD_TURKIC);/* Orkh */
#endif
  MATCH_SCRIPT (SAMARITAN);              /* Samr */
  MATCH_SCRIPT (TAI_VIET);               /* Tavt */
#endif

  /* Unicode-6.0 additions */
#if GLIB_CHECK_VERSION(2,28,0)
  MATCH_SCRIPT (BATAK);                  /* Batk */
  MATCH_SCRIPT (BRAHMI);                 /* Brah */
  MATCH_SCRIPT (MANDAIC);                /* Mand */
#endif

#undef MATCH_SCRIPT
#undef MATCH_SCRIPT2
  }

  return HB_SCRIPT_UNKNOWN;
}

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

static hb_unicode_funcs_t glib_ufuncs = {
  HB_REFERENCE_COUNT_INVALID, /* ref_count */
  NULL,
  TRUE, /* immutable */
  {
    hb_glib_get_general_category, NULL, NULL,
    hb_glib_get_combining_class, NULL, NULL,
    hb_glib_get_mirroring, NULL, NULL,
    hb_glib_get_script, NULL, NULL,
    hb_glib_get_eastasian_width, NULL, NULL
  }
};

hb_unicode_funcs_t *
hb_glib_get_unicode_funcs (void)
{
  return &glib_ufuncs;
}


HB_END_DECLS
