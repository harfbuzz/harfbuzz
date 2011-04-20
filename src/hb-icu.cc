/*
 * Copyright (C) 2009  Red Hat, Inc.
 * Copyright (C) 2009  Keith Stribley
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

#include "hb-icu.h"

#include "hb-unicode-private.hh"

#include <unicode/uversion.h>
#include <unicode/uchar.h>

HB_BEGIN_DECLS


hb_script_t
hb_icu_script_to_script (UScriptCode script)
{
  return hb_script_from_string (uscript_getShortName (script));
}

UScriptCode
hb_icu_script_from_script (hb_script_t script)
{
  switch ((int) script)
  {
#define CHECK_ICU_VERSION(major, minor) \
	U_ICU_VERSION_MAJOR_NUM > (major) || (U_ICU_VERSION_MAJOR_NUM == (major) && U_ICU_VERSION_MINOR_NUM >= (minor))
#define MATCH_SCRIPT(C) case HB_SCRIPT_##C: return USCRIPT_##C
#define MATCH_SCRIPT2(C1, C2) case HB_SCRIPT_##C2: return USCRIPT_##C1

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
  MATCH_SCRIPT (AVESTAN);
#if CHECK_ICU_VERSION (4, 4)
  MATCH_SCRIPT (BAMUM);
#endif
  MATCH_SCRIPT (EGYPTIAN_HIEROGLYPHS);
  MATCH_SCRIPT (IMPERIAL_ARAMAIC);
  MATCH_SCRIPT (INSCRIPTIONAL_PAHLAVI);
  MATCH_SCRIPT (INSCRIPTIONAL_PARTHIAN);
  MATCH_SCRIPT (JAVANESE);
  MATCH_SCRIPT (KAITHI);
  MATCH_SCRIPT2(LANNA, TAI_THAM);
#if CHECK_ICU_VERSION (4, 4)
  MATCH_SCRIPT (LISU);
#endif
  MATCH_SCRIPT2(MEITEI_MAYEK, MEETEI_MAYEK);
#if CHECK_ICU_VERSION (4, 4)
  MATCH_SCRIPT (OLD_SOUTH_ARABIAN);
#endif
  MATCH_SCRIPT2(ORKHON, OLD_TURKIC);
  MATCH_SCRIPT (SAMARITAN);
  MATCH_SCRIPT (TAI_VIET);

  /* Unicode-6.0 additions */
  MATCH_SCRIPT (BATAK);
  MATCH_SCRIPT (BRAHMI);
  MATCH_SCRIPT2(MANDAEAN, MANDAIC);

#undef CHECK_ICU_VERSION
#undef MATCH_SCRIPT
#undef MATCH_SCRIPT2
  }

  return USCRIPT_UNKNOWN;
}


static unsigned int
hb_icu_get_combining_class (hb_unicode_funcs_t *ufuncs,
			    hb_codepoint_t      unicode,
			    void               *user_data)

{
  return u_getCombiningClass (unicode);
}

static unsigned int
hb_icu_get_eastasian_width (hb_unicode_funcs_t *ufuncs,
			    hb_codepoint_t      unicode,
			    void               *user_data)
{
  switch (u_getIntPropertyValue(unicode, UCHAR_EAST_ASIAN_WIDTH))
  {
  case U_EA_WIDE:
  case U_EA_FULLWIDTH:
    return 2;
  case U_EA_NEUTRAL:
  case U_EA_AMBIGUOUS:
  case U_EA_HALFWIDTH:
  case U_EA_NARROW:
    return 1;
  }
  return 1;
}

static hb_unicode_general_category_t
hb_icu_get_general_category (hb_unicode_funcs_t *ufuncs,
			     hb_codepoint_t      unicode,
			     void               *user_data)
{
  switch (u_getIntPropertyValue(unicode, UCHAR_GENERAL_CATEGORY))
  {
  case U_UNASSIGNED:			return HB_UNICODE_GENERAL_CATEGORY_UNASSIGNED;

  case U_UPPERCASE_LETTER:		return HB_UNICODE_GENERAL_CATEGORY_UPPERCASE_LETTER;
  case U_LOWERCASE_LETTER:		return HB_UNICODE_GENERAL_CATEGORY_LOWERCASE_LETTER;
  case U_TITLECASE_LETTER:		return HB_UNICODE_GENERAL_CATEGORY_TITLECASE_LETTER;
  case U_MODIFIER_LETTER:		return HB_UNICODE_GENERAL_CATEGORY_MODIFIER_LETTER;
  case U_OTHER_LETTER:			return HB_UNICODE_GENERAL_CATEGORY_OTHER_LETTER;

  case U_NON_SPACING_MARK:		return HB_UNICODE_GENERAL_CATEGORY_NON_SPACING_MARK;
  case U_ENCLOSING_MARK:		return HB_UNICODE_GENERAL_CATEGORY_ENCLOSING_MARK;
  case U_COMBINING_SPACING_MARK:	return HB_UNICODE_GENERAL_CATEGORY_COMBINING_MARK;

  case U_DECIMAL_DIGIT_NUMBER:		return HB_UNICODE_GENERAL_CATEGORY_DECIMAL_NUMBER;
  case U_LETTER_NUMBER:			return HB_UNICODE_GENERAL_CATEGORY_LETTER_NUMBER;
  case U_OTHER_NUMBER:			return HB_UNICODE_GENERAL_CATEGORY_OTHER_NUMBER;

  case U_SPACE_SEPARATOR:		return HB_UNICODE_GENERAL_CATEGORY_SPACE_SEPARATOR;
  case U_LINE_SEPARATOR:		return HB_UNICODE_GENERAL_CATEGORY_LINE_SEPARATOR;
  case U_PARAGRAPH_SEPARATOR:		return HB_UNICODE_GENERAL_CATEGORY_PARAGRAPH_SEPARATOR;

  case U_CONTROL_CHAR:			return HB_UNICODE_GENERAL_CATEGORY_CONTROL;
  case U_FORMAT_CHAR:			return HB_UNICODE_GENERAL_CATEGORY_FORMAT;
  case U_PRIVATE_USE_CHAR:		return HB_UNICODE_GENERAL_CATEGORY_PRIVATE_USE;
  case U_SURROGATE:			return HB_UNICODE_GENERAL_CATEGORY_SURROGATE;


  case U_DASH_PUNCTUATION:		return HB_UNICODE_GENERAL_CATEGORY_DASH_PUNCTUATION;
  case U_START_PUNCTUATION:		return HB_UNICODE_GENERAL_CATEGORY_OPEN_PUNCTUATION;
  case U_END_PUNCTUATION:		return HB_UNICODE_GENERAL_CATEGORY_CLOSE_PUNCTUATION;
  case U_CONNECTOR_PUNCTUATION:		return HB_UNICODE_GENERAL_CATEGORY_CONNECT_PUNCTUATION;
  case U_OTHER_PUNCTUATION:		return HB_UNICODE_GENERAL_CATEGORY_OTHER_PUNCTUATION;

  case U_MATH_SYMBOL:			return HB_UNICODE_GENERAL_CATEGORY_MATH_SYMBOL;
  case U_CURRENCY_SYMBOL:		return HB_UNICODE_GENERAL_CATEGORY_CURRENCY_SYMBOL;
  case U_MODIFIER_SYMBOL:		return HB_UNICODE_GENERAL_CATEGORY_MODIFIER_SYMBOL;
  case U_OTHER_SYMBOL:			return HB_UNICODE_GENERAL_CATEGORY_OTHER_SYMBOL;

  case U_INITIAL_PUNCTUATION:		return HB_UNICODE_GENERAL_CATEGORY_INITIAL_PUNCTUATION;
  case U_FINAL_PUNCTUATION:		return HB_UNICODE_GENERAL_CATEGORY_FINAL_PUNCTUATION;
  }

  return HB_UNICODE_GENERAL_CATEGORY_UNASSIGNED;
}

static hb_codepoint_t
hb_icu_get_mirroring (hb_unicode_funcs_t *ufuncs,
		      hb_codepoint_t      unicode,
		      void               *user_data)
{
  return u_charMirror(unicode);
}

static hb_script_t
hb_icu_get_script (hb_unicode_funcs_t *ufuncs,
		   hb_codepoint_t      unicode,
		   void               *user_data)
{
  UErrorCode status = U_ZERO_ERROR;
  UScriptCode scriptCode = uscript_getScript(unicode, &status);

  return hb_icu_script_to_script (scriptCode);
}

static hb_unicode_funcs_t icu_ufuncs = {
  HB_REFERENCE_COUNT_INVALID, /* ref_count */
  NULL, /* parent */
  TRUE, /* immutable */
  {
    hb_icu_get_combining_class,
    hb_icu_get_eastasian_width,
    hb_icu_get_general_category,
    hb_icu_get_mirroring,
    hb_icu_get_script
  }
};

hb_unicode_funcs_t *
hb_icu_get_unicode_funcs (void)
{
  return &icu_ufuncs;
}


HB_END_DECLS
