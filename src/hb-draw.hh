/*
 * Copyright © 2020  Ebrahim Byagowi
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
 */

#ifndef HB_DRAW_HH
#define HB_DRAW_HH

#include "hb.hh"

struct hb_draw_pen_t
{
  hb_object_header_t header;

  hb_draw_pen_move_to_func_t _move_to;
  hb_draw_pen_line_to_func_t _line_to;
  hb_draw_pen_quadratic_to_func_t _quadratic_to;
  hb_draw_pen_cubic_to_func_t _cubic_to;
  hb_draw_pen_close_path_func_t _close_path;

  void *user_data;
  hb_position_t curr_x;
  hb_position_t curr_y;

  void
  move_to (hb_position_t to_x, hb_position_t to_y)
  {
    _move_to (to_x, to_y, user_data);
    curr_x = to_x; curr_y = to_y;
  }

  void
  line_to (hb_position_t to_x, hb_position_t to_y)
  {
    _line_to (to_x, to_y, user_data);
    curr_x = to_x; curr_y = to_y;
  }

  void
  quadratic_to (hb_position_t control_x, hb_position_t control_y,
		hb_position_t to_x, hb_position_t to_y)
  {
    if (_quadratic_to)
      _quadratic_to (control_x, control_y, to_x, to_y, user_data);
    else
    {
      /* based on https://github.com/fonttools/fonttools/blob/a37dab3/Lib/fontTools/pens/basePen.py#L218 */
      hb_position_t mid1_x = roundf ((float) curr_x + 0.6666666667f * (control_x - curr_x));
      hb_position_t mid1_y = roundf ((float) curr_y + 0.6666666667f * (control_y - curr_y));
      hb_position_t mid2_x = roundf ((float) to_x + 0.6666666667f * (control_x - to_x));
      hb_position_t mid2_y = roundf ((float) to_y + 0.6666666667f * (control_y - to_y));
      _cubic_to (mid1_x, mid1_y, mid2_x, mid2_y, to_x, to_y, user_data);
    }
    curr_x = to_x; curr_y = to_y;
  }

  void
  cubic_to (hb_position_t control1_x, hb_position_t control1_y,
	    hb_position_t control2_x, hb_position_t control2_y,
	    hb_position_t to_x, hb_position_t to_y)
  {
    _cubic_to (control1_x, control1_y, control2_x, control2_y, to_x, to_y, user_data);
    curr_x = to_x; curr_y = to_y;
  }

  void close_path () { _close_path (user_data); }
};


#endif /* HB_DRAW_HH */
