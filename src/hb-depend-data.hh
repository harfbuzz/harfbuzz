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
  hb_depend_data_t ()
#ifdef HB_DEPEND_API
    : current_context_set_index (HB_CODEPOINT_INVALID)
#endif
  {}

  ~hb_depend_data_t () {}

  void free_set (hb_codepoint_t set_index)
  {
    if (set_index >= sets.length)
    {
      DEBUG_MSG (SUBSET, nullptr, "Attempting to free invalid set %u (max is %u)",
                 set_index, sets.length - 1);
      return;
    }

    /* Add to free list for reuse */
    free_set_list.push (set_index);

    /* Clear the set to save memory */
    sets[set_index].clear ();
  }

  hb_codepoint_t new_set (hb_codepoint_t cp)
  {
    hb_codepoint_t set_index;

    if (free_set_list.length > 0)
    {
      /* Reuse freed set */
      set_index = free_set_list[free_set_list.length - 1];
      free_set_list.resize (free_set_list.length - 1);
      sets[set_index].clear ();
      sets[set_index].add (cp);
    }
    else
    {
      /* Allocate new set */
      set_index = sets.length;
      sets.push (std::initializer_list<hb_codepoint_t>({cp}));
    }

    return set_index;
  }

  hb_codepoint_t new_set (hb_set_t &set)
  {
    hb_codepoint_t set_index;

    if (free_set_list.length > 0)
    {
      /* Reuse freed set */
      set_index = free_set_list[free_set_list.length - 1];
      free_set_list.resize (free_set_list.length - 1);
      sets[set_index].set (set);
    }
    else
    {
      /* Allocate new set */
      set_index = sets.length;
      sets.push (set);
    }

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

  /* Compute hash of a set's contents (for deduplication) */
  uint64_t compute_set_hash (const hb_set_t &set) const
  {
    uint64_t hash = 0;
    hb_codepoint_t cp = HB_SET_VALUE_INVALID;
    /* Need to use non-const iteration since next() modifies internal state */
    hb_set_t temp_set;
    temp_set.set (set);
    while (temp_set.next (&cp))
      hash = hash * 31 + hb_hash (cp);
    return hash;
  }

  /* Find existing set with same contents, or create new one (with deduplication).
   * Use this for context sets. For ligature sets, call new_set() directly. */
  hb_codepoint_t find_or_create_set (const hb_set_t &set)
  {
    uint64_t hash = compute_set_hash (set);

    /* Check if we already have this set */
    if (set_hashes.has (hash))
    {
      hb_codepoint_t existing_idx = set_hashes.get (hash);
      if (sets[existing_idx] == set)
        return existing_idx;  /* Reuse existing set */

      /* Hash collision - different set, same hash */
      /* Fall through to create new set */
    }

    /* Create new set and store in hash table */
    hb_codepoint_t new_idx = new_set (const_cast<hb_set_t&>(set));
    set_hashes.set (hash, new_idx);
    return new_idx;
  }

#ifdef HB_DEPEND_API
  /* Build a context set from context information.
   * Encodes backtrack and lookahead requirements as a flattened set.
   * Returns HB_CODEPOINT_INVALID if no context.
   *
   * To ensure canonical encoding and avoid redundancy:
   * 1. First pass: collect all direct (single-glyph) requirements
   * 2. Second pass: create disjunction sets, subtracting direct requirements
   * 3. Combine: add direct requirements and filtered disjunction references
   */
  hb_codepoint_t build_context_set (const hb_vector_t<hb_set_t> *backtrack_sets,
                                     const hb_vector_t<hb_set_t> *lookahead_sets)
  {
    if ((!backtrack_sets || backtrack_sets->length == 0) &&
        (!lookahead_sets || lookahead_sets->length == 0))
      return HB_CODEPOINT_INVALID;

    /* First pass: collect all direct (single-glyph) requirements */
    hb_set_t direct_requirements;

    if (backtrack_sets)
    {
      for (const auto &back_set : *backtrack_sets)
      {
        if (back_set.get_population () == 1)
        {
          hb_codepoint_t gid = back_set.get_min ();
          direct_requirements.add (gid);
        }
      }
    }

    if (lookahead_sets)
    {
      for (const auto &look_set : *lookahead_sets)
      {
        if (look_set.get_population () == 1)
        {
          hb_codepoint_t gid = look_set.get_min ();
          direct_requirements.add (gid);
        }
      }
    }

    /* Second pass: create disjunction sets, filtering out direct requirements */
    hb_set_t context_elements;

    if (backtrack_sets)
    {
      for (const auto &back_set : *backtrack_sets)
      {
        if (back_set.get_population () > 1)
        {
          /* Multiple glyphs - filter and create set reference */
          hb_set_t filtered_set;
          filtered_set.set (back_set);
          filtered_set.subtract (direct_requirements);

          if (!filtered_set.is_empty ())
          {
            hb_codepoint_t set_idx = find_or_create_set (filtered_set);
            context_elements.add (0x80000000 | set_idx);
          }
        }
      }
    }

    if (lookahead_sets)
    {
      for (const auto &look_set : *lookahead_sets)
      {
        if (look_set.get_population () > 1)
        {
          /* Multiple glyphs - filter and create set reference */
          hb_set_t filtered_set;
          filtered_set.set (look_set);
          filtered_set.subtract (direct_requirements);

          if (!filtered_set.is_empty ())
          {
            hb_codepoint_t set_idx = find_or_create_set (filtered_set);
            context_elements.add (0x80000000 | set_idx);
          }
        }
      }
    }

    /* Add direct requirements */
    context_elements.union_ (direct_requirements);

    if (context_elements.is_empty ())
      return HB_CODEPOINT_INVALID;

    /* Store context elements with deduplication */
    return find_or_create_set (context_elements);
  }
#endif

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
                                              hb_codepoint_t lig_set_index,
                                              hb_codepoint_t context_set_index)
  {
    hb_vector_t<uint32_t> components;
    components.push(source);
    components.push(table_tag);
    components.push(layout_tag);
    components.push(dependent);

    // If there's a ligature set, add its contents in canonical order
    if (lig_set_index != HB_CODEPOINT_INVALID) {
      hb_codepoint_t cp = HB_SET_VALUE_INVALID;
      /* Need temp copy because next() modifies iterator state */
      hb_set_t temp_set;
      temp_set.set (sets[lig_set_index]);
      while (temp_set.next(&cp)) {
        components.push(cp);
      }
    }

    // If there's a context set, add its contents in canonical order
    if (context_set_index != HB_CODEPOINT_INVALID) {
      hb_codepoint_t cp = HB_SET_VALUE_INVALID;
      /* Need temp copy because next() modifies iterator state */
      hb_set_t temp_set;
      temp_set.set (sets[context_set_index]);
      while (temp_set.next(&cp)) {
        components.push(cp);
      }
    }

    return components;
  }

  uint64_t compute_edge_hash(hb_codepoint_t source,
                             hb_tag_t table_tag,
                             hb_tag_t layout_tag,
                             hb_codepoint_t dependent,
                             hb_codepoint_t lig_set_index,
                             hb_codepoint_t context_set_index)
  {
    // Polynomial rolling hash with prime multiplier (31)
    uint64_t hash = hb_hash(source);
    hash = hash * 31 + hb_hash(table_tag);
    hash = hash * 31 + hb_hash(layout_tag);
    hash = hash * 31 + hb_hash(dependent);

    // If there's a ligature set, incorporate its contents in canonical order
    if (lig_set_index != HB_CODEPOINT_INVALID) {
      hb_codepoint_t cp = HB_SET_VALUE_INVALID;
      /* Need temp copy because next() modifies iterator state */
      hb_set_t temp_set;
      temp_set.set (sets[lig_set_index]);
      while (temp_set.next(&cp)) {
        hash = hash * 31 + hb_hash(cp);
      }
    }

    // If there's a context set, incorporate its contents in canonical order
    if (context_set_index != HB_CODEPOINT_INVALID) {
      hb_codepoint_t cp = HB_SET_VALUE_INVALID;
      /* Need temp copy because next() modifies iterator state */
      hb_set_t temp_set;
      temp_set.set (sets[context_set_index]);
      while (temp_set.next(&cp)) {
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
                   hb_codepoint_t context_set,
                   const hb_depend_data_record_t &existing)
  {
    // Check basic fields
    if (existing.table_tag != table_tag ||
        existing.layout_tag != layout_tag ||
        existing.dependent != dependent)
      return false;

    // Check ligature sets
    if (existing.ligature_set != lig_set)
    {
      if (existing.ligature_set == HB_CODEPOINT_INVALID || lig_set == HB_CODEPOINT_INVALID)
        return false;
      if (!(sets[existing.ligature_set] == sets[lig_set]))
        return false;
    }

    // Check context sets
    if (existing.context_set != context_set)
    {
      if (existing.context_set == HB_CODEPOINT_INVALID || context_set == HB_CODEPOINT_INVALID)
        return false;
      if (!(sets[existing.context_set] == sets[context_set]))
        return false;
    }

    return true;
  }

  bool get_glyph_entry(hb_codepoint_t gid, hb_codepoint_t index,
                       hb_tag_t *table_tag, hb_codepoint_t *dependent,
                       hb_tag_t *layout_tag, hb_codepoint_t *ligature_set,
                       hb_codepoint_t *context_set)
  {
    if (gid < glyph_dependencies.length &&
        index < glyph_dependencies[gid].dependencies.length) {
      auto &d = glyph_dependencies[gid].dependencies[index];
      *table_tag = d.table_tag;
      *dependent = d.dependent;
      *layout_tag = d.layout_tag;
      *ligature_set = d.ligature_set;
      *context_set = d.context_set;
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

    // Compute edge hash for deduplication (including context_set)
    uint64_t edge_hash = compute_edge_hash(target, table_tag, layout_tag, dependent, lig_set, context_set);

    // Get component vector for this edge
    auto new_components = get_edge_components(target, table_tag, layout_tag, dependent, lig_set, context_set);

    // Check if we've already seen this hash (O(1) hash table lookup)
    if (edge_hashes.has(edge_hash)) {
      // Compare stored components with new components
      const auto &stored_components = edge_hashes.get(edge_hash);
      bool components_match = (stored_components == new_components);

      if (components_match) {
        return false;  // True duplicate - same hash, same components
      } else {
        // Hash collision! Same hash but different edge components
        // Fall back to O(n) search through this glyph's dependencies
        auto &gdr = glyph_dependencies[target];
        for (auto &dep : gdr.dependencies) {
          if (edges_equal(target, table_tag, layout_tag, dependent, lig_set, context_set, dep)) {
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
#ifdef HB_DEPEND_API
    /* Use pre-computed context set index for the current rule */
    if (context_set == HB_CODEPOINT_INVALID)
      context_set = current_context_set_index;
#endif

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
    add_depend_layout (target, table_tag, HB_CODEPOINT_INVALID, dependent, lig_set,
                       context_set);
  }

  hb_codepoint_t get_nominal_glyph(hb_codepoint_t cp)
  {
    return hb_map_get(&nominal_glyphs, cp);
  }

  hb_set_t unicodes;
  hb_map_t nominal_glyphs;

  hb_vector_t<hb_set_t> sets;
  hb_vector_t<hb_codepoint_t> free_set_list;  /* Indices of freed sets for reuse */

  hb_vector_t<hb_glyph_depend_record_t> glyph_dependencies;
  hb_vector_t<hb_set_t> lookup_features;

  /* Hash-based deduplication: edge hash -> component vector for collision detection */
  hb_hashmap_t<uint64_t, hb_vector_t<uint32_t>> edge_hashes;

  /* Set deduplication: set_hash -> set_index (for context sets) */
  hb_hashmap_t<uint64_t, hb_codepoint_t> set_hashes;

#ifdef HB_DEPEND_API
  /* Temporary: Context set index for the current rule.
   * Built once per rule by context_depend_lookup/chain_context_depend_lookup,
   * then used by all edges recorded from nested lookups. */
  hb_codepoint_t current_context_set_index;
#endif
};

#endif /* HB_DEPEND_API */

#endif /* HB_DEPEND_DATA_HH */
