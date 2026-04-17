#ifndef HB_VECTOR_PAINT_HH
#define HB_VECTOR_PAINT_HH

#include "hb.hh"

#include "hb-vector.h"
#include "hb-geometry.hh"
#include "hb-machinery.hh"
#include "hb-map.hh"
#include "hb-vector-path.hh"
#include "hb-vector-svg-utils.hh"
#include "hb-vector-svg.hh"


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
  hb_color_t background = HB_COLOR (0, 0, 0, 0);
  int palette = 0;
  hb_hashmap_t<unsigned, hb_color_t> custom_palette_colors;
  hb_vector_t<char> id_prefix;

  hb_buf_t defs;
  hb_buf_t path;
  hb_vector_t<hb_vector_t<char>> group_stack;
  uint64_t transform_group_open_mask = 0;
  unsigned transform_group_depth = 0;
  unsigned transform_group_overflow_depth = 0;

  unsigned clip_rect_counter = 0;
  unsigned clip_path_counter = 0;
  hb_vector_path_sink_t clip_path_sink = {nullptr, 0, 1.f, 1.f};
  unsigned gradient_counter = 0;
  unsigned color_glyph_counter = 0;
  unsigned color_glyph_depth = 0;
  hb_set_t *defined_outlines = nullptr;
  hb_set_t *defined_clips = nullptr;
  hb_set_t *active_color_glyphs = nullptr;
  hb_hashmap_t<hb_codepoint_t, unsigned> defined_color_glyphs;
  hb_vector_t<hb_color_stop_t> color_stops_scratch;
  hb_vector_t<char> captured_scratch;
  hb_blob_t *recycled_blob = nullptr;

  hb_font_t *cached_font = nullptr;
  unsigned cached_serial = (unsigned) -1;

  void changed ()
  {
    if (defined_outlines)
      hb_set_clear (defined_outlines);
    if (defined_clips)
      hb_set_clear (defined_clips);
    if (active_color_glyphs)
      hb_set_clear (active_color_glyphs);
    defined_color_glyphs.reset ();
    defs.shrink (0);
    path.shrink (0);
    for (auto &g : group_stack)
      g.shrink (0);
    color_glyph_counter = 0;
    clip_rect_counter = 0;
    clip_path_counter = 0;
    gradient_counter = 0;
  }

  void check_font (hb_font_t *font)
  {
    unsigned serial = hb_font_get_serial (font);
    if (font != cached_font || serial != cached_serial)
    {
      changed ();
      hb_font_destroy (cached_font);
      cached_font = hb_font_reference (font);
      cached_serial = serial;
    }
  }

  hb_vector_t<char> &current_body () { return group_stack.tail (); }

  void set_precision (unsigned p)
  {
    p = hb_min (p, 12u);
    defs.precision = p;
    path.precision = p;
  }

  unsigned get_precision () const { return path.precision; }

  bool ensure_initialized ()
  {
    if (group_stack.length)
      return !group_stack.in_error () &&
             !group_stack.tail ().in_error ();
    if (unlikely (!group_stack.push_or_fail ()))
      return false;
    group_stack.tail ().alloc (4096);
    if (unlikely (group_stack.tail ().in_error ()))
    {
      group_stack.pop ();
      return false;
    }
    return true;
  }
};

/* Implemented in hb-vector-paint-svg.cc */
HB_INTERNAL hb_paint_funcs_t * hb_vector_paint_svg_funcs_get ();
HB_INTERNAL hb_blob_t * hb_vector_paint_render_svg (hb_vector_paint_t *paint);

/* Implemented in hb-vector-paint-pdf.cc */
HB_INTERNAL hb_paint_funcs_t * hb_vector_paint_pdf_funcs_get ();
HB_INTERNAL hb_blob_t * hb_vector_paint_render_pdf (hb_vector_paint_t *paint);
HB_INTERNAL void hb_vector_paint_pdf_free_resources (hb_vector_paint_t *paint);

#endif /* HB_VECTOR_PAINT_HH */
