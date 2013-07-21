/*
 * Copyright © 2011,2012  Google, Inc.
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

#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN

#define HB_SHAPER uniscribe
#include "hb-shaper-impl-private.hh"

#include <windows.h>
#include <usp10.h>
#include <rpc.h>

#include "hb-uniscribe.h"

#include "hb-open-file-private.hh"
#include "hb-ot-name-table.hh"
#include "hb-ot-tag.h"


#ifndef HB_DEBUG_UNISCRIBE
#define HB_DEBUG_UNISCRIBE (HB_DEBUG+0)
#endif


typedef HRESULT (WINAPI *SIOT) /*ScriptItemizeOpenType*/(
  const WCHAR *pwcInChars,
  int cInChars,
  int cMaxItems,
  const SCRIPT_CONTROL *psControl,
  const SCRIPT_STATE *psState,
  SCRIPT_ITEM *pItems,
  OPENTYPE_TAG *pScriptTags,
  int *pcItems
);

typedef HRESULT (WINAPI *SSOT) /*ScriptShapeOpenType*/(
  HDC hdc,
  SCRIPT_CACHE *psc,
  SCRIPT_ANALYSIS *psa,
  OPENTYPE_TAG tagScript,
  OPENTYPE_TAG tagLangSys,
  int *rcRangeChars,
  TEXTRANGE_PROPERTIES **rpRangeProperties,
  int cRanges,
  const WCHAR *pwcChars,
  int cChars,
  int cMaxGlyphs,
  WORD *pwLogClust,
  SCRIPT_CHARPROP *pCharProps,
  WORD *pwOutGlyphs,
  SCRIPT_GLYPHPROP *pOutGlyphProps,
  int *pcGlyphs
);

typedef HRESULT (WINAPI *SPOT) /*ScriptPlaceOpenType*/(
  HDC hdc,
  SCRIPT_CACHE *psc,
  SCRIPT_ANALYSIS *psa,
  OPENTYPE_TAG tagScript,
  OPENTYPE_TAG tagLangSys,
  int *rcRangeChars,
  TEXTRANGE_PROPERTIES **rpRangeProperties,
  int cRanges,
  const WCHAR *pwcChars,
  WORD *pwLogClust,
  SCRIPT_CHARPROP *pCharProps,
  int cChars,
  const WORD *pwGlyphs,
  const SCRIPT_GLYPHPROP *pGlyphProps,
  int cGlyphs,
  int *piAdvance,
  GOFFSET *pGoffset,
  ABC *pABC
);


/* Fallback implementations. */

static HRESULT WINAPI
hb_ScriptItemizeOpenType(
  const WCHAR *pwcInChars,
  int cInChars,
  int cMaxItems,
  const SCRIPT_CONTROL *psControl,
  const SCRIPT_STATE *psState,
  SCRIPT_ITEM *pItems,
  OPENTYPE_TAG *pScriptTags,
  int *pcItems
)
{
{
  return ScriptItemize (pwcInChars,
			cInChars,
			cMaxItems,
			psControl,
			psState,
			pItems,
			pcItems);
}
}

static HRESULT WINAPI
hb_ScriptShapeOpenType(
  HDC hdc,
  SCRIPT_CACHE *psc,
  SCRIPT_ANALYSIS *psa,
  OPENTYPE_TAG tagScript,
  OPENTYPE_TAG tagLangSys,
  int *rcRangeChars,
  TEXTRANGE_PROPERTIES **rpRangeProperties,
  int cRanges,
  const WCHAR *pwcChars,
  int cChars,
  int cMaxGlyphs,
  WORD *pwLogClust,
  SCRIPT_CHARPROP *pCharProps,
  WORD *pwOutGlyphs,
  SCRIPT_GLYPHPROP *pOutGlyphProps,
  int *pcGlyphs
)
{
  SCRIPT_VISATTR *psva = (SCRIPT_VISATTR *) pOutGlyphProps;
  return ScriptShape (hdc,
		      psc,
		      pwcChars,
		      cChars,
		      cMaxGlyphs,
		      psa,
		      pwOutGlyphs,
		      pwLogClust,
		      psva,
		      pcGlyphs);
}

static HRESULT WINAPI
hb_ScriptPlaceOpenType(
  HDC hdc,
  SCRIPT_CACHE *psc,
  SCRIPT_ANALYSIS *psa,
  OPENTYPE_TAG tagScript,
  OPENTYPE_TAG tagLangSys,
  int *rcRangeChars,
  TEXTRANGE_PROPERTIES **rpRangeProperties,
  int cRanges,
  const WCHAR *pwcChars,
  WORD *pwLogClust,
  SCRIPT_CHARPROP *pCharProps,
  int cChars,
  const WORD *pwGlyphs,
  const SCRIPT_GLYPHPROP *pGlyphProps,
  int cGlyphs,
  int *piAdvance,
  GOFFSET *pGoffset,
  ABC *pABC
)
{
  SCRIPT_VISATTR *psva = (SCRIPT_VISATTR *) pGlyphProps;
  return ScriptPlace (hdc,
		      psc,
		      pwGlyphs,
		      cGlyphs,
		      psva,
		      psa,
		      piAdvance,
		      pGoffset,
		      pABC);
}


struct hb_uniscribe_shaper_funcs_t {
  SIOT ScriptItemizeOpenType;
  SSOT ScriptShapeOpenType;
  SPOT ScriptPlaceOpenType;

  inline void init (void)
  {
    HMODULE hinstLib;
    this->ScriptItemizeOpenType = NULL;
    this->ScriptShapeOpenType   = NULL;
    this->ScriptPlaceOpenType   = NULL;

    hinstLib = GetModuleHandle("usp10.dll");
    if (hinstLib)
    {
      this->ScriptItemizeOpenType = (SIOT) GetProcAddress (hinstLib, "ScriptItemizeOpenType");
      this->ScriptShapeOpenType   = (SSOT) GetProcAddress (hinstLib, "ScriptShapeOpenType");
      this->ScriptPlaceOpenType   = (SPOT) GetProcAddress (hinstLib, "ScriptPlaceOpenType");
    }
    if (!this->ScriptItemizeOpenType ||
	!this->ScriptShapeOpenType   ||
	!this->ScriptPlaceOpenType)
    {
      DEBUG_MSG (UNISCRIBE, NULL, "OpenType versions of functions not found; falling back.");
      this->ScriptItemizeOpenType = hb_ScriptItemizeOpenType;
      this->ScriptShapeOpenType   = hb_ScriptShapeOpenType;
      this->ScriptPlaceOpenType   = hb_ScriptPlaceOpenType;
    }
  }
};
static hb_uniscribe_shaper_funcs_t *uniscribe_funcs;

static inline void
free_uniscribe_funcs (void)
{
  free (uniscribe_funcs);
}


static hb_uniscribe_shaper_funcs_t *
hb_uniscribe_shaper_get_funcs (void)
{
retry:
  hb_uniscribe_shaper_funcs_t *funcs = (hb_uniscribe_shaper_funcs_t *) hb_atomic_ptr_get (&uniscribe_funcs);

  if (unlikely (!funcs))
  {
    funcs = (hb_uniscribe_shaper_funcs_t *) calloc (1, sizeof (hb_uniscribe_shaper_funcs_t));
    if (unlikely (!funcs))
      return NULL;

    funcs->init ();

    if (!hb_atomic_ptr_cmpexch (&uniscribe_funcs, NULL, funcs)) {
      free (funcs);
      goto retry;
    }

#ifdef HAVE_ATEXIT
    atexit (free_uniscribe_funcs); /* First person registers atexit() callback. */
#endif
  }

  return funcs;
}


HB_SHAPER_DATA_ENSURE_DECLARE(uniscribe, face)
HB_SHAPER_DATA_ENSURE_DECLARE(uniscribe, font)


/*
 * shaper face data
 */

struct hb_uniscribe_shaper_face_data_t {
  HANDLE fh;
  hb_uniscribe_shaper_funcs_t *funcs;
  wchar_t face_name[LF_FACESIZE];
};

/* Destroys blob. */
static hb_blob_t *
_hb_rename_font (hb_blob_t *blob, wchar_t *new_name)
{
  unsigned int length, new_length = 0;
  const char *orig_sfnt_data = hb_blob_get_data (blob, &length);
  /* We'll create a private name for the font from a UUID using a simple,
   * somewhat base64-like encoding scheme */
  const char *enc = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";
  UUID id;
  UuidCreate ((UUID*) &id);
  unsigned int name_str_len = 0;
  new_name[name_str_len++] = 'F';
  new_name[name_str_len++] = '_';
  unsigned char *p = (unsigned char *) &id;
  for (unsigned int i = 0; i < 16; i += 2)
  {
    /* Spread the 16 bits from two bytes of the UUID across three chars of face_name,
     * using the bits in groups of 5,5,6 to select chars from enc.
     * This will generate 24 characters; with the 'F_' prefix we already provided,
     * the name will be 26 chars (plus the NUL terminator), so will always fit within
     * face_name (LF_FACESIZE = 32). */
    new_name[name_str_len++] = enc[p[i] >> 3];
    new_name[name_str_len++] = enc[((p[i] << 2) | (p[i + 1] >> 6)) & 0x1f];
    new_name[name_str_len++] = enc[p[i + 1] & 0x3f];
  }
  new_name[name_str_len] = 0;

  /* Create a copy of the font data, with the 'name' table replaced by a table
   * that names the font with our private F_* name created above.
   * For simplicity, we just append a new 'name' table and update the sfnt directory;
   * the original table is left in place, but unused. */

  /* The new table will contain just 5 name IDs: family, style, unique, full, PS.
   * All of them point to the same name data with our unique F_* name. */

  static const uint16_t name_IDs[] = { 1, 2, 3, 4, 6 };

  const unsigned int name_header_size = 3 * 2; /* NameTableHeader: USHORT x 3 */
  const unsigned int name_record_size = 6 * 2; /* NameRecord: USHORT x 6 */
  const unsigned int name_count = sizeof (name_IDs) / sizeof (name_IDs[0]);
  unsigned int name_table_length = name_header_size +
                                   name_count * name_record_size +
                                   name_str_len * 2; /* for name data in UTF16BE form */

  unsigned int name_table_offset = (length + 3) & ~3;

  new_length = name_table_offset + (name_table_length + 3) & ~3;
  char *new_sfnt_data = (char *) calloc (1, new_length);
  if (!new_sfnt_data)
    return NULL;

  memcpy(new_sfnt_data, orig_sfnt_data, length);

  unsigned char *name_table_data = (unsigned char *) (new_sfnt_data + name_table_offset);

  p = name_table_data;
  *p++ = 0x00; *p++ = 0x00; /* format = 0 */
  *p++ = 0x00; *p++ = name_count; /* number of name records */
  *p++ = 0x00; *p++ = name_header_size + name_count * name_record_size; /* offset to string data */

  for (unsigned int i = 0; i < name_count; i++) {
    *p++ = 0x00; *p++ = 0x03; /* platformID */
    *p++ = 0x00; *p++ = 0x01; /* encodingID */
    *p++ = 0x04; *p++ = 0x09; /* languageID = 0x0409 */
    *p++ = 0x00; *p++ = name_IDs[i]; /* nameID */
    *p++ = 0x00; *p++ = name_str_len * 2; /* string length (bytes) */
    *p++ = 0x00; *p++ = 0x00; /* string offset */
  }

  for (unsigned int i = 0; i < name_str_len; i++) {
    /* copy string data from face_name, converting wchar_t to UTF16BE */
    *p++ = new_name[i] >> 8;
    *p++ = new_name[i] & 0xff;
  }

  /* calculate new name table checksum */
  uint32_t checksum = 0;
  while (name_table_data < p) {
    checksum += (name_table_data[0] << 24) +
                (name_table_data[1] << 16) +
                (name_table_data[2] << 8) +
                 name_table_data[3];
    name_table_data += 4;
  }

  /* adjust name table entry to point to new name table */
  OT::OpenTypeFontFace *face = (OT::OpenTypeFontFace *) (new_sfnt_data);
  unsigned int index;
  if (face->find_table_index (HB_OT_TAG_name, &index))
  {
    const OT::TableRecord& record = face->get_table (index);
    OT::TableRecord *rp = const_cast<OT::TableRecord *> (&record);
    rp->checkSum.set(checksum);
    rp->offset.set(name_table_offset);
    rp->length.set(name_table_length);
  }

  /* The checkSumAdjustment field in the 'head' table is now wrong,
     but AFAIK that doesn't actually cause any problems so I haven't
     bothered to fix it. */

  hb_blob_destroy (blob);
  return hb_blob_create (new_sfnt_data, new_length, HB_MEMORY_MODE_WRITABLE, NULL, free);
}

hb_uniscribe_shaper_face_data_t *
_hb_uniscribe_shaper_face_data_create (hb_face_t *face)
{
  hb_uniscribe_shaper_face_data_t *data = (hb_uniscribe_shaper_face_data_t *) calloc (1, sizeof (hb_uniscribe_shaper_face_data_t));
  if (unlikely (!data))
    return NULL;

  data->funcs = hb_uniscribe_shaper_get_funcs ();
  if (unlikely (!data->funcs))
  {
    free (data);
    return NULL;
  }

  hb_blob_t *blob = hb_face_reference_blob (face);
  if (unlikely (!hb_blob_get_length (blob)))
    DEBUG_MSG (UNISCRIBE, face, "Face has empty blob");

  blob = _hb_rename_font (blob, data->face_name);

  DWORD num_fonts_installed;
  data->fh = AddFontMemResourceEx ((void *) hb_blob_get_data (blob, NULL),
				   hb_blob_get_length (blob),
				   0, &num_fonts_installed);
  if (unlikely (!data->fh)) {
    DEBUG_MSG (UNISCRIBE, face, "Face AddFontMemResourceEx() failed");
    free (data);
    return NULL;
  }

  return data;
}

void
_hb_uniscribe_shaper_face_data_destroy (hb_uniscribe_shaper_face_data_t *data)
{
  RemoveFontMemResourceEx (data->fh);
  free (data);
}


/*
 * shaper font data
 */

struct hb_uniscribe_shaper_font_data_t {
  HDC hdc;
  LOGFONTW log_font;
  HFONT hfont;
  SCRIPT_CACHE script_cache;
};

static bool
populate_log_font (LOGFONTW  *lf,
		   hb_font_t *font)
{
  memset (lf, 0, sizeof (*lf));
  lf->lfHeight = -font->y_scale;
  lf->lfCharSet = DEFAULT_CHARSET;

  hb_face_t *face = font->face;
  hb_uniscribe_shaper_face_data_t *face_data = HB_SHAPER_DATA_GET (face);

  memcpy (lf->lfFaceName, face_data->face_name, sizeof (lf->lfFaceName));

  return true;
}

hb_uniscribe_shaper_font_data_t *
_hb_uniscribe_shaper_font_data_create (hb_font_t *font)
{
  if (unlikely (!hb_uniscribe_shaper_face_data_ensure (font->face))) return NULL;

  hb_uniscribe_shaper_font_data_t *data = (hb_uniscribe_shaper_font_data_t *) calloc (1, sizeof (hb_uniscribe_shaper_font_data_t));
  if (unlikely (!data))
    return NULL;

  data->hdc = GetDC (NULL);

  if (unlikely (!populate_log_font (&data->log_font, font))) {
    DEBUG_MSG (UNISCRIBE, font, "Font populate_log_font() failed");
    _hb_uniscribe_shaper_font_data_destroy (data);
    return NULL;
  }

  data->hfont = CreateFontIndirectW (&data->log_font);
  if (unlikely (!data->hfont)) {
    DEBUG_MSG (UNISCRIBE, font, "Font CreateFontIndirectW() failed");
    _hb_uniscribe_shaper_font_data_destroy (data);
     return NULL;
  }

  if (!SelectObject (data->hdc, data->hfont)) {
    DEBUG_MSG (UNISCRIBE, font, "Font SelectObject() failed");
    _hb_uniscribe_shaper_font_data_destroy (data);
     return NULL;
  }

  return data;
}

void
_hb_uniscribe_shaper_font_data_destroy (hb_uniscribe_shaper_font_data_t *data)
{
  if (data->hdc)
    ReleaseDC (NULL, data->hdc);
  if (data->hfont)
    DeleteObject (data->hfont);
  if (data->script_cache)
    ScriptFreeCache (&data->script_cache);
  free (data);
}

LOGFONTW *
hb_uniscribe_font_get_logfontw (hb_font_t *font)
{
  if (unlikely (!hb_uniscribe_shaper_font_data_ensure (font))) return NULL;
  hb_uniscribe_shaper_font_data_t *font_data =  HB_SHAPER_DATA_GET (font);
  return &font_data->log_font;
}

HFONT
hb_uniscribe_font_get_hfont (hb_font_t *font)
{
  if (unlikely (!hb_uniscribe_shaper_font_data_ensure (font))) return NULL;
  hb_uniscribe_shaper_font_data_t *font_data =  HB_SHAPER_DATA_GET (font);
  return font_data->hfont;
}


/*
 * shaper shape_plan data
 */

struct hb_uniscribe_shaper_shape_plan_data_t {};

hb_uniscribe_shaper_shape_plan_data_t *
_hb_uniscribe_shaper_shape_plan_data_create (hb_shape_plan_t    *shape_plan HB_UNUSED,
					     const hb_feature_t *user_features HB_UNUSED,
					     unsigned int        num_user_features HB_UNUSED)
{
  return (hb_uniscribe_shaper_shape_plan_data_t *) HB_SHAPER_DATA_SUCCEEDED;
}

void
_hb_uniscribe_shaper_shape_plan_data_destroy (hb_uniscribe_shaper_shape_plan_data_t *data HB_UNUSED)
{
}


/*
 * shaper
 */


hb_bool_t
_hb_uniscribe_shape (hb_shape_plan_t    *shape_plan,
		     hb_font_t          *font,
		     hb_buffer_t        *buffer,
		     const hb_feature_t *features,
		     unsigned int        num_features)
{
  hb_face_t *face = font->face;
  hb_uniscribe_shaper_face_data_t *face_data = HB_SHAPER_DATA_GET (face);
  hb_uniscribe_shaper_font_data_t *font_data = HB_SHAPER_DATA_GET (font);
  hb_uniscribe_shaper_funcs_t *funcs = face_data->funcs;

#define FAIL(...) \
  HB_STMT_START { \
    DEBUG_MSG (UNISCRIBE, NULL, __VA_ARGS__); \
    return false; \
  } HB_STMT_END;

  HRESULT hr;

retry:

  unsigned int scratch_size;
  char *scratch = (char *) buffer->get_scratch_buffer (&scratch_size);

  /* Allocate char buffers; they all fit */

#define ALLOCATE_ARRAY(Type, name, len) \
  Type *name = (Type *) scratch; \
  scratch += (len) * sizeof ((name)[0]); \
  scratch_size -= (len) * sizeof ((name)[0]);

#define utf16_index() var1.u32

  WCHAR *pchars = (WCHAR *) scratch;
  unsigned int chars_len = 0;
  for (unsigned int i = 0; i < buffer->len; i++) {
    hb_codepoint_t c = buffer->info[i].codepoint;
    buffer->info[i].utf16_index() = chars_len;
    if (likely (c < 0x10000))
      pchars[chars_len++] = c;
    else if (unlikely (c >= 0x110000))
      pchars[chars_len++] = 0xFFFD;
    else {
      pchars[chars_len++] = 0xD800 + ((c - 0x10000) >> 10);
      pchars[chars_len++] = 0xDC00 + ((c - 0x10000) & ((1 << 10) - 1));
    }
  }

  ALLOCATE_ARRAY (WCHAR, wchars, chars_len);
  ALLOCATE_ARRAY (WORD, log_clusters, chars_len);
  ALLOCATE_ARRAY (SCRIPT_CHARPROP, char_props, chars_len);

  /* On Windows, we don't care about alignment...*/
  unsigned int glyphs_size = scratch_size / (sizeof (WORD) +
					     sizeof (SCRIPT_GLYPHPROP) +
					     sizeof (int) +
					     sizeof (GOFFSET) +
					     sizeof (uint32_t));

  ALLOCATE_ARRAY (WORD, glyphs, glyphs_size);
  ALLOCATE_ARRAY (SCRIPT_GLYPHPROP, glyph_props, glyphs_size);
  ALLOCATE_ARRAY (int, advances, glyphs_size);
  ALLOCATE_ARRAY (GOFFSET, offsets, glyphs_size);
  ALLOCATE_ARRAY (uint32_t, vis_clusters, glyphs_size);

  /* Note:
   * We can't touch the contents of glyph_props.  Our fallback
   * implementations of Shape and Place functions use that buffer
   * by casting it to a different type.  It works because they
   * both agree about it, but if we want to access it here we
   * need address that issue first.
   */

#undef ALLOCATE_ARRAY

#define MAX_ITEMS 256

  SCRIPT_ITEM items[MAX_ITEMS + 1];
  SCRIPT_CONTROL bidi_control = {0};
  SCRIPT_STATE bidi_state = {0};
  ULONG script_tags[MAX_ITEMS];
  int item_count;

  /* MinGW32 doesn't define fMergeNeutralItems, so we bruteforce */
  //bidi_control.fMergeNeutralItems = true;
  *(uint32_t*)&bidi_control |= 1<<24;

  bidi_state.uBidiLevel = HB_DIRECTION_IS_FORWARD (buffer->props.direction) ? 0 : 1;
  bidi_state.fOverrideDirection = 1;

  hr = funcs->ScriptItemizeOpenType (wchars,
				     chars_len,
				     MAX_ITEMS,
				     &bidi_control,
				     &bidi_state,
				     items,
				     script_tags,
				     &item_count);
  if (unlikely (FAILED (hr)))
    FAIL ("ScriptItemizeOpenType() failed: 0x%08xL", hr);

#undef MAX_ITEMS

  int *range_char_counts = NULL;
  TEXTRANGE_PROPERTIES **range_properties = NULL;
  int range_count = 0;
  if (num_features) {
    /* TODO setup ranges */
  }

  OPENTYPE_TAG language_tag = hb_uint32_swap (hb_ot_tag_from_language (buffer->props.language));

  unsigned int glyphs_offset = 0;
  unsigned int glyphs_len;
  bool backward = HB_DIRECTION_IS_BACKWARD (buffer->props.direction);
  for (unsigned int j = 0; j < item_count; j++)
  {
    unsigned int i = backward ? item_count - 1 - j : j;
    unsigned int chars_offset = items[i].iCharPos;
    unsigned int item_chars_len = items[i + 1].iCharPos - chars_offset;

  retry_shape:
    hr = funcs->ScriptShapeOpenType (font_data->hdc,
				     &font_data->script_cache,
				     &items[i].a,
				     script_tags[i],
				     language_tag,
				     range_char_counts,
				     range_properties,
				     range_count,
				     wchars + chars_offset,
				     item_chars_len,
				     glyphs_size - glyphs_offset,
				     /* out */
				     log_clusters + chars_offset,
				     char_props + chars_offset,
				     glyphs + glyphs_offset,
				     glyph_props + glyphs_offset,
				     (int *) &glyphs_len);

    if (unlikely (items[i].a.fNoGlyphIndex))
      FAIL ("ScriptShapeOpenType() set fNoGlyphIndex");
    if (unlikely (hr == E_OUTOFMEMORY))
    {
      buffer->ensure (buffer->allocated * 2);
      if (buffer->in_error)
	FAIL ("Buffer resize failed");
      goto retry;
    }
    if (unlikely (hr == USP_E_SCRIPT_NOT_IN_FONT))
    {
      if (items[i].a.eScript == SCRIPT_UNDEFINED)
	FAIL ("ScriptShapeOpenType() failed: Font doesn't support script");
      items[i].a.eScript = SCRIPT_UNDEFINED;
      goto retry_shape;
    }
    if (unlikely (FAILED (hr)))
    {
      FAIL ("ScriptShapeOpenType() failed: 0x%08xL", hr);
    }

    for (unsigned int j = chars_offset; j < chars_offset + item_chars_len; j++)
      log_clusters[j] += glyphs_offset;

    hr = funcs->ScriptPlaceOpenType (font_data->hdc,
				     &font_data->script_cache,
				     &items[i].a,
				     script_tags[i],
				     language_tag,
				     range_char_counts,
				     range_properties,
				     range_count,
				     wchars + chars_offset,
				     log_clusters + chars_offset,
				     char_props + chars_offset,
				     item_chars_len,
				     glyphs + glyphs_offset,
				     glyph_props + glyphs_offset,
				     glyphs_len,
				     /* out */
				     advances + glyphs_offset,
				     offsets + glyphs_offset,
				     NULL);
    if (unlikely (FAILED (hr)))
      FAIL ("ScriptPlaceOpenType() failed: 0x%08xL", hr);

    glyphs_offset += glyphs_len;
  }
  glyphs_len = glyphs_offset;

  /* Ok, we've got everything we need, now compose output buffer,
   * very, *very*, carefully! */

  /* Calculate visual-clusters.  That's what we ship. */
  for (unsigned int i = 0; i < glyphs_len; i++)
    vis_clusters[i] = -1;
  for (unsigned int i = 0; i < buffer->len; i++) {
    uint32_t *p = &vis_clusters[log_clusters[buffer->info[i].utf16_index()]];
    *p = MIN (*p, buffer->info[i].cluster);
  }
  if (!backward) {
    for (unsigned int i = 1; i < glyphs_len; i++)
      if (vis_clusters[i] == -1)
	vis_clusters[i] = vis_clusters[i - 1];
  } else {
    for (int i = glyphs_len - 2; i >= 0; i--)
      if (vis_clusters[i] == -1)
	vis_clusters[i] = vis_clusters[i + 1];
  }

#undef utf16_index

  buffer->ensure (glyphs_len);
  if (buffer->in_error)
    FAIL ("Buffer in error");

#undef FAIL

  /* Set glyph infos */
  buffer->len = 0;
  for (unsigned int i = 0; i < glyphs_len; i++)
  {
    hb_glyph_info_t *info = &buffer->info[buffer->len++];

    info->codepoint = glyphs[i];
    info->cluster = vis_clusters[i];

    /* The rest is crap.  Let's store position info there for now. */
    info->mask = advances[i];
    info->var1.u32 = offsets[i].du;
    info->var2.u32 = offsets[i].dv;
  }

  /* Set glyph positions */
  buffer->clear_positions ();
  for (unsigned int i = 0; i < glyphs_len; i++)
  {
    hb_glyph_info_t *info = &buffer->info[i];
    hb_glyph_position_t *pos = &buffer->pos[i];

    /* TODO vertical */
    pos->x_advance = info->mask;
    pos->x_offset = info->var1.u32;
    pos->y_offset = info->var2.u32;
  }

  /* Wow, done! */
  return true;
}


