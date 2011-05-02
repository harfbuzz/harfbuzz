/*
 * Copyright © 2009  Red Hat, Inc.
 * Copyright © 2009  Keith Stribley
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
 * Red Hat Author(s): Behdad Esfahbod
 * Google Author(s): Behdad Esfahbod
 */

#include "hb-private.hh"

#include "hb-icu.h"

#include "hb-unicode-private.hh"

#include <unicode/uversion.h>
#include <unicode/uchar.h>

HB_BEGIN_DECLS


hb_script_t
hb_icu_script_to_script (UScriptCode script)
{
  if (unlikely (script == USCRIPT_INVALID_CODE))
    return HB_SCRIPT_INVALID;

  return hb_script_from_string (uscript_getShortName (script));
}

UScriptCode
hb_icu_script_from_script (hb_script_t script)
{
  if (unlikely (script == HB_SCRIPT_INVALID))
    return USCRIPT_INVALID_CODE;

  for (unsigned int i = 0; i < USCRIPT_CODE_LIMIT; i++)
    if (unlikely (hb_icu_script_to_script ((UScriptCode) i) == script))
      return (UScriptCode) i;

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

  if (unlikely (status != U_ZERO_ERROR))
    return HB_SCRIPT_UNKNOWN;

  return hb_icu_script_to_script (scriptCode);
}

extern HB_INTERNAL hb_unicode_funcs_t _hb_unicode_funcs_icu;
hb_unicode_funcs_t _hb_icu_unicode_funcs = {
  HB_OBJECT_HEADER_STATIC,

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
  return &_hb_icu_unicode_funcs;
}


HB_END_DECLS
