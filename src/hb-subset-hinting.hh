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

#include "hb.hh"
#include "hb-open-type.hh"
#include "hb-ot-face.hh"
#include "hb-ot-glyf-table.hh"
#include "hb-ot-name-table.hh"
#include "hb-ot-os2-table.hh"
#include "hb-utf.hh"

#ifndef HB_NO_NAME
/* Some legacy fonts have hinting programs that are required for correct
 * rendering; dropping them changes glyph advances and outlines.  FreeType
 * likewise refuses to load these fonts without hinting.  The family names are
 * from src/truetype/ttobjs.c in FreeType. */
static const char hint_dependent_font_families[][19] = {
    "cpop",	   "DFGirl-W6-WIN-BF", "DFGothic-EB",	 "DFGyoSho-Lt",
    "DFHei",	   "DFHSGothic-W5",    "DFHSMincho-W3",	 "DFHSMincho-W7",
    "DFKaiSho-SB", "DFKaiShu",	       "DFKai-SB",	 "DFMing",
    "DLC",	   "HuaTianKaiTi?",    "HuaTianSongTi?", "Ming(for ISO10646)",
    "MingLiU",	   "MingMedium",       "PMingLiU",	 "MingLi43",
};

/* Choose the name record FreeType would use for @name_id:
 * Windows English, then Macintosh English/Roman, then Unicode. */
static inline const OT::NameRecord *
_find_hint_dependent_name_record (const OT::name &name,
				  hb_ot_name_id_t name_id,
				  bool *utf16)
{
  const OT::NameRecord *windows = nullptr;
  const OT::NameRecord *apple_english = nullptr;
  const OT::NameRecord *apple_roman = nullptr;
  const OT::NameRecord *unicode = nullptr;
  bool windows_is_english = false;

  for (unsigned i = 0; i < (unsigned) name.count; i++)
  {
    const OT::NameRecord &record = name.nameRecordZ[i];
    if (record.nameID != name_id || !record.length) continue;

    switch ((unsigned) record.platformID)
    {
    case 0:
    case 2: unicode = &record; break;

    case 1:
      if (record.languageID == 0) apple_english = &record;
      if (record.encodingID == 0) apple_roman = &record;
      break;

    case 3: {
      bool english = (record.languageID & 0x3ff) == 0x009;
      if ((!windows || english) &&
	  (record.encodingID == 0 || record.encodingID == 1 ||
	   record.encodingID == 10))
      {
	windows = &record;
	windows_is_english = english;
      }
      break;
    }
    }
  }

  const OT::NameRecord *apple = apple_english ? apple_english : apple_roman;
  if (windows && (!apple || windows_is_english))
  {
    *utf16 = true;
    return windows;
  }
  if (apple)
  {
    *utf16 = false;
    return apple;
  }
  if (unicode)
  {
    *utf16 = true;
    return unicode;
  }

  return nullptr;
}

static inline bool
_name_matches_hint_dependent_family (hb_face_t *face,
				     hb_ot_name_id_t name_id,
				     bool *found)
{
  *found = false;
  hb_blob_ptr_t<OT::name> name_table =
      hb_sanitize_context_t ().reference_table<OT::name> (face);
  unsigned table_length = name_table.get_length ();
  if (!table_length) return false;

  const OT::name &table = *name_table;
  bool utf16 = false;
  const OT::NameRecord *record =
      _find_hint_dependent_name_record (table, name_id, &utf16);
  if (!record) return false;

  *found = true;

  unsigned string_offset = table.stringOffset;
  if (string_offset >= table_length) return false;

  unsigned pool_length = table_length - string_offset;
  unsigned record_offset = record->offset;
  if (record_offset >= pool_length) return false;

  unsigned record_length =
      hb_min ((unsigned) record->length, pool_length - record_offset);
  const char *string_data =
      (const char *) &table + string_offset + record_offset;
  unsigned max_chars = utf16 ? record_length / 2 : record_length;

  char stack_name[128];
  hb_vector_t<char> name;
  if (max_chars + 1 <= sizeof (stack_name))
  {
    name.set_storage (stack_name);
    name.shrink (max_chars + 1);
  }
  else if (unlikely (!name.resize (max_chars + 1)))
    return false;

  char *name_string = &name[0];
  char *out = name_string;
  if (utf16)
  {
    for (unsigned i = 0; i + 1 < record_length; i += 2)
    {
      uint32_t cp =
	  ((uint8_t) string_data[i] << 8) | (uint8_t) string_data[i + 1];
      if (!cp) break;
      *out++ = (cp >= 32 && cp <= 127) ? (char) cp : '?';
    }
  }
  else
  {
    for (unsigned i = 0; i < record_length; i++)
    {
      uint8_t cp = string_data[i];
      if (!cp) break;
      *out++ = (cp >= 32 && cp <= 127) ? (char) cp : '?';
    }
  }
  *out = '\0';

  /* PDF subset tags are exactly six uppercase ASCII letters followed by
   * `+`; FreeType checks the same with ft_isupper(), so don't accept
   * lowercase or digits here. */
  const char *family = name_string;
  bool tagged =
      (uint8_t *) out - (uint8_t *) name_string >= 8 && family[6] == '+';
  if (tagged)
    for (unsigned i = 0; i < 6; i++)
      if (!ISALPHA ((unsigned char) family[i]) ||
	  TOUPPER ((unsigned char) family[i]) != (unsigned char) family[i])
      {
	tagged = false;
	break;
      }
  if (tagged) family += 7;

  for (const auto &candidate : hint_dependent_font_families)
    if (strstr (family, candidate)) return true;

  return false;
}
#endif

struct hint_dependent_table_id_t
{
  bool operator== (const hint_dependent_table_id_t &o) const
  { return checksum == o.checksum && length == o.length; }

  uint32_t checksum;
  uint32_t length;
};

struct hint_dependent_font_id_t
{
  bool operator== (const hint_dependent_font_id_t &o) const
  { return cvt == o.cvt && fpgm == o.fpgm && prep == o.prep; }

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
    id.checksum = OT::CheckSum::CalcTableChecksum (
	(const OT::HBUINT32 *) data, data_length);
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
    if (id == candidate) return true;

  return false;
}

enum class hint_dependent_cache_t : uintptr_t
{
  UNCACHED,
  CACHED_FALSE,
  CACHED_TRUE,
};

static hb_user_data_key_t _hb_subset_hint_dependent_user_data_key = {0};

static inline bool
_hb_subset_face_requires_hinting_uncached (hb_face_t *face)
{
  if (!face->table.glyf->has_data ()) return false;

#ifndef HB_NO_NAME
  /* Match FreeType's choice: with the OS/2 WWS bit set the typographic
   * family is preferred; otherwise the WWS family wins over it. */
  hb_ot_name_id_t name_ids[3];
  unsigned name_count;
  if (face->table.OS2->fsSelection & OT::OS2::WWS)
  {
    name_ids[0] = HB_OT_NAME_ID_TYPOGRAPHIC_FAMILY;
    name_ids[1] = HB_OT_NAME_ID_FONT_FAMILY;
    name_count = 2;
  }
  else
  {
    name_ids[0] = HB_OT_NAME_ID_WWS_FAMILY;
    name_ids[1] = HB_OT_NAME_ID_TYPOGRAPHIC_FAMILY;
    name_ids[2] = HB_OT_NAME_ID_FONT_FAMILY;
    name_count = 3;
  }
  for (unsigned i = 0; i < name_count; i++)
  {
    bool found = false;
    if (_name_matches_hint_dependent_family (face, name_ids[i], &found))
      return true;
    /* Do not fall through to a lower-priority family name if the
     * preferred one exists but does not match. */
    if (found) break;
  }
#endif

  return _matches_hint_dependent_ids (face);
}

static inline bool
_hb_subset_face_requires_hinting (hb_face_t *face)
{
  /* The user-data pointer is the enum value; UNCACHED is zero. */
  hint_dependent_cache_t cached = static_cast<hint_dependent_cache_t> (
      (uintptr_t) hb_face_get_user_data (face,
					 &_hb_subset_hint_dependent_user_data_key));
  if (cached != hint_dependent_cache_t::UNCACHED)
    return cached == hint_dependent_cache_t::CACHED_TRUE;

  bool result = _hb_subset_face_requires_hinting_uncached (face);

  hint_dependent_cache_t cache = result ? hint_dependent_cache_t::CACHED_TRUE
					: hint_dependent_cache_t::CACHED_FALSE;
  hb_face_set_user_data (face, &_hb_subset_hint_dependent_user_data_key,
			 (void *) static_cast<uintptr_t> (cache),
			 nullptr, false);

  return result;
}

#endif /* HB_SUBSET_HINTING_HH */
