/*
 * Copyright © 2017  Google, Inc.
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

#include "hb-open-type.hh"

#include "hb-ot-face.hh"
#include "hb-aat-layout.hh"
#include "hb-aat-layout-ankr-table.hh"
#include "hb-aat-layout-bsln-table.hh" // Just so we compile it; unused otherwise.
#include "hb-aat-layout-feat-table.hh" // Just so we compile it; unused otherwise.
#include "hb-aat-layout-kerx-table.hh"
#include "hb-aat-layout-morx-table.hh"
#include "hb-aat-layout-trak-table.hh"
#include "hb-aat-ltag-table.hh" // Just so we compile it; unused otherwise.


/* Table data courtesy of Apple.  Converted from mnemonics to integers
 * when moving to this file.  See hb-coretext.cc before 2018-10-13 for
 * more verbose version. */
static const hb_aat_feature_mapping_t feature_mappings[] =
{
    { 'c2pc',   kUpperCaseType,             kUpperCasePetiteCapsSelector,           kDefaultUpperCaseSelector },
    { 'c2sc',   kUpperCaseType,             kUpperCaseSmallCapsSelector,            kDefaultUpperCaseSelector },
    { 'calt',   kContextualAlternatesType,  kContextualAlternatesOnSelector,        kContextualAlternatesOffSelector },
    { 'case',   kCaseSensitiveLayoutType,   kCaseSensitiveLayoutOnSelector,         kCaseSensitiveLayoutOffSelector },
    { 'clig',   kLigaturesType,             kContextualLigaturesOnSelector,         kContextualLigaturesOffSelector },
    { 'cpsp',   kCaseSensitiveLayoutType,   kCaseSensitiveSpacingOnSelector,        kCaseSensitiveSpacingOffSelector },
    { 'cswh',   kContextualAlternatesType,  kContextualSwashAlternatesOnSelector,   kContextualSwashAlternatesOffSelector },
    { 'dlig',   kLigaturesType,             kRareLigaturesOnSelector,               kRareLigaturesOffSelector },
    { 'expt',   kCharacterShapeType,        kExpertCharactersSelector,              16 },
    { 'frac',   kFractionsType,             kDiagonalFractionsSelector,             kNoFractionsSelector },
    { 'fwid',   kTextSpacingType,           kMonospacedTextSelector,                7 },
    { 'halt',   kTextSpacingType,           kAltHalfWidthTextSelector,              7 },
    { 'hist',   kLigaturesType,             kHistoricalLigaturesOnSelector,         kHistoricalLigaturesOffSelector },
    { 'hkna',   kAlternateKanaType,         kAlternateHorizKanaOnSelector,          kAlternateHorizKanaOffSelector, },
    { 'hlig',   kLigaturesType,             kHistoricalLigaturesOnSelector,         kHistoricalLigaturesOffSelector },
    { 'hngl',   kTransliterationType,       kHanjaToHangulSelector,                 kNoTransliterationSelector },
    { 'hojo',   kCharacterShapeType,        kHojoCharactersSelector,                16 },
    { 'hwid',   kTextSpacingType,           kHalfWidthTextSelector,                 7 },
    { 'ital',   kItalicCJKRomanType,        kCJKItalicRomanOnSelector,              kCJKItalicRomanOffSelector },
    { 'jp04',   kCharacterShapeType,        kJIS2004CharactersSelector,             16 },
    { 'jp78',   kCharacterShapeType,        kJIS1978CharactersSelector,             16 },
    { 'jp83',   kCharacterShapeType,        kJIS1983CharactersSelector,             16 },
    { 'jp90',   kCharacterShapeType,        kJIS1990CharactersSelector,             16 },
    { 'liga',   kLigaturesType,             kCommonLigaturesOnSelector,             kCommonLigaturesOffSelector },
    { 'lnum',   kNumberCaseType,            kUpperCaseNumbersSelector,              2 },
    { 'mgrk',   kMathematicalExtrasType,    kMathematicalGreekOnSelector,           kMathematicalGreekOffSelector },
    { 'nlck',   kCharacterShapeType,        kNLCCharactersSelector,                 16 },
    { 'onum',   kNumberCaseType,            kLowerCaseNumbersSelector,              2 },
    { 'ordn',   kVerticalPositionType,      kOrdinalsSelector,                      kNormalPositionSelector },
    { 'palt',   kTextSpacingType,           kAltProportionalTextSelector,           7 },
    { 'pcap',   kLowerCaseType,             kLowerCasePetiteCapsSelector,           kDefaultLowerCaseSelector },
    { 'pkna',   kTextSpacingType,           kProportionalTextSelector,              7 },
    { 'pnum',   kNumberSpacingType,         kProportionalNumbersSelector,           4 },
    { 'pwid',   kTextSpacingType,           kProportionalTextSelector,              7 },
    { 'qwid',   kTextSpacingType,           kQuarterWidthTextSelector,              7 },
    { 'ruby',   kRubyKanaType,              kRubyKanaOnSelector,                    kRubyKanaOffSelector },
    { 'sinf',   kVerticalPositionType,      kScientificInferiorsSelector,           kNormalPositionSelector },
    { 'smcp',   kLowerCaseType,             kLowerCaseSmallCapsSelector,            kDefaultLowerCaseSelector },
    { 'smpl',   kCharacterShapeType,        kSimplifiedCharactersSelector,          16 },
    { 'ss01',   kStylisticAlternativesType, kStylisticAltOneOnSelector,             kStylisticAltOneOffSelector },
    { 'ss02',   kStylisticAlternativesType, kStylisticAltTwoOnSelector,             kStylisticAltTwoOffSelector },
    { 'ss03',   kStylisticAlternativesType, kStylisticAltThreeOnSelector,           kStylisticAltThreeOffSelector },
    { 'ss04',   kStylisticAlternativesType, kStylisticAltFourOnSelector,            kStylisticAltFourOffSelector },
    { 'ss05',   kStylisticAlternativesType, kStylisticAltFiveOnSelector,            kStylisticAltFiveOffSelector },
    { 'ss06',   kStylisticAlternativesType, kStylisticAltSixOnSelector,             kStylisticAltSixOffSelector },
    { 'ss07',   kStylisticAlternativesType, kStylisticAltSevenOnSelector,           kStylisticAltSevenOffSelector },
    { 'ss08',   kStylisticAlternativesType, kStylisticAltEightOnSelector,           kStylisticAltEightOffSelector },
    { 'ss09',   kStylisticAlternativesType, kStylisticAltNineOnSelector,            kStylisticAltNineOffSelector },
    { 'ss10',   kStylisticAlternativesType, kStylisticAltTenOnSelector,             kStylisticAltTenOffSelector },
    { 'ss11',   kStylisticAlternativesType, kStylisticAltElevenOnSelector,          kStylisticAltElevenOffSelector },
    { 'ss12',   kStylisticAlternativesType, kStylisticAltTwelveOnSelector,          kStylisticAltTwelveOffSelector },
    { 'ss13',   kStylisticAlternativesType, kStylisticAltThirteenOnSelector,        kStylisticAltThirteenOffSelector },
    { 'ss14',   kStylisticAlternativesType, kStylisticAltFourteenOnSelector,        kStylisticAltFourteenOffSelector },
    { 'ss15',   kStylisticAlternativesType, kStylisticAltFifteenOnSelector,         kStylisticAltFifteenOffSelector },
    { 'ss16',   kStylisticAlternativesType, kStylisticAltSixteenOnSelector,         kStylisticAltSixteenOffSelector },
    { 'ss17',   kStylisticAlternativesType, kStylisticAltSeventeenOnSelector,       kStylisticAltSeventeenOffSelector },
    { 'ss18',   kStylisticAlternativesType, kStylisticAltEighteenOnSelector,        kStylisticAltEighteenOffSelector },
    { 'ss19',   kStylisticAlternativesType, kStylisticAltNineteenOnSelector,        kStylisticAltNineteenOffSelector },
    { 'ss20',   kStylisticAlternativesType, kStylisticAltTwentyOnSelector,          kStylisticAltTwentyOffSelector },
    { 'subs',   kVerticalPositionType,      kInferiorsSelector,                     kNormalPositionSelector },
    { 'sups',   kVerticalPositionType,      kSuperiorsSelector,                     kNormalPositionSelector },
    { 'swsh',   kContextualAlternatesType,  kSwashAlternatesOnSelector,             kSwashAlternatesOffSelector },
    { 'titl',   kStyleOptionsType,          kTitlingCapsSelector,                   kNoStyleOptionsSelector },
    { 'tnam',   kCharacterShapeType,        kTraditionalNamesCharactersSelector,    16 },
    { 'tnum',   kNumberSpacingType,         kMonospacedNumbersSelector,             4 },
    { 'trad',   kCharacterShapeType,        kTraditionalCharactersSelector,         16 },
    { 'twid',   kTextSpacingType,           kThirdWidthTextSelector,                7 },
    { 'unic',   kLetterCaseType,            14,                                     15 },
    { 'valt',   kTextSpacingType,           kAltProportionalTextSelector,           7 },
    { 'vert',   kVerticalSubstitutionType,  kSubstituteVerticalFormsOnSelector,     kSubstituteVerticalFormsOffSelector },
    { 'vhal',   kTextSpacingType,           kAltHalfWidthTextSelector,              7 },
    { 'vkna',   kAlternateKanaType,         kAlternateVertKanaOnSelector,           kAlternateVertKanaOffSelector },
    { 'vpal',   kTextSpacingType,           kAltProportionalTextSelector,           7 },
    { 'vrt2',   kVerticalSubstitutionType,  kSubstituteVerticalFormsOnSelector,     kSubstituteVerticalFormsOffSelector },
    { 'zero',   kTypographicExtrasType,     kSlashedZeroOnSelector,                 kSlashedZeroOffSelector },
};

const hb_aat_feature_mapping_t *
hb_aat_layout_find_feature_mapping (hb_tag_t tag)
{
  return bsearch (&tag,
		  feature_mappings,
		  ARRAY_LENGTH (feature_mappings),
		  sizeof (feature_mappings[0]),
		  hb_aat_feature_mapping_t::cmp);
}


/*
 * morx/kerx/trak
 */

static inline const AAT::morx&
_get_morx (hb_face_t *face, hb_blob_t **blob = nullptr)
{
  if (unlikely (!hb_ot_shaper_face_data_ensure (face)))
  {
    if (blob)
      *blob = hb_blob_get_empty ();
    return Null(AAT::morx);
  }
  const AAT::morx& morx = *(hb_ot_face_data (face)->morx.get ());
  if (blob)
    *blob = hb_ot_face_data (face)->morx.get_blob ();
  return morx;
}
static inline const AAT::kerx&
_get_kerx (hb_face_t *face, hb_blob_t **blob = nullptr)
{
  if (unlikely (!hb_ot_shaper_face_data_ensure (face)))
  {
    if (blob)
      *blob = hb_blob_get_empty ();
    return Null(AAT::kerx);
  }
  const AAT::kerx& kerx = *(hb_ot_face_data (face)->kerx.get ());
  if (blob)
    *blob = hb_ot_face_data (face)->kerx.get_blob ();
  return kerx;
}
static inline const AAT::ankr&
_get_ankr (hb_face_t *face, hb_blob_t **blob = nullptr)
{
  if (unlikely (!hb_ot_shaper_face_data_ensure (face)))
  {
    if (blob)
      *blob = hb_blob_get_empty ();
    return Null(AAT::ankr);
  }
  const AAT::ankr& ankr = *(hb_ot_face_data (face)->ankr.get ());
  if (blob)
    *blob = hb_ot_face_data (face)->ankr.get_blob ();
  return ankr;
}
static inline const AAT::trak&
_get_trak (hb_face_t *face)
{
  if (unlikely (!hb_ot_shaper_face_data_ensure (face))) return Null(AAT::trak);
  return *(hb_ot_face_data (face)->trak.get ());
}


hb_bool_t
hb_aat_layout_has_substitution (hb_face_t *face)
{
  return _get_morx (face).has_data ();
}

void
hb_aat_layout_substitute (hb_ot_shape_plan_t *plan,
			  hb_font_t *font,
			  hb_buffer_t *buffer)
{
  hb_blob_t *blob;
  const AAT::morx& morx = _get_morx (font->face, &blob);

  AAT::hb_aat_apply_context_t c (plan, font, buffer, blob);
  morx.apply (&c);
}


hb_bool_t
hb_aat_layout_has_positioning (hb_face_t *face)
{
  return _get_kerx (face).has_data ();
}

void
hb_aat_layout_position (hb_ot_shape_plan_t *plan,
			hb_font_t *font,
			hb_buffer_t *buffer)
{
  hb_blob_t *blob;
  const AAT::kerx& kerx = _get_kerx (font->face, &blob);

  hb_blob_t *ankr_blob;
  const AAT::ankr& ankr = _get_ankr (font->face, &ankr_blob);

  AAT::hb_aat_apply_context_t c (plan, font, buffer, blob,
				 ankr, ankr_blob->data + ankr_blob->length);
  kerx.apply (&c);
}

hb_bool_t
hb_aat_layout_has_tracking (hb_face_t *face)
{
  return _get_trak (face).has_data ();
}

void
hb_aat_layout_track (hb_ot_shape_plan_t *plan,
		     hb_font_t *font,
		     hb_buffer_t *buffer)
{
  const AAT::trak& trak = _get_trak (font->face);

  AAT::hb_aat_apply_context_t c (plan, font, buffer);
  trak.apply (&c);
}
