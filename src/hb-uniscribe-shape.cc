/*
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
 * Google Author(s): Behdad Esfahbod
 */

#define _WIN32_WINNT 0x0500

#include "hb-private.hh"

#include <windows.h>
#include <usp10.h>

typedef ULONG WIN_ULONG;

#include "hb-uniscribe.h"

#include "hb-ot-name-private.hh"
#include "hb-ot-tag.h"

#include "hb-font-private.hh"
#include "hb-buffer-private.hh"



#ifndef HB_DEBUG_UNISCRIBE
#define HB_DEBUG_UNISCRIBE (HB_DEBUG+0)
#endif


/*
DWORD GetFontData(
  __in   HDC hdc,
  __in   DWORD dwTable,
  __in   DWORD dwOffset,
  __out  LPVOID lpvBuffer,
  __in   DWORD cbData
);
*/

static bool
populate_log_font (LOGFONTW  *lf,
		   HDC        hdc,
		   hb_font_t *font)
{
  memset (lf, 0, sizeof (*lf));
  int dpi = GetDeviceCaps (hdc, LOGPIXELSY);
  lf->lfHeight = -font->y_scale;

  hb_blob_t *blob = Sanitizer<name>::sanitize (hb_face_reference_table (font->face, HB_TAG ('n','a','m','e')));
  const name *name_table = Sanitizer<name>::lock_instance (blob);
  unsigned int len = name_table->get_name (3, 1, 0x409, 4,
					   lf->lfFaceName,
					   sizeof (lf->lfFaceName[0]) * LF_FACESIZE)
					  / sizeof (lf->lfFaceName[0]);
  if (unlikely (!len)) {
    DEBUG_MSG (UNISCRIBE, NULL, "Didn't find English name table entry");
    return FALSE;
  }
  if (unlikely (len >= LF_FACESIZE)) {
    DEBUG_MSG (UNISCRIBE, NULL, "Font name too long");
    return FALSE;
  }

  for (unsigned int i = 0; i < len; i++)
    lf->lfFaceName[i] = hb_be_uint16 (lf->lfFaceName[i]);
  lf->lfFaceName[len] = 0;

  return TRUE;
}

hb_bool_t
hb_uniscribe_shape (hb_font_t          *font,
		    hb_buffer_t        *buffer,
		    const hb_feature_t *features,
		    unsigned int        num_features,
		    const char         *shaper_options)
{
  buffer->guess_properties ();

  HRESULT hr;

  if (unlikely (!buffer->len))
    return TRUE;

retry:

  unsigned int scratch_size;
  char *scratch = (char *) buffer->get_scratch_buffer (&scratch_size);

  /* Allocate char buffers; they all fit */

#define ALLOCATE_ARRAY(Type, name, len) \
  Type *name = (Type *) scratch; \
  scratch += len * sizeof (name[0]); \
  scratch_size -= len * sizeof (name[0]);

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


#define FAIL(...) \
  HB_STMT_START { \
    DEBUG_MSG (UNISCRIBE, NULL, __VA_ARGS__); \
    return FALSE; \
  } HB_STMT_END;


#define MAX_ITEMS 10

  SCRIPT_ITEM items[MAX_ITEMS + 1];
  SCRIPT_STATE bidi_state = {0};
  WIN_ULONG script_tags[MAX_ITEMS];
  int item_count;

  bidi_state.uBidiLevel = HB_DIRECTION_IS_FORWARD (buffer->props.direction) ? 0 : 1;
  bidi_state.fOverrideDirection = 1;

  hr = ScriptItemizeOpenType (wchars,
			      chars_len,
			      MAX_ITEMS,
			      NULL,
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
    /* XXX setup ranges */
  }

  hb_blob_t *blob = hb_face_get_blob (font->face);
  unsigned int blob_length;
  const char *blob_data = hb_blob_get_data (blob, &blob_length);
  if (unlikely (!blob_length))
    FAIL ("Empty font blob");

  DWORD num_fonts_installed;
  HANDLE fh = AddFontMemResourceEx ((void *) blob_data, blob_length, 0, &num_fonts_installed);
  hb_blob_destroy (blob);
  if (unlikely (!fh))
    FAIL ("AddFontMemResourceEx() failed");

  /* FREE stuff, specially when taking fallback... */

  HDC hdc = GetDC (NULL); /* XXX The DC should be cached on the face I guess? */

  LOGFONTW log_font;
  if (unlikely (!populate_log_font (&log_font, hdc, font)))
    FAIL ("populate_log_font() failed");

  HFONT hfont = CreateFontIndirectW (&log_font);
  SelectObject (hdc, hfont);

  SCRIPT_CACHE script_cache = NULL;
  OPENTYPE_TAG language_tag = hb_ot_tag_from_language (buffer->props.language);

  unsigned int glyphs_offset = 0;
  unsigned int glyphs_len;
  for (unsigned int i = 0; i < item_count; i++)
  {
      unsigned int chars_offset = items[i].iCharPos;
      unsigned int item_chars_len = items[i + 1].iCharPos - chars_offset;
      OPENTYPE_TAG script_tag = script_tags[i]; /* XXX buffer->props.script */

      hr = ScriptShapeOpenType (hdc,
				&script_cache,
				&items[i].a,
				script_tag,
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
	FAIL ("ScriptShapeOpenType() failed: Font doesn't support script");
      if (unlikely (FAILED (hr)))
	FAIL ("ScriptShapeOpenType() failed: 0x%08xL", hr);

      hr = ScriptPlaceOpenType (hdc,
				&script_cache,
				&items[i].a,
				script_tag,
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

  ReleaseDC (NULL, hdc);
  DeleteObject (hfont);
  RemoveFontMemResourceEx (fh);

  /* Ok, we've got everything we need, now compose output buffer,
   * very, *very*, carefully! */

  /* Calculate visual-clusters.  That's what we ship. */
  for (unsigned int i = 0; i < buffer->len; i++)
    vis_clusters[i] = 0;
  for (unsigned int i = 0; i < buffer->len; i++) {
    uint32_t *p = &vis_clusters[log_clusters[buffer->info[i].utf16_index()]];
    *p = MIN (*p, buffer->info[i].cluster);
  }
  for (unsigned int i = 1; i < glyphs_len; i++)
    if (!glyph_props[i].sva.fClusterStart)
    vis_clusters[i] = vis_clusters[i - 1];

#undef utf16_index

  buffer->ensure (glyphs_len);
  if (buffer->in_error)
    FAIL ("Buffer in error");

#undef FAIL

  /* Set glyph infos */
  for (unsigned int i = 0; i < glyphs_len; i++)
  {
    hb_glyph_info_t *info = &buffer->info[i];

    info->codepoint = glyphs[i];
    info->cluster = vis_clusters[i];

    /* The rest is crap.  Let's store position info there for now. */
    info->mask = advances[i];
    info->var1.u32 = offsets[i].du;
    info->var2.u32 = offsets[i].dv;
  }
  buffer->len = glyphs_len;

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
  return TRUE;
}


