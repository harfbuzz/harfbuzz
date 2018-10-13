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
 * when moving to this file. */
static const hb_aat_feature_mapping_t feature_mappings[] =
{
  {HB_TAG ('c','2','p','c'),	38/*kUpperCaseType*/,			2/*kUpperCasePetiteCapsSelector*/,		0/*kDefaultUpperCaseSelector*/},
  {HB_TAG ('c','2','s','c'),	38/*kUpperCaseType*/,			1/*kUpperCaseSmallCapsSelector*/,		0/*kDefaultUpperCaseSelector*/},
  {HB_TAG ('c','a','l','t'),	36/*kContextualAlternatesType*/,	0/*kContextualAlternatesOnSelector*/,		1/*kContextualAlternatesOffSelector*/},
  {HB_TAG ('c','a','s','e'),	33/*kCaseSensitiveLayoutType*/,		0/*kCaseSensitiveLayoutOnSelector*/,		1/*kCaseSensitiveLayoutOffSelector*/},
  {HB_TAG ('c','l','i','g'),	1/*kLigaturesType*/,			18/*kContextualLigaturesOnSelector*/,		19/*kContextualLigaturesOffSelector*/},
  {HB_TAG ('c','p','s','p'),	33/*kCaseSensitiveLayoutType*/,		2/*kCaseSensitiveSpacingOnSelector*/,		3/*kCaseSensitiveSpacingOffSelector*/},
  {HB_TAG ('c','s','w','h'),	36/*kContextualAlternatesType*/,	4/*kContextualSwashAlternatesOnSelector*/,	5/*kContextualSwashAlternatesOffSelector*/},
  {HB_TAG ('d','l','i','g'),	1/*kLigaturesType*/,			4/*kRareLigaturesOnSelector*/,			5/*kRareLigaturesOffSelector*/},
  {HB_TAG ('e','x','p','t'),	20/*kCharacterShapeType*/,		10/*kExpertCharactersSelector*/,		16},
  {HB_TAG ('f','r','a','c'),	11/*kFractionsType*/,			2/*kDiagonalFractionsSelector*/,		0/*kNoFractionsSelector*/},
  {HB_TAG ('f','w','i','d'),	22/*kTextSpacingType*/,			1/*kMonospacedTextSelector*/,			7},
  {HB_TAG ('h','a','l','t'),	22/*kTextSpacingType*/,			6/*kAltHalfWidthTextSelector*/,			7},
  {HB_TAG ('h','i','s','t'),	1/*kLigaturesType*/,			20/*kHistoricalLigaturesOnSelector*/,		21/*kHistoricalLigaturesOffSelector*/},
  {HB_TAG ('h','k','n','a'),	34/*kAlternateKanaType*/,		0/*kAlternateHorizKanaOnSelector*/,		1/*kAlternateHorizKanaOffSelector*/,	},
  {HB_TAG ('h','l','i','g'),	1/*kLigaturesType*/,			20/*kHistoricalLigaturesOnSelector*/,		21/*kHistoricalLigaturesOffSelector*/},
  {HB_TAG ('h','n','g','l'),	23/*kTransliterationType*/,		1/*kHanjaToHangulSelector*/,			0/*kNoTransliterationSelector*/},
  {HB_TAG ('h','o','j','o'),	20/*kCharacterShapeType*/,		12/*kHojoCharactersSelector*/,			16},
  {HB_TAG ('h','w','i','d'),	22/*kTextSpacingType*/,			2/*kHalfWidthTextSelector*/,			7},
  {HB_TAG ('i','t','a','l'),	32/*kItalicCJKRomanType*/,		2/*kCJKItalicRomanOnSelector*/,			3/*kCJKItalicRomanOffSelector*/},
  {HB_TAG ('j','p','0','4'),	20/*kCharacterShapeType*/,		11/*kJIS2004CharactersSelector*/,		16},
  {HB_TAG ('j','p','7','8'),	20/*kCharacterShapeType*/,		2/*kJIS1978CharactersSelector*/,		16},
  {HB_TAG ('j','p','8','3'),	20/*kCharacterShapeType*/,		3/*kJIS1983CharactersSelector*/,		16},
  {HB_TAG ('j','p','9','0'),	20/*kCharacterShapeType*/,		4/*kJIS1990CharactersSelector*/,		16},
  {HB_TAG ('l','i','g','a'),	1/*kLigaturesType*/,			2/*kCommonLigaturesOnSelector*/,		3/*kCommonLigaturesOffSelector*/},
  {HB_TAG ('l','n','u','m'),	21/*kNumberCaseType*/,			1/*kUpperCaseNumbersSelector*/,			2},
  {HB_TAG ('m','g','r','k'),	15/*kMathematicalExtrasType*/,		10/*kMathematicalGreekOnSelector*/,		11/*kMathematicalGreekOffSelector*/},
  {HB_TAG ('n','l','c','k'),	20/*kCharacterShapeType*/,		13/*kNLCCharactersSelector*/,			16},
  {HB_TAG ('o','n','u','m'),	21/*kNumberCaseType*/,			0/*kLowerCaseNumbersSelector*/,			2},
  {HB_TAG ('o','r','d','n'),	10/*kVerticalPositionType*/,		3/*kOrdinalsSelector*/,				0/*kNormalPositionSelector*/},
  {HB_TAG ('p','a','l','t'),	22/*kTextSpacingType*/,			5/*kAltProportionalTextSelector*/,		7},
  {HB_TAG ('p','c','a','p'),	37/*kLowerCaseType*/,			2/*kLowerCasePetiteCapsSelector*/,		0/*kDefaultLowerCaseSelector*/},
  {HB_TAG ('p','k','n','a'),	22/*kTextSpacingType*/,			0/*kProportionalTextSelector*/,			7},
  {HB_TAG ('p','n','u','m'),	6/*kNumberSpacingType*/,		1/*kProportionalNumbersSelector*/,		4},
  {HB_TAG ('p','w','i','d'),	22/*kTextSpacingType*/,			0/*kProportionalTextSelector*/,			7},
  {HB_TAG ('q','w','i','d'),	22/*kTextSpacingType*/,			4/*kQuarterWidthTextSelector*/,			7},
  {HB_TAG ('r','u','b','y'),	28/*kRubyKanaType*/,			2/*kRubyKanaOnSelector*/,			3/*kRubyKanaOffSelector*/},
  {HB_TAG ('s','i','n','f'),	10/*kVerticalPositionType*/,		4/*kScientificInferiorsSelector*/,		0/*kNormalPositionSelector*/},
  {HB_TAG ('s','m','c','p'),	37/*kLowerCaseType*/,			1/*kLowerCaseSmallCapsSelector*/,		0/*kDefaultLowerCaseSelector*/},
  {HB_TAG ('s','m','p','l'),	20/*kCharacterShapeType*/,		1/*kSimplifiedCharactersSelector*/,		16},
  {HB_TAG ('s','s','0','1'),	35/*kStylisticAlternativesType*/,	2/*kStylisticAltOneOnSelector*/,		3/*kStylisticAltOneOffSelector*/},
  {HB_TAG ('s','s','0','2'),	35/*kStylisticAlternativesType*/,	4/*kStylisticAltTwoOnSelector*/,		5/*kStylisticAltTwoOffSelector*/},
  {HB_TAG ('s','s','0','3'),	35/*kStylisticAlternativesType*/,	6/*kStylisticAltThreeOnSelector*/,		7/*kStylisticAltThreeOffSelector*/},
  {HB_TAG ('s','s','0','4'),	35/*kStylisticAlternativesType*/,	8/*kStylisticAltFourOnSelector*/,		9/*kStylisticAltFourOffSelector*/},
  {HB_TAG ('s','s','0','5'),	35/*kStylisticAlternativesType*/,	10/*kStylisticAltFiveOnSelector*/,		11/*kStylisticAltFiveOffSelector*/},
  {HB_TAG ('s','s','0','6'),	35/*kStylisticAlternativesType*/,	12/*kStylisticAltSixOnSelector*/,		13/*kStylisticAltSixOffSelector*/},
  {HB_TAG ('s','s','0','7'),	35/*kStylisticAlternativesType*/,	14/*kStylisticAltSevenOnSelector*/,		15/*kStylisticAltSevenOffSelector*/},
  {HB_TAG ('s','s','0','8'),	35/*kStylisticAlternativesType*/,	16/*kStylisticAltEightOnSelector*/,		17/*kStylisticAltEightOffSelector*/},
  {HB_TAG ('s','s','0','9'),	35/*kStylisticAlternativesType*/,	18/*kStylisticAltNineOnSelector*/,		19/*kStylisticAltNineOffSelector*/},
  {HB_TAG ('s','s','1','0'),	35/*kStylisticAlternativesType*/,	20/*kStylisticAltTenOnSelector*/,		21/*kStylisticAltTenOffSelector*/},
  {HB_TAG ('s','s','1','1'),	35/*kStylisticAlternativesType*/,	22/*kStylisticAltElevenOnSelector*/,		23/*kStylisticAltElevenOffSelector*/},
  {HB_TAG ('s','s','1','2'),	35/*kStylisticAlternativesType*/,	24/*kStylisticAltTwelveOnSelector*/,		25/*kStylisticAltTwelveOffSelector*/},
  {HB_TAG ('s','s','1','3'),	35/*kStylisticAlternativesType*/,	26/*kStylisticAltThirteenOnSelector*/,		27/*kStylisticAltThirteenOffSelector*/},
  {HB_TAG ('s','s','1','4'),	35/*kStylisticAlternativesType*/,	28/*kStylisticAltFourteenOnSelector*/,		29/*kStylisticAltFourteenOffSelector*/},
  {HB_TAG ('s','s','1','5'),	35/*kStylisticAlternativesType*/,	30/*kStylisticAltFifteenOnSelector*/,		31/*kStylisticAltFifteenOffSelector*/},
  {HB_TAG ('s','s','1','6'),	35/*kStylisticAlternativesType*/,	32/*kStylisticAltSixteenOnSelector*/,		33/*kStylisticAltSixteenOffSelector*/},
  {HB_TAG ('s','s','1','7'),	35/*kStylisticAlternativesType*/,	34/*kStylisticAltSeventeenOnSelector*/,		35/*kStylisticAltSeventeenOffSelector*/},
  {HB_TAG ('s','s','1','8'),	35/*kStylisticAlternativesType*/,	36/*kStylisticAltEighteenOnSelector*/,		37/*kStylisticAltEighteenOffSelector*/},
  {HB_TAG ('s','s','1','9'),	35/*kStylisticAlternativesType*/,	38/*kStylisticAltNineteenOnSelector*/,		39/*kStylisticAltNineteenOffSelector*/},
  {HB_TAG ('s','s','2','0'),	35/*kStylisticAlternativesType*/,	40/*kStylisticAltTwentyOnSelector*/,		41/*kStylisticAltTwentyOffSelector*/},
  {HB_TAG ('s','u','b','s'),	10/*kVerticalPositionType*/,		2/*kInferiorsSelector*/,			0/*kNormalPositionSelector*/},
  {HB_TAG ('s','u','p','s'),	10/*kVerticalPositionType*/,		1/*kSuperiorsSelector*/,			0/*kNormalPositionSelector*/},
  {HB_TAG ('s','w','s','h'),	36/*kContextualAlternatesType*/,	2/*kSwashAlternatesOnSelector*/,		3/*kSwashAlternatesOffSelector*/},
  {HB_TAG ('t','i','t','l'),	19/*kStyleOptionsType*/,		4/*kTitlingCapsSelector*/,			0/*kNoStyleOptionsSelector*/},
  {HB_TAG ('t','n','a','m'),	20/*kCharacterShapeType*/,		14/*kTraditionalNamesCharactersSelector*/,	16},
  {HB_TAG ('t','n','u','m'),	6/*kNumberSpacingType*/,		0/*kMonospacedNumbersSelector*/,		4},
  {HB_TAG ('t','r','a','d'),	20/*kCharacterShapeType*/,		0/*kTraditionalCharactersSelector*/,		16},
  {HB_TAG ('t','w','i','d'),	22/*kTextSpacingType*/,			3/*kThirdWidthTextSelector*/,			7},
  {HB_TAG ('u','n','i','c'),	3/*kLetterCaseType*/,			14,						15},
  {HB_TAG ('v','a','l','t'),	22/*kTextSpacingType*/,			5/*kAltProportionalTextSelector*/,		7},
  {HB_TAG ('v','e','r','t'),	4/*kVerticalSubstitutionType*/,		0/*kSubstituteVerticalFormsOnSelector*/,	1/*kSubstituteVerticalFormsOffSelector*/},
  {HB_TAG ('v','h','a','l'),	22/*kTextSpacingType*/,			6/*kAltHalfWidthTextSelector*/,			7},
  {HB_TAG ('v','k','n','a'),	34/*kAlternateKanaType*/,		2/*kAlternateVertKanaOnSelector*/,		3/*kAlternateVertKanaOffSelector*/},
  {HB_TAG ('v','p','a','l'),	22/*kTextSpacingType*/,			5/*kAltProportionalTextSelector*/,		7},
  {HB_TAG ('v','r','t','2'),	4/*kVerticalSubstitutionType*/,		0/*kSubstituteVerticalFormsOnSelector*/,	1/*kSubstituteVerticalFormsOffSelector*/},
  {HB_TAG ('z','e','r','o'),	14/*kTypographicExtrasType*/,		4/*kSlashedZeroOnSelector*/,			5/*kSlashedZeroOffSelector*/},
};

const hb_aat_feature_mapping_t *
hb_aat_layout_find_feature_mapping (hb_tag_t tag)
{
  return (const hb_aat_feature_mapping_t *) bsearch (&tag,
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
