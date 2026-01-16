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

/**
 * hb_depend_data_record_t:
 *
 * Internal structure representing a single dependency relationship.
 * Records that glyph A depends on glyph B through a specific OpenType
 * mechanism (table_tag), optionally with additional context (layout_tag,
 * ligature_set).
 */
struct hb_depend_data_record_t {
  hb_depend_data_record_t() = delete;
  hb_depend_data_record_t(hb_tag_t table_tag,
                   hb_codepoint_t dependent,
                   hb_tag_t layout_tag,
                   hb_codepoint_t ligature_set,
                   hb_codepoint_t context_set) : table_tag(table_tag),
     dependent(dependent), layout_tag(layout_tag),
     ligature_set(ligature_set), context_set(context_set)
     {}
  hb_tag_t table_tag;
  hb_codepoint_t dependent;
  hb_tag_t layout_tag;
  hb_codepoint_t ligature_set;
  hb_codepoint_t context_set;
};

/**
 * hb_glyph_depend_record_t:
 *
 * Internal structure holding all dependency records for a single glyph.
 * Contains a vector of hb_depend_data_record_t entries, indexed sequentially
 * starting from 0.
 */
struct hb_glyph_depend_record_t {
  hb_vector_t<hb_depend_data_record_t> dependencies;
};

/**
 * hb_lookup_feature_record_t:
 *
 * Internal structure tracking which features are associated with a lookup.
 * Used during GSUB dependency analysis to record the feature tags that
 * activate each lookup.
 */
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

/**
 * hb_depend_data_t:
 *
 * Internal container for all dependency graph data. Stores:
 * - Per-glyph dependency records (glyph_dependencies)
 * - Ligature sets indexed by set ID (sets)
 * - Nominal glyph mapping from Unicode codepoints (nominal_glyphs)
 * - Set of all Unicode codepoints in the font (unicodes)
 * - Lookup to feature mapping (lookup_features)
 *
 * This structure is owned by hb_depend_t and freed when the depend
 * object is destroyed.
 */
struct hb_depend_data_t
{
  hb_depend_data_t () {}

  ~hb_depend_data_t () {}

  void free_set (hb_codepoint_t set_index)
  {
    // Should only be called to free the most recently allocated set
    if (set_index + 1 == sets.length) {
      sets.resize(sets.length - 1);  // Pop it off
    } else {
      // Unexpected - freeing a set that's not the last one
      DEBUG_MSG (SUBSET, nullptr, "Attempting to free set %u but last set is %u",
                 set_index, sets.length - 1);
    }
  }

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

  hb_vector_t<uint32_t> get_edge_components(hb_codepoint_t source,
                                              hb_tag_t table_tag,
                                              hb_tag_t layout_tag,
                                              hb_codepoint_t dependent,
                                              hb_codepoint_t lig_set_index)
  {
    hb_vector_t<uint32_t> components;
    components.push(source);
    components.push(table_tag);
    components.push(layout_tag);
    components.push(dependent);

    // If there's a ligature set, add its contents in canonical order
    if (lig_set_index != HB_CODEPOINT_INVALID) {
      hb_codepoint_t cp = HB_SET_VALUE_INVALID;
      while (sets[lig_set_index].next(&cp)) {
        components.push(cp);
      }
    }

    return components;
  }

  uint64_t compute_edge_hash(hb_codepoint_t source,
                             hb_tag_t table_tag,
                             hb_tag_t layout_tag,
                             hb_codepoint_t dependent,
                             hb_codepoint_t lig_set_index)
  {
    // Polynomial rolling hash with prime multiplier (31)
    uint64_t hash = hb_hash(source);
    hash = hash * 31 + hb_hash(table_tag);
    hash = hash * 31 + hb_hash(layout_tag);
    hash = hash * 31 + hb_hash(dependent);

    // If there's a ligature set, incorporate its contents in canonical order
    if (lig_set_index != HB_CODEPOINT_INVALID) {
      hb_codepoint_t cp = HB_SET_VALUE_INVALID;
      while (sets[lig_set_index].next(&cp)) {
        hash = hash * 31 + hb_hash(cp);
      }
    }

    return hash;
  }

  bool edges_equal(hb_codepoint_t target,
                   hb_tag_t table_tag,
                   hb_tag_t layout_tag,
                   hb_codepoint_t dependent,
                   hb_codepoint_t lig_set,
                   const hb_depend_data_record_t &existing)
  {
    // Check basic fields (not including context_set)
    if (existing.table_tag != table_tag ||
        existing.layout_tag != layout_tag ||
        existing.dependent != dependent)
      return false;

    // Check ligature sets
    if (existing.ligature_set == HB_CODEPOINT_INVALID && lig_set == HB_CODEPOINT_INVALID)
      return true;

    if (existing.ligature_set != HB_CODEPOINT_INVALID && lig_set != HB_CODEPOINT_INVALID)
      return sets[existing.ligature_set] == sets[lig_set];

    return false;
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

  bool add_depend_layout (hb_codepoint_t target, hb_tag_t table_tag,
                          hb_tag_t layout_tag,
                          hb_codepoint_t dependent,
                          hb_codepoint_t lig_set = HB_CODEPOINT_INVALID,
                          hb_codepoint_t context_set = HB_CODEPOINT_INVALID)
  {
    if (target >= glyph_dependencies.length) {
      DEBUG_MSG (SUBSET, nullptr, "Dependency glyph %u for %c%c%c%c too large",
                 target, HB_UNTAG(table_tag));
      return false;
    }

    // Compute edge hash for deduplication (without context_set)
    uint64_t edge_hash = compute_edge_hash(target, table_tag, layout_tag, dependent, lig_set);

    // Get component vector for this edge
    auto new_components = get_edge_components(target, table_tag, layout_tag, dependent, lig_set);

    // Check if we've already seen this hash (O(1) hash table lookup)
    if (edge_hashes.has(edge_hash)) {
      // Compare stored components with new components
      const auto &stored_components = edge_hashes.get(edge_hash);
      bool components_match = (stored_components == new_components);

      // DEBUG: Check if vector comparison might be wrong
      if (components_match && stored_components.length != new_components.length) {
        printf("[BUG] Vector lengths differ but comparison says equal! stored=%u new=%u\n",
               (unsigned)stored_components.length, (unsigned)new_components.length);
      }

      if (components_match) {
        return false;  // True duplicate - same hash, same components
      } else {
        // Hash collision! Same hash but different edge components
        // Fall back to O(n) search through this glyph's dependencies
        // real_collisions++;  // TODO: Remove before production

        auto &gdr = glyph_dependencies[target];
        for (auto &dep : gdr.dependencies) {
          if (edges_equal(target, table_tag, layout_tag, dependent, lig_set, dep)) {
            return false;  // Found via fallback
          }
        }
        // DON'T update hash table - leave original stored components
        // Just add the edge
        gdr.dependencies.push(table_tag, dependent, layout_tag, lig_set,
                              context_set);
        return true;
      }
    }

    // New edge - store components in hash table and add to dependency list
    edge_hashes.set(edge_hash, new_components);

    auto &gdr = glyph_dependencies[target];
    gdr.dependencies.push(table_tag, dependent, layout_tag, lig_set, context_set);
    return true;
  }

  bool add_gsub_lookup (hb_codepoint_t target, hb_codepoint_t lookup_index,
                        hb_codepoint_t dependent,
                        hb_codepoint_t lig_set = HB_CODEPOINT_INVALID,
                        hb_codepoint_t context_set = HB_CODEPOINT_INVALID)
  {
    bool any_added = false;
    for (auto t : lookup_features[lookup_index]) {
      if (add_depend_layout(target, HB_OT_TAG_GSUB, t, dependent, lig_set, context_set))
        any_added = true;
    }
    return any_added;
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

  // Hash-based deduplication: edge hash -> component vector for collision detection
  hb_hashmap_t<uint64_t, hb_vector_t<uint32_t>> edge_hashes;
};

#endif /* HB_DEPEND_API */

#endif /* HB_DEPEND_DATA_HH */
