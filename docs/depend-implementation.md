# Depend API Implementation

## Introduction

The depend API implementation was derived from HarfBuzz's existing closure code paths. The closure infrastructure computes which glyphs are reachable from a starting set by following substitution rules, while the depend API extracts the underlying dependency graph that makes those substitutions possible.

This document describes the depend implementation by contrasting it with the closure paths, focusing on what is different and why.

## Closure vs. Depend: Core Difference

**Closure** answers the question: "Starting from these glyphs, what glyphs can be reached?"

**Depend** answers the question: "What is the complete dependency graph showing which glyphs depend on which other glyphs?"

### Example

Given a ligature rule: `f + i → fi`

- **Closure** with input `{f}`: Returns `{f, fi}` (reachable glyphs)
- **Depend**: Records edges `f → fi` and `i → fi` (the dependency graph)

The closure only tells you what's reachable from a specific starting point. The depend graph captures all possible relationships regardless of starting point.

## Graph Extraction Strategy

The depend implementation extracts the complete dependency graph by walking the
font structure once to record all possible dependency edges.

### Active Glyph Filtering

During graph extraction, depend filters by active glyphs using `parent_active_glyphs()`
(same as closure does). This prevents recording spurious edges for glyphs that can
never participate in a particular substitution.

**Critical difference from closure:** Depend does NOT do sequential accumulation.
In a contextual rule with multiple lookups, closure accumulates outputs from earlier
lookups as inputs to later lookups. Depend records edges from ALL lookups independently
to capture the complete graph structure.

### Context Requirement Recording

For Context and ChainContext GSUB rules, depend records positional requirements
(backtrack/lookahead) in context_set indices. This allows filtering during closure
computation - edges are only followed when their positional requirements are satisfied.

Implementation in `context_depend_recurse_lookups()` (hb-ot-layout-gsubgpos.hh):
1. Build backtrack and lookahead glyph sets for the rule
2. Call `build_context_set()` to store these requirements
3. Set `current_context_set_index` so nested edges inherit the context
4. Recurse into nested lookups
5. Clear context index when done

### Edge Flags for Over-Approximation Detection

Certain contextual edges cannot be precisely modeled by the static dependency graph.
The depend API marks these edges with flags so closure implementations can distinguish
between "expected" over-approximation (a known limitation) and "unexpected" over-approximation
(a bug).

Two flags are defined in `hb-depend-data.hh`:

- **`HB_DEPEND_EDGE_FLAG_FROM_CONTEXT_POSITION` (0x01)**: Set when an edge comes from a
  multi-position contextual rule (inputCount > 1). Depend extraction records edges based
  on what glyphs COULD be at each position per the static input coverage/class. But during
  closure computation, lookups within the rule are applied sequentially: lookups at earlier
  positions may transform later positions, and multiple lookups at the same position may
  interact (one produces a glyph, another immediately consumes it as an "intermediate").
  So depend may record edges from glyphs that wouldn't persist at that position.

- **`HB_DEPEND_EDGE_FLAG_FROM_NESTED_CONTEXT` (0x02)**: Set when a lookup is called from
  within another contextual lookup. The outer context's requirements are not preserved,
  so the edge may over-approximate.

These flags are set in `context_depend_recurse_lookups()`:
```c++
if (inputCount > 1) {
  c->depend_data->current_edge_flags |= HB_DEPEND_EDGE_FLAG_FROM_CONTEXT_POSITION;
}
if (nested_contextual) {
  c->depend_data->current_edge_flags |= HB_DEPEND_EDGE_FLAG_FROM_NESTED_CONTEXT;
}
```

Closure implementations should track whether any flagged edges contributed to the result.
If so, any extra glyphs compared to subset's closure are "expected" over-approximation.

### Graph Completeness

The depend graph captures all edges that exist in the font structure. Recent testing
with proper ligature_set, context_set, and feature filtering has not generated
over-approximation cases when computing closures.

### Using the Graph for Closure Computation

To compute closures using the extracted graph, see `depend-for-closure.md` which
documents how to implement closure by iteratively following edges in the dependency
graph with appropriate filtering.

## Data Structures

### Closure Data Structures

Closure modifies sets in place:
```c++
hb_set_t *glyphs;  // Modified during traversal
hb_map_t done_lookups_glyph_count;
hb_hashmap_t<unsigned, hb::unique_ptr<hb_set_t>> done_lookups_glyph_set;
```

### Depend Data Structures

Depend builds a persistent graph with deduplication:

```c++
struct hb_depend_data_t {
  hb_vector_t<hb_glyph_depend_record_t> glyph_dependencies;  // Per-glyph edge lists
  hb_map_t<uint64_t, hb_vector_t<uint32_t>> edge_hashes;    // Deduplication
  hb_vector_t<hb_set_t> sets;                                // Ligature and context sets
  hb_map_t lookup_features[256];                             // Feature tracking
  hb_codepoint_t current_context_set_index;                  // Current context during extraction
  uint8_t current_edge_flags;                                // Flags for edges being extracted
};
```

**Key features**:
- **Deduplication**: Hash-based duplicate detection prevents storing the same edge multiple times
- **Ligature sets**: Tracks complete set of component glyphs needed for each ligature
- **Context sets**: Records backtrack/lookahead requirements for contextual rules
- **Feature tracking**: Records which features activate which lookups
- **Edge flags**: Marks edges that may cause expected over-approximation (FROM_CONTEXT_POSITION, FROM_NESTED_CONTEXT)
- **Persistent storage**: Graph survives beyond initial extraction

## Graph Lifecycle and Memory Management

The depend graph is immutable after construction:

1. **Construction**: `hb_depend_from_face_or_fail()` walks all relevant OpenType tables once to build the complete graph
2. **Temporary structures freed**: After graph extraction, temporary structures used only during construction are freed:
   - `edge_hashes` - deduplication hash table
   - `unicodes` - Unicode codepoint set
   - `nominal_glyphs` - codepoint-to-glyph mapping
   - `lookup_features` - lookup-to-feature mapping
3. **Persistent structures retained**:
   - `glyph_dependencies` - the core dependency graph
   - `sets` - ligature sets for querying

This reduces memory footprint while keeping the graph queryable for the lifetime of the `hb_depend_t` object.

Dependencies are stored per-glyph and indexed sequentially starting from 0. Use `hb_depend_get_glyph_entry()` with incrementing index values to iterate through all dependencies for a given glyph.

## Context Filtering and Recording

### Closure Behavior

Closure filters glyphs by context during execution:

```c++
// In ContextSubst Format 1
const auto &input_coverage = this+coverageZ[0];
+ hb_zip (ruleSet, input_coverage)
| hb_filter (glyphs, hb_second)  // Filter by active glyphs
| hb_map (hb_first)
| hb_apply ([&] (const RuleSet &_) { _.closure (c); })
;
```

### Depend Behavior

Depend filters glyphs AND records positional requirements:

```c++
// In ContextSubst Format 1 depend method
+ hb_zip (ruleSet, input_coverage)
| hb_filter (c->parent_active_glyphs(), hb_second)
| hb_apply ([&] (const hb_pair_t<...> &_) {
    const RuleSet& rs = this+_.first;
    hb_codepoint_t value = _.second;
    rs.depend (c, value);  // Pass context value
  })
;
```

The `value` parameter identifies which glyph from the coverage was matched, allowing
the depend code to filter nested lookups appropriately.

Additionally, for Context and ChainContext rules, depend builds and records context_set:

```c++
// Build context set for this rule (in context_depend_recurse_lookups)
hb_codepoint_t context_set_idx = c->depend_data->build_context_set(
    &lookahead_sets, nullptr);  // or both backtrack and lookahead
c->depend_data->current_context_set_index = context_set_idx;

// Recurse into lookups - all edges inherit this context_set
context_depend_recurse_lookups(...);

// Clear context after processing
c->depend_data->current_context_set_index = HB_CODEPOINT_INVALID;
```

This records which backtrack and lookahead glyphs must be present for each edge,
enabling accurate closure computation that matches subset behavior in recent testing.

## Feature Tagging

Depend tracks which OpenType features are associated with each dependency edge. This allows subsetting tools to determine which glyphs are needed for specific feature sets.

### Implementation

During GSUB initialization in `hb_depend_t::get_gsub_dependencies()`:

```c++
// Map features to lookups
for (unsigned i = 0; i < feature_count; i++) {
  hb_tag_t ft = gsub.get_feature_tag(i);
  const Feature* feature_ptr = &(fv.get_feature(i));

  for (auto lookup_index : lookup_indexes) {
    data.lookup_features[lookup_index].add(ft);
  }
}
```

When recording dependencies:

```c++
// Look up which features activate this lookup
hb_tag_t layout_tag = HB_TAG_NONE;
if (!data.lookup_features[lookup_index].is_empty()) {
  layout_tag = *data.lookup_features[lookup_index].iter();
}

// Record edge with feature tag
data.add_depend_layout(gid, HB_OT_TAG_GSUB, layout_tag, dependent, ...);
```

This is stored in the `layout_tag` field of each dependency record and can be queried via `hb_depend_get_glyph_entry()`.

## Ligature Sets

For ligature substitutions, depend tracks the complete set of component glyphs needed
to form each ligature. This is stored as a set of glyphs indexed by a `ligature_set` ID.

### Why This Matters

Consider a ligature substitution rule:
```
"f" + "f" → "ff_ligature"
```

The ligature output depends on BOTH component glyphs. During closure computation, the
ligature should only be added when ALL components are present in the closure.

The ligature_set field identifies which components must be present together. All edges
for the same ligature (from each component to the ligature output) share the same
ligature_set index, allowing closure computation to check if all components are available
before adding the ligature output.

This enables correct ligature handling during closure computation.

## COLR Cycles and Self-References

### Cycle Handling

The OpenType specification requires COLR paint graphs to be directed acyclic graphs (DAGs), but invalid or malicious fonts may contain cycles. The depend API faithfully reports the graph structure as it exists in the font, including any cycles.

Cycles can occur through COLRv1 PaintColrGlyph (Format 11) operations that reference base glyphs recursively, creating paths like A→B, B→C, C→A. The depend implementation does not perform cycle detection during graph extraction - it records all edges as they appear in the font.

### PaintGlyph Self-Reference Filtering

COLRv1 PaintGlyph (Format 10) self-references are specifically filtered out and NOT reported as dependencies. This is because Format 10 references glyph outlines (from glyf/CFF tables) as geometry, not paint graphs. A base glyph referencing its own outline to fill is valid per spec.

Implementation in `colrv1-depend.hh:56`:
```c++
// PaintGlyph (Format 10) - filter self-references
if (gid != c->source_gid)
  c->depend_data->add_depend(c->source_gid, HB_OT_TAG_COLR, gid);
```

Contrast with PaintColrGlyph (Format 11) which does NOT filter self-references, as these represent actual paint graph dependencies that may form cycles in invalid fonts.

**Important**: Implementations traversing the depend graph should implement cycle detection to protect against invalid fonts.

## Method Naming Convention

The depend methods follow the closure naming pattern with `depend` instead of `closure`:

| Closure Method | Depend Method | Purpose |
|---------------|---------------|---------|
| `closure(hb_closure_context_t*)` | `depend(hb_depend_context_t*)` | Process this subtable |
| `closure_lookups(...)` | N/A | Depend doesn't need lookup closure |
| `intersects(...)` | `intersects(...)` | Shared - check if subtable is relevant |

All depend methods are guarded by `#ifdef HB_DEPEND_API` to allow conditional compilation.

## Performance Characteristics

### Closure
- **First query**: O(font_size) - must walk entire font
- **Subsequent queries**: O(font_size) - must walk entire font again
- **Space**: O(1) - no persistent storage beyond input sets

### Depend
- **Graph extraction**: O(font_size) - one-time cost
- **Closure queries**: O(edges_in_closure) - much faster, uses pre-built graph
- **Space**: O(edges) - stores complete dependency graph

The depend API trades space for time - it pays the cost of extracting the full graph once, then subsequent closure queries are much faster because they just follow pre-computed edges.

## Code Structure

### Key Files

- **src/hb-depend.h** - Public C API
- **src/hb-depend.hh** - Internal C++ API
- **src/hb-depend.cc** - Main implementation (table dispatching)
- **src/hb-depend-data.hh** - Data structures and edge storage
- **src/hb-ot-layout-gsubgpos.hh** - Contextual lookup handling (shared between closure and depend)
- **src/OT/Layout/GSUB/*.hh** - GSUB format-specific depend methods
- **src/OT/Color/COLR/colrv1-depend.hh** - COLRv1 depend implementation

**Note**: The public API includes `hb_depend_print()` for debugging, which prints the full graph to stdout. For programmatic access, use `hb_depend_get_glyph_entry()` instead.

### Parallel Structure

Each closure method has a corresponding depend method in the same file:

```c++
// In SingleSubstFormat1.hh

void closure (hb_closure_context_t *c) const {
  // Closure implementation
}

#ifdef HB_DEPEND_API
bool depend (hb_depend_context_t *c) const {
  // Depend implementation - similar structure, different output
}
#endif
```

This keeps related code together and makes it easier to maintain consistency.

## Summary of Key Differences

| Aspect | Closure | Depend |
|--------|---------|--------|
| **Goal** | Compute reachable glyphs from a starting set | Extract complete dependency graph |
| **Output** | Modified glyph set | Persistent graph with edges |
| **Repeated queries** | Must re-walk font each time | O(1) edge lookups in graph |
| **Active glyph filtering** | Filters by active glyphs during traversal | Filters by parent_active_glyphs() during extraction |
| **Sequential accumulation** | Accumulates outputs for next lookup | Does NOT accumulate (records all edges) |
| **Context filtering** | Filters during execution | Records requirements in context_set |
| **Edge flags** | N/A | Marks edges for expected over-approximation |
| **Feature tagging** | Not tracked | Tracks which features activate edges |
| **Ligature sets** | Not tracked | Tracks complete component sets |
| **Deduplication** | Not needed | Hash-based to prevent duplicate edges |
| **Memory management** | Minimal - no persistent graph | Temporary structures freed after construction |
| **Space complexity** | O(1) | O(edges) |
| **Time complexity** | O(font_size) per query | O(font_size) once, then O(edges) per query |

## Future Considerations

The depend API currently focuses on GSUB, glyf, cmap, COLR, and MATH tables. Future extensions could include:

- **GPOS dependencies**: Some GPOS rules reference glyphs (e.g., mark-to-base positioning)
- **MORX dependencies**: Apple Advanced Typography morph tables
- **Optimization**: The graph could be compressed or indexed differently for even faster queries
- **Incremental updates**: Support for modifying the graph as the font changes

The foundation is designed to be extensible - adding support for new tables follows the same pattern established by existing implementations.
