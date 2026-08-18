# HarfType review notes

This is a maintainer working document for material that still needs review.
It is tracked alongside the HarfType sources but is not linked into the public
documentation.

## Baseline corrections

- Treat ISO/IEC 14496-22:2026 as baseline.
- `VARC`, `avar` version 2, extended condition formats, and standardized
  24-bit structures are Open Font Format features, not HarfType extensions.
- Apple also implements `avar` version 2, although its format is not publicly
  documented by Apple.
- Keep specification status separate from implementation maturity. Some ISO
  structures are still guarded by HarfBuzz experimental-build options.

## Proposed table categories

- Required tables
- Glyph outlines and components
- Character mapping
- Metrics
- Layout
  - OpenType Layout
  - Apple Advanced Typography Layout
- Variations
- Color glyphs
- Compatibility and error recovery
- Experimental extensions

## Possible HarfType consumption semantics

- `GSUB` MultipleSubst sequences with zero substitutes delete the input glyph.
- Reserved `GPOS` ValueFormat bits do not add ValueRecord fields.
- Microsoft symbol `cmap` lookup retries U+0000..U+00FF at U+F000..U+F0FF.
- AAT `trak` uses 12pt when point size is unavailable.
- AAT `kerx` applies cross-stream kerning in vertical text and implements the
  reset flag described by Apple's example but not its prose.

## Error recovery, not authoring permission

- `avar` segment maps missing required anchors or containing duplicate input
  coordinates.
- `morx` data extending beyond a subtable's claimed length.
- Unchecked SFNT checksums and search-optimization fields.
- Invalid `head.unitsPerEm` fallback.
- The Cambria Math constant workaround.

## Scope decisions

- Decide whether optional delegated formats such as Graphite are outside
  HarfType.
- Keep the experimental `Wasm` shaping table outside stable HarfType until it
  graduates.
- Distinguish conforming HarfType data, required consumption behavior, and
  best-effort error recovery.
