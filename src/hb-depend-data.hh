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

/* This file exists to break include cycles: hb-ot-layout-gsubgpos.hh needs
 * hb_depend_data_builder_t for hb_depend_context_t, but hb-depend.hh pulls
 * in table headers that include hb-ot-layout-gsubgpos.hh. */

#include "hb.hh"

#include "hb-multimap.hh"

#ifndef HB_NO_SUBSET_DEPEND

/**
 * Edge flags for hb_depend_edge_t.
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
 * hb_depend_edge_t:
 *
 * Internal structure representing a single dependency edge in the graph.
 * Records that glyph A depends on glyph B through a specific OpenType
 * mechanism (table_tag), with additional metadata:
 *
 * - table_tag: Source table (GSUB, glyf, CFF, COLR, MATH)
 * - dependent: Target glyph ID
 * - layout_tag: Feature tag (for GSUB), else 0
 * - ligature_set: Index into sets array for ligature components, else INVALID
 * - context_set: Index into sets array for context requirements, else INVALID
 * - flags: Edge flags (FROM_CONTEXT_POSITION, FROM_NESTED_CONTEXT)
 */
struct hb_depend_edge_t {
  hb_depend_edge_t() = delete;
  hb_depend_edge_t(hb_tag_t table_tag,
                   hb_codepoint_t dependent,
                   hb_tag_t layout_tag,
                   hb_codepoint_t ligature_set,
                   hb_codepoint_t context_set,
                   uint8_t flags = 0) : table_tag(table_tag),
     dependent(dependent), layout_tag(layout_tag),
     ligature_set(ligature_set), context_set(context_set),
     flags(flags)
     {}

  bool operator == (const hb_depend_edge_t &o) const
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
 * hb_depend_edge_key_t:
 *
 * Key structure for edge deduplication. Combines source glyph with edge record.
 * Uses the record's equality and hash operators for comparison.
 */
struct hb_depend_edge_key_t {
  hb_codepoint_t source;
  hb_depend_edge_t record;

  hb_depend_edge_key_t () : source (0), record (0, 0, 0, HB_CODEPOINT_INVALID, HB_CODEPOINT_INVALID, 0) {}

  hb_depend_edge_key_t (hb_codepoint_t source,
              hb_tag_t table_tag,
              hb_tag_t layout_tag,
              hb_codepoint_t dependent,
              hb_codepoint_t ligature_set,
              hb_codepoint_t context_set)
    : source (source),
      record (table_tag, dependent, layout_tag, ligature_set, context_set, 0) {}

  bool operator == (const hb_depend_edge_key_t &o) const
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
 * hb_depend_glyph_record_t:
 *
 * Internal structure holding all dependency edges for a single glyph.
 */
struct hb_depend_glyph_record_t {
  hb_vector_t<hb_depend_edge_t> dependencies;
};

/**
 * hb_depend_lookup_revmap_t:
 *
 * Internal structure tracking which features are associated with a lookup.
 * Used during GSUB dependency analysis to record the feature tags that
 * activate each lookup.
 */
struct hb_depend_lookup_revmap_t
{
  hb_depend_lookup_revmap_t () = default;
  explicit hb_depend_lookup_revmap_t (bool full) : full (full) {}
  hb_depend_lookup_revmap_t (const hb_depend_lookup_revmap_t &o) : full(o.full),
                                                                     fv_indexes(o.fv_indexes) {}
  hb_depend_lookup_revmap_t (hb_depend_lookup_revmap_t &&o) : full(o.full),
                                                                fv_indexes(std::move(o.fv_indexes)) {}
  hb_depend_lookup_revmap_t& operator = (const hb_depend_lookup_revmap_t &o)
  {
    full = o.full;
    fv_indexes = o.fv_indexes;
    return *this;
  }

  bool full = true;
  hb_set_t fv_indexes;
};

/**
 * hb_depend_data_t:
 *
 * Persistent dependency graph data, retained for the lifetime of hb_depend_t.
 * Contains only what is needed at query time:
 * - glyph_dependencies: Per-glyph edge lists
 * - sets: Ligature and context sets indexed by set ID
 *
 * Constructed via hb_depend_data_builder_t; immutable thereafter.
 */
struct hb_depend_data_t
{
  /* Set storage: vector of heap-allocated sets (both ligature and context sets).
   * Using unique_ptr follows HarfBuzz pattern and provides stable pointers. */
  hb_vector_t<hb::unique_ptr<hb_set_t>> sets;

  hb_vector_t<hb_depend_glyph_record_t> glyph_dependencies;

  const hb_set_t *get_set_from_index (hb_codepoint_t index)
  {
    if (index < sets.length)
      return sets[index].get ();
    return nullptr;
  }

  bool get_glyph_entry (hb_codepoint_t gid, hb_codepoint_t index,
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

  void print ()
  {
    for (unsigned i = 0; i < glyph_dependencies.length; i++) {
      auto &gd = glyph_dependencies[i];
      if (!gd.dependencies.length)
        continue;
      printf ("GID %u:\n", i);
      for (auto &d : gd.dependencies) {
        if (d.table_tag == HB_OT_TAG_GSUB) {
          printf ("  layout %c%c%c%c -> %u", HB_UNTAG(d.layout_tag), d.dependent);
          if (d.ligature_set != HB_CODEPOINT_INVALID)
            printf ("  (ligature)");
        } else {
          printf ("  %c%c%c%c -> %u", HB_UNTAG(d.table_tag), d.dependent);
        }
        printf ("\n");
      }
    }
  }
};

/**
 * hb_depend_data_builder_t:
 *
 * Temporary builder for constructing hb_depend_data_t. Holds all state
 * needed during graph extraction; goes out of scope (and is freed) when
 * construction is complete, leaving only hb_depend_data_t.
 *
 * Temporary state (freed on destruction):
 * - lookup_features: Lookup index to feature tag mapping
 * - seen_edges: Struct-based edge deduplication table
 * - set_to_index: Content-based context set deduplication map
 * - free_set_list: Indices of freed sets available for reuse
 * - current_context_set_index: Context requirements for current rule
 * - current_edge_flags: Flags to apply to edges being recorded
 */
struct hb_depend_data_builder_t
{
  hb_depend_data_builder_t (hb_depend_data_t &data_)
    : current_context_set_index (HB_CODEPOINT_INVALID),
      current_edge_flags (0),
      data (data_) {}

  /* Forward to data for use during construction (e.g. from hb_depend_context_t) */
  const hb_set_t *get_set_from_index (hb_codepoint_t index)
  { return data.get_set_from_index (index); }

  /* Free an unused ligature set for reuse.
   * Only called for ligature sets that were allocated but had no edges added. */
  void free_ligature_set (hb_codepoint_t set_index)
  {
    if (set_index >= data.sets.length)
    {
      DEBUG_MSG (SUBSET, nullptr, "Attempting to free invalid set %u (max is %u)",
                 set_index, data.sets.length - 1);
      return;
    }
    data.sets[set_index]->clear ();
    free_set_list.push (set_index);
  }

  /* Allocate a new ligature set (no deduplication). */
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
      set_index = free_set_list.pop ();
      data.sets[set_index]->set (set);
    }
    else
    {
      set_index = data.sets.length;
      hb_set_t *new_set = hb_set_create ();
      if (unlikely (!new_set))
        return HB_CODEPOINT_INVALID;
      new_set->set (set);
      if (unlikely (!data.sets.push_or_fail (hb::unique_ptr<hb_set_t> {new_set})))
        return HB_CODEPOINT_INVALID;
    }

    return set_index;
  }

  /* Find existing context set with same contents, or create new one (with deduplication).
   * Uses content-based deduplication via hb_hashmap_t with pointer keys. */
  hb_codepoint_t find_or_create_context_set (const hb_set_t &set)
  {
    hb_codepoint_t *existing_idx = nullptr;
    if (set_to_index.has (&set, &existing_idx))
      return *existing_idx;

    hb_codepoint_t new_idx = new_ligature_set (const_cast<hb_set_t&>(set));
    if (unlikely (new_idx == HB_CODEPOINT_INVALID))
      return HB_CODEPOINT_INVALID;

    set_to_index.set (data.sets[new_idx].get (), new_idx);
    return new_idx;
  }

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
          direct_requirements.add (back_set.get_min ());
      }
    }

    if (lookahead_sets)
    {
      for (const auto &look_set : *lookahead_sets)
      {
        if (look_set.get_population () == 1)
          direct_requirements.add (look_set.get_min ());
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

    context_elements.union_ (direct_requirements);

    if (context_elements.is_empty ())
      return HB_CODEPOINT_INVALID;

    return find_or_create_context_set (context_elements);
  }

  bool add_depend_layout (hb_codepoint_t target, hb_tag_t table_tag,
                          hb_tag_t layout_tag,
                          hb_codepoint_t dependent,
                          hb_codepoint_t lig_set = HB_CODEPOINT_INVALID,
                          hb_codepoint_t context_set = HB_CODEPOINT_INVALID,
                          uint8_t flags = 0)
  {
    if (target >= data.glyph_dependencies.length) {
      DEBUG_MSG (SUBSET, nullptr, "Dependency glyph %u for %c%c%c%c too large",
                 target, HB_UNTAG(table_tag));
      return false;
    }

    hb_depend_edge_key_t key (target, table_tag, layout_tag, dependent, lig_set, context_set);
    if (seen_edges.has (key))
      return false;

    seen_edges.set (key, true);

    auto &gdr = data.glyph_dependencies[target];
    gdr.dependencies.push (table_tag, dependent, layout_tag, lig_set,
                           context_set, flags);
    return true;
  }

  bool add_gsub_lookup (hb_codepoint_t target, hb_codepoint_t lookup_index,
                        hb_codepoint_t dependent,
                        hb_codepoint_t lig_set = HB_CODEPOINT_INVALID,
                        hb_codepoint_t context_set = HB_CODEPOINT_INVALID)
  {
    if (context_set == HB_CODEPOINT_INVALID)
      context_set = current_context_set_index;
    uint8_t flags = current_edge_flags;

    bool any_added = false;
    for (auto t : lookup_features[lookup_index]) {
      if (add_depend_layout (target, HB_OT_TAG_GSUB, t, dependent, lig_set, context_set, flags))
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

  hb_codepoint_t get_nominal_glyph (hb_codepoint_t cp)
  { return hb_map_get (&nominal_glyphs, cp); }

  hb_set_t unicodes;
  hb_map_t nominal_glyphs;
  hb_vector_t<hb_set_t> lookup_features;
  hb_hashmap_t<hb_depend_edge_key_t, bool> seen_edges;
  hb_hashmap_t<const hb_set_t*, hb_codepoint_t> set_to_index;
  hb_vector_t<hb_codepoint_t> free_set_list;
  hb_codepoint_t current_context_set_index;
  uint8_t current_edge_flags;

  hb_depend_data_t &data;
};

#endif /* !HB_NO_SUBSET_DEPEND */

#endif /* HB_DEPEND_DATA_HH */
