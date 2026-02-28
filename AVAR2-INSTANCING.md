# avar2 Partial Instancing for HarfBuzz Subset Library

## Context

The HarfBuzz subset library currently rejects partial instancing of fonts with
avar2 tables (`"Partial-instancing avar2 table is not supported."`). This work
ports the "Offset Compensation" algorithm from fonttools (design doc:
`partial-instancing-avar2-design.md` in the fonttools repo) to HarfBuzz.

**Key architectural difference:** In fonttools, the subsetter and instancer are
separate. In HarfBuzz, they are fused -- each table's `subset()` handles both
glyph subsetting and variation instancing in a single pass. This means we can't
simply "skip" tables; we must route them to their existing non-instancing code
paths that still handle glyph subsetting.

**Scope:** Full implementation including self-contained axis detection and
variation culling.


## Algorithm Summary

When partial-instancing a font with avar2:

1. **Normalize using avar v1 only** -- compute intermediate-space axis locations
2. **Rebase avar2 IVS regions** via standard `rebaseTent` solver
3. **Detect self-contained pinned axes** -- whose final coordinate is constant
4. **Add offset compensation deltas** to the IVS so final coordinates remain in
   old coordinate space
5. **Skip variation instancing in other tables** for non-self-contained axes
6. **Run standard instancing for self-contained axes** at their old-space final
   coordinates
7. **Cull dead variations** in gvar/cvar/HVAR/VVAR/MVAR/GDEF whose regions fall
   outside reachable old-space final-coord ranges

See `partial-instancing-avar2-design.md` in the fonttools repo for the full
mathematical proof.


## Implementation Plan

### Chunk 1: Plan Members and Normalization

**Files:** `src/hb-subset-plan-member-list.hh`, `src/hb-subset-plan-var.cc`,
`src/hb-ot-var-avar-table.hh`

#### 1a. New plan members (`hb-subset-plan-member-list.hh`)

Add after `axes_triple_distances` (line 131):
```cpp
HB_SUBSET_PLAN_MEMBER (bool, has_avar2)
HB_SUBSET_PLAN_MEMBER (hb_hashmap_t E(<hb_tag_t, Triple>), old_intermediates)
```

- `has_avar2`: true when font has avar2 data AND partial instancing is requested
  (not all axes pinned).
- `old_intermediates`: maps axis tag -> Triple(a_i, d_i, b_i) -- old intermediate
  coords at new min/default/max. These are the coordinates needed for offset
  compensation.

#### 1b. avar v1-only mapping method (`hb-ot-var-avar-table.hh`)

Add `map_coords_v1_only()` to `struct avar`. Identical to `map_coords_2_14()`
but omits the avar v2 tail processing. Returns the avar-v1-mapped values in
floats (2.14 representation) for each axis:

```cpp
bool map_coords_v1_only (float *coords, unsigned int coords_length) const
{
    unsigned int count = hb_min (coords_length, axisCount);
    const SegmentMaps *map = &firstAxisSegmentMaps;
    for (unsigned int i = 0; i < count; i++)
    {
        int v = roundf (map->map_float (coords[i]) * 16384.f);
        coords[i] = v / 16384.f;
        map = &StructAfter<SegmentMaps> (*map);
    }
    return true;
}
```

#### 1c. Normalization changes (`hb-subset-plan-var.cc`, `normalize_axes_location()`)

Replace the avar2 early-exit `return false` (lines 254-258) with avar2-aware
normalization:

1. **When avar2 partial instancing is detected** (`has_v2_data() && !all_axes_pinned`):
   - Set `plan->has_avar2 = true`
   - Call `map_coords_v1_only()` (instead of `map_coords_2_14()`) for mins,
     defaults, and maxs arrays
   - Store the v1-only mapped values as both `plan->old_intermediates` (for
     offset compensation later) and `plan->axes_location` (they are the same --
     both are intermediate-space coords)
   - **Keep ALL axes in `axes_index_map`** (including pinned ones), so pinned
     axes remain in fvar as hidden. Re-populate `axes_index_map` to include all
     axes
   - Set `all_axes_pinned = false` to prevent table dropping

2. **Else:** use existing path (`map_coords_2_14()` with full avar v1+v2)

**Why v1-only:** `axes_location` must be in intermediate coordinate space
(post-fvar, post-avar-v1, pre-avar-v2) because:
- avar2 IVS regions are in intermediate space -- `rebaseTent` needs matching
  limits
- `SegmentMaps::subset()` uses `axes_location` and unmaps through avar v1 --
  correct with v1-only values
- Other tables must NOT use these values for instancing (see Chunk 5)


### Chunk 2: avar2 Subsetting (Core Algorithm)

**File:** `src/hb-ot-var-avar-table.hh`, `avar::subset()` and new helper

#### 2a. Version and segment maps

Modify `avar::subset()` to:
- Output version 2.0 when `plan->has_avar2` is true
- For avar2 pinned axes: serialize identity segment maps `{-1->-1, 0->0, 1->1}`
  (compile() needs entries for all fvar axes)
- For avar2 restricted axes: use existing `SegmentMaps::subset()` (works
  correctly with intermediate-space `axes_location`)

#### 2b. avar2 IVS instancing (`_subset_avar2()` helper)

Add helper method `_subset_avar2(hb_subset_context_t *c)`:

1. **Locate original avar2 data**: walk past all SegmentMaps to find
   `avarV2Tail`, resolve `varIdxMap` and `varStore`.

2. **Compute default deltas**: evaluate original VarStore at old-default
   intermediate coordinates (d_1, d_2, ..., d_n). For each varIdx,
   `default_delta = varStore.get_delta(varIdx, default_coords)`.

3. **Rebase IVS regions**: call `item_vars.create_from_item_varstore()` then
   `item_vars.instantiate_tuple_vars()` using `plan->axes_location`
   (intermediate-space limits). The standard instancing subtracts default deltas
   via `merge_tuple_variations()`.

4. **Detect self-contained pinned axes**: After IVS rebasing, for each pinned
   axis, check if any remaining TupleVariation has a non-zero delta at that
   axis's inner position. If no variation remains: axis is self-contained. Its
   final coord = `d_i + default_delta_i`. These axes can be removed from fvar
   and their contributions folded into gvar/HVAR/etc.

5. **Add offset compensation tuples**: For each fvar axis:
   - **Restricted or non-self-contained pinned axis** (in `user_axes_location`):
     - If `varIdx == NO_VARIATION_INDEX`: create new VarData via
       `item_vars.add_vardata(1)`, create/update VarIdxMap entry
     - Add bias tuple: empty region, delta = `d_int + round(defaultDelta)`
     - If not pinned, add neg tent: region `{tag: (-1,-1,0)}`, delta =
       `a_int + (1<<14) - d_int`
     - If not pinned, add pos tent: region `{tag: (0,+1,+1)}`, delta =
       `b_int - (1<<14) - d_int`
   - **Self-contained pinned axis**: skip offset compensation (will be folded)
   - **Free/private axis** (not in `user_axes_location`):
     - If `round(defaultDelta) != 0`: add bias tuple with `round(defaultDelta)`

6. **Remove self-contained axes from axis order** before building VarStore, so
   VarRegionList matches post-removal fvar axis count.

7. **Finalize**: call `item_vars.build_region_list()` +
   `item_vars.as_item_varstore(optimize=true, use_no_variation_idx=false)`.

8. **Serialize avarV2Tail**: serialize `DeltaSetIndexMap` (with updated varIdx
   entries after optimization via `item_vars.get_varidx_map()`) and
   `ItemVariationStore`. Handle Offset32To resolution relative to avar table start.

9. **Store self-contained axes info** in plan for use by other tables.


### Chunk 3: `item_variations_t` Extensions

**File:** `src/hb-ot-var-common.hh`

Add public methods to `item_variations_t`:

```cpp
// Like instantiate_tuple_vars but does NOT call build_region_list().
// Caller adds tuples between this call and build_region_list().
bool instantiate_tuple_vars_no_region_build (
    const hb_hashmap_t<hb_tag_t, Triple>& axes_location,
    const hb_hashmap_t<hb_tag_t, TripleDistances>& axes_triple_distances)

// Add a new VarData subtable. Returns outer index.
unsigned add_vardata (unsigned item_count)

// Get item count for a VarData subtable.
unsigned get_item_count (unsigned outer) const

// Add a tuple with a single non-zero delta at position inner.
void add_tuple (unsigned outer,
                hb_hashmap_t<hb_tag_t, Triple>&& axis_tuples,
                unsigned inner, int delta, unsigned item_count)
```

These methods allow the avar2 subsetting code to manipulate the IVS
representation between rebasing and finalization.


### Chunk 4: Skip Instancing in Other Tables

For avar2 partial instancing, variation tables must handle glyph subsetting but
skip variation instancing for non-self-contained axes. Route to existing
non-instancing code paths.

#### HVAR/VVAR (`src/hb-ot-var-hvar-table.hh`, `_subset()`)

Change: `if (c->plan->normalized_coords)` ->
`if (c->plan->normalized_coords && !c->plan->has_avar2)`

The `else` branch already handles glyph subsetting (serializes VarStore with
`inner_maps` filtering, no instancing).

#### GDEF (`src/OT/Layout/GDEF/GDEF.hh`, `subset()`)

Change: `else if (c->plan->normalized_coords)` ->
`else if (c->plan->normalized_coords && !c->plan->has_avar2)`

The `else` branch handles layout subsetting without instancing.

#### MVAR (`src/hb-ot-var-mvar-table.hh`)

MVAR has no glyph subsetting (global metrics). For avar2: passthrough the entire
table via `_hb_subset_table_passthrough`.

#### cvar (`src/hb-ot-var-cvar-table.hh` and `src/hb-subset.cc`)

No glyph subsetting (CVT values). For avar2: passthrough. Also skip cvt delta
application in `hb-subset.cc`.

#### CFF2 (`src/hb-subset-cff2.cc`)

Change: `pinned = (bool) plan->normalized_coords;` ->
`pinned = (bool) plan->normalized_coords && !plan->has_avar2;`

This skips blend instancing while preserving glyph subsetting.

#### gvar (`src/hb-ot-var-gvar-table.hh`)

Change: `if (c->plan->normalized_coords)` ->
`if (c->plan->normalized_coords && !c->plan->has_avar2)`

This routes to the non-instancing glyph subsetting path.


### Chunk 5: fvar Changes

**File:** `src/hb-ot-var-fvar-table.hh`

#### AxisRecord::subset

For avar2 pinned axes: set HIDDEN flag and `min=default=max=pinned_value`:
```cpp
if (c->plan->has_avar2 && axis_limit->is_point())
{
    out->minValue.set_float (axis_limit->middle);
    out->defaultValue.set_float (axis_limit->middle);
    out->maxValue.set_float (axis_limit->middle);
    out->flags = out->flags | AxisRecord::AXIS_FLAG_HIDDEN;
}
```

#### InstanceRecord::subset

For avar2: don't skip pinned axes when writing instance coordinates (they're
still in fvar):
```cpp
if (axis_limit->is_point () && !c->plan->has_avar2) continue;
```


### Chunk 6: Table Ordering

**File:** `src/hb-subset.cc`, `_dependencies_satisfied()`

Add avar as a dependency for variation tables when avar2 is present. The avar2
subsetting step computes self-contained axes info that other tables need.

```cpp
case HB_TAG('g','v','a','r'):
case HB_TAG('c','v','a','r'):
case HB_TAG('H','V','A','R'):
case HB_TAG('V','V','A','R'):
case HB_TAG('M','V','A','R'):
case HB_TAG('G','D','E','F'):
case HB_TAG('C','F','F','2'):
    return !plan->has_avar2 || !pending_subset_tags.has(HB_TAG('a','v','a','r'));
```


### Chunk 7: Self-Contained Axis Instancing

After avar subsetting detects self-contained axes, run standard variation
instancing for those axes at their old-space final coordinates.

**New plan members:**
```cpp
HB_SUBSET_PLAN_MEMBER (hb_hashmap_t E(<hb_tag_t, float>), self_contained_axes)
```

The avar2 subsetting step populates this. Other tables check it:
- If `has_avar2` is true but a table also sees self-contained axes, it can
  instance for those axes. The self-contained axes have constant final
  coordinates, so standard instancing (at that coordinate) is correct.
- The instancing path constructs a restricted `axes_location` containing only
  the self-contained axes (as pinned points).

This is the most complex chunk and can be deferred. The initial implementation
(Chunks 1-6) produces correct fonts without self-contained axis optimization --
all pinned axes are kept as hidden in fvar.


### Chunk 8: Variation Culling

After all tables are subsetted, cull dead variations in gvar/cvar/HVAR/VVAR/
MVAR/GDEF whose axis regions fall outside the reachable old-space final-coord
ranges.

For each axis:
- **NO_VARIATION_INDEX axes**: exact reachable range is `[a_i, b_i]` from
  `old_intermediates`.
- **IVS axes**: conservative bounds from `getExtremes` on the instanced avar2
  VarStore (including offset compensation).
- **Private axes** (originally hidden, not user-restricted): pinned at (0,0,0)
  since their intermediate is always 0.

A TupleVariation is dead if any axis in its region has no overlap with the
reachable range for that axis.

This is an optimization-only chunk and can be deferred. Fonts are correct
without it, just potentially slightly larger.


## Verification

1. Take an avar2 font (e.g., RobotoFlex-avar2.ttf), partial-instance with both
   fonttools and HarfBuzz, compare avar2 VarStore output
2. Render glyphs at various coordinates using the instanced font -- verify visual
   match with the original
3. Test full pinning (all axes) still works (existing path)
4. Test with NO_VARIATION_INDEX axes, fonts without explicit VarIdxMap, multiple
   restricted axes
5. Test self-contained axis detection and removal
6. Test variation culling produces smaller output without visual changes
