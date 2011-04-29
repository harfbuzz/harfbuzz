/*
 * Copyright © 2011  Codethink Limited
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
 * Codethink Author(s): Ryan Lortie
 * Google Author(s): Behdad Esfahbod
 */

#include "hb-test.h"

/* Unit tests for hb-unicode.h */


#ifdef HAVE_GLIB
#include <hb-glib.h>
#endif
#ifdef HAVE_ICU
#include <hb-icu.h>
#endif


/* Check all properties */

/* Some of the following tables where adapted from glib/glib/tests/utf8-misc.c.
 * The license is compatible. */

typedef struct {
  hb_codepoint_t unicode;
  unsigned int   value;
} test_pair_t;

static const test_pair_t combining_class_tests[] =
{
  {   0x111111, (unsigned int) 0 },

  {   0x0020, (unsigned int) 0 },
  {   0x0334, (unsigned int) 1 },
  {   0x093C, (unsigned int) 7 },
  {   0x3099, (unsigned int) 8 },
  {   0x094D, (unsigned int) 9 },
  {   0x05B0, (unsigned int) 10 },
  {   0x05B1, (unsigned int) 11 },
  {   0x05B2, (unsigned int) 12 },
  {   0x05B3, (unsigned int) 13 },
  {   0x05B4, (unsigned int) 14 },
  {   0x05B5, (unsigned int) 15 },
  {   0x05B6, (unsigned int) 16 },
  {   0x05B7, (unsigned int) 17 },
  {   0x05B8, (unsigned int) 18 },
  {   0x05B9, (unsigned int) 19 },
  {   0x05BB, (unsigned int) 20 },
  {   0x05BC, (unsigned int) 21 },
  {   0x05BD, (unsigned int) 22 },
  {   0x05BF, (unsigned int) 23 },
  {   0x05C1, (unsigned int) 24 },
  {   0x05C2, (unsigned int) 25 },
  {   0xFB1E, (unsigned int) 26 },
  {   0x064B, (unsigned int) 27 },
  {   0x064C, (unsigned int) 28 },
  {   0x064D, (unsigned int) 29 },
  /* ... */
  {   0x05AE, (unsigned int) 228 },
  {   0x0300, (unsigned int) 230 },
  {   0x302C, (unsigned int) 232 },
  {   0x0362, (unsigned int) 233 },
  {   0x0360, (unsigned int) 234 },
  {   0x1DCD, (unsigned int) 234 },
  {   0x0345, (unsigned int) 240 },
};
static const test_pair_t combining_class_tests_more[] =
{
  /* Unicode-5.2 character additions */
  {   0xA8E0, (unsigned int) 230 },

  /* Unicode-6.0 character additions */
  {   0x135D, (unsigned int) 230 },
};

static const test_pair_t eastasian_width_tests[] =
{
  {   0x111111, (unsigned int) 1 },

  /* Neutral */
  {   0x0000, (unsigned int) 1 },
  {   0x0483, (unsigned int) 1 },
  {   0x0641, (unsigned int) 1 },
  {   0xFFFC, (unsigned int) 1 },
  {   0x10000, (unsigned int) 1 },
  {   0xE0001, (unsigned int) 1 },

  /* Narrow */
  {   0x0020, (unsigned int) 1 },
  {   0x0041, (unsigned int) 1 },
  {   0x27E6, (unsigned int) 1 },

  /* Halfwidth */
  {   0x20A9, (unsigned int) 1 },
  {   0xFF61, (unsigned int) 1 },
  {   0xFF69, (unsigned int) 1 },
  {   0xFFEE, (unsigned int) 1 },

  /* Ambiguous */
  {   0x00A1, (unsigned int) 1 },
  {   0x00D8, (unsigned int) 1 },
  {   0x02DD, (unsigned int) 1 },
  {   0xE0100, (unsigned int) 1 },
  {   0x100000, (unsigned int) 1 },

  /* Fullwidth */
  {   0x3000, (unsigned int) 2 },
  {   0xFF60, (unsigned int) 2 },

  /* Wide */
  {   0x2329, (unsigned int) 2 },
  {   0x3001, (unsigned int) 2 },
  {   0xFE69, (unsigned int) 2 },
  {   0x30000, (unsigned int) 2 },
  {   0x3FFFD, (unsigned int) 2 },
};
static const test_pair_t eastasian_width_tests_more[] =
{
  /* Default Wide blocks */
  {   0x4DBF, (unsigned int) 2 },
  {   0x9FFF, (unsigned int) 2 },
  {   0xFAFF, (unsigned int) 2 },
  {   0x2A6DF, (unsigned int) 2 },
  {   0x2B73F, (unsigned int) 2 },
  {   0x2B81F, (unsigned int) 2 },
  {   0x2FA1F, (unsigned int) 2 },

  /* Uniode-5.2 character additions */
  /* Wide */
  {   0x115F, (unsigned int) 2 },

  /* Uniode-6.0 character additions */
  /* Wide */
  {   0x2B740, (unsigned int) 2 },
  {   0x1B000, (unsigned int) 2 },
};

static const test_pair_t general_category_tests[] =
{
  { 0x111111, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_UNASSIGNED },

  {   0x000D, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_CONTROL },
  {   0x200E, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_FORMAT },
  {   0x0378, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_UNASSIGNED },
  {   0xE000, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_PRIVATE_USE },
  {   0xD800, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_SURROGATE },
  {   0x0061, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_LOWERCASE_LETTER },
  {   0x02B0, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_MODIFIER_LETTER },
  {   0x3400, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_OTHER_LETTER },
  {   0x01C5, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_TITLECASE_LETTER },
  {   0xFF21, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_UPPERCASE_LETTER },
  {   0x0903, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_COMBINING_MARK },
  {   0x20DD, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_ENCLOSING_MARK },
  {   0xA806, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_NON_SPACING_MARK },
  {   0xFF10, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_DECIMAL_NUMBER },
  {   0x16EE, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_LETTER_NUMBER },
  {   0x17F0, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_OTHER_NUMBER },
  {   0x005F, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_CONNECT_PUNCTUATION },
  {   0x058A, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_DASH_PUNCTUATION },
  {   0x0F3B, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_CLOSE_PUNCTUATION },
  {   0x2019, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_FINAL_PUNCTUATION },
  {   0x2018, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_INITIAL_PUNCTUATION },
  {   0x2016, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_OTHER_PUNCTUATION },
  {   0x0F3A, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_OPEN_PUNCTUATION },
  {   0x20A0, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_CURRENCY_SYMBOL },
  {   0x309B, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_MODIFIER_SYMBOL },
  {   0xFB29, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_MATH_SYMBOL },
  {   0x00A6, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_OTHER_SYMBOL },
  {   0x2028, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_LINE_SEPARATOR },
  {   0x2029, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_PARAGRAPH_SEPARATOR },
  {   0x202F, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_SPACE_SEPARATOR }
};
static const test_pair_t general_category_tests_more[] =
{
  /* Unicode-5.2 character additions */
  {  0x1F131, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_OTHER_SYMBOL },

  /* Unicode-6.0 character additions */
  {   0x0620, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_OTHER_LETTER },
};

static const test_pair_t mirroring_tests[] =
{
  { 0x111111, (unsigned int) 0x111111 },

  /* Some characters that do NOT mirror */
  {   0x0020, (unsigned int) 0x0020 },
  {   0x0041, (unsigned int) 0x0041 },
  {   0x00F0, (unsigned int) 0x00F0 },
  {   0x27CC, (unsigned int) 0x27CC },
  {  0xE01EF, (unsigned int) 0xE01EF },
  {  0x1D7C3, (unsigned int) 0x1D7C3 },
  { 0x100000, (unsigned int) 0x100000 },

  /* Some characters that do mirror */
  {   0x0029, (unsigned int) 0x0028 },
  {   0x0028, (unsigned int) 0x0029 },
  {   0x003E, (unsigned int) 0x003C },
  {   0x003C, (unsigned int) 0x003E },
  {   0x005D, (unsigned int) 0x005B },
  {   0x005B, (unsigned int) 0x005D },
  {   0x007D, (unsigned int) 0x007B },
  {   0x007B, (unsigned int) 0x007D },
  {   0x00BB, (unsigned int) 0x00AB },
  {   0x00AB, (unsigned int) 0x00BB },
  {   0x226B, (unsigned int) 0x226A },
  {   0x226A, (unsigned int) 0x226B },
  {   0x22F1, (unsigned int) 0x22F0 },
  {   0x22F0, (unsigned int) 0x22F1 },
  {   0xFF60, (unsigned int) 0xFF5F },
  {   0xFF5F, (unsigned int) 0xFF60 },
  {   0xFF63, (unsigned int) 0xFF62 },
  {   0xFF62, (unsigned int) 0xFF63 },
};
static const test_pair_t mirroring_tests_more[] =
{
  /* No new mirroring characters have been encoded in recent Unicode versions. */
  {   0xFFFF, (unsigned int) 0xFFFF },
};

static const test_pair_t script_tests[] =
{
  { 0x111111, (unsigned int) HB_SCRIPT_UNKNOWN },

  {   0x002A, (unsigned int) HB_SCRIPT_COMMON },
  {   0x0670, (unsigned int) HB_SCRIPT_INHERITED },
  {   0x060D, (unsigned int) HB_SCRIPT_ARABIC },
  {   0x0559, (unsigned int) HB_SCRIPT_ARMENIAN },
  {   0x09CD, (unsigned int) HB_SCRIPT_BENGALI },
  {   0x31B6, (unsigned int) HB_SCRIPT_BOPOMOFO },
  {   0x13A2, (unsigned int) HB_SCRIPT_CHEROKEE },
  {   0x2CFD, (unsigned int) HB_SCRIPT_COPTIC },
  {   0x0482, (unsigned int) HB_SCRIPT_CYRILLIC },
  {  0x10401, (unsigned int) HB_SCRIPT_DESERET },
  {   0x094D, (unsigned int) HB_SCRIPT_DEVANAGARI },
  {   0x1258, (unsigned int) HB_SCRIPT_ETHIOPIC },
  {   0x10FC, (unsigned int) HB_SCRIPT_GEORGIAN },
  {  0x10341, (unsigned int) HB_SCRIPT_GOTHIC },
  {   0x0375, (unsigned int) HB_SCRIPT_GREEK },
  {   0x0A83, (unsigned int) HB_SCRIPT_GUJARATI },
  {   0x0A3C, (unsigned int) HB_SCRIPT_GURMUKHI },
  {   0x3005, (unsigned int) HB_SCRIPT_HAN },
  {   0x1100, (unsigned int) HB_SCRIPT_HANGUL },
  {   0x05BF, (unsigned int) HB_SCRIPT_HEBREW },
  {   0x309F, (unsigned int) HB_SCRIPT_HIRAGANA },
  {   0x0CBC, (unsigned int) HB_SCRIPT_KANNADA },
  {   0x30FF, (unsigned int) HB_SCRIPT_KATAKANA },
  {   0x17DD, (unsigned int) HB_SCRIPT_KHMER },
  {   0x0EDD, (unsigned int) HB_SCRIPT_LAO },
  {   0x0061, (unsigned int) HB_SCRIPT_LATIN },
  {   0x0D3D, (unsigned int) HB_SCRIPT_MALAYALAM },
  {   0x1843, (unsigned int) HB_SCRIPT_MONGOLIAN },
  {   0x1031, (unsigned int) HB_SCRIPT_MYANMAR },
  {   0x169C, (unsigned int) HB_SCRIPT_OGHAM },
  {  0x10322, (unsigned int) HB_SCRIPT_OLD_ITALIC },
  {   0x0B3C, (unsigned int) HB_SCRIPT_ORIYA },
  {   0x16EF, (unsigned int) HB_SCRIPT_RUNIC },
  {   0x0DBD, (unsigned int) HB_SCRIPT_SINHALA },
  {   0x0711, (unsigned int) HB_SCRIPT_SYRIAC },
  {   0x0B82, (unsigned int) HB_SCRIPT_TAMIL },
  {   0x0C03, (unsigned int) HB_SCRIPT_TELUGU },
  {   0x07B1, (unsigned int) HB_SCRIPT_THAANA },
  {   0x0E31, (unsigned int) HB_SCRIPT_THAI },
  {   0x0FD4, (unsigned int) HB_SCRIPT_TIBETAN },
  {   0x1401, (unsigned int) HB_SCRIPT_CANADIAN_ABORIGINAL },
  {   0xA015, (unsigned int) HB_SCRIPT_YI },
  {   0x1700, (unsigned int) HB_SCRIPT_TAGALOG },
  {   0x1720, (unsigned int) HB_SCRIPT_HANUNOO },
  {   0x1740, (unsigned int) HB_SCRIPT_BUHID },
  {   0x1760, (unsigned int) HB_SCRIPT_TAGBANWA },

  /* Unicode-4.0 additions */
  {   0x2800, (unsigned int) HB_SCRIPT_BRAILLE },
  {  0x10808, (unsigned int) HB_SCRIPT_CYPRIOT },
  {   0x1932, (unsigned int) HB_SCRIPT_LIMBU },
  {  0x10480, (unsigned int) HB_SCRIPT_OSMANYA },
  {  0x10450, (unsigned int) HB_SCRIPT_SHAVIAN },
  {  0x10000, (unsigned int) HB_SCRIPT_LINEAR_B },
  {   0x1950, (unsigned int) HB_SCRIPT_TAI_LE },
  {  0x1039F, (unsigned int) HB_SCRIPT_UGARITIC },

  /* Unicode-4.1 additions */
  {   0x1980, (unsigned int) HB_SCRIPT_NEW_TAI_LUE },
  {   0x1A1F, (unsigned int) HB_SCRIPT_BUGINESE },
  {   0x2C00, (unsigned int) HB_SCRIPT_GLAGOLITIC },
  {   0x2D6F, (unsigned int) HB_SCRIPT_TIFINAGH },
  {   0xA800, (unsigned int) HB_SCRIPT_SYLOTI_NAGRI },
  {  0x103D0, (unsigned int) HB_SCRIPT_OLD_PERSIAN },
  {  0x10A3F, (unsigned int) HB_SCRIPT_KHAROSHTHI },

  /* Unicode-5.0 additions */
  {   0x0378, (unsigned int) HB_SCRIPT_UNKNOWN },
  {   0x1B04, (unsigned int) HB_SCRIPT_BALINESE },
  {  0x12000, (unsigned int) HB_SCRIPT_CUNEIFORM },
  {  0x10900, (unsigned int) HB_SCRIPT_PHOENICIAN },
  {   0xA840, (unsigned int) HB_SCRIPT_PHAGS_PA },
  {   0x07C0, (unsigned int) HB_SCRIPT_NKO },

  /* Unicode-5.1 additions */
  {   0xA900, (unsigned int) HB_SCRIPT_KAYAH_LI },
  {   0x1C00, (unsigned int) HB_SCRIPT_LEPCHA },
  {   0xA930, (unsigned int) HB_SCRIPT_REJANG },
  {   0x1B80, (unsigned int) HB_SCRIPT_SUNDANESE },
  {   0xA880, (unsigned int) HB_SCRIPT_SAURASHTRA },
  {   0xAA00, (unsigned int) HB_SCRIPT_CHAM },
  {   0x1C50, (unsigned int) HB_SCRIPT_OL_CHIKI },
  {   0xA500, (unsigned int) HB_SCRIPT_VAI },
  {  0x102A0, (unsigned int) HB_SCRIPT_CARIAN },
  {  0x10280, (unsigned int) HB_SCRIPT_LYCIAN },
  {  0x1093F, (unsigned int) HB_SCRIPT_LYDIAN },
};
static const test_pair_t script_tests_more[] =
{
  /* Unicode-5.2 additions */
  {  0x10B00, (unsigned int) HB_SCRIPT_AVESTAN },
  {   0xA6A0, (unsigned int) HB_SCRIPT_BAMUM },
  {  0x13000, (unsigned int) HB_SCRIPT_EGYPTIAN_HIEROGLYPHS },
  {  0x10840, (unsigned int) HB_SCRIPT_IMPERIAL_ARAMAIC },
  {  0x10B60, (unsigned int) HB_SCRIPT_INSCRIPTIONAL_PAHLAVI },
  {  0x10B40, (unsigned int) HB_SCRIPT_INSCRIPTIONAL_PARTHIAN },
  {   0xA980, (unsigned int) HB_SCRIPT_JAVANESE },
  {  0x11082, (unsigned int) HB_SCRIPT_KAITHI },
  {   0xA4D0, (unsigned int) HB_SCRIPT_LISU },
  {   0xABE5, (unsigned int) HB_SCRIPT_MEETEI_MAYEK },
  {  0x10A60, (unsigned int) HB_SCRIPT_OLD_SOUTH_ARABIAN },
  {  0x10C00, (unsigned int) HB_SCRIPT_OLD_TURKIC },
  {   0x0800, (unsigned int) HB_SCRIPT_SAMARITAN },
  {   0x1A20, (unsigned int) HB_SCRIPT_TAI_THAM },
  {   0xAA80, (unsigned int) HB_SCRIPT_TAI_VIET },

  /* Unicode-6.0 additions */
  {   0x1BC0, (unsigned int) HB_SCRIPT_BATAK },
  {  0x11000, (unsigned int) HB_SCRIPT_BRAHMI },
  {   0x0840, (unsigned int) HB_SCRIPT_MANDAIC },

  /* Unicode-5.2 character additions */
  {   0x1CED, (unsigned int) HB_SCRIPT_INHERITED },
  {   0x1400, (unsigned int) HB_SCRIPT_CANADIAN_ABORIGINAL },
};


typedef unsigned int (*get_func_t)         (hb_unicode_funcs_t *ufuncs,
					    hb_codepoint_t      unicode,
					    void               *user_data);
typedef unsigned int (*func_setter_func_t) (hb_unicode_funcs_t *ufuncs,
					    get_func_t         *func,
					    void               *user_data,
					    hb_destroy_func_t   destroy);
typedef unsigned int (*getter_func_t)      (hb_unicode_funcs_t *ufuncs,
					    hb_codepoint_t      unicode);

typedef struct {
  const char         *name;
  func_setter_func_t  func_setter;
  getter_func_t       getter;
  const test_pair_t  *tests;
  unsigned int        num_tests;
  const test_pair_t  *tests_more;
  unsigned int        num_tests_more;
  unsigned int        default_value;
} property_t;

#define RETURNS_UNICODE_ITSELF ((unsigned int) -1)

#define PROPERTY(name, DEFAULT) \
  { \
    #name, \
    (func_setter_func_t) hb_unicode_funcs_set_##name##_func, \
    (getter_func_t) hb_unicode_get_##name, \
    name##_tests, \
    G_N_ELEMENTS (name##_tests), \
    name##_tests_more, \
    G_N_ELEMENTS (name##_tests_more), \
    DEFAULT \
  }
static const property_t properties[] =
{
  PROPERTY (combining_class, 0),
  PROPERTY (eastasian_width, 1),
  PROPERTY (general_category, (unsigned int) HB_UNICODE_GENERAL_CATEGORY_OTHER_LETTER),
  PROPERTY (mirroring, RETURNS_UNICODE_ITSELF),
  PROPERTY (script, (unsigned int) HB_SCRIPT_UNKNOWN),
};
#undef PROPERTY

static void
test_unicode_properties (gconstpointer user_data)
{
  hb_unicode_funcs_t *uf = (hb_unicode_funcs_t *) user_data;
  unsigned int i, j;

  g_assert (hb_unicode_funcs_is_immutable (uf));

  for (i = 0; i < G_N_ELEMENTS (properties); i++) {
    const property_t *p = &properties[i];
    const test_pair_t *tests;

    tests = p->tests;
    for (j = 0; j < p->num_tests; j++)
      g_assert_cmpint (p->getter (uf, tests[j].unicode), ==, tests[j].value);
  }
}

static hb_codepoint_t
default_value (hb_codepoint_t default_value, hb_codepoint_t unicode)
{
  return default_value == RETURNS_UNICODE_ITSELF ?  unicode : default_value;
}

static void
test_unicode_properties_nil (void)
{
  hb_unicode_funcs_t *uf = hb_unicode_funcs_create (NULL);
  unsigned int i, j;

  g_assert (!hb_unicode_funcs_is_immutable (uf));

  for (i = 0; i < G_N_ELEMENTS (properties); i++) {
    const property_t *p = &properties[i];
    const test_pair_t *tests;

    tests = p->tests;
    for (j = 0; j < p->num_tests; j++)
      g_assert_cmpint (p->getter (uf, tests[j].unicode), ==, default_value (p->default_value, tests[j].unicode));
  }

  hb_unicode_funcs_destroy (uf);
}

#define MAGIC0 0x12345678
#define MAGIC1 0x76543210

typedef struct {
  int value;
  gboolean freed;
} data_t;

typedef struct {
  data_t data[2];
} data_fixture_t;
static void
data_fixture_init (data_fixture_t *f, gconstpointer user_data)
{
  f->data[0].value = MAGIC0;
  f->data[1].value = MAGIC1;
}
static void
data_fixture_finish (data_fixture_t *f, gconstpointer user_data)
{
}

static void free_up (void *p)
{
  data_t *data = (data_t *) p;

  g_assert (data->value == MAGIC0 || data->value == MAGIC1);
  g_assert (data->freed == FALSE);
  data->freed = TRUE;
}

static hb_script_t
simple_get_script (hb_unicode_funcs_t *ufuncs,
                   hb_codepoint_t      codepoint,
                   void               *user_data)
{
  data_t *data = (data_t *) user_data;

  g_assert (hb_unicode_funcs_get_parent (ufuncs) == NULL);
  g_assert (data->value == MAGIC0);
  g_assert (data->freed == FALSE);

  if ('a' <= codepoint && codepoint <= 'z')
    return HB_SCRIPT_LATIN;
  else
    return HB_SCRIPT_UNKNOWN;
}

static hb_script_t
a_is_for_arabic_get_script (hb_unicode_funcs_t *ufuncs,
                            hb_codepoint_t      codepoint,
                            void               *user_data)
{
  data_t *data = (data_t *) user_data;

  g_assert (hb_unicode_funcs_get_parent (ufuncs) != NULL);
  g_assert (data->value == MAGIC1);
  g_assert (data->freed == FALSE);

  if (codepoint == 'a') {
    return HB_SCRIPT_ARABIC;
  } else {
    hb_unicode_funcs_t *parent = hb_unicode_funcs_get_parent (ufuncs);

    return hb_unicode_get_script (parent, codepoint);
  }
}

static void
test_unicode_custom (data_fixture_t *f, gconstpointer user_data)
{
  hb_unicode_funcs_t *uf = hb_unicode_funcs_create (NULL);

  hb_unicode_funcs_set_script_func (uf, simple_get_script,
                                    &f->data[0], free_up);

  g_assert_cmpint (hb_unicode_get_script (uf, 'a'), ==, HB_SCRIPT_LATIN);
  g_assert_cmpint (hb_unicode_get_script (uf, '0'), ==, HB_SCRIPT_UNKNOWN);

  g_assert (!hb_unicode_funcs_is_immutable (uf));
  hb_unicode_funcs_make_immutable (uf);
  g_assert (hb_unicode_funcs_is_immutable (uf));

  /* Since uf is immutable now, the following setter should do nothing. */
  hb_unicode_funcs_set_script_func (uf, a_is_for_arabic_get_script,
                                    &f->data[1], free_up);

  g_assert (!f->data[0].freed && !f->data[1].freed);
  hb_unicode_funcs_destroy (uf);
  g_assert (f->data[0].freed && !f->data[1].freed);
}

static void
test_unicode_subclassing_nil (data_fixture_t *f, gconstpointer user_data)
{
  hb_unicode_funcs_t *uf, *aa;

  uf = hb_unicode_funcs_create (NULL);

  aa = hb_unicode_funcs_create (uf);

  hb_unicode_funcs_destroy (uf);

  hb_unicode_funcs_set_script_func (aa, a_is_for_arabic_get_script,
                                    &f->data[1], free_up);

  g_assert_cmpint (hb_unicode_get_script (aa, 'a'), ==, HB_SCRIPT_ARABIC);
  g_assert_cmpint (hb_unicode_get_script (aa, 'b'), ==, HB_SCRIPT_UNKNOWN);

  g_assert (!f->data[0].freed && !f->data[1].freed);
  hb_unicode_funcs_destroy (aa);
  g_assert (!f->data[0].freed && f->data[1].freed);
}

static void
test_unicode_subclassing_default (data_fixture_t *f, gconstpointer user_data)
{
  hb_unicode_funcs_t *uf, *aa;

  uf = hb_unicode_funcs_get_default ();
  aa = hb_unicode_funcs_create (uf);

  hb_unicode_funcs_set_script_func (aa, a_is_for_arabic_get_script,
                                    &f->data[1], free_up);

  g_assert_cmpint (hb_unicode_get_script (aa, 'a'), ==, HB_SCRIPT_ARABIC);
  g_assert_cmpint (hb_unicode_get_script (aa, 'b'), ==, HB_SCRIPT_LATIN);

  g_assert (!f->data[0].freed && !f->data[1].freed);
  hb_unicode_funcs_destroy (aa);
  g_assert (!f->data[0].freed && f->data[1].freed);
}

static void
test_unicode_subclassing_deep (data_fixture_t *f, gconstpointer user_data)
{
  hb_unicode_funcs_t *uf, *aa;

  uf = hb_unicode_funcs_create (NULL);

  hb_unicode_funcs_set_script_func (uf, simple_get_script,
                                    &f->data[0], free_up);

  aa = hb_unicode_funcs_create (uf);

  hb_unicode_funcs_destroy (uf);

  /* make sure the 'uf' didn't get freed, since 'aa' holds a ref */
  g_assert (!f->data[0].freed);

  hb_unicode_funcs_set_script_func (aa, a_is_for_arabic_get_script,
                                    &f->data[1], free_up);

  g_assert_cmpint (hb_unicode_get_script (aa, 'a'), ==, HB_SCRIPT_ARABIC);
  g_assert_cmpint (hb_unicode_get_script (aa, 'b'), ==, HB_SCRIPT_LATIN);
  g_assert_cmpint (hb_unicode_get_script (aa, '0'), ==, HB_SCRIPT_UNKNOWN);

  g_assert (!f->data[0].freed && !f->data[1].freed);
  hb_unicode_funcs_destroy (aa);
  g_assert (f->data[0].freed && f->data[1].freed);
}


int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_unicode_properties_nil);

  hb_test_add_data_flavor (hb_unicode_funcs_get_default (), "default", test_unicode_properties);
#ifdef HAVE_GLIB
  hb_test_add_data_flavor (hb_glib_get_unicode_funcs (),    "glib",    test_unicode_properties);
#endif
#ifdef HAVE_ICU
  hb_test_add_data_flavor (hb_icu_get_unicode_funcs (),     "icu",    test_unicode_properties);
#endif

  hb_test_add_fixture (data_fixture, NULL, test_unicode_custom);
  hb_test_add_fixture (data_fixture, NULL, test_unicode_subclassing_nil);
  hb_test_add_fixture (data_fixture, NULL, test_unicode_subclassing_default);
  hb_test_add_fixture (data_fixture, NULL, test_unicode_subclassing_deep);

  /* XXX test icu ufuncs */
  /* XXX test _more tests (warn?) */
  /* XXX test chainup */
  /* XXX test glib & icu two-way script conversion */

  return hb_test_run ();
}
