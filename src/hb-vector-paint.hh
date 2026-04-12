#ifndef HB_VECTOR_PAINT_HH
#define HB_VECTOR_PAINT_HH

#include "hb.hh"

#include "hb-vector.h"
#include "hb-geometry.hh"
#include "hb-machinery.hh"
#include "hb-map.hh"
#include "hb-vector-svg-utils.hh"
#include "hb-vector-svg.hh"

struct hb_vector_color_glyph_cache_key_t
{
  hb_codepoint_t glyph = HB_CODEPOINT_INVALID;
  unsigned palette = 0;
  hb_color_t foreground = 0;

  hb_vector_color_glyph_cache_key_t () = default;
  hb_vector_color_glyph_cache_key_t (hb_codepoint_t g, unsigned p, hb_color_t f)
    : glyph (g), palette (p), foreground (f) {}

  bool operator == (const hb_vector_color_glyph_cache_key_t &o) const
  {
    return glyph == o.glyph &&
           palette == o.palette &&
           foreground == o.foreground;
  }

  uint32_t hash () const
  {
    uint32_t h = hb_hash (glyph);
    h = h * 31u + hb_hash (palette);
    h = h * 31u + hb_hash (foreground);
    return h;
  }
};

struct hb_vector_paint_t
{
  hb_object_header_t header;

  hb_vector_format_t format = HB_VECTOR_FORMAT_INVALID;
  hb_transform_t<> transform = {1, 0, 0, 1, 0, 0};
  float x_scale_factor = 1.f;
  float y_scale_factor = 1.f;
  hb_vector_extents_t extents = {0, 0, 0, 0};
  bool has_extents = false;

  hb_color_t foreground = HB_COLOR (0, 0, 0, 255);
  int palette = 0;
  hb_hashmap_t<unsigned, hb_color_t> custom_palette_colors;
  unsigned precision = 2;
  bool flat = false;

  hb_vector_t<char> defs;
  hb_vector_t<char> path;
  hb_vector_t<hb_vector_t<char>> group_stack;
  uint64_t transform_group_open_mask = 0;
  unsigned transform_group_depth = 0;
  unsigned transform_group_overflow_depth = 0;

  unsigned clip_rect_counter = 0;
  unsigned gradient_counter = 0;
  unsigned color_glyph_counter = 0;
  unsigned color_glyph_depth = 0;
  hb_set_t *defined_outlines = nullptr;
  hb_set_t *defined_clips = nullptr;
  hb_set_t *active_color_glyphs = nullptr;
  hb_hashmap_t<hb_vector_color_glyph_cache_key_t, unsigned> defined_color_glyphs;
  hb_vector_t<hb_color_stop_t> color_stops_scratch;
  hb_vector_t<char> captured_scratch;
  hb_blob_t *recycled_blob = nullptr;

  hb_vector_t<char> &current_body () { return group_stack.tail (); }
};

/* Implemented in hb-vector-paint-pdf.cc */
HB_INTERNAL hb_paint_funcs_t * hb_vector_paint_pdf_funcs_get ();
HB_INTERNAL hb_blob_t * hb_vector_paint_render_pdf (hb_vector_paint_t *paint);

#endif /* HB_VECTOR_PAINT_HH */
