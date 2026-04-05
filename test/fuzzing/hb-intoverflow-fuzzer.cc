/*
 * Targeted integer-overflow fuzzer for HarfBuzz.
 *
 * Exercises code paths identified during audit where integer arithmetic
 * on font-controlled values feeds into allocation sizes, loop bounds,
 * or array indices.  The existing shape/subset/raster/vector fuzzers
 * do not specifically stress extreme axis counts, class counts, region
 * counts, or coordinate magnitudes — this fuzzer fills that gap.
 *
 * Attack surface exercised:
 *   1. VarRegionList — large axisCount × regionCount products
 *   2. gvar          — large axisCount × sharedTupleCount
 *   3. PairPosFormat2 — large class1Count × class2Count × record_len
 *   4. Raster paint  — extreme coordinates / transforms
 *   5. VARC          — deep composite nesting with variation
 *   6. CFF2 blend    — many regions, wide region counts
 *   7. Subsetting     — ItemVariationStore with large counts
 */

#include "hb-fuzzer.hh"

#include <hb-ot.h>
#include <hb-subset.h>

/* ── Helpers ───────────────────────────────────────────────── */

/* Shape with specific scales to exercise fixed-point overflow paths. */
static void
shape_with_scale (hb_font_t *font, int x_scale, int y_scale)
{
  hb_font_set_scale (font, x_scale, y_scale);

  hb_buffer_t *buffer = hb_buffer_create ();
  hb_buffer_add_utf8 (buffer, "ABCDEFGHIJ", -1, 0, -1);
  hb_buffer_guess_segment_properties (buffer);
  hb_shape (font, buffer, nullptr, 0);
  hb_buffer_destroy (buffer);
}

/* Get extents for every glyph — exercises VARC, glyf composite,
 * CFF2 charstring paths. */
static void
exercise_glyph_extents (hb_font_t *font, hb_face_t *face, unsigned limit)
{
  unsigned count = hb_face_get_glyph_count (face);
  if (count > limit) count = limit;
  for (unsigned gid = 0; gid < count; gid++)
  {
    hb_glyph_extents_t ext;
    (void) hb_font_get_glyph_extents (font, gid, &ext);
  }
}

/* Draw every glyph — exercises VARC get_path, CFF2 charstring,
 * glyf recursive composites. */
static void
exercise_glyph_draw (hb_font_t *font, hb_face_t *face, unsigned limit)
{
  unsigned count = hb_face_get_glyph_count (face);
  if (count > limit) count = limit;

  hb_draw_funcs_t *dfuncs = hb_draw_funcs_create ();
  /* null callbacks — we just want the code path exercised */
  hb_draw_funcs_make_immutable (dfuncs);
  for (unsigned gid = 0; gid < count; gid++)
    hb_font_draw_glyph (font, gid, dfuncs, nullptr);
  hb_draw_funcs_destroy (dfuncs);
}

/* Paint every glyph — exercises COLRv1, SVG, sbix, CBDT paths. */
static void
exercise_glyph_paint (hb_font_t *font, hb_face_t *face, unsigned limit)
{
  unsigned count = hb_face_get_glyph_count (face);
  if (count > limit) count = limit;

  hb_paint_funcs_t *pfuncs = hb_paint_funcs_create ();
  hb_paint_funcs_make_immutable (pfuncs);
  for (unsigned gid = 0; gid < count; gid++)
    hb_font_paint_glyph (font, gid, pfuncs, nullptr, 0, HB_COLOR (0, 0, 0, 255));
  hb_paint_funcs_destroy (pfuncs);
}

/* Set variation axes to their extremes — maximizes region scalar
 * computations, delta blending, CFF2 blend operator. */
static void
set_axes_to_extremes (hb_font_t *font, hb_face_t *face,
                      const uint8_t *data, size_t size)
{
  unsigned axes_count = hb_ot_var_get_axis_count (face);
  if (!axes_count) return;

  hb_ot_var_axis_info_t axes[64];
  unsigned count = axes_count > 64 ? 64 : axes_count;
  hb_ot_var_get_axis_infos (face, 0, &count, axes);

  hb_variation_t vars[64];
  for (unsigned i = 0; i < count; i++)
  {
    vars[i].tag = axes[i].tag;
    /* Use fuzz input to pick between min/default/max */
    unsigned mode = (i < size) ? data[i] % 3 : 2;
    switch (mode)
    {
      case 0: vars[i].value = axes[i].min_value; break;
      case 1: vars[i].value = axes[i].default_value; break;
      case 2: vars[i].value = axes[i].max_value; break;
    }
  }
  hb_font_set_variations (font, vars, count);
}

/* Subset with instancing — exercises VarData serialization,
 * ItemVariationStore overflow paths, gvar delta compilation. */
static void
exercise_subset_instancing (hb_face_t *face,
                            const uint8_t *data, size_t size)
{
  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  if (!input) return;

  /* Keep all glyphs so we exercise maximum table sizes */
  hb_set_t *glyphs = hb_subset_input_glyph_set (input);
  unsigned glyph_count = hb_face_get_glyph_count (face);
  if (glyph_count > 256) glyph_count = 256;
  for (unsigned i = 0; i < glyph_count; i++)
    hb_set_add (glyphs, i);

  /* Pin all axes to default to trigger instancing code paths */
  hb_subset_input_pin_all_axes_to_default (input, face);

  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_RETAIN_GIDS);

  hb_face_t *subset = hb_subset_or_fail (face, input);
  if (subset)
  {
    /* Read entire output to trigger any deferred writes */
    hb_blob_t *blob = hb_face_reference_blob (subset);
    unsigned len = 0;
    const char *out = hb_blob_get_data (blob, &len);
    volatile unsigned checksum = 0;
    for (unsigned i = 0; i < len; i++)
      checksum += (unsigned char) out[i];
    (void) checksum;
    hb_blob_destroy (blob);
    hb_face_destroy (subset);
  }
  hb_subset_input_destroy (input);
}

#ifndef HB_NO_RASTER
#include <hb-raster.h>

/* Raster with extreme scales — exercises signed overflow in edge
 * slope computation, float-to-int casts, stride calculations. */
static void
exercise_raster_extreme (hb_font_t *font, hb_face_t *face,
                         int scale_x, int scale_y)
{
  hb_font_set_scale (font, scale_x, scale_y);

  hb_raster_paint_t *paint = hb_raster_paint_create_or_fail ();
  if (!paint) return;
  hb_raster_paint_set_foreground (paint, HB_COLOR (0, 0, 0, 255));

  unsigned count = hb_face_get_glyph_count (face);
  if (count > 8) count = 8;
  for (unsigned gid = 0; gid < count; gid++)
    hb_raster_paint_glyph (paint, font, gid, 0, 0, 0, HB_COLOR (0, 0, 0, 255));

  hb_raster_image_t *img = hb_raster_paint_render (paint);
  if (img)
  {
    hb_raster_extents_t ext;
    hb_raster_image_get_extents (img, &ext);
    volatile unsigned x = ext.width + ext.height;
    (void) x;
    hb_raster_paint_recycle_image (paint, img);
  }
  hb_raster_paint_destroy (paint);
}
#endif

/* Exercise COLRv1 paint at extreme scales — triggers
 * scale_glyph_extents overflow at hb-font.hh:205-206
 * and push_clip_rectangle signed overflow at COLR.hh:2842-2845.
 * Both are IN Chromium via hb_font_paint_glyph. */
static void
exercise_colr_paint_extreme (hb_font_t *font, hb_face_t *face, unsigned limit)
{
  unsigned count = hb_face_get_glyph_count (face);
  if (count > limit) count = limit;

  hb_paint_funcs_t *pfuncs = hb_paint_funcs_create ();
  hb_paint_funcs_make_immutable (pfuncs);

  /* Try multiple palettes including invalid indices */
  for (unsigned palette = 0; palette < 3; palette++)
    for (unsigned gid = 0; gid < count; gid++)
      hb_font_paint_glyph (font, gid, pfuncs, nullptr,
                            palette, HB_COLOR (0, 0, 0, 255));

  hb_paint_funcs_destroy (pfuncs);
}

/* Exercise OT layout with the font. Shape with many features
 * to stress GPOS/GSUB lookup chains and value record processing. */
static void
exercise_ot_layout (hb_font_t *font)
{
  /* Shape with specific features that exercise GPOS ValueRecords,
   * PairPos, MarkBasePos etc. */
  static const char *feature_strings[] = {
    "kern", "liga", "calt", "mark", "mkmk",
    "ccmp", "rlig", "rclt", "curs", "smcp",
  };

  for (unsigned f = 0; f < sizeof (feature_strings) / sizeof (feature_strings[0]); f++)
  {
    hb_feature_t feature;
    hb_feature_from_string (feature_strings[f], -1, &feature);
    feature.value = 1;

    hb_buffer_t *buffer = hb_buffer_create ();
    hb_buffer_add_utf8 (buffer, "ABCDEFGHIJ\xD8\xA7\xD9\x84\xD8\xB3\xD9\x84\xD8\xA7\xD9\x85", -1, 0, -1);
    hb_buffer_guess_segment_properties (buffer);
    hb_shape (font, buffer, &feature, 1);
    hb_buffer_destroy (buffer);
  }
}

/* Exercise hb_font_get_glyph_name / get_glyph_from_name for CFF1 */
static void
exercise_glyph_names (hb_font_t *font, hb_face_t *face, unsigned limit)
{
  unsigned count = hb_face_get_glyph_count (face);
  if (count > limit) count = limit;

  char name[64];
  for (unsigned gid = 0; gid < count; gid++)
  {
    hb_font_get_glyph_name (font, gid, name, sizeof (name));
    hb_codepoint_t glyph;
    hb_font_get_glyph_from_name (font, name, -1, &glyph);
  }
}

/* ── Main fuzzer entry ─────────────────────────────────────── */

extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  _fuzzing_skip_leading_comment (&data, &size);
  alloc_state = _fuzzing_alloc_state (data, size);

  if (size < 16) return 0;

  hb_blob_t *blob = hb_blob_create ((const char *) data, size,
                                     HB_MEMORY_MODE_READONLY, nullptr, nullptr);
  hb_face_t *face = hb_face_create (blob, 0);
  hb_font_t *font = hb_font_create (face);

  /* ── Phase 1: Normal scale — baseline coverage ─────────── */
  hb_font_set_scale (font, 1000, 1000);
  set_axes_to_extremes (font, face, data, size);

  exercise_glyph_extents (font, face, 64);
  exercise_glyph_draw (font, face, 32);
  exercise_glyph_paint (font, face, 32);
  exercise_glyph_names (font, face, 32);

  /* Shape with variation coordinates at extremes */
  shape_with_scale (font, 1000, 1000);
  exercise_ot_layout (font);

  /* ── Phase 2: Large scales — integer overflow in fixed-point ── */
  /* These target: scale_glyph_extents overflow (hb-font.hh:205-206),
   * COLR clip rectangle overflow (COLR.hh:2842-2845),
   * raster edge slope computation, float-to-int casts. */
  static const int extreme_scales[] = {
    1, -1,
    0x7FFF, -0x7FFF,       /* 32767 — near int16 max */
    0x7FFFFF, -0x7FFFFF,   /* ~8M — near fixed-point saturation */
    0x7FFFFFFF, -0x7FFFFFFF /* INT_MAX — maximum overflow pressure */
  };

  for (unsigned i = 0; i < sizeof (extreme_scales) / sizeof (extreme_scales[0]); i += 2)
  {
    int sx = extreme_scales[i];
    int sy = extreme_scales[i + 1];
    shape_with_scale (font, sx, sy);
    exercise_glyph_extents (font, face, 8);
    exercise_colr_paint_extreme (font, face, 8);
  }

#ifndef HB_NO_RASTER
  /* ── Phase 3: Raster with extreme scales ────────────────── */
  exercise_raster_extreme (font, face, 0x7FFF, 0x7FFF);
  exercise_raster_extreme (font, face, 0x7FFFFF, 0x7FFFFF);
#endif

  /* ── Phase 4: Subsetting with instancing ────────────────── */
  exercise_subset_instancing (face, data, size);

  hb_font_destroy (font);
  hb_face_destroy (face);
  hb_blob_destroy (blob);
  return 0;
}
