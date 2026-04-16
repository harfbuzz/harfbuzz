/*
 * Copyright © 2026  Behdad Esfahbod
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Author(s): Behdad Esfahbod
 */

#include "hb.hh"

#include "hb-geometry.hh"
#include "hb-machinery.hh"
#include "hb-vector-path.hh"
#include "hb-vector-svg-utils.hh"
#include "hb-vector-svg.hh"

static inline void
hb_vector_path_append_xy (hb_vector_path_sink_t *s, float x, float y)
{
  hb_buf_append_num (s->path, x, s->precision);
  hb_buf_append_c   (s->path,
                     s->format == HB_VECTOR_FORMAT_PDF ? ' ' : ',');
  hb_buf_append_num (s->path, y, s->precision);
}

static void
hb_vector_path_move_to (hb_draw_funcs_t *,
                        void *draw_data,
                        hb_draw_state_t *,
                        float to_x, float to_y,
                        void *)
{
  auto *s = (hb_vector_path_sink_t *) draw_data;
  switch (s->format)
  {
    case HB_VECTOR_FORMAT_PDF:
      hb_vector_path_append_xy (s, to_x, to_y);
      hb_buf_append_str (s->path, " m\n");
      break;
    case HB_VECTOR_FORMAT_SVG:
      hb_buf_append_c (s->path, 'M');
      hb_vector_path_append_xy (s, to_x, to_y);
      break;
    case HB_VECTOR_FORMAT_INVALID: default: break;
  }
}

static void
hb_vector_path_line_to (hb_draw_funcs_t *,
                        void *draw_data,
                        hb_draw_state_t *,
                        float to_x, float to_y,
                        void *)
{
  auto *s = (hb_vector_path_sink_t *) draw_data;
  switch (s->format)
  {
    case HB_VECTOR_FORMAT_PDF:
      hb_vector_path_append_xy (s, to_x, to_y);
      hb_buf_append_str (s->path, " l\n");
      break;
    case HB_VECTOR_FORMAT_SVG:
      hb_buf_append_c (s->path, 'L');
      hb_vector_path_append_xy (s, to_x, to_y);
      break;
    case HB_VECTOR_FORMAT_INVALID: default: break;
  }
}

static void
hb_vector_path_quadratic_to (hb_draw_funcs_t *,
                             void *draw_data,
                             hb_draw_state_t *st,
                             float cx, float cy,
                             float to_x, float to_y,
                             void *)
{
  auto *s = (hb_vector_path_sink_t *) draw_data;
  switch (s->format)
  {
    case HB_VECTOR_FORMAT_PDF:
    {
      /* PDF has no quadratic operator; promote to cubic using
       * the current pen position from draw-state. */
      float sx = st->current_x, sy = st->current_y;
      float c1x = sx + 2.f / 3.f * (cx - sx);
      float c1y = sy + 2.f / 3.f * (cy - sy);
      float c2x = to_x + 2.f / 3.f * (cx - to_x);
      float c2y = to_y + 2.f / 3.f * (cy - to_y);
      hb_vector_path_append_xy (s, c1x, c1y);
      hb_buf_append_c (s->path, ' ');
      hb_vector_path_append_xy (s, c2x, c2y);
      hb_buf_append_c (s->path, ' ');
      hb_vector_path_append_xy (s, to_x, to_y);
      hb_buf_append_str (s->path, " c\n");
      break;
    }
    case HB_VECTOR_FORMAT_SVG:
      hb_buf_append_c (s->path, 'Q');
      hb_vector_path_append_xy (s, cx, cy);
      hb_buf_append_c (s->path, ' ');
      hb_vector_path_append_xy (s, to_x, to_y);
      break;
    case HB_VECTOR_FORMAT_INVALID: default: break;
  }
}

static void
hb_vector_path_cubic_to (hb_draw_funcs_t *,
                         void *draw_data,
                         hb_draw_state_t *,
                         float c1x, float c1y,
                         float c2x, float c2y,
                         float to_x, float to_y,
                         void *)
{
  auto *s = (hb_vector_path_sink_t *) draw_data;
  switch (s->format)
  {
    case HB_VECTOR_FORMAT_PDF:
      hb_vector_path_append_xy (s, c1x, c1y);
      hb_buf_append_c (s->path, ' ');
      hb_vector_path_append_xy (s, c2x, c2y);
      hb_buf_append_c (s->path, ' ');
      hb_vector_path_append_xy (s, to_x, to_y);
      hb_buf_append_str (s->path, " c\n");
      break;
    case HB_VECTOR_FORMAT_SVG:
      hb_buf_append_c (s->path, 'C');
      hb_vector_path_append_xy (s, c1x, c1y);
      hb_buf_append_c (s->path, ' ');
      hb_vector_path_append_xy (s, c2x, c2y);
      hb_buf_append_c (s->path, ' ');
      hb_vector_path_append_xy (s, to_x, to_y);
      break;
    case HB_VECTOR_FORMAT_INVALID: default: break;
  }
}

static void
hb_vector_path_close_path (hb_draw_funcs_t *,
                           void *draw_data,
                           hb_draw_state_t *,
                           void *)
{
  auto *s = (hb_vector_path_sink_t *) draw_data;
  switch (s->format)
  {
    case HB_VECTOR_FORMAT_PDF:
      hb_buf_append_str (s->path, "h\n");
      break;
    case HB_VECTOR_FORMAT_SVG:
      hb_buf_append_c (s->path, 'Z');
      break;
    case HB_VECTOR_FORMAT_INVALID: default: break;
  }
}

static inline void
free_static_vector_path_draw_funcs ();

static struct hb_vector_path_draw_funcs_lazy_loader_t
  : hb_draw_funcs_lazy_loader_t<hb_vector_path_draw_funcs_lazy_loader_t>
{
  static hb_draw_funcs_t *create ()
  {
    hb_draw_funcs_t *funcs = hb_draw_funcs_create ();
    hb_draw_funcs_set_move_to_func (funcs, (hb_draw_move_to_func_t) hb_vector_path_move_to, nullptr, nullptr);
    hb_draw_funcs_set_line_to_func (funcs, (hb_draw_line_to_func_t) hb_vector_path_line_to, nullptr, nullptr);
    hb_draw_funcs_set_quadratic_to_func (funcs, (hb_draw_quadratic_to_func_t) hb_vector_path_quadratic_to, nullptr, nullptr);
    hb_draw_funcs_set_cubic_to_func (funcs, (hb_draw_cubic_to_func_t) hb_vector_path_cubic_to, nullptr, nullptr);
    hb_draw_funcs_set_close_path_func (funcs, (hb_draw_close_path_func_t) hb_vector_path_close_path, nullptr, nullptr);
    hb_draw_funcs_make_immutable (funcs);
    hb_atexit (free_static_vector_path_draw_funcs);
    return funcs;
  }
} static_vector_path_draw_funcs;

static inline void
free_static_vector_path_draw_funcs ()
{
  static_vector_path_draw_funcs.free_instance ();
}

hb_draw_funcs_t *
hb_vector_path_draw_funcs_get (void)
{
  return static_vector_path_draw_funcs.get_unconst ();
}
