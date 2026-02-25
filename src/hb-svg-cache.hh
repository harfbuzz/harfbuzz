/*
 * Copyright © 2026  Behdad Esfahbod
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
 */

#ifndef HB_SVG_CACHE_HH
#define HB_SVG_CACHE_HH

#include "hb.hh"

#include "hb-blob.hh"
#include "hb-face.h"

struct hb_svg_id_span_t
{
  const char *p;
  unsigned len;

  bool operator == (const hb_svg_id_span_t &o) const
  {
    return len == o.len && !memcmp (p, o.p, len);
  }

  uint32_t hash () const
  {
    uint32_t h = hb_hash (len);
    for (unsigned i = 0; i < len; i++)
      h = h * 33u + (unsigned char) p[i];
    return h;
  }
};

struct hb_svg_defs_entry_t
{
  hb_svg_id_span_t id;
  unsigned start;
  unsigned end;
};

struct hb_svg_doc_cache_t;

HB_INTERNAL const hb_svg_doc_cache_t *
hb_svg_get_or_make_doc_cache (hb_face_t *face,
                              hb_blob_t *image,
                              const char *svg,
                              unsigned len,
                              unsigned doc_index,
                              hb_codepoint_t start_glyph,
                              hb_codepoint_t end_glyph);

HB_INTERNAL const char *
hb_svg_doc_cache_get_svg (const hb_svg_doc_cache_t *doc,
                          unsigned *len);

HB_INTERNAL const hb_vector_t<hb_svg_defs_entry_t> *
hb_svg_doc_cache_get_defs_entries (const hb_svg_doc_cache_t *doc);

HB_INTERNAL bool
hb_svg_doc_cache_get_glyph_span (const hb_svg_doc_cache_t *doc,
                                 hb_codepoint_t glyph,
                                 unsigned *start,
                                 unsigned *end);

HB_INTERNAL bool
hb_svg_doc_cache_find_id_span (const hb_svg_doc_cache_t *doc,
                               hb_svg_id_span_t id,
                               unsigned *start,
                               unsigned *end);

HB_INTERNAL bool
hb_svg_doc_cache_find_id_cstr (const hb_svg_doc_cache_t *doc,
                               const char *id,
                               unsigned *start,
                               unsigned *end);

#endif /* HB_SVG_CACHE_HH */
