#ifndef HB_VECTOR_DRAW_HH
#define HB_VECTOR_DRAW_HH

#include "hb.hh"

#include "hb-vector.h"
#include "hb-geometry.hh"
#include "hb-machinery.hh"
#include "hb-vector-svg-utils.hh"
#include "hb-vector-svg.hh"

struct hb_vector_draw_t
{
  hb_object_header_t header;

  hb_vector_format_t format = HB_VECTOR_FORMAT_INVALID;
  hb_transform_t<> transform = {1, 0, 0, 1, 0, 0};
  float x_scale_factor = 1.f;
  float y_scale_factor = 1.f;
  hb_vector_extents_t extents = {0, 0, 0, 0};
  bool has_extents = false;
  unsigned precision = 2;
  bool flat = false;
  hb_vector_t<char> id_prefix;

  hb_vector_t<char> defs;
  hb_vector_t<char> body;
  hb_vector_t<char> path;
  hb_set_t *defined_glyphs = nullptr;
  hb_blob_t *recycled_blob = nullptr;

  void append_xy (float x, float y)
  {
    float tx, ty;
    hb_vector_transform_point (transform, x_scale_factor, y_scale_factor, x, y, &tx, &ty);
    hb_buf_append_num (&path, tx, precision);
    switch (format)
    {
      case HB_VECTOR_FORMAT_PDF: hb_buf_append_c (&path, ' '); break;
      case HB_VECTOR_FORMAT_SVG: hb_buf_append_c (&path, ','); break;
      case HB_VECTOR_FORMAT_INVALID: default: break;
    }
    hb_buf_append_num (&path, ty, precision);
  }
};

#endif /* HB_VECTOR_DRAW_HH */
