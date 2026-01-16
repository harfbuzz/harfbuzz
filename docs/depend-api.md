# Introduction

**⚠️ WARNING: This API is highly experimental and subject to change. It is not
enabled by default and should not be used in production without understanding
the stability implications. The API may change significantly in future releases.**

The dependency API provides access to a glyph dependency graph that represents
how glyphs in an OpenType font reference or produce other glyphs. The graph
maps input glyphs to output glyphs through various OpenType mechanisms:

- **Character mapping (cmap)**: Maps Unicode codepoints to glyphs, including
  Unicode Variation Sequences (UVS)
- **Glyph substitution (GSUB)**: Tracks which glyphs can be substituted for
  other glyphs through OpenType Layout features
- **Composite glyphs (glyf, CFF)**: Records component glyphs used in composite
  glyph construction (TrueType glyf composites and CFF1 SEAC)
- **Color layers (COLR)**: Identifies glyphs used as color layers
- **Math variants (MATH)**: Tracks mathematical variant glyphs

The dependency graph enables finding the transitive closure of all glyphs
reachable from a given input set, which is useful for font subsetting,
analyzing glyph coverage, and optimizing font delivery.

# Usage

## Basic Example

```c
#include <hb.h>

hb_blob_t *blob = hb_blob_create_from_file("font.ttf");
hb_face_t *face = hb_face_create(blob, 0);

// Extract dependency graph
hb_depend_t *depend = hb_depend_from_face(face);

// Query dependencies for a specific glyph
hb_codepoint_t gid = 42;  // Glyph to query
hb_codepoint_t index = 0;

hb_tag_t table_tag;
hb_codepoint_t dependent;
hb_tag_t layout_tag;
hb_codepoint_t ligature_set;

printf("Dependencies for glyph %u:\n", gid);
while (hb_depend_get_glyph_entry(depend, gid, index++,
                                  &table_tag, &dependent,
                                  &layout_tag, &ligature_set)) {
  printf("  -> glyph %u via %c%c%c%c\n",
         dependent, HB_UNTAG(table_tag));
}

// Clean up
hb_depend_destroy(depend);
hb_face_destroy(face);
hb_blob_destroy(blob);
```

## Iterating Dependencies

To programmatically access dependency information for a specific glyph:

```c
hb_codepoint_t gid = 42;  // Glyph ID to query
hb_codepoint_t index = 0; // Dependency entry index

hb_tag_t table_tag;
hb_codepoint_t dependent;
hb_tag_t layout_tag;
hb_codepoint_t ligature_set;

// Iterate through all dependencies for this glyph
while (hb_depend_get_glyph_entry(depend, gid, index,
                                  &table_tag, &dependent,
                                  &layout_tag, &ligature_set)) {
  // Process dependency information
  if (table_tag == HB_OT_TAG_GSUB) {
    // GSUB dependency: layout_tag contains feature tag
    printf("GID %u -> %u via GSUB feature '%c%c%c%c'\n",
           gid, dependent, HB_UNTAG(layout_tag));
  } else if (table_tag == HB_TAG('c','m','a','p')) {
    // cmap dependency: layout_tag contains UVS codepoint if applicable
    printf("GID %u -> %u via cmap\n", gid, dependent);
  } else {
    // Other dependencies (glyf, COLR, MATH)
    printf("GID %u -> %u via %c%c%c%c\n",
           gid, dependent, HB_UNTAG(table_tag));
  }

  index++;
}
```

## Finding Reachable Glyphs

To compute all glyphs reachable from a starting set (transitive closure):

```c
void find_reachable_glyphs(hb_depend_t *depend, hb_set_t *reachable)
{
  hb_set_t *to_process = hb_set_create();
  hb_set_union(to_process, reachable);

  // Process glyphs until none remain
  while (!hb_set_is_empty(to_process)) {
    hb_codepoint_t gid = hb_set_get_min(to_process);
    hb_set_del(to_process, gid);

    hb_codepoint_t index = 0;
    hb_tag_t table_tag;
    hb_codepoint_t dependent;
    hb_tag_t layout_tag;
    hb_codepoint_t ligature_set;

    while (hb_depend_get_glyph_entry(depend, gid, index++,
                                      &table_tag, &dependent,
                                      &layout_tag, &ligature_set)) {
      if (!hb_set_has(reachable, dependent)) {
        hb_set_add(reachable, dependent);
        hb_set_add(to_process, dependent);
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

**Note:** This simple algorithm doesn't handle ligature dependencies correctly.
Ligatures should only be added when all component glyphs in their ligature set
are present. For a production-quality implementation that correctly handles
ligatures and feature filtering to compute closures similar to HarfBuzz's subset
API, see `compute_depend_closure()` in `test/fuzzing/hb-depend-fuzzer.cc` and
`docs/depend-for-closure.md`.

## Working with Ligature Sets

When a dependency entry belongs to a ligature set, you can find all other members
of that set using `hb_depend_get_set_from_index()`:

```c
// After finding ligature_set != HB_CODEPOINT_INVALID from hb_depend_get_glyph_entry()...

// Get all glyphs in this ligature set
hb_set_t *ligature_glyphs = hb_set_create();
if (hb_depend_get_set_from_index(depend, ligature_set, ligature_glyphs)) {
  hb_codepoint_t lig_gid = HB_SET_VALUE_INVALID;

  printf("Members of ligature set %u:\n", ligature_set);
  while (hb_set_next(ligature_glyphs, &lig_gid)) {
    // Find the dependency entry index for this glyph in this ligature set
    hb_codepoint_t lig_index = 0;
    hb_tag_t lig_table_tag;
    hb_codepoint_t lig_dependent;
    hb_tag_t lig_layout_tag;
    hb_codepoint_t lig_ligature_set;

    while (hb_depend_get_glyph_entry(depend, lig_gid, lig_index++,
                                      &lig_table_tag, &lig_dependent,
                                      &lig_layout_tag, &lig_ligature_set)) {
      if (lig_ligature_set == ligature_set) {
        printf("  gid=%u, index=%u\n", lig_gid, lig_index - 1);
        break;
      }
    }
  }
}
hb_set_destroy(ligature_glyphs);
```

# API Details

## Dependency Entry Fields

Each dependency entry returned by `hb_depend_get_glyph_entry()` contains:

- **table_tag**: Source table (e.g., `HB_OT_TAG_GSUB`, `HB_TAG('c','m','a','p')`,
  `HB_TAG('g','l','y','f')`, `HB_TAG('C','O','L','R')`, `HB_TAG('M','A','T','H')`)
- **dependent**: The dependent glyph ID
- **layout_tag**:
  - For GSUB: the feature tag (e.g., `HB_TAG('l','i','g','a')`)
  - For cmap with UVS: the variation selector codepoint
  - Otherwise: `HB_CODEPOINT_INVALID`
- **ligature_set**: For ligatures, identifies which ligature set; otherwise
  `HB_CODEPOINT_INVALID`

## Compilation

The dependency API is optional and controlled by the `depend_api` build option.
**This is a highly experimental feature and disabled by default.**

```bash
meson setup build -Ddepend_api=true
meson compile -C build
```

In code, check for availability:

```c
#ifdef HB_DEPEND_API
  // Use dependency API
#endif
```

**Important:** This API is experimental and may change without notice. Do not
use in production applications without being prepared for breaking changes in
future HarfBuzz releases.

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

## Font Binning

Partition a font into smaller subsets where each bin contains glyphs reachable
from a specific set of input characters, enabling more efficient font delivery
for web applications.

## Testing

Verify that font modifications haven't inadvertently broken glyph references or
substitution chains.
