# Using Depend for Closure Computation

The depend API can be used to compute glyph closures - determining which glyphs
are reachable from a given set of input codepoints and active features. This
document explains how to compute closures using the depend graph and provides a
reference implementation.

## Computing Closure with Depend

To compute a closure using the depend graph:

1. **Map input codepoints to starting glyphs** using the cmap table
2. **Initialize the reachable set** with these starting glyphs
3. **Expand through dependencies** by following edges in the depend graph
4. **Filter by active features** to only follow edges for enabled GSUB features
5. **Handle ligatures correctly** - only add ligature outputs when all component
   glyphs are present
6. **Check context requirements** - only follow edges when positional requirements
   (backtrack/lookahead) are satisfied

### Context Set Filtering

Context and ChainContext GSUB rules have positional requirements encoded in the
context_set field. Each context_set specifies which backtrack and/or lookahead
glyphs must be present for a dependency edge to apply.

When context_set filtering is implemented, recent testing has not generated any
over-approximation cases - the depend-based closure matches the subset-based
closure in all tested scenarios.

**With context_set filtering:** Recent testing shows no over-approximation
**Without context_set filtering:** Conservative over-approximation (safe but may include extra glyphs)

See `docs/depend-api.md` section "Working with Context Sets" for details on the
context_set encoding and how to check context requirements.

### Reference Implementation

The test suite includes a production-quality implementation of depend-based
closure in `test/fuzzing/hb-depend-closure-parity.cc`. The `compute_depend_closure()`
function demonstrates proper handling of:

- Feature filtering (only following edges for active GSUB features)
- Ligature sets (only adding ligature outputs when all components present)
- Context sets (only following edges when positional requirements satisfied)
- Non-GSUB dependencies (cmap, glyf, CFF, COLR, MATH)

Key aspects of the implementation:

```c++
// Feature filtering - only follow GSUB edges for active features
if (table_tag == HB_OT_TAG_GSUB) {
  if (active_features && !hb_set_has(active_features, layout_tag))
    continue;  // Skip edge if feature not active
}

// Context filtering - only follow edges when requirements satisfied
if (context_set != HB_CODEPOINT_INVALID) {
  if (!check_context_satisfied(depend, context_set, glyphs))
    continue;  // Skip edge if context not satisfied
}

// Ligature handling - only add ligature when all components present
if (ligature_set != HB_CODEPOINT_INVALID) {
  hb_set_t *ligature_glyphs = hb_set_create();
  hb_depend_get_set_from_index(depend, ligature_set, ligature_glyphs);

  // Check if all component glyphs are in reachable set
  if (!hb_set_is_subset(ligature_glyphs, glyphs)) {
    hb_set_destroy(ligature_glyphs);
    continue;  // Skip ligature if not all components present
  }
  hb_set_destroy(ligature_glyphs);
}

// Add dependent glyph to closure
if (!hb_set_has(glyphs, dependent)) {
  hb_set_add(glyphs, dependent);
  hb_set_add(to_process, dependent);
}
```

## Optional Over-Approximation

You can choose to skip context_set checking for:

- **Simpler implementation** - Fewer steps, less complex code
- **Coverage analysis** - Finding all glyphs that participate in a feature, regardless
  of whether they're reachable for specific input
- **Conservative estimates** - Ensuring no glyphs are missed

Skipping context_set produces a safe over-approximation - all needed glyphs are
included, possibly with some additional glyphs that wouldn't be reached for the
specific input.

## Use Cases

### Font Subsetting

Determine which glyphs must be retained to properly render a specific set of
characters, accounting for all OpenType substitutions and compositions.

With context_set filtering, depend-based subsetting produces results that match
subset-based subsetting in recent testing.

### Coverage Analysis

Analyze which features or scripts require which glyphs. For this use case,
skipping context_set filtering may be desirable - it shows all glyphs that
participate in a feature, not just those reachable for specific input.

### Font Segmentation

Partition a font into smaller subsets where each segment contains glyphs reachable
from a specific set of input characters, enabling more efficient font delivery
for web applications.

### Testing

Verify that font modifications haven't inadvertently broken glyph references or
substitution chains. The depend graph provides a complete view of font structure
that can be compared before and after modifications.

## Implementation Notes

### Non-GSUB Dependencies

Dependencies from non-GSUB tables do not require context filtering:
- **Character mapping (cmap)**: Direct Unicode to glyph mappings
- **Composite glyphs (glyf, CFF)**: Structural component relationships
- **Color layers (COLR)**: Layer composition
- **Math variants (MATH)**: Size variant relationships

These dependencies should always be followed during closure computation.

### Feature Filtering

GSUB edges should be filtered by active features. The layout_tag field contains
the feature tag for GSUB dependencies. Only follow edges where the feature is
active in your shaping configuration.

### Ligature Filtering

Ligature dependencies should only be followed when ALL component glyphs in the
ligature_set are present in the current closure. This prevents adding ligature
outputs prematurely.

## Further Reading

For detailed API documentation:
- `docs/depend-api.md` - API usage guide with examples
- `docs/depend-implementation.md` - Implementation details and architecture

For reference implementation:
- `test/fuzzing/hb-depend-closure-parity.cc` - Production-quality closure computation
- `test/api/test-ot-depend.c` - Unit tests for depend API
