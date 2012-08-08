/*
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This is part of HarfBuzz, an OpenType Layout engine library.
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

#ifndef HARFBUZZ_EXTERNAL_H
#define HARFBUZZ_EXTERNAL_H

#define HB_H_IN
#include <hb-unicode.h>
#include "harfbuzz-global.h"

HB_BEGIN_HEADER

/* This header contains some methods that are not part of
   Harfbuzz itself, but referenced by it.
   They need to be provided by the application/library
*/


typedef enum 
{
    HB_Mark_NonSpacing		= HB_UNICODE_GENERAL_CATEGORY_NON_SPACING_MARK,		/* Mn */
    HB_Mark_SpacingCombining	= HB_UNICODE_GENERAL_CATEGORY_SPACING_MARK,		/* Mc */
    HB_Mark_Enclosing		= HB_UNICODE_GENERAL_CATEGORY_ENCLOSING_MARK,		/* Me */

    HB_Number_DecimalDigit	= HB_UNICODE_GENERAL_CATEGORY_DECIMAL_NUMBER,		/* Nd */
    HB_Number_Letter		= HB_UNICODE_GENERAL_CATEGORY_LETTER_NUMBER,		/* Nl */
    HB_Number_Other		= HB_UNICODE_GENERAL_CATEGORY_OTHER_NUMBER,		/* No */

    HB_Separator_Space		= HB_UNICODE_GENERAL_CATEGORY_SPACE_SEPARATOR,		/* Zs */
    HB_Separator_Line		= HB_UNICODE_GENERAL_CATEGORY_LINE_SEPARATOR,		/* Zl */
    HB_Separator_Paragraph	= HB_UNICODE_GENERAL_CATEGORY_PARAGRAPH_SEPARATOR,	/* Zp */

    HB_Other_Control		= HB_UNICODE_GENERAL_CATEGORY_CONTROL,			/* Cc */
    HB_Other_Format		= HB_UNICODE_GENERAL_CATEGORY_FORMAT,			/* Cf */
    HB_Other_Surrogate		= HB_UNICODE_GENERAL_CATEGORY_SURROGATE,		/* Cs */
    HB_Other_PrivateUse		= HB_UNICODE_GENERAL_CATEGORY_PRIVATE_USE,		/* Co */
    HB_Other_NotAssigned	= HB_UNICODE_GENERAL_CATEGORY_UNASSIGNED,		/* Cn */

    HB_Letter_Uppercase		= HB_UNICODE_GENERAL_CATEGORY_UPPERCASE_LETTER,		/* Lu */
    HB_Letter_Lowercase		= HB_UNICODE_GENERAL_CATEGORY_LOWERCASE_LETTER,		/* Ll */
    HB_Letter_Titlecase		= HB_UNICODE_GENERAL_CATEGORY_TITLECASE_LETTER,		/* Lt */
    HB_Letter_Modifier		= HB_UNICODE_GENERAL_CATEGORY_MODIFIER_LETTER,		/* Lm */
    HB_Letter_Other		= HB_UNICODE_GENERAL_CATEGORY_OTHER_LETTER,		/* Lo */

    HB_Punctuation_Connector	= HB_UNICODE_GENERAL_CATEGORY_CONNECT_PUNCTUATION,	/* Pc */
    HB_Punctuation_Dash		= HB_UNICODE_GENERAL_CATEGORY_DASH_PUNCTUATION,		/* Pd */
    HB_Punctuation_Open		= HB_UNICODE_GENERAL_CATEGORY_OPEN_PUNCTUATION,		/* Ps */
    HB_Punctuation_Close	= HB_UNICODE_GENERAL_CATEGORY_CLOSE_PUNCTUATION,	/* Pe */
    HB_Punctuation_InitialQuote	= HB_UNICODE_GENERAL_CATEGORY_INITIAL_PUNCTUATION,	/* Pi */
    HB_Punctuation_FinalQuote	= HB_UNICODE_GENERAL_CATEGORY_FINAL_PUNCTUATION,	/* Pf */
    HB_Punctuation_Other	= HB_UNICODE_GENERAL_CATEGORY_OTHER_PUNCTUATION,	/* Po */

    HB_Symbol_Math		= HB_UNICODE_GENERAL_CATEGORY_MATH_SYMBOL,		/* Sm */
    HB_Symbol_Currency		= HB_UNICODE_GENERAL_CATEGORY_CURRENCY_SYMBOL,		/* Sc */
    HB_Symbol_Modifier		= HB_UNICODE_GENERAL_CATEGORY_MODIFIER_SYMBOL,		/* Sk */
    HB_Symbol_Other		= HB_UNICODE_GENERAL_CATEGORY_OTHER_SYMBOL		/* So */
} HB_CharCategory;


static inline HB_CharCategory HB_GetUnicodeCharCategory(HB_UChar32 ch)
{
  return (HB_CharCategory) hb_unicode_general_category (hb_unicode_funcs_get_default (), ch);
}

static inline int HB_GetUnicodeCharCombiningClass(HB_UChar32 ch)
{
  return hb_unicode_combining_class (hb_unicode_funcs_get_default (), ch);
}

static inline HB_UChar16 HB_GetMirroredChar(HB_UChar16 ch)
{
  return hb_unicode_mirroring (hb_unicode_funcs_get_default (), ch);
}

static inline void HB_GetUnicodeCharProperties(HB_UChar32 ch, HB_CharCategory *category, int *combiningClass)
{
  if (category)
    *category = HB_GetUnicodeCharCategory (ch);
  if (combiningClass)
    *combiningClass = HB_GetUnicodeCharCombiningClass (ch);
}

HB_END_HEADER

#endif
