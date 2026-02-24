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
 *
 * Author(s): Behdad Esfahbod
 */

#include "hb.hh"

#include "hb-vector-svg-subset.hh"
#include "hb-ot-color.h"
#include "hb-map.hh"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

struct hb_svg_id_span_t
{
  const char *p;
  unsigned len;
};

struct hb_svg_defs_entry_t
{
  hb_svg_id_span_t id;
  unsigned start;
  unsigned end;
};

struct hb_svg_doc_cache_t
{
  hb_blob_t *blob = nullptr;
  const char *svg = nullptr;
  unsigned len = 0;
  hb_vector_t<hb_svg_defs_entry_t> defs_entries;
  hb_hashmap_t<hb_codepoint_t, uint64_t> glyph_spans;
};

using hb_svg_doc_t = hb_atomic_t<hb_svg_doc_cache_t *>;

struct hb_svg_face_cache_t
{
  hb_vector_t<hb_svg_doc_t> slots;
};

static hb_user_data_key_t hb_svg_face_cache_user_data_key;

static bool
hb_svg_append_len (hb_vector_t<char> *buf,
                   const char *s,
                   unsigned len)
{
  unsigned old_len = buf->length;
  if (unlikely (!buf->resize_dirty ((int) (old_len + len))))
    return false;
  hb_memcpy (buf->arrayZ + old_len, s, len);
  return true;
}

static bool
hb_svg_append_c (hb_vector_t<char> *buf, char c)
{
  return !!buf->push (c);
}

static int
hb_svg_find_substr (const char *s,
                    unsigned n,
                    unsigned from,
                    const char *needle,
                    unsigned needle_len)
{
  if (!needle_len || from >= n || needle_len > n)
    return -1;
  for (unsigned i = from; i + needle_len <= n; i++)
    if (s[i] == needle[0] && !memcmp (s + i, needle, needle_len))
      return (int) i;
  return -1;
}

static bool
hb_svg_append_with_prefix (hb_vector_t<char> *out,
                           const char *s,
                           unsigned n,
                           const char *prefix,
                           unsigned prefix_len)
{
  unsigned i = 0;
  while (i < n)
  {
    if (i + 4 <= n && !memcmp (s + i, "id=\"", 4))
    {
      if (!hb_svg_append_len (out, s + i, 4)) return false;
      i += 4;
      if (!hb_svg_append_len (out, prefix, prefix_len)) return false;
      while (i < n && s[i] != '"')
      {
        if (!hb_svg_append_c (out, s[i])) return false;
        i++;
      }
      continue;
    }
    if (i + 5 <= n && !memcmp (s + i, "id='", 4))
    {
      if (!hb_svg_append_len (out, s + i, 4)) return false;
      i += 4;
      if (!hb_svg_append_len (out, prefix, prefix_len)) return false;
      while (i < n && s[i] != '\'')
      {
        if (!hb_svg_append_c (out, s[i])) return false;
        i++;
      }
      continue;
    }
    if (i + 7 <= n && !memcmp (s + i, "href=\"#", 7))
    {
      if (!hb_svg_append_len (out, s + i, 7)) return false;
      i += 7;
      if (!hb_svg_append_len (out, prefix, prefix_len)) return false;
      while (i < n && s[i] != '"')
      {
        if (!hb_svg_append_c (out, s[i])) return false;
        i++;
      }
      continue;
    }
    if (i + 13 <= n && !memcmp (s + i, "xlink:href=\"#", 13))
    {
      if (!hb_svg_append_len (out, s + i, 13)) return false;
      i += 13;
      if (!hb_svg_append_len (out, prefix, prefix_len)) return false;
      while (i < n && s[i] != '"')
      {
        if (!hb_svg_append_c (out, s[i])) return false;
        i++;
      }
      continue;
    }
    if (i + 5 <= n && !memcmp (s + i, "url(#", 5))
    {
      if (!hb_svg_append_len (out, s + i, 5)) return false;
      i += 5;
      if (!hb_svg_append_len (out, prefix, prefix_len)) return false;
      while (i < n && s[i] != ')')
      {
        if (!hb_svg_append_c (out, s[i])) return false;
        i++;
      }
      continue;
    }
    if (!hb_svg_append_c (out, s[i]))
      return false;
    i++;
  }
  return true;
}

static bool
hb_svg_add_unique_id (hb_vector_t<hb_svg_id_span_t> *v,
                      const char *p,
                      unsigned n)
{
  if (!n) return false;
  for (unsigned i = 0; i < v->length; i++)
    if (v->arrayZ[i].len == n && !memcmp (v->arrayZ[i].p, p, n))
      return false;
  auto *slot = v->push ();
  if (!slot) return false;
  slot->p = p;
  slot->len = n;
  return true;
}

static bool
hb_svg_collect_refs (const char *s,
                     unsigned n,
                     hb_vector_t<hb_svg_id_span_t> *ids)
{
  unsigned i = 0;
  while (i < n)
  {
    if (i + 7 <= n && !memcmp (s + i, "href=\"#", 7))
    {
      i += 7;
      unsigned b = i;
      while (i < n && s[i] != '"' && s[i] != '\'' && s[i] != ' ' && s[i] != '>') i++;
      if (i > b && unlikely (!hb_svg_add_unique_id (ids, s + b, i - b))) return false;
      continue;
    }
    if (i + 13 <= n && !memcmp (s + i, "xlink:href=\"#", 13))
    {
      i += 13;
      unsigned b = i;
      while (i < n && s[i] != '"' && s[i] != '\'' && s[i] != ' ' && s[i] != '>') i++;
      if (i > b && unlikely (!hb_svg_add_unique_id (ids, s + b, i - b))) return false;
      continue;
    }
    if (i + 5 <= n && !memcmp (s + i, "url(#", 5))
    {
      i += 5;
      unsigned b = i;
      while (i < n && s[i] != ')') i++;
      if (i > b && unlikely (!hb_svg_add_unique_id (ids, s + b, i - b))) return false;
      continue;
    }
    i++;
  }
  return true;
}

static bool
hb_svg_find_element_bounds (const char *svg,
                            unsigned len,
                            unsigned elem_start,
                            unsigned *elem_end)
{
  int gt = hb_svg_find_substr (svg, len, elem_start, ">", 1);
  if (gt < 0) return false;
  unsigned open_end = (unsigned) gt;
  if (open_end > elem_start && svg[open_end - 1] == '/')
  {
    *elem_end = open_end + 1;
    return true;
  }
  unsigned depth = 1;
  unsigned pos = open_end + 1;
  while (pos < len)
  {
    int lt_i = hb_svg_find_substr (svg, len, pos, "<", 1);
    if (lt_i < 0) return false;
    unsigned lt = (unsigned) lt_i;
    if (lt + 4 <= len && !memcmp (svg + lt, "<!--", 4))
    {
      int cend = hb_svg_find_substr (svg, len, lt + 4, "-->", 3);
      if (cend < 0) return false;
      pos = (unsigned) cend + 3;
      continue;
    }
    if (lt + 9 <= len && !memcmp (svg + lt, "<![CDATA[", 9))
    {
      int cend = hb_svg_find_substr (svg, len, lt + 9, "]]>", 3);
      if (cend < 0) return false;
      pos = (unsigned) cend + 3;
      continue;
    }
    int gt_i = hb_svg_find_substr (svg, len, lt, ">", 1);
    if (gt_i < 0) return false;
    unsigned gt2 = (unsigned) gt_i;
    bool closing = (lt + 1 < len && svg[lt + 1] == '/');
    bool self_closing = (gt2 > lt && svg[gt2 - 1] == '/');
    if (!closing && !self_closing)
      depth++;
    else if (closing)
    {
      if (!depth) return false;
      depth--;
      if (!depth)
      {
        *elem_end = gt2 + 1;
        return true;
      }
    }
    pos = gt2 + 1;
  }
  return false;
}

static bool
hb_svg_parse_id_in_start_tag (const char *svg,
                              unsigned tag_start,
                              unsigned tag_end,
                              hb_svg_id_span_t *id)
{
  unsigned p = tag_start;
  while (p + 4 <= tag_end)
  {
    if (!memcmp (svg + p, "id=\"", 4))
    {
      unsigned b = p + 4;
      unsigned e = b;
      while (e < tag_end && svg[e] != '"') e++;
      if (e <= tag_end && e > b)
      {
        *id = {svg + b, e - b};
        return true;
      }
    }
    if (!memcmp (svg + p, "id='", 4))
    {
      unsigned b = p + 4;
      unsigned e = b;
      while (e < tag_end && svg[e] != '\'') e++;
      if (e <= tag_end && e > b)
      {
        *id = {svg + b, e - b};
        return true;
      }
    }
    p++;
  }
  return false;
}

static bool
hb_svg_find_element_by_id (const char *svg,
                           unsigned len,
                           const char *id,
                           unsigned id_len,
                           unsigned *start,
                           unsigned *end)
{
  unsigned p = 0;
  while (p < len)
  {
    int k = hb_svg_find_substr (svg, len, p, "id=\"", 4);
    if (k < 0) break;
    unsigned id_pos = (unsigned) k + 4;
    if (id_pos + id_len <= len &&
        !memcmp (svg + id_pos, id, id_len) &&
        id_pos + id_len < len &&
        svg[id_pos + id_len] == '"')
    {
      unsigned s = (unsigned) k;
      while (s && svg[s] != '<')
        s--;
      if (svg[s] == '<' && hb_svg_find_element_bounds (svg, len, s, end))
      {
        *start = s;
        return true;
      }
    }
    p = (unsigned) k + 1;
  }
  return false;
}

static bool
hb_svg_parse_defs_entries (const char *svg,
                           unsigned len,
                           hb_vector_t<hb_svg_defs_entry_t> *defs_entries)
{
  int defs_open_i = hb_svg_find_substr (svg, len, 0, "<defs", 5);
  int defs_close_i = hb_svg_find_substr (svg, len, 0, "</defs>", 7);
  defs_entries->alloc (64);
  if (defs_open_i < 0 || defs_close_i <= defs_open_i)
    return true;

  int defs_gt = hb_svg_find_substr (svg, len, (unsigned) defs_open_i, ">", 1);
  if (defs_gt <= defs_open_i)
    return true;

  unsigned p = (unsigned) defs_gt + 1;
  unsigned defs_end = (unsigned) defs_close_i;
  while (p < defs_end)
  {
    int lt_i = hb_svg_find_substr (svg, len, p, "<", 1);
    if (lt_i < 0 || (unsigned) lt_i >= defs_end)
      break;
    unsigned lt = (unsigned) lt_i;
    if (lt + 2 <= len && svg[lt + 1] == '/')
      break;
    unsigned elem_end = 0;
    if (!hb_svg_find_element_bounds (svg, len, lt, &elem_end) || elem_end > defs_end)
      break;
    int gt_i = hb_svg_find_substr (svg, len, lt, ">", 1);
    if (gt_i < 0 || (unsigned) gt_i >= elem_end)
      break;
    hb_svg_id_span_t id = {nullptr, 0};
    if (hb_svg_parse_id_in_start_tag (svg, lt, (unsigned) gt_i, &id))
    {
      auto *slot = defs_entries->push ();
      if (unlikely (!slot))
        return false;
      slot->id = id;
      slot->start = lt;
      slot->end = elem_end;
    }
    p = elem_end;
  }

  return true;
}

struct hb_svg_open_elem_t
{
  unsigned start;
  hb_svg_id_span_t id;
  bool in_defs_content;
  bool is_defs;
};

static const unsigned HB_SVG_PARSE_MAX_DEPTH = 128;

static bool
hb_svg_parse_glyph_id_span (const hb_svg_id_span_t &id,
                            hb_codepoint_t *glyph)
{
  if (id.len <= 5 || memcmp (id.p, "glyph", 5))
    return false;

  hb_codepoint_t gid = 0;
  for (unsigned i = 5; i < id.len; i++)
  {
    unsigned char c = (unsigned char) id.p[i];
    if (c < '0' || c > '9')
      return false;
    gid = (hb_codepoint_t) (gid * 10 + (c - '0'));
  }

  *glyph = gid;
  return true;
}

static bool
hb_svg_parse_cache_entries_linear (const char *svg,
                                   unsigned len,
                                   hb_vector_t<hb_svg_defs_entry_t> *defs_entries,
                                   hb_hashmap_t<hb_codepoint_t, uint64_t> *glyph_spans)
{
  hb_svg_open_elem_t stack[HB_SVG_PARSE_MAX_DEPTH];
  unsigned depth = 0;
  defs_entries->alloc (256);

  unsigned defs_depth = 0;
  unsigned i = 0;
  while (i < len)
  {
    if (svg[i] != '<')
    {
      i++;
      continue;
    }

    if (i + 4 <= len && !memcmp (svg + i, "<!--", 4))
    {
      int cend = hb_svg_find_substr (svg, len, i + 4, "-->", 3);
      if (cend < 0) return false;
      i = (unsigned) cend + 3;
      continue;
    }
    if (i + 9 <= len && !memcmp (svg + i, "<![CDATA[", 9))
    {
      int cend = hb_svg_find_substr (svg, len, i + 9, "]]>", 3);
      if (cend < 0) return false;
      i = (unsigned) cend + 3;
      continue;
    }

    bool closing = (i + 1 < len && svg[i + 1] == '/');
    bool special = (i + 1 < len && (svg[i + 1] == '!' || svg[i + 1] == '?'));

    unsigned gt = i + 1;
    char quote = 0;
    while (gt < len)
    {
      char c = svg[gt];
      if (quote)
      {
        if (c == quote) quote = 0;
      }
      else
      {
        if (c == '"' || c == '\'')
          quote = c;
        else if (c == '>')
          break;
      }
      gt++;
    }
    if (gt >= len)
      return false;

    if (special)
    {
      i = gt + 1;
      continue;
    }

    unsigned p = i + (closing ? 2 : 1);
    while (p < gt && isspace ((unsigned char) svg[p])) p++;
    const char *name = svg + p;
    unsigned name_len = 0;
    while (p + name_len < gt)
    {
      unsigned char c = (unsigned char) name[name_len];
      if (!(isalnum (c) || c == '_' || c == '-' || c == ':'))
        break;
      name_len++;
    }
    bool is_defs = (name_len == 4 && !memcmp (name, "defs", 4));

    if (closing)
    {
      if (!depth)
      {
        i = gt + 1;
        continue;
      }

      hb_svg_open_elem_t e = stack[--depth];
      unsigned end = gt + 1;

      if (e.id.len)
      {
        if (e.in_defs_content)
        {
          auto *slot = defs_entries->push ();
          if (unlikely (!slot))
            return false;
          slot->id = e.id;
          slot->start = e.start;
          slot->end = end;
        }

        hb_codepoint_t gid;
        if (hb_svg_parse_glyph_id_span (e.id, &gid))
          glyph_spans->set (gid, ((uint64_t) e.start << 32) | (uint64_t) end);
      }

      if (e.is_defs && defs_depth)
        defs_depth--;

      i = end;
      continue;
    }

    hb_svg_id_span_t id = {nullptr, 0};
    hb_svg_parse_id_in_start_tag (svg, i, gt, &id);

    unsigned r = gt;
    while (r > i && isspace ((unsigned char) svg[r - 1])) r--;
    bool self_closing = (r > i && svg[r - 1] == '/');

    hb_svg_open_elem_t e = {i, id, defs_depth > 0, is_defs};

    if (self_closing)
    {
      unsigned end = gt + 1;
      if (e.id.len)
      {
        if (e.in_defs_content)
        {
          auto *slot = defs_entries->push ();
          if (unlikely (!slot))
            return false;
          slot->id = e.id;
          slot->start = e.start;
          slot->end = end;
        }

        hb_codepoint_t gid;
        if (hb_svg_parse_glyph_id_span (e.id, &gid))
          glyph_spans->set (gid, ((uint64_t) e.start << 32) | (uint64_t) end);
      }
    }
    else
    {
      if (unlikely (depth >= HB_SVG_PARSE_MAX_DEPTH))
        return false;
      stack[depth++] = e;
      if (is_defs)
        defs_depth++;
    }

    i = gt + 1;
  }

  return true;
}

static void
hb_svg_doc_cache_destroy (hb_svg_doc_cache_t *doc)
{
  if (!doc)
    return;
  doc->glyph_spans.fini ();
  doc->defs_entries.fini ();
  hb_blob_destroy (doc->blob);
  hb_free (doc);
}

static void
hb_svg_face_cache_destroy (void *data)
{
  auto *face_cache = (hb_svg_face_cache_t *) data;
  if (!face_cache)
    return;
  for (unsigned i = 0; i < face_cache->slots.length; i++)
    hb_svg_doc_cache_destroy (face_cache->slots.arrayZ[i].get_relaxed ());
  face_cache->slots.fini ();
  hb_free (face_cache);
}

static hb_svg_doc_cache_t *
hb_svg_make_doc_cache (hb_blob_t *image,
                       const char *svg,
                       unsigned len)
{
  auto *doc = (hb_svg_doc_cache_t *) hb_malloc (sizeof (hb_svg_doc_cache_t));
  if (!doc)
    return nullptr;
  doc->blob = nullptr;
  doc->svg = nullptr;
  doc->len = 0;
  doc->defs_entries.init ();
  doc->glyph_spans.init ();

  doc->blob = hb_blob_reference (image);
  doc->svg = svg;
  doc->len = len;

  if (!hb_svg_parse_cache_entries_linear (svg, len,
                                          &doc->defs_entries,
                                          &doc->glyph_spans))
  {
    hb_svg_doc_cache_destroy (doc);
    return nullptr;
  }

  return doc;
}

static hb_svg_face_cache_t *
hb_svg_get_or_make_face_cache (hb_face_t *face)
{
  if (!face)
    return nullptr;

  auto *face_cache = (hb_svg_face_cache_t *) hb_face_get_user_data (face, &hb_svg_face_cache_user_data_key);
  if (face_cache)
    return face_cache;

  face_cache = (hb_svg_face_cache_t *) hb_calloc (1, sizeof (hb_svg_face_cache_t));
  if (!face_cache)
    return nullptr;

  unsigned doc_count = hb_ot_color_get_svg_document_count (face);
  if (doc_count && !face_cache->slots.resize (doc_count))
  {
    hb_free (face_cache);
    return nullptr;
  }
  for (unsigned i = 0; i < doc_count; i++)
    face_cache->slots.arrayZ[i].set_relaxed (nullptr);

  if (!hb_face_set_user_data (face,
                              &hb_svg_face_cache_user_data_key,
                              face_cache,
                              hb_svg_face_cache_destroy,
                              false))
  {
    hb_free (face_cache);
    face_cache = (hb_svg_face_cache_t *) hb_face_get_user_data (face, &hb_svg_face_cache_user_data_key);
  }

  return face_cache;
}

static hb_svg_doc_cache_t *
hb_svg_get_or_make_doc_cache (hb_face_t *face,
                              hb_blob_t *image,
                              const char *svg,
                              unsigned len,
                              unsigned doc_index)
{
  auto *face_cache = hb_svg_get_or_make_face_cache (face);
  if (!face_cache || doc_index >= face_cache->slots.length)
    return nullptr;

  auto &slot = face_cache->slots.arrayZ[doc_index];
  auto *doc = slot.get_acquire ();
  if (doc)
    return doc;

  auto *fresh = hb_svg_make_doc_cache (image, svg, len);
  if (!fresh)
    return nullptr;

  auto *expected = (hb_svg_doc_cache_t *) nullptr;
  if (slot.cmpexch (expected, fresh))
    return fresh;

  hb_svg_doc_cache_destroy (fresh);
  return expected;
}

bool
hb_svg_subset_glyph_image (hb_face_t *face,
                           hb_blob_t *image,
                           hb_codepoint_t glyph,
                           unsigned *image_counter,
                           hb_vector_t<char> *defs_dst,
                           hb_vector_t<char> *body_dst)
{
  if (glyph == HB_CODEPOINT_INVALID || !image_counter || !defs_dst || !body_dst)
    return false;

  unsigned len;
  const char *svg = hb_blob_get_data (image, &len);
  if (!svg || !len)
    return false;

  unsigned doc_index = 0;
  if (!hb_ot_color_glyph_get_svg_document_index (face, glyph, &doc_index))
    return false;

  auto *doc_cache = hb_svg_get_or_make_doc_cache (face, image, svg, len, doc_index);
  if (doc_cache)
  {
    svg = doc_cache->svg;
    len = doc_cache->len;
  }

  unsigned glyph_start = 0, glyph_end = 0;
  if (doc_cache)
  {
    uint64_t *span = nullptr;
    if (!doc_cache->glyph_spans.has (glyph, &span) || !span)
      return false;
    glyph_start = (unsigned) (*span >> 32);
    glyph_end = (unsigned) *span;
  }
  else
  {
    char glyph_id[48];
    int gid_len = snprintf (glyph_id, sizeof (glyph_id), "glyph%u", glyph);
    if (gid_len <= 0 || (unsigned) gid_len >= sizeof (glyph_id))
      return false;
    if (!hb_svg_find_element_by_id (svg, len, glyph_id, (unsigned) gid_len, &glyph_start, &glyph_end))
      return false;
  }

  hb_vector_t<hb_svg_defs_entry_t> defs_entries_local;
  auto *defs_entries = doc_cache ? &doc_cache->defs_entries : &defs_entries_local;
  if (!doc_cache && !hb_svg_parse_defs_entries (svg, len, defs_entries))
    return false;

  hb_vector_t<hb_svg_id_span_t> needed_ids;
  needed_ids.alloc (16);
  if (!hb_svg_collect_refs (svg + glyph_start, glyph_end - glyph_start, &needed_ids))
    return false;

  hb_vector_t<unsigned> chosen_defs;
  chosen_defs.alloc (16);
  for (unsigned qi = 0; qi < needed_ids.length; qi++)
  {
    const auto &need = needed_ids.arrayZ[qi];
    for (unsigned i = 0; i < defs_entries->length; i++)
    {
      const auto &e = defs_entries->arrayZ[i];
      if (e.id.len == need.len && !memcmp (e.id.p, need.p, need.len))
      {
        bool seen = false;
        for (unsigned j = 0; j < chosen_defs.length; j++)
          if (chosen_defs.arrayZ[j] == i) { seen = true; break; }
        if (!seen)
        {
          if (unlikely (!chosen_defs.push (i)))
            return false;
          if (!hb_svg_collect_refs (svg + e.start, e.end - e.start, &needed_ids))
            return false;
        }
        break;
      }
    }
  }

  char prefix[32];
  int prefix_len = snprintf (prefix, sizeof (prefix), "hbimg%u_", (*image_counter)++);
  if (prefix_len <= 0 || (unsigned) prefix_len >= sizeof (prefix))
    return false;

  body_dst->alloc (body_dst->length + (glyph_end - glyph_start) + (unsigned) prefix_len + 32);

  for (unsigned i = 0; i < chosen_defs.length; i++)
  {
    const auto &e = defs_entries->arrayZ[chosen_defs.arrayZ[i]];
    if (!hb_svg_append_with_prefix (defs_dst, svg + e.start, e.end - e.start, prefix, (unsigned) prefix_len))
      return false;
    if (!hb_svg_append_c (defs_dst, '\n')) return false;
  }

  return hb_svg_append_with_prefix (body_dst,
                                    svg + glyph_start,
                                    glyph_end - glyph_start,
                                    prefix,
                                    (unsigned) prefix_len);
}
