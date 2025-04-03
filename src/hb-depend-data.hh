/*
 * Copyright © 2024  Adobe, Inc.
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
 * Adobe Author(s): Skef Iterum
 */

#ifndef HB_DEPEND_DATA_HH
#define HB_DEPEND_DATA_HH

#include "hb.hh"

#include "hb-multimap.hh"

#ifdef HB_DEPEND_API

struct hb_depend_data_record_t {
  hb_depend_data_record_t() = delete;
  hb_depend_data_record_t(hb_tag_t table_tag,
                   hb_codepoint_t dependent,
                   hb_tag_t layout_tag,
                   hb_codepoint_t ligature_set,
                   hb_codepoint_t context_set  // ,
                   // hb_codepoint_t fv_context_set,
                   // bool fv_only
                   ) : table_tag(table_tag),
     dependent(dependent), layout_tag(layout_tag),
     ligature_set(ligature_set), context_set(context_set) // ,
     // fv_context_set(fv_context_set), fv_only(fv_only)
     {}
  hb_tag_t table_tag;
  hb_codepoint_t dependent;
  hb_tag_t layout_tag;
  hb_codepoint_t ligature_set;
  hb_codepoint_t context_set;
  // hb_codepoint_t fv_context_set;
  // bool fv_only {false};
};

struct hb_glyph_depend_record_t {
  hb_vector_t<hb_depend_data_record_t> dependencies;
};

struct hb_lookup_feature_record_t {
  hb_lookup_feature_record_t () = default;
  explicit hb_lookup_feature_record_t (bool full) : full (full) {}
  hb_lookup_feature_record_t (const hb_lookup_feature_record_t &o) : full(o.full),
                                                                     fv_indexes(o.fv_indexes) {}
  hb_lookup_feature_record_t (hb_lookup_feature_record_t &&o) : full(o.full),
                                                                fv_indexes(std::move(o.fv_indexes)) {}
  hb_lookup_feature_record_t& operator = (const hb_lookup_feature_record_t &o)
  {
    full = o.full;
    fv_indexes = o.fv_indexes;
    return *this;
  }

  bool full {true};
  hb_set_t fv_indexes;
};

struct hb_depend_data_t
{
  hb_depend_data_t () {}

  ~hb_depend_data_t () {}

  hb_codepoint_t new_set (hb_codepoint_t cp)
  {
    hb_codepoint_t set_index = sets.length;
    sets.push(std::initializer_list<hb_codepoint_t>({cp}));
    return set_index;
  }

  hb_codepoint_t new_set (hb_set_t &set)
  {
    hb_codepoint_t set_index = sets.length;
    sets.push(set);
    return set_index;
  }

  hb_codepoint_t add_to_set (hb_codepoint_t set_index, hb_codepoint_t cp) {
    if (set_index == HB_CODEPOINT_INVALID)
      set_index = new_set(cp);
    else if (set_index < sets.length)
      sets[set_index].add(cp);
    // else error
    return set_index;
  }

  void print()
  {
    for (unsigned i = 0; i < glyph_dependencies.length; i++) {
      auto &gd = glyph_dependencies[i];
      if (!gd.dependencies.length)
        continue;
      printf("GID %u:\n", i);
      for (auto &d : gd.dependencies) {
        if (d.table_tag == HB_OT_TAG_GSUB) {
          printf("  layout %c%c%c%c -> %u", HB_UNTAG(d.layout_tag), d.dependent);
          if (d.ligature_set != HB_CODEPOINT_INVALID)
            printf("  (ligature)");
        } else {
          printf("  %c%c%c%c -> %u", HB_UNTAG(d.table_tag), d.dependent);
          if (d.table_tag == HB_TAG('c','m','a','p'))
            printf(" (UVS is %u)", d.layout_tag);
        }
        printf("\n");
      }
    }
  }

  bool get_glyph_entry(hb_codepoint_t gid, hb_codepoint_t index,
                       hb_tag_t *table_tag, hb_codepoint_t *dependent,
                       hb_tag_t *layout_tag, hb_codepoint_t *ligature_set)
  {
    if (gid < glyph_dependencies.length &&
        index < glyph_dependencies[gid].dependencies.length) {
      auto &d = glyph_dependencies[gid].dependencies[index];
      *table_tag = d.table_tag;
      *dependent = d.dependent;
      *layout_tag = d.layout_tag;
      *ligature_set = d.ligature_set;
      return true;
    }
    return false;
  }

  bool get_set_from_index(hb_codepoint_t index, hb_set_t *out)
  {
    if (index < sets.length) {
      out->set(sets[index]);
      return true;
    }
    return false;
  }

  void add_depend_layout (hb_codepoint_t target, hb_tag_t table_tag,
                          hb_tag_t layout_tag,
                          hb_codepoint_t dependent,
                          hb_codepoint_t lig_set = HB_CODEPOINT_INVALID,
                          hb_codepoint_t context_set = HB_CODEPOINT_INVALID // ,
                          // hb_codepoint_t fv_context_set = HB_CODEPOINT_INVALID,
                          // bool fv_only = false
                          )
  {
    if (target >= glyph_dependencies.length) {
      DEBUG_MSG (SUBSET, nullptr, "Dependency glyph %u for %c%c%c%c too large",
                 target, HB_UNTAG(table_tag));
      return;
    }
    auto &gdr = glyph_dependencies[target];
    for (auto &dep : gdr.dependencies) {
      if (dep.table_tag == table_tag && dep.layout_tag == layout_tag &&
          dep.dependent == dependent) {
        if (dep.ligature_set == HB_CODEPOINT_INVALID && lig_set == HB_CODEPOINT_INVALID)
          return;   // dup
        if (dep.ligature_set != HB_CODEPOINT_INVALID && lig_set != HB_CODEPOINT_INVALID &&
            sets[dep.ligature_set] == sets[lig_set])
          return;   // dup
      }
    }
    gdr.dependencies.push(table_tag, dependent, layout_tag, lig_set,
                          context_set); // , fv_context_set, fv_only);
  }

  void add_gsub_lookup (hb_codepoint_t target, hb_codepoint_t lookup_index,
                        hb_codepoint_t dependent,
                        hb_codepoint_t lig_set = HB_CODEPOINT_INVALID,
                        hb_codepoint_t context_set = HB_CODEPOINT_INVALID)
  {
    for (auto t : lookup_features[lookup_index]) {
      // hb_codepoint_t fv_context_set {HB_CODEPOINT_INVALID};
      // if (!e.second.fv_indexes.is_empty ())
      //   fv_context_set = new_set(e.second.fv_indexes);
      add_depend_layout(target, HB_OT_TAG_GSUB, t, dependent, lig_set,
                        context_set); // , fv_context_set, !e.second.full);
    }
  }

  void add_depend (hb_codepoint_t target, hb_tag_t table_tag,
                   hb_codepoint_t dependent,
                   hb_codepoint_t lig_set = HB_CODEPOINT_INVALID,
                   hb_codepoint_t context_set = HB_CODEPOINT_INVALID)
  {
    add_depend_layout (target, table_tag, 0, dependent, lig_set,
                       context_set);
  }

  hb_codepoint_t get_nominal_glyph(hb_codepoint_t cp)
  {
    return hb_map_get(&nominal_glyphs, cp);
  }

  hb_set_t unicodes;
  hb_map_t nominal_glyphs;

  hb_vector_t<hb_set_t> sets;

  hb_vector_t<hb_glyph_depend_record_t> glyph_dependencies;
  hb_vector_t<hb_set_t> lookup_features;
};

#endif /* HB_DEPEND_API */

#endif /* HB_DEPEND_DATA_HH */
