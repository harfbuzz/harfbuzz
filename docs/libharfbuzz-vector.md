# libharfbuzz-vector Design and API

## Goals

- Add a new optional library: `libharfbuzz-vector`.
- Start with SVG generation.
- Reuse the existing conversion logic currently in `util/hb-draw` (especially `util/draw-output.hh` and `util/draw-output-color.hh`), moving implementation into `src/`.
- Keep API shape similar to `libharfbuzz-raster` object lifecycle and callback flow.

## Non-goals (initial version)

- No text layout API in this library.
- No logical/ink bbox policy in the library API.
- No PDF backend yet (but API should allow adding one later).

## High-level architecture

- Public API is format-agnostic: `hb_vector_*`.
- Backends are internal and format-specific (SVG now, PDF later).
- Dispatch is internal via function tables, not C++ virtual methods.

This follows HarfBuzz style and keeps ABI explicit and stable.

## Naming and layering

- Library name: `harfbuzz-vector`
- Public header: `src/hb-vector.h`
- Core dispatch/docs: `src/hb-vector.cc`
- SVG backend: `src/hb-vector-svg.cc`
- Optional split:
  - `src/hb-vector-draw.cc` for outline draw callbacks
  - `src/hb-vector-paint.cc` for COLR paint callbacks

Note: this coexists with internal template container header `src/hb-vector.hh`.

## Core API model

Two opaque context objects:

- `hb_vector_draw_t`: consume `hb_draw_funcs_t` callbacks, render vector output.
- `hb_vector_paint_t`: consume `hb_paint_funcs_t` callbacks, render vector output.

Both follow HB object conventions:

- `create_or_fail`, `reference`, `destroy`
- `set_user_data`, `get_user_data`
- mutable configuration via setters
- `render`, `reset`

## Extents policy

Extents are required before `render()`.

- Caller sets extents explicitly with `set_extents(...)`.
- `set_glyph_extents(...)` is a convenience helper for single-glyph cases and sets extents based on current transform/scale.
- If extents are not set, `render()` returns `NULL`.

The library does not decide logical-vs-ink bounds.

## Output format policy

Public API includes a format enum for backend selection:

- `HB_VECTOR_FORMAT_SVG` (initial and default)
- Future: `HB_VECTOR_FORMAT_PDF`, etc.

Public API stays generic; backend-specific options are exposed as extension setters.

## Backend-specific options

Do not put SVG-only options in generic setters.

SVG-specific API:

- `hb_vector_svg_set_flat(...)`
- `hb_vector_svg_set_precision(...)`

Future PDF-specific options should be `hb_vector_pdf_set_*`.

## Draft API surface (proposed)

```c
typedef enum {
  HB_VECTOR_FORMAT_SVG = 0,
} hb_vector_format_t;

typedef struct hb_vector_extents_t {
  float x, y, width, height;
} hb_vector_extents_t;

typedef struct hb_vector_draw_t hb_vector_draw_t;
typedef struct hb_vector_paint_t hb_vector_paint_t;

/* draw */
hb_vector_draw_t *hb_vector_draw_create_or_fail (void);
hb_vector_draw_t *hb_vector_draw_reference (hb_vector_draw_t *draw);
void hb_vector_draw_destroy (hb_vector_draw_t *draw);
hb_bool_t hb_vector_draw_set_user_data (hb_vector_draw_t*, hb_user_data_key_t*, void*, hb_destroy_func_t, hb_bool_t);
void *hb_vector_draw_get_user_data (hb_vector_draw_t*, hb_user_data_key_t*);
void hb_vector_draw_set_format (hb_vector_draw_t*, hb_vector_format_t format);
void hb_vector_draw_set_transform (hb_vector_draw_t*, float xx, float yx, float xy, float yy, float dx, float dy);
void hb_vector_draw_set_scale_factor (hb_vector_draw_t*, float x_scale_factor, float y_scale_factor);
void hb_vector_draw_set_extents (hb_vector_draw_t*, const hb_vector_extents_t *extents);
hb_bool_t hb_vector_draw_set_glyph_extents (hb_vector_draw_t*, const hb_glyph_extents_t *glyph_extents);
hb_draw_funcs_t *hb_vector_draw_get_funcs (void);
hb_blob_t *hb_vector_draw_render (hb_vector_draw_t *draw);
void hb_vector_draw_reset (hb_vector_draw_t *draw);

/* paint */
hb_vector_paint_t *hb_vector_paint_create_or_fail (void);
hb_vector_paint_t *hb_vector_paint_reference (hb_vector_paint_t *paint);
void hb_vector_paint_destroy (hb_vector_paint_t *paint);
hb_bool_t hb_vector_paint_set_user_data (hb_vector_paint_t*, hb_user_data_key_t*, void*, hb_destroy_func_t, hb_bool_t);
void *hb_vector_paint_get_user_data (hb_vector_paint_t*, hb_user_data_key_t*);
void hb_vector_paint_set_format (hb_vector_paint_t*, hb_vector_format_t format);
void hb_vector_paint_set_transform (hb_vector_paint_t*, float xx, float yx, float xy, float yy, float dx, float dy);
void hb_vector_paint_set_scale_factor (hb_vector_paint_t*, float x_scale_factor, float y_scale_factor);
void hb_vector_paint_set_extents (hb_vector_paint_t*, const hb_vector_extents_t *extents);
hb_bool_t hb_vector_paint_set_glyph_extents (hb_vector_paint_t*, const hb_glyph_extents_t *glyph_extents);
void hb_vector_paint_set_foreground (hb_vector_paint_t*, hb_color_t foreground);
void hb_vector_paint_set_palette (hb_vector_paint_t*, int palette);
hb_paint_funcs_t *hb_vector_paint_get_funcs (void);
hb_blob_t *hb_vector_paint_render (hb_vector_paint_t *paint);
void hb_vector_paint_reset (hb_vector_paint_t *paint);

/* SVG-specific options */
void hb_vector_svg_set_flat (hb_vector_draw_t *draw, hb_bool_t flat);
void hb_vector_svg_set_precision (hb_vector_draw_t *draw, unsigned precision);
void hb_vector_svg_paint_set_flat (hb_vector_paint_t *paint, hb_bool_t flat);
void hb_vector_svg_paint_set_precision (hb_vector_paint_t *paint, unsigned precision);
```

## CLI migration

- `util/hb-draw` will be renamed to `hb-vector`.
- Default output format is SVG.
- CLI retains layout-oriented policies (line placement, bbox mode decisions, etc.).
- Library API remains low-level conversion-only.

## Build integration plan

Meson:

- Add `hb_vector_sources`, `hb_vector_headers`.
- Build/install `libharfbuzz-vector`.
- Generate `harfbuzz-vector.def`.
- Generate `harfbuzz-vector.pc`.
- Add dependency override: `meson.override_dependency('harfbuzz-vector', ...)`.

CMake:

- Not currently wiring `harfbuzz-raster`; vector integration may start in Meson first.
- If desired, add CMake target in a follow-up for parity.

## Expected implementation extraction

From utility code to `src/`:

- Path building / numeric formatting
- Reuse-vs-flat SVG emission
- Paint graph to SVG conversion:
  - clip, transform, group/composite
  - color/image paint
  - linear/radial/sweep gradients

## Behavioral contract summary

- API is explicit and deterministic.
- Caller controls extents.
- Missing extents => `render()` returns `NULL`.
- Output is owned as `hb_blob_t*`.
- Generic API remains backend-neutral; format-specific options stay format-specific.
