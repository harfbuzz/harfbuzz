/*
 * Copyright © 2026  Ebrahim Byagowi
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
 */

#ifndef HB_SUBSET_HINTING_HH
#define HB_SUBSET_HINTING_HH

#include "hb-ot-name.h"
#include "hb-utf.hh"
#include "hb.hh"

#ifndef HB_NO_NAME
/* Some legacy fonts have hinting programs that are required for correct
 * rendering; dropping them changes glyph advances and outlines.  FreeType
 * likewise refuses to load these fonts without hinting.  The family names are
 * from src/truetype/ttobjs.c in FreeType. */
static const char *hint_dependent_font_families[] = {
    "cpop",	   "DFGirl-W6-WIN-BF", "DFGothic-EB",	 "DFGyoSho-Lt",
    "DFHei",	   "DFHSGothic-W5",    "DFHSMincho-W3",	 "DFHSMincho-W7",
    "DFKaiSho-SB", "DFKaiShu",	       "DFKai-SB",	 "DFMing",
    "DLC",	   "HuaTianKaiTi?",    "HuaTianSongTi?", "Ming(for ISO10646)",
    "MingLiU",	   "MingMedium",       "PMingLiU",	 "MingLi43",
};

static inline bool
_name_matches_hint_dependent_family (hb_face_t *face,
				     hb_ot_name_id_t name_id,
				     hb_language_t language)
{
  unsigned len =
      hb_ot_name_get_utf8 (face, name_id, language, nullptr, nullptr);
  if (!len) return false;

  char stack_name[128];
  char *name = stack_name;
  char *heap_name = nullptr;
  if (len + 1 > sizeof (stack_name))
  {
    heap_name = (char *) hb_malloc (len + 1);
    if (unlikely (!heap_name)) return true; /* Be conservative. */
    name = heap_name;
  }

  unsigned size = len + 1;
  hb_ot_name_get_utf8 (face, name_id, language, &size, name);

  /* Match FreeType: keep ASCII, replace other characters with `?`, stop at
   * the first NUL, and skip a PDF font randomization tag. */
  const uint8_t *p = (const uint8_t *) name;
  const uint8_t *end = p + len;
  uint8_t *out = (uint8_t *) name;
  while (p < end)
  {
    hb_codepoint_t cp = 0;
    p = hb_utf8_t::next (p, end, &cp, '?');
    if (!cp) break;
    *out++ = (cp >= 32 && cp <= 127) ? (uint8_t) cp : '?';
  }
  *out = '\0';

  const char *family = name;
  if (out - (uint8_t *) name >= 8 && family[0] >= 'A' && family[0] <= 'Z' &&
      family[1] >= 'A' && family[1] <= 'Z' && family[2] >= 'A' &&
      family[2] <= 'Z' && family[3] >= 'A' && family[3] <= 'Z' &&
      family[4] >= 'A' && family[4] <= 'Z' && family[5] >= 'A' &&
      family[5] <= 'Z' && family[6] == '+')
    family += 7;

  bool match = false;
  for (const char *candidate : hint_dependent_font_families)
    if (strstr (family, candidate))
    {
      match = true;
      break;
    }

  hb_free (heap_name);
  return match;
}
#endif

struct hint_dependent_table_id_t
{
  uint32_t checksum;
  uint32_t length;
};

struct hint_dependent_font_id_t
{
  hint_dependent_table_id_t cvt;
  hint_dependent_table_id_t fpgm;
  hint_dependent_table_id_t prep;
};

/* cvt/fpgm/prep identities for fonts that FreeType identifies even without a
 * usable name table, from src/truetype/ttobjs.c in FreeType. */
static const hint_dependent_font_id_t hint_dependent_font_ids[] = {
    /* MingLiU 1995 */
    {{0x05BCF058, 0x000002E4},
     {0x28233BF1, 0x000087C4},
     {0xA344A1EA, 0x000001E1}},
    /* MingLiU 1996- */
    {{0x05BCF058, 0x000002E4},
     {0x28233BF1, 0x000087C4},
     {0xA344A1EB, 0x000001E1}},
    /* DFGothic-EB */
    {{0x12C3EBB2, 0x00000350},
     {0xB680EE64, 0x000087A7},
     {0xCE939563, 0x00000758}},
    /* DFGyoSho-Lt */
    {{0x11E5EAD4, 0x00000350},
     {0xCE5956E9, 0x0000BC85},
     {0x8272F416, 0x00000045}},
    /* DFHei-Md-HK-BF */
    {{0x1257EB46, 0x00000350},
     {0xF699D160, 0x0000715F},
     {0xD222F568, 0x000003BC}},
    /* DFHSGothic-W5 */
    {{0x1262EB4E, 0x00000350},
     {0xE86A5D64, 0x00007940},
     {0x7850F729, 0x000005FF}},
    /* DFHSMincho-W3 */
    {{0x122DEB0A, 0x00000350},
     {0x3D16328A, 0x0000859B},
     {0xA93FC33B, 0x000002CB}},
    /* DFHSMincho-W7 */
    {{0x125FEB26, 0x00000350},
     {0xA5ACC982, 0x00007EE1},
     {0x90999196, 0x0000041F}},
    /* DFKaiShu */
    {{0x11E5EAD4, 0x00000350},
     {0x5A30CA3B, 0x00009063},
     {0x13A42602, 0x0000007E}},
    /* DFKaiShu, variant */
    {{0x11E5EAD4, 0x00000350},
     {0xA6E78C01, 0x00008998},
     {0x13A42602, 0x0000007E}},
    /* DFKaiShu-Md-HK-BF */
    {{0x11E5EAD4, 0x00000360},
     {0x9DB282B2, 0x0000C06E},
     {0x53E6D7CA, 0x00000082}},
    /* DFMing-Bd-HK-BF */
    {{0x1243EB18, 0x00000350},
     {0xBA0A8C30, 0x000074AD},
     {0xF3D83409, 0x0000037B}},
    /* DLCLiShu */
    {{0x07DCF546, 0x00000308},
     {0x40FE7C90, 0x00008E2A},
     {0x608174B5, 0x0000007A}},
    /* DLCHayBold */
    {{0xEB891238, 0x00000308},
     {0xD2E4DCD4, 0x0000676F},
     {0x8EA5F293, 0x000003B8}},
    /* HuaTianKaiTi */
    {{0xFFFBFFFC, 0x00000008},
     {0x9C9E48B8, 0x0000BEA2},
     {0x70020112, 0x00000008}},
    /* HuaTianSongTi */
    {{0xFFFBFFFC, 0x00000008},
     {0x0A5A0483, 0x00017C39},
     {0x70020112, 0x00000008}},
    /* NEC fadpop7.ttf */
    {{0x00000000, 0x00000000},
     {0x40C92555, 0x000000E5},
     {0xA39B58E3, 0x0000117C}},
    /* NEC fadrei5.ttf */
    {{0x00000000, 0x00000000},
     {0x33C41652, 0x000000E5},
     {0x26D6C52A, 0x00000F6A}},
    /* NEC fangot7.ttf */
    {{0x00000000, 0x00000000},
     {0x6DB1651D, 0x0000019D},
     {0x6C6E4B03, 0x00002492}},
    /* NEC fangyo5.ttf */
    {{0x00000000, 0x00000000},
     {0x40C92555, 0x000000E5},
     {0xDE51FAD0, 0x0000117C}},
    /* NEC fankyo5.ttf */
    {{0x00000000, 0x00000000},
     {0x85E47664, 0x000000E5},
     {0xA6C62831, 0x00001CAA}},
    /* NEC fanrgo5.ttf */
    {{0x00000000, 0x00000000},
     {0x2D891CFD, 0x0000019D},
     {0xA0604633, 0x00001DE8}},
    /* NEC fangot5.ttc */
    {{0x00000000, 0x00000000},
     {0x40AA774C, 0x000001CB},
     {0x9B5CAA96, 0x00001F9A}},
    /* NEC fanmin3.ttc */
    {{0x00000000, 0x00000000},
     {0x0D3DE9CB, 0x00000141},
     {0xD4127766, 0x00002280}},
    /* NEC FA-Gothic, 1996 */
    {{0x00000000, 0x00000000},
     {0x4A692698, 0x000001F0},
     {0x340D4346, 0x00001FCA}},
    /* NEC FA-Minchou, 1996 */
    {{0x00000000, 0x00000000},
     {0xCD34C604, 0x00000166},
     {0x6CF31046, 0x000022B0}},
    /* NEC FA-RoundGothicB, 1996 */
    {{0x00000000, 0x00000000},
     {0x5DA75315, 0x0000019D},
     {0x40745A5F, 0x000022E0}},
    /* NEC FA-RoundGothicM, 1996 */
    {{0x00000000, 0x00000000},
     {0xF055FC48, 0x000001C2},
     {0x3900DED3, 0x00001E18}},
    /* MINGLI.TTF, 1992 */
    {{0x00170003, 0x00000060},
     {0xDBB4306E, 0x000058AA},
     {0xD643482A, 0x00000035}},
    /* DFHei-Bd-WIN-HK-BF */
    {{0x1269EB58, 0x00000350},
     {0x5CD5957A, 0x00006A4E},
     {0xF758323A, 0x00000380}},
    /* DFMing-Md-WIN-HK-BF */
    {{0x122FEB0B, 0x00000350},
     {0x7F10919A, 0x000070A9},
     {0x7CD7E7B7, 0x0000025C}},
};

static inline uint32_t
_table_checksum (const char *data, unsigned length)
{
  uint32_t sum = 0;
  unsigned i = 0;
  for (; i + 4 <= length; i += 4)
    sum += ((uint32_t) (uint8_t) data[i] << 24) |
	   ((uint32_t) (uint8_t) data[i + 1] << 16) |
	   ((uint32_t) (uint8_t) data[i + 2] << 8) |
	   (uint32_t) (uint8_t) data[i + 3];

  uint32_t rem = 0;
  if (i < length) rem |= (uint32_t) (uint8_t) data[i] << 24;
  if (i + 1 < length) rem |= (uint32_t) (uint8_t) data[i + 1] << 16;
  if (i + 2 < length) rem |= (uint32_t) (uint8_t) data[i + 2] << 8;

  return sum + rem;
}

static inline hint_dependent_table_id_t
_table_id (hb_face_t *face, hb_tag_t tag)
{
  hint_dependent_table_id_t id = {0, 0};

  hb_blob_t *blob = hb_face_reference_table (face, tag);
  unsigned length = hb_blob_get_length (blob);
  if (length)
  {
    unsigned data_length;
    const char *data = hb_blob_get_data (blob, &data_length);
    id.checksum = _table_checksum (data, data_length);
    id.length = data_length;
  }
  hb_blob_destroy (blob);

  return id;
}

static inline bool
_matches_hint_dependent_ids (hb_face_t *face)
{
  hint_dependent_font_id_t id;
  id.cvt = _table_id (face, HB_TAG ('c', 'v', 't', ' '));
  id.fpgm = _table_id (face, HB_TAG ('f', 'p', 'g', 'm'));
  id.prep = _table_id (face, HB_TAG ('p', 'r', 'e', 'p'));

  for (const auto &candidate : hint_dependent_font_ids)
    if (id.cvt.checksum == candidate.cvt.checksum &&
	id.cvt.length == candidate.cvt.length &&
	id.fpgm.checksum == candidate.fpgm.checksum &&
	id.fpgm.length == candidate.fpgm.length &&
	id.prep.checksum == candidate.prep.checksum &&
	id.prep.length == candidate.prep.length)
      return true;

  return false;
}

static inline bool
_has_glyf_outlines (hb_face_t *face)
{
  static const hb_tag_t tags[] = {
      HB_TAG ('g', 'l', 'y', 'f'),
#ifndef HB_NO_BEYOND_64K
      HB_TAG ('G', 'L', 'Y', 'F'),
#endif
  };

  for (hb_tag_t tag : tags)
  {
    hb_blob_t *blob = hb_face_reference_table (face, tag);
    bool has_data = hb_blob_get_length (blob) != 0;
    hb_blob_destroy (blob);
    if (has_data) return true;
  }

  return false;
}

static inline bool
_hb_face_requires_hinting (hb_face_t *face)
{
  if (!_has_glyf_outlines (face)) return false;

#ifndef HB_NO_NAME
  unsigned count;
  const hb_ot_name_entry_t *names = hb_ot_name_list_names (face, &count);
  for (unsigned i = 0; i < count; i++)
  {
    hb_ot_name_id_t name_id = names[i].name_id;
    if (name_id != HB_OT_NAME_ID_FONT_FAMILY &&
	name_id != HB_OT_NAME_ID_TYPOGRAPHIC_FAMILY &&
	name_id != HB_OT_NAME_ID_WWS_FAMILY)
      continue;

    if (_name_matches_hint_dependent_family (face, name_id, names[i].language))
      return true;
  }
#endif

  return _matches_hint_dependent_ids (face);
}

#endif /* HB_SUBSET_HINTING_HH */
