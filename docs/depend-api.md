# Introduction

**⚠️ WARNING: This API is highly experimental and subject to change. It is not
enabled by default and should not be used in production without understanding
the stability implications. The API may change significantly in future releases.**

The dependency API provides access to a glyph dependency graph that represents
how glyphs in an OpenType font reference or produce other glyphs. The graph
maps input glyphs to output glyphs through various OpenType mechanisms:

- **Glyph substitution (GSUB)**: Tracks which glyphs can be substituted for
  other glyphs through OpenType Layout features
- **Composite glyphs (glyf, CFF)**: Records component glyphs used in composite
  glyph construction (TrueType glyf composites and CFF1 SEAC)
- **Color layers (COLR)**: Identifies glyphs used as color layers
- **Math variants (MATH)**: Tracks mathematical variant glyphs

**Note**: Unicode Variation Sequences (UVS) are not included in the dependency
graph. Handle UVS separately using `hb_font_get_variation_glyph()`.

The dependency graph enables finding the transitive closure of all glyphs
reachable from a given input set, which is useful for font subsetting,
analyzing glyph coverage, and optimizing font delivery.

# Building with the Depend API

The dependency API is guarded by `HB_NO_SUBSET_DEPEND`, which is defined by
default. To enable it, either enable the `experimental_api` build option or
define `HB_SUBSET_DEPEND_API` to opt in to just this API.

## Enabling via build option

```bash
meson setup build -Dexperimental_api=true
meson compile -C build
```

In code, check for availability:

```c
#ifdef HB_SUBSET_DEPEND_API
  // Use dependency API
#endif
```

## Enabling just the depend API

Define `HB_SUBSET_DEPEND_API` on your compiler command line or in a config
header to enable the depend API without enabling all experimental APIs:

```bash
cc ... -DHB_SUBSET_DEPEND_API ...
```

## Discovering available options

To see all available HarfBuzz build options, including `experimental_api`:

```bash
# After setting up a build directory
meson configure build

# Or to see just the option values
meson introspect build --buildoptions | jq '.[] | select(.section == "user")'
```

You can also view all available options in the source file `meson_options.txt`.

**Important:** This API is experimental and may change without notice. Do not
use in production applications without being prepared for breaking changes in
future HarfBuzz releases.

# Usage

## Basic Example

```c
#include <hb.h>

hb_blob_t *blob = hb_blob_create_from_file("font.ttf");
hb_face_t *face = hb_face_create(blob, 0);

// Extract dependency graph
hb_subset_depend_t *depend = hb_subset_depend_from_face_or_fail(face);
if (!depend) {
    // Handle error
    return;
}

// Query dependencies for a specific glyph
hb_codepoint_t gid = 42;  // Glyph to query

unsigned int total = hb_subset_depend_lookup_glyph(depend, gid, 0, NULL, NULL);
printf("Dependencies for glyph %u:\n", gid);
for (unsigned int i = 0; i < total; i++) {
    hb_subset_depend_entry_t entry;
    unsigned int count = 1;
    hb_subset_depend_lookup_glyph(depend, gid, i, &count, &entry);
    printf("  -> glyph %u via %c%c%c%c\n",
           entry.dependent, HB_UNTAG(entry.table_tag));
}

// Clean up
hb_subset_depend_destroy(depend);
hb_face_destroy(face);
hb_blob_destroy(blob);
```

## Iterating Dependencies

To programmatically access dependency information for a specific glyph:

```c
hb_codepoint_t gid = 42;  // Glyph ID to query

// Get total count, then iterate
unsigned int total = hb_subset_depend_lookup_glyph(depend, gid, 0, NULL, NULL);
for (unsigned int i = 0; i < total; i++) {
    hb_subset_depend_entry_t entry;
    unsigned int count = 1;
    hb_subset_depend_lookup_glyph(depend, gid, i, &count, &entry);

    // Process dependency information
    if (entry.table_tag == HB_OT_TAG_GSUB) {
        // GSUB dependency: layout_tag contains feature tag
        printf("GID %u -> %u via GSUB feature '%c%c%c%c'\n",
               gid, entry.dependent, HB_UNTAG(entry.layout_tag));
    } else {
        // Other dependencies (glyf, CFF, COLR, MATH)
        printf("GID %u -> %u via %c%c%c%c\n",
               gid, entry.dependent, HB_UNTAG(entry.table_tag));
    }
}
```

## Finding Reachable Glyphs

To compute all glyphs reachable from a starting set (transitive closure):

```c
void find_reachable_glyphs(hb_subset_depend_t *depend, hb_set_t *reachable)
{
    hb_set_t *to_process = hb_set_create();
    hb_set_union(to_process, reachable);

    // Process glyphs until none remain
    while (!hb_set_is_empty(to_process)) {
        hb_codepoint_t gid = hb_set_get_min(to_process);
        hb_set_del(to_process, gid);

        unsigned int total = hb_subset_depend_lookup_glyph(depend, gid, 0, NULL, NULL);
        for (unsigned int i = 0; i < total; i++) {
            hb_subset_depend_entry_t entry;
            unsigned int count = 1;
            hb_subset_depend_lookup_glyph(depend, gid, i, &count, &entry);

            if (!hb_set_has(reachable, entry.dependent)) {
                hb_set_add(reachable, entry.dependent);
                hb_set_add(to_process, entry.dependent);
            }
        }
    }

    hb_set_destroy(to_process);
}

// Usage:
hb_set_t *reachable = hb_set_create();
hb_set_add(reachable, 100);
hb_set_add(reachable, 101);

find_reachable_glyphs(depend, reachable);

printf("Found %u reachable glyphs\n", hb_set_get_population(reachable));
hb_set_destroy(reachable);
```

**Note:** This simple algorithm doesn't handle ligature dependencies or context sets
correctly. Ligatures should only be added when all component glyphs in their ligature
set are present, and contextual dependencies should only be followed when their positional
requirements are satisfied. For a production-quality implementation that correctly handles
ligatures, context filtering, and feature filtering to compute closures similar to
HarfBuzz's subset API, see `compute_depend_closure()` in `test/fuzzing/hb-depend-closure-parity.cc`
and `docs/depend-for-closure.md`.

## Working with Ligature Sets

When a dependency entry belongs to a ligature set, you can find all other members
of that set using `hb_subset_depend_lookup_set()`:

```c
// After finding entry.ligature_set_index != HB_CODEPOINT_INVALID...

// Get all glyphs in this ligature set
hb_set_t *ligature_glyphs = hb_set_create();
if (hb_subset_depend_lookup_set(depend, entry.ligature_set_index, ligature_glyphs)) {
    hb_codepoint_t lig_gid = HB_SET_VALUE_INVALID;

    printf("Members of ligature set %u:\n", entry.ligature_set_index);
    while (hb_set_next(ligature_glyphs, &lig_gid)) {
        // Find the dependency entry index for this glyph in this ligature set
        unsigned int lig_total = hb_subset_depend_lookup_glyph(depend, lig_gid, 0, NULL, NULL);
        for (unsigned int j = 0; j < lig_total; j++) {
            hb_subset_depend_entry_t lig_entry;
            unsigned int count = 1;
            hb_subset_depend_lookup_glyph(depend, lig_gid, j, &count, &lig_entry);
            if (lig_entry.ligature_set_index == entry.ligature_set_index) {
                printf("  gid=%u, index=%u\n", lig_gid, j);
                break;
            }
        }
    }
}
hb_set_destroy(ligature_glyphs);
```

## Working with Context Sets

Context and ChainContext GSUB rules apply lookups only when specific backtrack and/or
lookahead glyphs are present. The depend API records these requirements in
`context_set_index` fields as optional information that can refine dependency traversal.

**Note:** Context sets are optional refinement information. Uses of the dependency graph
that ignore `context_set_index` (treating it as always satisfied) work fine - they produce a
conservative over-approximation. Context sets allow more precise closure computation when
desired.

### Understanding Context Sets

When `entry.context_set_index != HB_CODEPOINT_INVALID`, the edge includes positional
requirements from a Context or ChainContext rule. Context set elements use two encodings:

1. **Direct GID reference** (value < 0x80000000): A specific glyph ID
2. **Indirect set reference** (value >= 0x80000000): An index into the sets array
   (mask off high bit with `value & 0x7FFFFFFF` to get the set index)

The high bit (0x80000000) distinguishes between direct and indirect references. For an
edge to apply, ALL elements in the context set must be satisfied:
- Direct references: that specific glyph must be present in the closure
- Indirect references: at least ONE glyph from the referenced set must be present (disjunction)

### Checking Context Requirements

```c
bool check_context_satisfied(hb_subset_depend_t *depend,
                             hb_codepoint_t context_set_idx,
                             hb_set_t *current_closure)
{
    if (context_set_idx == HB_CODEPOINT_INVALID)
        return true;  // No context requirements

    hb_set_t *context_elements = hb_set_create();
    if (!hb_subset_depend_lookup_set(depend, context_set_idx, context_elements))
    {
        hb_set_destroy(context_elements);
        return false;  // Error retrieving context
    }

    bool satisfied = true;
    hb_codepoint_t elem = HB_SET_VALUE_INVALID;

    while (hb_set_next(context_elements, &elem))
    {
        if (elem < 0x80000000)
        {
            // Direct reference: check if specific glyph is in closure
            if (!hb_set_has(current_closure, elem))
            {
                satisfied = false;
                break;
            }
        }
        else
        {
            // Indirect reference: check if ANY glyph from set is in closure
            hb_codepoint_t set_idx = elem & 0x7FFFFFFF;
            hb_set_t *required_set = hb_set_create();

            if (hb_subset_depend_lookup_set(depend, set_idx, required_set))
            {
                // Check if ANY element from required_set is in current_closure
                bool any_found = false;
                hb_codepoint_t glyph = HB_SET_VALUE_INVALID;
                while (hb_set_next(required_set, &glyph))
                {
                    if (hb_set_has(current_closure, glyph))
                    {
                        any_found = true;
                        break;
                    }
                }
                if (!any_found)
                    satisfied = false;
            }
            else
            {
                satisfied = false;  // Error retrieving set
            }

            hb_set_destroy(required_set);
            if (!satisfied)
                break;
        }
    }

    hb_set_destroy(context_elements);
    return satisfied;
}
```

For a complete implementation of context-aware closure computation, see the
`compute_depend_closure()` function in `test/fuzzing/hb-depend-closure-parity.cc`.

# API Details

## Dependency Entry Fields

Each `hb_subset_depend_entry_t` filled by `hb_subset_depend_lookup_glyph()` contains:

- **table_tag**: Source table (e.g., `HB_OT_TAG_GSUB`,
  `HB_TAG('g','l','y','f')`, `HB_TAG('C','F','F',' ')`, `HB_TAG('C','O','L','R')`,
  `HB_TAG('M','A','T','H')`)
- **dependent**: The dependent glyph ID
- **layout_tag**:
  - For GSUB: the feature tag (e.g., `HB_TAG('l','i','g','a')`)
  - Otherwise: `HB_CODEPOINT_INVALID`
- **ligature_set_index**: For ligatures, identifies which ligature set; otherwise
  `HB_CODEPOINT_INVALID`
- **context_set_index**: Optional refinement information for Context and ChainContext GSUB
  rules. When not `HB_CODEPOINT_INVALID`, identifies positional requirements
  (backtrack/lookahead) for this dependency. Can be ignored (treated as always satisfied)
  for conservative over-approximation, or checked for more precise closure computation.
  See "Working with Context Sets" section for details.
- **flags**: Edge metadata flags (`hb_subset_depend_edge_flags_t`). Currently defined:
  - `HB_SUBSET_DEPEND_EDGE_FLAG_FROM_CONTEXT_POSITION` (0x01): Edge created from a
    multi-position contextual rule (Context or ChainContext with inputCount > 1). Depend
    extraction records edges based on what glyphs COULD be at each position according to
    the static input coverage/class. But during closure computation, lookups within the
    rule are applied sequentially: lookups at earlier positions may transform glyphs at
    later positions, and multiple lookups at the same position may interact (one produces
    a glyph, another immediately consumes it as an "intermediate"). So a glyph matching
    the coverage might not actually persist at that position when closure traverses the
    rule. These edges may cause over-approximation.
  - `HB_SUBSET_DEPEND_EDGE_FLAG_FROM_NESTED_CONTEXT` (0x02): Edge created from a lookup
    that was called from within another contextual lookup (nested context). The outer
    context's requirements are not preserved on this edge, so it may over-approximate.

  These flags help distinguish between "true" over-approximation (a bug) and "expected"
  over-approximation (a known limitation of the static dependency analysis). Closure
  implementations can use these flags to report which type of over-approximation occurred.

# Implementation Notes

**⚠️ Warning: Graph Cycles in Invalid Fonts**

The dependency graph may contain cycles when processing invalid or malicious fonts.
While the OpenType specification requires COLR paint graphs to be directed acyclic
graphs (DAGs), the depend API faithfully reports the graph structure as it exists
in the font, including any cycles that may be present. Implementations traversing
the depend graph should implement cycle detection to protect against invalid fonts.

For details on how cycles can occur and how PaintGlyph self-references are filtered,
see the "COLR Cycles and Self-References" section in `docs/depend-implementation.md`.

For detailed information about the depend API implementation, including memory
management, data structures, and performance characteristics, see
`docs/depend-implementation.md`.

# Use Cases

## Font Subsetting

Determine which glyphs must be retained to properly render a specific set of
characters, accounting for all OpenType substitutions and compositions.

## Coverage Analysis

Analyze which features or scripts require which glyphs, useful for font
optimization and planning.

## Font Segmentation

Partition a font into smaller subsets where each segment contains glyphs reachable
from a specific set of input characters, enabling more efficient font delivery
for web applications.

## Testing

Verify that font modifications haven't inadvertently broken glyph references or
substitution chains.
