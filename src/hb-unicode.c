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

#include "hb-unicode-private.h"

HB_BEGIN_DECLS


/*
 * hb_unicode_funcs_t
 */

static hb_codepoint_t hb_unicode_get_mirroring_nil (hb_codepoint_t unicode) { return unicode; }
static hb_category_t hb_unicode_get_general_category_nil (hb_codepoint_t unicode HB_UNUSED) { return HB_CATEGORY_OTHER_LETTER; }
static hb_script_t hb_unicode_get_script_nil (hb_codepoint_t unicode HB_UNUSED) { return HB_SCRIPT_UNKNOWN; }
static unsigned int hb_unicode_get_combining_class_nil (hb_codepoint_t unicode HB_UNUSED) { return 0; }
static unsigned int hb_unicode_get_eastasian_width_nil (hb_codepoint_t unicode HB_UNUSED) { return 1; }

hb_unicode_funcs_t _hb_unicode_funcs_nil = {
  HB_REFERENCE_COUNT_INVALID, /* ref_count */
  TRUE, /* immutable */
  {
    hb_unicode_get_general_category_nil,
    hb_unicode_get_combining_class_nil,
    hb_unicode_get_mirroring_nil,
    hb_unicode_get_script_nil,
    hb_unicode_get_eastasian_width_nil
  }
};

hb_unicode_funcs_t *
hb_unicode_funcs_create (void)
{
  hb_unicode_funcs_t *ufuncs;

  if (!HB_OBJECT_DO_CREATE (hb_unicode_funcs_t, ufuncs))
    return &_hb_unicode_funcs_nil;

  ufuncs->v = _hb_unicode_funcs_nil.v;

  return ufuncs;
}

hb_unicode_funcs_t *
hb_unicode_funcs_reference (hb_unicode_funcs_t *ufuncs)
{
  HB_OBJECT_DO_REFERENCE (ufuncs);
}

unsigned int
hb_unicode_funcs_get_reference_count (hb_unicode_funcs_t *ufuncs)
{
  HB_OBJECT_DO_GET_REFERENCE_COUNT (ufuncs);
}

void
hb_unicode_funcs_destroy (hb_unicode_funcs_t *ufuncs)
{
  HB_OBJECT_DO_DESTROY (ufuncs);

  free (ufuncs);
}

hb_unicode_funcs_t *
hb_unicode_funcs_copy (hb_unicode_funcs_t *other_ufuncs)
{
  hb_unicode_funcs_t *ufuncs;

  if (!HB_OBJECT_DO_CREATE (hb_unicode_funcs_t, ufuncs))
    return &_hb_unicode_funcs_nil;

  ufuncs->v = other_ufuncs->v;

  return ufuncs;
}

void
hb_unicode_funcs_make_immutable (hb_unicode_funcs_t *ufuncs)
{
  if (HB_OBJECT_IS_INERT (ufuncs))
    return;

  ufuncs->immutable = TRUE;
}

hb_bool_t
hb_unicode_funcs_is_immutable (hb_unicode_funcs_t *ufuncs)
{
  return ufuncs->immutable;
}


void
hb_unicode_funcs_set_mirroring_func (hb_unicode_funcs_t *ufuncs,
				     hb_unicode_get_mirroring_func_t mirroring_func)
{
  if (ufuncs->immutable)
    return;

  ufuncs->v.get_mirroring = mirroring_func ? mirroring_func : hb_unicode_get_mirroring_nil;
}

void
hb_unicode_funcs_set_general_category_func (hb_unicode_funcs_t *ufuncs,
					    hb_unicode_get_general_category_func_t general_category_func)
{
  if (ufuncs->immutable)
    return;

  ufuncs->v.get_general_category = general_category_func ? general_category_func : hb_unicode_get_general_category_nil;
}

void
hb_unicode_funcs_set_script_func (hb_unicode_funcs_t *ufuncs,
				  hb_unicode_get_script_func_t script_func)
{
  if (ufuncs->immutable)
    return;

  ufuncs->v.get_script = script_func ? script_func : hb_unicode_get_script_nil;
}

void
hb_unicode_funcs_set_combining_class_func (hb_unicode_funcs_t *ufuncs,
					   hb_unicode_get_combining_class_func_t combining_class_func)
{
  if (ufuncs->immutable)
    return;

  ufuncs->v.get_combining_class = combining_class_func ? combining_class_func : hb_unicode_get_combining_class_nil;
}

void
hb_unicode_funcs_set_eastasian_width_func (hb_unicode_funcs_t *ufuncs,
					   hb_unicode_get_eastasian_width_func_t eastasian_width_func)
{
  if (ufuncs->immutable)
    return;

  ufuncs->v.get_eastasian_width = eastasian_width_func ? eastasian_width_func : hb_unicode_get_eastasian_width_nil;
}


hb_unicode_get_mirroring_func_t
hb_unicode_funcs_get_mirroring_func (hb_unicode_funcs_t *ufuncs)
{
  return ufuncs->v.get_mirroring;
}

hb_unicode_get_general_category_func_t
hb_unicode_funcs_get_general_category_func (hb_unicode_funcs_t *ufuncs)
{
  return ufuncs->v.get_general_category;
}

hb_unicode_get_script_func_t
hb_unicode_funcs_get_script_func (hb_unicode_funcs_t *ufuncs)
{
  return ufuncs->v.get_script;
}

hb_unicode_get_combining_class_func_t
hb_unicode_funcs_get_combining_class_func (hb_unicode_funcs_t *ufuncs)
{
  return ufuncs->v.get_combining_class;
}

hb_unicode_get_eastasian_width_func_t
hb_unicode_funcs_get_eastasian_width_func (hb_unicode_funcs_t *ufuncs)
{
  return ufuncs->v.get_eastasian_width;
}



hb_codepoint_t
hb_unicode_get_mirroring (hb_unicode_funcs_t *ufuncs,
			  hb_codepoint_t unicode)
{
  return ufuncs->v.get_mirroring (unicode);
}

hb_category_t
hb_unicode_get_general_category (hb_unicode_funcs_t *ufuncs,
				 hb_codepoint_t unicode)
{
  return ufuncs->v.get_general_category (unicode);
}

hb_script_t
hb_unicode_get_script (hb_unicode_funcs_t *ufuncs,
		       hb_codepoint_t unicode)
{
  return ufuncs->v.get_script (unicode);
}

unsigned int
hb_unicode_get_combining_class (hb_unicode_funcs_t *ufuncs,
				hb_codepoint_t unicode)
{
  return ufuncs->v.get_combining_class (unicode);
}

unsigned int
hb_unicode_get_eastasian_width (hb_unicode_funcs_t *ufuncs,
				hb_codepoint_t unicode)
{
  return ufuncs->v.get_eastasian_width (unicode);
}


/* hb_script_t */

static const hb_tag_t script_to_iso15924_tag[] =
{
  HB_TAG('Z','y','y','y'),	/* HB_SCRIPT_COMMON */
  HB_TAG('Q','a','a','i'),	/* HB_SCRIPT_INHERITED */
  HB_TAG('A','r','a','b'),	/* HB_SCRIPT_ARABIC */
  HB_TAG('A','r','m','n'),	/* HB_SCRIPT_ARMENIAN */
  HB_TAG('B','e','n','g'),	/* HB_SCRIPT_BENGALI */
  HB_TAG('B','o','p','o'),	/* HB_SCRIPT_BOPOMOFO */
  HB_TAG('C','h','e','r'),	/* HB_SCRIPT_CHEROKEE */
  HB_TAG('Q','a','a','c'),	/* HB_SCRIPT_COPTIC */
  HB_TAG('C','y','r','l'),	/* HB_SCRIPT_CYRILLIC */
  HB_TAG('D','s','r','t'),	/* HB_SCRIPT_DESERET */
  HB_TAG('D','e','v','a'),	/* HB_SCRIPT_DEVANAGARI */
  HB_TAG('E','t','h','i'),	/* HB_SCRIPT_ETHIOPIC */
  HB_TAG('G','e','o','r'),	/* HB_SCRIPT_GEORGIAN */
  HB_TAG('G','o','t','h'),	/* HB_SCRIPT_GOTHIC */
  HB_TAG('G','r','e','k'),	/* HB_SCRIPT_GREEK */
  HB_TAG('G','u','j','r'),	/* HB_SCRIPT_GUJARATI */
  HB_TAG('G','u','r','u'),	/* HB_SCRIPT_GURMUKHI */
  HB_TAG('H','a','n','i'),	/* HB_SCRIPT_HAN */
  HB_TAG('H','a','n','g'),	/* HB_SCRIPT_HANGUL */
  HB_TAG('H','e','b','r'),	/* HB_SCRIPT_HEBREW */
  HB_TAG('H','i','r','a'),	/* HB_SCRIPT_HIRAGANA */
  HB_TAG('K','n','d','a'),	/* HB_SCRIPT_KANNADA */
  HB_TAG('K','a','n','a'),	/* HB_SCRIPT_KATAKANA */
  HB_TAG('K','h','m','r'),	/* HB_SCRIPT_KHMER */
  HB_TAG('L','a','o','o'),	/* HB_SCRIPT_LAO */
  HB_TAG('L','a','t','n'),	/* HB_SCRIPT_LATIN */
  HB_TAG('M','l','y','m'),	/* HB_SCRIPT_MALAYALAM */
  HB_TAG('M','o','n','g'),	/* HB_SCRIPT_MONGOLIAN */
  HB_TAG('M','y','m','r'),	/* HB_SCRIPT_MYANMAR */
  HB_TAG('O','g','a','m'),	/* HB_SCRIPT_OGHAM */
  HB_TAG('I','t','a','l'),	/* HB_SCRIPT_OLD_ITALIC */
  HB_TAG('O','r','y','a'),	/* HB_SCRIPT_ORIYA */
  HB_TAG('R','u','n','r'),	/* HB_SCRIPT_RUNIC */
  HB_TAG('S','i','n','h'),	/* HB_SCRIPT_SINHALA */
  HB_TAG('S','y','r','c'),	/* HB_SCRIPT_SYRIAC */
  HB_TAG('T','a','m','l'),	/* HB_SCRIPT_TAMIL */
  HB_TAG('T','e','l','u'),	/* HB_SCRIPT_TELUGU */
  HB_TAG('T','h','a','a'),	/* HB_SCRIPT_THAANA */
  HB_TAG('T','h','a','i'),	/* HB_SCRIPT_THAI */
  HB_TAG('T','i','b','t'),	/* HB_SCRIPT_TIBETAN */
  HB_TAG('C','a','n','s'),	/* HB_SCRIPT_CANADIAN_ABORIGINAL */
  HB_TAG('Y','i','i','i'),	/* HB_SCRIPT_YI */
  HB_TAG('T','g','l','g'),	/* HB_SCRIPT_TAGALOG */
  HB_TAG('H','a','n','o'),	/* HB_SCRIPT_HANUNOO */
  HB_TAG('B','u','h','d'),	/* HB_SCRIPT_BUHID */
  HB_TAG('T','a','g','b'),	/* HB_SCRIPT_TAGBANWA */

  /* Unicode-4.0 additions */
  HB_TAG('B','r','a','i'),	/* HB_SCRIPT_BRAILLE */
  HB_TAG('C','p','r','t'),	/* HB_SCRIPT_CYPRIOT */
  HB_TAG('L','i','m','b'),	/* HB_SCRIPT_LIMBU */
  HB_TAG('O','s','m','a'),	/* HB_SCRIPT_OSMANYA */
  HB_TAG('S','h','a','w'),	/* HB_SCRIPT_SHAVIAN */
  HB_TAG('L','i','n','b'),	/* HB_SCRIPT_LINEAR_B */
  HB_TAG('T','a','l','e'),	/* HB_SCRIPT_TAI_LE */
  HB_TAG('U','g','a','r'),	/* HB_SCRIPT_UGARITIC */

  /* Unicode-4.1 additions */
  HB_TAG('T','a','l','u'),	/* HB_SCRIPT_NEW_TAI_LUE */
  HB_TAG('B','u','g','i'),	/* HB_SCRIPT_BUGINESE */
  HB_TAG('G','l','a','g'),	/* HB_SCRIPT_GLAGOLITIC */
  HB_TAG('T','f','n','g'),	/* HB_SCRIPT_TIFINAGH */
  HB_TAG('S','y','l','o'),	/* HB_SCRIPT_SYLOTI_NAGRI */
  HB_TAG('X','p','e','o'),	/* HB_SCRIPT_OLD_PERSIAN */
  HB_TAG('K','h','a','r'),	/* HB_SCRIPT_KHAROSHTHI */

  /* Unicode-5.0 additions */
  HB_TAG('Z','z','z','z'),	/* HB_SCRIPT_UNKNOWN */
  HB_TAG('B','a','l','i'),	/* HB_SCRIPT_BALINESE */
  HB_TAG('X','s','u','x'),	/* HB_SCRIPT_CUNEIFORM */
  HB_TAG('P','h','n','x'),	/* HB_SCRIPT_PHOENICIAN */
  HB_TAG('P','h','a','g'),	/* HB_SCRIPT_PHAGS_PA */
  HB_TAG('N','k','o','o'),	/* HB_SCRIPT_NKO */

  /* Unicode-5.1 additions */
  HB_TAG('K','a','l','i'),	/* HB_SCRIPT_KAYAH_LI */
  HB_TAG('L','e','p','c'),	/* HB_SCRIPT_LEPCHA */
  HB_TAG('R','j','n','g'),	/* HB_SCRIPT_REJANG */
  HB_TAG('S','u','n','d'),	/* HB_SCRIPT_SUNDANESE */
  HB_TAG('S','a','u','r'),	/* HB_SCRIPT_SAURASHTRA */
  HB_TAG('C','h','a','m'),	/* HB_SCRIPT_CHAM */
  HB_TAG('O','l','c','k'),	/* HB_SCRIPT_OL_CHIKI */
  HB_TAG('V','a','i','i'),	/* HB_SCRIPT_VAI */
  HB_TAG('C','a','r','i'),	/* HB_SCRIPT_CARIAN */
  HB_TAG('L','y','c','i'),	/* HB_SCRIPT_LYCIAN */
  HB_TAG('L','y','d','i'),	/* HB_SCRIPT_LYDIAN */

  /* Unicode-5.2 additions */
  HB_TAG('A','v','s','t'),	/* HB_SCRIPT_AVESTAN */
  HB_TAG('B','a','m','u'),	/* HB_SCRIPT_BAMUM */
  HB_TAG('E','g','y','p'),	/* HB_SCRIPT_EGYPTIAN_HIEROGLYPHS */
  HB_TAG('A','r','m','i'),	/* HB_SCRIPT_IMPERIAL_ARAMAIC */
  HB_TAG('P','h','l','i'),	/* HB_SCRIPT_INSCRIPTIONAL_PAHLAVI */
  HB_TAG('P','r','t','i'),	/* HB_SCRIPT_INSCRIPTIONAL_PARTHIAN */
  HB_TAG('J','a','v','a'),	/* HB_SCRIPT_JAVANESE */
  HB_TAG('K','t','h','i'),	/* HB_SCRIPT_KAITHI */
  HB_TAG('L','i','s','u'),	/* HB_SCRIPT_LISU */
  HB_TAG('M','t','e','i'),	/* HB_SCRIPT_MEETEI_MAYEK */
  HB_TAG('S','a','r','b'),	/* HB_SCRIPT_OLD_SOUTH_ARABIAN */
  HB_TAG('O','r','k','h'),	/* HB_SCRIPT_OLD_TURKIC */
  HB_TAG('S','a','m','r'),	/* HB_SCRIPT_SAMARITAN */
  HB_TAG('L','a','n','a'),	/* HB_SCRIPT_TAI_THAM */
  HB_TAG('T','a','v','t'),	/* HB_SCRIPT_TAI_VIET */

  /* Unicode-6.0 additions */
  HB_TAG('B','a','t','k'),	/* HB_SCRIPT_BATAK */
  HB_TAG('B','r','a','h'),	/* HB_SCRIPT_BRAHMI */
  HB_TAG('M','a','n','d') 	/* HB_SCRIPT_MANDAIC */
};

struct tag_script_pair {
  hb_tag_t tag;
  hb_script_t script;
};
static const struct tag_script_pair script_from_iso15924_tag[] =
{
  {HB_TAG('A','r','a','b'), HB_SCRIPT_ARABIC},
  {HB_TAG('A','r','m','i'), HB_SCRIPT_IMPERIAL_ARAMAIC},
  {HB_TAG('A','r','m','n'), HB_SCRIPT_ARMENIAN},
  {HB_TAG('A','v','s','t'), HB_SCRIPT_AVESTAN},
  {HB_TAG('B','a','l','i'), HB_SCRIPT_BALINESE},
  {HB_TAG('B','a','m','u'), HB_SCRIPT_BAMUM},
  {HB_TAG('B','a','t','k'), HB_SCRIPT_BATAK},
  {HB_TAG('B','e','n','g'), HB_SCRIPT_BENGALI},
  {HB_TAG('B','o','p','o'), HB_SCRIPT_BOPOMOFO},
  {HB_TAG('B','r','a','h'), HB_SCRIPT_BRAHMI},
  {HB_TAG('B','r','a','i'), HB_SCRIPT_BRAILLE},
  {HB_TAG('B','u','g','i'), HB_SCRIPT_BUGINESE},
  {HB_TAG('B','u','h','d'), HB_SCRIPT_BUHID},
  {HB_TAG('C','a','n','s'), HB_SCRIPT_CANADIAN_ABORIGINAL},
  {HB_TAG('C','a','r','i'), HB_SCRIPT_CARIAN},
  {HB_TAG('C','h','a','m'), HB_SCRIPT_CHAM},
  {HB_TAG('C','h','e','r'), HB_SCRIPT_CHEROKEE},
  {HB_TAG('C','p','r','t'), HB_SCRIPT_CYPRIOT},
  {HB_TAG('C','y','r','l'), HB_SCRIPT_CYRILLIC},
  {HB_TAG('C','y','r','s'), HB_SCRIPT_CYRILLIC},
  {HB_TAG('D','e','v','a'), HB_SCRIPT_DEVANAGARI},
  {HB_TAG('D','s','r','t'), HB_SCRIPT_DESERET},
  {HB_TAG('E','g','y','p'), HB_SCRIPT_EGYPTIAN_HIEROGLYPHS},
  {HB_TAG('E','t','h','i'), HB_SCRIPT_ETHIOPIC},
  {HB_TAG('G','e','o','a'), HB_SCRIPT_GEORGIAN},
  {HB_TAG('G','e','o','n'), HB_SCRIPT_GEORGIAN},
  {HB_TAG('G','e','o','r'), HB_SCRIPT_GEORGIAN},
  {HB_TAG('G','l','a','g'), HB_SCRIPT_GLAGOLITIC},
  {HB_TAG('G','o','t','h'), HB_SCRIPT_GOTHIC},
  {HB_TAG('G','r','e','k'), HB_SCRIPT_GREEK},
  {HB_TAG('G','u','j','r'), HB_SCRIPT_GUJARATI},
  {HB_TAG('G','u','r','u'), HB_SCRIPT_GURMUKHI},
  {HB_TAG('H','a','n','g'), HB_SCRIPT_HANGUL},
  {HB_TAG('H','a','n','i'), HB_SCRIPT_HAN},
  {HB_TAG('H','a','n','o'), HB_SCRIPT_HANUNOO},
  {HB_TAG('H','e','b','r'), HB_SCRIPT_HEBREW},
  {HB_TAG('H','i','r','a'), HB_SCRIPT_HIRAGANA},
  {HB_TAG('I','t','a','l'), HB_SCRIPT_OLD_ITALIC},
  {HB_TAG('J','a','v','a'), HB_SCRIPT_JAVANESE},
  {HB_TAG('K','a','l','i'), HB_SCRIPT_KAYAH_LI},
  {HB_TAG('K','a','n','a'), HB_SCRIPT_KATAKANA},
  {HB_TAG('K','h','a','r'), HB_SCRIPT_KHAROSHTHI},
  {HB_TAG('K','h','m','r'), HB_SCRIPT_KHMER},
  {HB_TAG('K','n','d','a'), HB_SCRIPT_KANNADA},
  {HB_TAG('K','t','h','i'), HB_SCRIPT_KAITHI},
  {HB_TAG('L','a','n','a'), HB_SCRIPT_TAI_THAM},
  {HB_TAG('L','a','o','o'), HB_SCRIPT_LAO},
  {HB_TAG('L','a','t','f'), HB_SCRIPT_LATIN},
  {HB_TAG('L','a','t','g'), HB_SCRIPT_LATIN},
  {HB_TAG('L','a','t','n'), HB_SCRIPT_LATIN},
  {HB_TAG('L','e','p','c'), HB_SCRIPT_LEPCHA},
  {HB_TAG('L','i','m','b'), HB_SCRIPT_LIMBU},
  {HB_TAG('L','i','n','b'), HB_SCRIPT_LINEAR_B},
  {HB_TAG('L','i','s','u'), HB_SCRIPT_LISU},
  {HB_TAG('L','y','c','i'), HB_SCRIPT_LYCIAN},
  {HB_TAG('L','y','d','i'), HB_SCRIPT_LYDIAN},
  {HB_TAG('M','a','n','d'), HB_SCRIPT_MANDAIC},
  {HB_TAG('M','l','y','m'), HB_SCRIPT_MALAYALAM},
  {HB_TAG('M','o','n','g'), HB_SCRIPT_MONGOLIAN},
  {HB_TAG('M','t','e','i'), HB_SCRIPT_MEETEI_MAYEK},
  {HB_TAG('M','y','m','r'), HB_SCRIPT_MYANMAR},
  {HB_TAG('N','k','o','o'), HB_SCRIPT_NKO},
  {HB_TAG('O','g','a','m'), HB_SCRIPT_OGHAM},
  {HB_TAG('O','l','c','k'), HB_SCRIPT_OL_CHIKI},
  {HB_TAG('O','r','k','h'), HB_SCRIPT_OLD_TURKIC},
  {HB_TAG('O','r','y','a'), HB_SCRIPT_ORIYA},
  {HB_TAG('O','s','m','a'), HB_SCRIPT_OSMANYA},
  {HB_TAG('P','h','a','g'), HB_SCRIPT_PHAGS_PA},
  {HB_TAG('P','h','l','i'), HB_SCRIPT_INSCRIPTIONAL_PAHLAVI},
  {HB_TAG('P','h','n','x'), HB_SCRIPT_PHOENICIAN},
  {HB_TAG('P','r','t','i'), HB_SCRIPT_INSCRIPTIONAL_PARTHIAN},
  {HB_TAG('Q','a','a','c'), HB_SCRIPT_COPTIC},
  {HB_TAG('Q','a','a','i'), HB_SCRIPT_INHERITED},
  {HB_TAG('R','j','n','g'), HB_SCRIPT_REJANG},
  {HB_TAG('R','u','n','r'), HB_SCRIPT_RUNIC},
  {HB_TAG('S','a','m','r'), HB_SCRIPT_SAMARITAN},
  {HB_TAG('S','a','r','b'), HB_SCRIPT_OLD_SOUTH_ARABIAN},
  {HB_TAG('S','a','u','r'), HB_SCRIPT_SAURASHTRA},
  {HB_TAG('S','h','a','w'), HB_SCRIPT_SHAVIAN},
  {HB_TAG('S','i','n','h'), HB_SCRIPT_SINHALA},
  {HB_TAG('S','u','n','d'), HB_SCRIPT_SUNDANESE},
  {HB_TAG('S','y','l','o'), HB_SCRIPT_SYLOTI_NAGRI},
  {HB_TAG('S','y','r','c'), HB_SCRIPT_SYRIAC},
  {HB_TAG('S','y','r','e'), HB_SCRIPT_SYRIAC},
  {HB_TAG('S','y','r','n'), HB_SCRIPT_SYRIAC},
  {HB_TAG('T','a','g','b'), HB_SCRIPT_TAGBANWA},
  {HB_TAG('T','a','l','e'), HB_SCRIPT_TAI_LE},
  {HB_TAG('T','a','l','u'), HB_SCRIPT_NEW_TAI_LUE},
  {HB_TAG('T','a','m','l'), HB_SCRIPT_TAMIL},
  {HB_TAG('T','a','v','t'), HB_SCRIPT_TAI_VIET},
  {HB_TAG('T','e','l','u'), HB_SCRIPT_TELUGU},
  {HB_TAG('T','f','n','g'), HB_SCRIPT_TIFINAGH},
  {HB_TAG('T','g','l','g'), HB_SCRIPT_TAGALOG},
  {HB_TAG('T','h','a','a'), HB_SCRIPT_THAANA},
  {HB_TAG('T','h','a','i'), HB_SCRIPT_THAI},
  {HB_TAG('T','i','b','t'), HB_SCRIPT_TIBETAN},
  {HB_TAG('U','g','a','r'), HB_SCRIPT_UGARITIC},
  {HB_TAG('V','a','i','i'), HB_SCRIPT_VAI},
  {HB_TAG('X','p','e','o'), HB_SCRIPT_OLD_PERSIAN},
  {HB_TAG('X','s','u','x'), HB_SCRIPT_CUNEIFORM},
  {HB_TAG('Y','i','i','i'), HB_SCRIPT_YI},
  {HB_TAG('Z','y','y','y'), HB_SCRIPT_COMMON},
  {HB_TAG('Z','z','z','z'), HB_SCRIPT_UNKNOWN}
};

static int
_tag_cmp (hb_tag_t *pa, hb_tag_t *pb)
{
  hb_tag_t a = *pa, b = *pb;
  return a < b ? -1 : a == b ? 0 : +1;
}


hb_script_t
hb_script_from_iso15924_tag (hb_tag_t tag)
{
  const struct tag_script_pair *pair;

  /* Be lenient, adjust case (one capital letter followed by three small letters) */
  tag = (tag & 0xDFDFDFDF) | 0x00202020;

  pair = (const struct tag_script_pair *) bsearch (&tag,
						   script_from_iso15924_tag,
						   ARRAY_LENGTH (script_from_iso15924_tag),
						   sizeof (script_from_iso15924_tag[0]),
						   (hb_compare_func_t) _tag_cmp);

  if (pair)
    return pair->script;

  /* If it looks right, just use the tag as a script */
  if (((uint32_t) tag & 0xE0E0E0E0) == 0x40606060)
    return (hb_script_t) tag;

  /* Otherwise, return unknown */
  return HB_SCRIPT_UNKNOWN;
}

hb_tag_t
hb_script_to_iso15924_tag (hb_script_t script)
{
  if (likely ((unsigned int) script < ARRAY_LENGTH (script_to_iso15924_tag)))
    return script_to_iso15924_tag[script];

  /* if script is of the right shape (one capital letter followed by three small letters),
   * return as is. */
  if (((uint32_t) script & 0xE0E0E0E0) == 0x40606060)
    return (hb_tag_t) script;

  /* Otherwise, we don't know what that is */
  return script_to_iso15924_tag[HB_SCRIPT_UNKNOWN];
}


#define LTR HB_DIRECTION_LTR
#define RTL HB_DIRECTION_RTL
const hb_direction_t horiz_dir[] =
{
  LTR,	/* Zyyy */
  LTR,	/* Qaai */
  RTL,	/* Arab */
  LTR,	/* Armn */
  LTR,	/* Beng */
  LTR,	/* Bopo */
  LTR,	/* Cher */
  LTR,	/* Qaac */
  LTR,	/* Cyrl (Cyrs) */
  LTR,	/* Dsrt */
  LTR,	/* Deva */
  LTR,	/* Ethi */
  LTR,	/* Geor (Geon, Geoa) */
  LTR,	/* Goth */
  LTR,	/* Grek */
  LTR,	/* Gujr */
  LTR,	/* Guru */
  LTR,	/* Hani */
  LTR,	/* Hang */
  RTL,	/* Hebr */
  LTR,	/* Hira */
  LTR,	/* Knda */
  LTR,	/* Kana */
  LTR,	/* Khmr */
  LTR,	/* Laoo */
  LTR,	/* Latn (Latf, Latg) */
  LTR,	/* Mlym */
  LTR,	/* Mong */
  LTR,	/* Mymr */
  LTR,	/* Ogam */
  LTR,	/* Ital */
  LTR,	/* Orya */
  LTR,	/* Runr */
  LTR,	/* Sinh */
  RTL,	/* Syrc (Syrj, Syrn, Syre) */
  LTR,	/* Taml */
  LTR,	/* Telu */
  RTL,	/* Thaa */
  LTR,	/* Thai */
  LTR,	/* Tibt */
  LTR,	/* Cans */
  LTR,	/* Yiii */
  LTR,	/* Tglg */
  LTR,	/* Hano */
  LTR,	/* Buhd */
  LTR,	/* Tagb */

  /* Unicode-4.0 additions */
  LTR,	/* Brai */
  RTL,	/* Cprt */
  LTR,	/* Limb */
  LTR,	/* Osma */
  LTR,	/* Shaw */
  LTR,	/* Linb */
  LTR,	/* Tale */
  LTR,	/* Ugar */

  /* Unicode-4.1 additions */
  LTR,	/* Talu */
  LTR,	/* Bugi */
  LTR,	/* Glag */
  LTR,	/* Tfng */
  LTR,	/* Sylo */
  LTR,	/* Xpeo */
  LTR,	/* Khar */

  /* Unicode-5.0 additions */
  LTR,	/* Zzzz */
  LTR,	/* Bali */
  LTR,	/* Xsux */
  RTL,	/* Phnx */
  LTR,	/* Phag */
  RTL,	/* Nkoo */

  /* Unicode-5.1 additions */
  LTR,	/* Kali */
  LTR,	/* Lepc */
  LTR,	/* Rjng */
  LTR,	/* Sund */
  LTR,	/* Saur */
  LTR,	/* Cham */
  LTR,	/* Olck */
  LTR,	/* Vaii */
  LTR,	/* Cari */
  LTR,	/* Lyci */
  LTR,	/* Lydi */

  /* Unicode-5.2 additions */
  RTL,	/* Avst */
  LTR,	/* Bamu */
  LTR,	/* Egyp */
  RTL,	/* Armi */
  RTL,	/* Phli */
  RTL,	/* Prti */
  LTR,	/* Java */
  LTR,	/* Kthi */
  LTR,	/* Lisu */
  LTR,	/* Mtei */
  RTL,	/* Sarb */
  RTL,	/* Orkh */
  RTL,	/* Samr */
  LTR,	/* Lana */
  LTR,	/* Tavt */

  /* Unicode-6.0 additions */
  LTR,	/* Batk */
  LTR,	/* Brah */
  RTL 	/* Mand */
};
#undef LTR
#undef RTL

hb_direction_t
hb_script_get_horizontal_direction (hb_script_t script)
{
  if (unlikely ((unsigned int) script >= ARRAY_LENGTH (horiz_dir)))
    return HB_DIRECTION_LTR;

  return horiz_dir[script];
}


HB_END_DECLS
