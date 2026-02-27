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
 * Edge flags for hb_depend_data_record_t.
 *
 * These flags mark edges that may cause "expected" over-approximation when
 * computing closure via the depend graph compared to hb_ot_layout_lookups_substitute_closure().
 * They help distinguish between bugs (unexpected over-approx) and known limitations
 * of static dependency analysis (expected over-approx).
 *
 * HB_DEPEND_EDGE_FLAG_FROM_CONTEXT_POSITION (0x01):
 *   Edge created from a multi-position contextual rule (Context or ChainContext
 *   with inputCount > 1). Depend extraction records edges based on what glyphs
 *   COULD be at each position according to the static input coverage/class. But
 *   during closure computation, lookups within the rule are applied sequentially:
 *   lookups at earlier positions may transform glyphs at later positions, and
 *   multiple lookups at the same position may interact (one produces a glyph,
 *   another immediately consumes it as an "intermediate"). So a glyph matching
 *   the coverage might not actually persist at that position when closure
 *   traverses the rule. These edges may cause over-approximation.
 *
 * HB_DEPEND_EDGE_FLAG_FROM_NESTED_CONTEXT (0x02):
 *   Edge created from a lookup called within another contextual lookup (nested
 *   context). The outer context's requirements are not preserved on this edge,
 *   so it may over-approximate.
 */
#define HB_DEPEND_EDGE_FLAG_FROM_CONTEXT_POSITION 0x01
#define HB_DEPEND_EDGE_FLAG_FROM_NESTED_CONTEXT   0x02

/**
 * hb_depend_data_record_t:
 *
 * Internal structure representing a single dependency edge in the graph.
 * Records that glyph A depends on glyph B through a specific OpenType
 * mechanism (table_tag), with additional metadata:
 *
 * - table_tag: Source table (GSUB, cmap, glyf, CFF, COLR, MATH)
 * - dependent: Target glyph ID
 * - layout_tag: Feature tag (for GSUB) or UVS codepoint (for cmap), else 0
 * - ligature_set: Index into sets array for ligature components, else INVALID
 * - context_set: Index into sets array for context requirements, else INVALID
 * - flags: Edge flags (FROM_CONTEXT_POSITION, FROM_NESTED_CONTEXT)
 */
struct hb_depend_data_record_t {
  hb_depend_data_record_t() = delete;
  hb_depend_data_record_t(hb_tag_t table_tag,
                   hb_codepoint_t dependent,
                   hb_tag_t layout_tag,
                   hb_codepoint_t ligature_set,
                   hb_codepoint_t context_set,
                   uint8_t flags = 0) : table_tag(table_tag),
     dependent(dependent), layout_tag(layout_tag),
     ligature_set(ligature_set), context_set(context_set),
     flags(flags)
     {}

  bool operator == (const hb_depend_data_record_t &o) const
  {
    /* NOTE: flags intentionally excluded from equality comparison.
     * Flags are metadata about how an edge was discovered (e.g.,
     * FROM_CONTEXT_POSITION, FROM_NESTED_CONTEXT), not part of the
     * edge's identity. Multiple discoveries of the same edge via
     * different paths should be treated as duplicates. */
    return table_tag == o.table_tag &&
           dependent == o.dependent &&
           layout_tag == o.layout_tag &&
           ligature_set == o.ligature_set &&
           context_set == o.context_set;
  }

  uint32_t hash () const
  {
    /* FNV-1a hash of all identity fields (excludes flags) */
    uint32_t current = 0x84222325;  /* FNV-1a offset basis */
    current = current ^ hb_hash (table_tag);
    current = current * 16777619;   /* FNV-1a prime */
    current = current ^ hb_hash (dependent);
    current = current * 16777619;
    current = current ^ hb_hash (layout_tag);
    current = current * 16777619;
    current = current ^ hb_hash (ligature_set);
    current = current * 16777619;
    current = current ^ hb_hash (context_set);
    current = current * 16777619;
    return current;
  }

  hb_tag_t table_tag;
  hb_codepoint_t dependent;
  hb_tag_t layout_tag;
  hb_codepoint_t ligature_set;
  hb_codepoint_t context_set;
  uint8_t flags;
};

/**
 * edge_key_t:
 *
 * Key structure for edge deduplication. Combines source glyph with edge record.
 * Uses the record's equality and hash operators for comparison.
 */
struct edge_key_t {
  hb_codepoint_t source;
  hb_depend_data_record_t record;

  edge_key_t () : source (0), record (0, 0, 0, HB_CODEPOINT_INVALID, HB_CODEPOINT_INVALID, 0) {}

  edge_key_t (hb_codepoint_t source,
              hb_tag_t table_tag,
              hb_tag_t layout_tag,
              hb_codepoint_t dependent,
              hb_codepoint_t ligature_set,
              hb_codepoint_t context_set)
    : source (source),
      record (table_tag, dependent, layout_tag, ligature_set, context_set, 0) {}

  bool operator == (const edge_key_t &o) const
  {
    return source == o.source && record == o.record;
  }

  uint32_t hash () const
  {
    /* Combine source hash with record hash */
    uint32_t current = 0x84222325;  /* FNV-1a offset basis */
    current = current ^ hb_hash (source);
    current = current * 16777619;   /* FNV-1a prime */
    current = current ^ record.hash ();
    current = current * 16777619;
    return current;
  }
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
 * Internal container for all dependency graph data.
 *
 * Persistent data (retained after graph construction):
 * - glyph_dependencies: Per-glyph edge lists
 * - sets: Ligature sets and context sets indexed by set ID
 *
 * Temporary data (freed after construction via cleanup()):
 * - unicodes: Set of all Unicode codepoints in the font
 * - nominal_glyphs: Codepoint to nominal glyph mapping
 * - lookup_features: Lookup index to feature tag mapping
 * - seen_edges: Struct-based edge deduplication table
 * - set_to_index: Content-based context set deduplication map
 * - free_set_list: Indices of freed sets available for reuse
 *
 * Extraction state (used during graph construction):
 * - current_context_set_index: Context requirements for current rule
 * - current_edge_flags: Flags to apply to edges being recorded
 *
 * This structure is owned by hb_depend_t and freed when the depend
 * object is destroyed.
 */
struct hb_depend_data_t
{
  hb_depend_data_t ()
#ifdef HB_DEPEND_API
    : current_context_set_index (HB_CODEPOINT_INVALID),
      current_edge_flags (0)
#endif
  {}

  ~hb_depend_data_t () {}

  /* Free an unused ligature set for reuse.
   * Only called for ligature sets that were allocated but had no edges added.
   * Ligature sets are never in set_to_index, so no hashmap manipulation needed. */
  void free_ligature_set (hb_codepoint_t set_index)
  {
    if (set_index >= sets.length)
    {
      DEBUG_MSG (SUBSET, nullptr, "Attempting to free invalid set %u (max is %u)",
                 set_index, sets.length - 1);
      return;
    }

    /* Clear the set to save memory */
    sets[set_index]->clear ();

    /* Add to free list for reuse */
    free_set_list.push (set_index);
  }

  /* Allocate a new ligature set (no deduplication).
   * Used for ligature component sets - each must be unique per API contract. */
  hb_codepoint_t new_ligature_set (hb_codepoint_t cp)
  {
    hb_set_t temp_set;
    temp_set.add (cp);
    return new_ligature_set (temp_set);
  }

  hb_codepoint_t new_ligature_set (hb_set_t &set)
  {
    hb_codepoint_t set_index;

    if (free_set_list.length > 0)
    {
      /* Reuse freed set */
      set_index = free_set_list[free_set_list.length - 1];
      free_set_list.resize (free_set_list.length - 1);
      sets[set_index]->set (set);
    }
    else
    {
      /* Allocate new set */
      set_index = sets.length;
      hb_set_t *new_set = hb_set_create ();
      if (unlikely (!new_set))
        return HB_CODEPOINT_INVALID;
      new_set->set (set);
      sets.push (hb::unique_ptr<hb_set_t> {new_set});
    }

    return set_index;
  }


  /* Find existing context set with same contents, or create new one (with deduplication).
   * Only for context sets. For ligature sets, call new_ligature_set() directly.
   *
   * Uses content-based deduplication via hb_hashmap_t with pointer keys:
   * hb_hash dereferences pointers and hashes content, so identical context sets
   * are found regardless of pointer address.
   *
   * Note: Context sets may reuse indices from ligature sets if the contents match.
   * This is acceptable since the API only guarantees uniqueness for ligature sets. */
  hb_codepoint_t find_or_create_context_set (const hb_set_t &set)
  {
    /* Check if we already have this set (content-based lookup via pointer hash) */
    hb_codepoint_t *existing_idx = nullptr;
    if (set_to_index.has (&set, &existing_idx))
      return *existing_idx;  /* Reuse existing set */

    /* Create new set (allocated as if it were a ligature set, but will be deduplicated) */
    hb_codepoint_t new_idx = new_ligature_set (const_cast<hb_set_t&>(set));
    if (unlikely (new_idx == HB_CODEPOINT_INVALID))
      return HB_CODEPOINT_INVALID;

    /* Add to deduplication map for future context set lookups */
    set_to_index.set (sets[new_idx].get (), new_idx);
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
            hb_codepoint_t set_idx = find_or_create_context_set (filtered_set);
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
            hb_codepoint_t set_idx = find_or_create_context_set (filtered_set);
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
    return find_or_create_context_set (context_elements);
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

  bool get_glyph_entry(hb_codepoint_t gid, hb_codepoint_t index,
                       hb_tag_t *table_tag, hb_codepoint_t *dependent,
                       hb_tag_t *layout_tag, hb_codepoint_t *ligature_set,
                       hb_codepoint_t *context_set, uint8_t *flags)
  {
    if (gid < glyph_dependencies.length &&
        index < glyph_dependencies[gid].dependencies.length) {
      auto &d = glyph_dependencies[gid].dependencies[index];
      *table_tag = d.table_tag;
      *dependent = d.dependent;
      *layout_tag = d.layout_tag;
      *ligature_set = d.ligature_set;
      *context_set = d.context_set;
      if (flags) *flags = d.flags;
      return true;
    }
    return false;
  }

  bool get_set_from_index(hb_codepoint_t index, hb_set_t *out)
  {
    if (index < sets.length) {
      out->set(*sets[index]);
      return true;
    }
    return false;
  }

  bool add_depend_layout (hb_codepoint_t target, hb_tag_t table_tag,
                          hb_tag_t layout_tag,
                          hb_codepoint_t dependent,
                          hb_codepoint_t lig_set = HB_CODEPOINT_INVALID,
                          hb_codepoint_t context_set = HB_CODEPOINT_INVALID,
                          uint8_t flags = 0)
  {
    if (target >= glyph_dependencies.length) {
      DEBUG_MSG (SUBSET, nullptr, "Dependency glyph %u for %c%c%c%c too large",
                 target, HB_UNTAG(table_tag));
      return false;
    }

    /* Check if we've already seen this edge (uses struct-based hashing) */
    edge_key_t key (target, table_tag, layout_tag, dependent, lig_set, context_set);
    if (seen_edges.has (key))
      return false;  /* Duplicate edge */

    /* Record as seen and add to dependency list */
    seen_edges.set (key, true);

    auto &gdr = glyph_dependencies[target];
    gdr.dependencies.push (table_tag, dependent, layout_tag, lig_set,
                           context_set, flags);
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

    /* Use current edge flags */
    uint8_t flags = current_edge_flags;
#else
    uint8_t flags = 0;
#endif

    bool any_added = false;
    for (auto t : lookup_features[lookup_index]) {
      if (add_depend_layout(target, HB_OT_TAG_GSUB, t, dependent, lig_set, context_set, flags))
        any_added = true;
    }
    return any_added;
  }

  void add_depend (hb_codepoint_t target, hb_tag_t table_tag,
                   hb_codepoint_t dependent,
                   hb_codepoint_t lig_set = HB_CODEPOINT_INVALID,
                   hb_codepoint_t context_set = HB_CODEPOINT_INVALID,
                   uint8_t flags = 0)
  {
    add_depend_layout (target, table_tag, HB_CODEPOINT_INVALID, dependent, lig_set,
                       context_set, flags);
  }

  hb_codepoint_t get_nominal_glyph(hb_codepoint_t cp)
  {
    return hb_map_get(&nominal_glyphs, cp);
  }

  hb_set_t unicodes;
  hb_map_t nominal_glyphs;

  /* Set storage: vector of heap-allocated sets (both ligature and context sets)
   * Using unique_ptr follows HarfBuzz pattern and provides stable pointers. */
  hb_vector_t<hb::unique_ptr<hb_set_t>> sets;
  hb_vector_t<hb_codepoint_t> free_set_list;  /* Indices of freed sets for reuse */
  /* Context set deduplication: set pointer (content-hashed) -> set_index.
   * Uses HarfBuzz's established pattern where hb_hash dereferences pointers
   * to hash contents, enabling content-based deduplication.
   * Only context sets are added to this map; ligature sets remain unique. */
  hb_hashmap_t<const hb_set_t*, hb_codepoint_t> set_to_index;

  hb_vector_t<hb_glyph_depend_record_t> glyph_dependencies;
  hb_vector_t<hb_set_t> lookup_features;

  /* Edge deduplication: struct-based hashing via edge_key_t */
  hb_hashmap_t<edge_key_t, bool> seen_edges;

#ifdef HB_DEPEND_API
  /* Temporary: Context set index for the current rule.
   * Built once per rule by context_depend_lookup/chain_context_depend_lookup,
   * then used by all edges recorded from nested lookups. */
  hb_codepoint_t current_context_set_index;

  /* Temporary: Edge flags for edges being recorded.
   * Set by context_depend_recurse_lookups before recursing into nested lookups.
   * FROM_CONTEXT_POSITION: multi-position contextual rule (inputCount > 1)
   * FROM_NESTED_CONTEXT: lookup called from within another contextual lookup */
  uint8_t current_edge_flags;
#endif
};

#endif /* HB_DEPEND_API */

#endif /* HB_DEPEND_DATA_HH */
