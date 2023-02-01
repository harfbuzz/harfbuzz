/*
 * Copyright © 2023  Behdad Esfahbod
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

#ifndef HB_DRAW_EMBOLDEN_HH
#define HB_DRAW_EMBOLDEN_HH

#include "hb.hh"

#include "hb-draw.hh"


struct hb_contour_point_t
{
  enum class type_t
  {
    MOVE_TO,
    LINE_TO,
    QUADRATIC_TO,
    CUBIC_TO,
  };

  hb_contour_point_t (float x, float y, type_t type) :
    x (x), y (y), type (type) {}

  float x, y;
  type_t type;
};

using hb_contour_t = hb_vector_t<hb_contour_point_t>;

template <typename context_t>
struct hb_filter_outline_t
{
  void replay (hb_draw_funcs_t *pen, void *pen_data)
  {
    hb_draw_state_t st = HB_DRAW_STATE_DEFAULT;

    unsigned first = 0;
    for (unsigned contour : contours)
    {
      auto it = points.as_array ().sub_array (first, contour - first);
      while (it)
      {
	hb_contour_point_t p1 = *it++;
	switch (p1.type)
	{
	  case hb_contour_point_t::type_t::MOVE_TO:
	  {
	    pen->move_to (pen_data, st,
			     p1.x, p1.y);
	  }
	  break;
	  case hb_contour_point_t::type_t::LINE_TO:
	  {
	    pen->line_to (pen_data, st,
			     p1.x, p1.y);
	  }
	  break;
	  case hb_contour_point_t::type_t::QUADRATIC_TO:
	  {
	    hb_contour_point_t p2 = *it++;
	    pen->quadratic_to (pen_data, st,
				  p1.x, p1.y,
				  p2.x, p2.y);
	  }
	  break;
	  case hb_contour_point_t::type_t::CUBIC_TO:
	  {
	    hb_contour_point_t p2 = *it++;
	    hb_contour_point_t p3 = *it++;
	    pen->cubic_to (pen_data, st,
			      p1.x, p1.y,
			      p2.x, p2.y,
			      p3.x, p3.y);
	  }
	  break;
	}
      }
      pen->close_path (pen_data, st);
      first = contour;
    }
  }

  hb_vector_t<hb_contour_point_t> points;
  hb_vector_t<unsigned> contours;
};

template <typename context_t>
static void
hb_filter_outline_move_to (hb_draw_funcs_t *dfuncs HB_UNUSED,
			   void *data,
			   hb_draw_state_t *st,
			   float to_x, float to_y,
			   void *user_data HB_UNUSED)
{
  context_t *c = (context_t *) data;

  c->points.push (hb_contour_point_t {to_x, to_y, hb_contour_point_t::type_t::MOVE_TO});
}

template <typename context_t>
static void
hb_filter_outline_line_to (hb_draw_funcs_t *dfuncs HB_UNUSED,
			   void *data,
			   hb_draw_state_t *st,
			   float to_x, float to_y,
			   void *user_data HB_UNUSED)
{
  context_t *c = (context_t *) data;

  c->points.push (hb_contour_point_t {to_x, to_y, hb_contour_point_t::type_t::LINE_TO});
}

template <typename context_t>
static void
hb_filter_outline_quadratic_to (hb_draw_funcs_t *dfuncs HB_UNUSED,
				void *data,
				hb_draw_state_t *st,
				float control_x, float control_y,
				float to_x, float to_y,
				void *user_data HB_UNUSED)
{
  context_t *c = (context_t *) data;

  c->points.push (hb_contour_point_t {control_x, control_y, hb_contour_point_t::type_t::QUADRATIC_TO});
  c->points.push (hb_contour_point_t {to_x, to_y, hb_contour_point_t::type_t::QUADRATIC_TO});
}

template <typename context_t>
static void
hb_filter_outline_cubic_to (hb_draw_funcs_t *dfuncs HB_UNUSED,
			    void *data,
			    hb_draw_state_t *st,
			    float control1_x, float control1_y,
			    float control2_x, float control2_y,
			    float to_x, float to_y,
			    void *user_data HB_UNUSED)
{
  context_t *c = (context_t *) data;

  c->points.push (hb_contour_point_t {control1_x, control1_y, hb_contour_point_t::type_t::CUBIC_TO});
  c->points.push (hb_contour_point_t {control2_x, control2_y, hb_contour_point_t::type_t::CUBIC_TO});
  c->points.push (hb_contour_point_t {to_x, to_y, hb_contour_point_t::type_t::CUBIC_TO});
}

template <typename context_t>
static void
hb_filter_outline_close_path (hb_draw_funcs_t *dfuncs HB_UNUSED,
			      void *data,
			      hb_draw_state_t *st,
			      void *user_data HB_UNUSED)
{
  context_t *c = (context_t *) data;

  c->contours.push (c->points.length);
}


struct hb_draw_embolden_context_t : hb_filter_outline_t<hb_draw_embolden_context_t>
{
  void operator () (float x_strength, float y_strength)
  {
    if (!x_strength && !y_strength) return;
    if (!points) return;

    x_strength /= 2.f;
    y_strength /= 2.f;

    bool orientation_negative = false;

    struct vector_t
    {
      float normalize_len ()
      {
	float len = hypotf (x, y);
	if (len)
	{
	  x /= len;
	  y /= len;
	}
	return len;
      }

      float x, y;
    };

    signed first = 0;
    for (unsigned c = 0; c < contours.length; c++)
    {
      vector_t in, out, anchor, shift;
      float l_in, l_out, l_anchor = 0, l, q, d;

      l_in = 0;
      signed last = (int) contours[c] - 1;

      /* pacify compiler */
      in.x = in.y = anchor.x = anchor.y = 0;

      /* Counter j cycles though the points; counter i advances only  */
      /* when points are moved; anchor k marks the first moved point. */
      for ( signed i = last, j = first, k = -1;
	    j != i && i != k;
	    j = j < last ? j + 1 : first )
      {
	if ( j != k )
	{
	  out.x = points[j].x - points[i].x;
	  out.y = points[j].y - points[i].y;
	  l_out = out.normalize_len ();

	  if ( l_out == 0 )
	    continue;
	}
	else
	{
	  out   = anchor;
	  l_out = l_anchor;
	}

	if ( l_in != 0 )
	{
	  if ( k < 0 )
	  {
	    k        = i;
	    anchor   = in;
	    l_anchor = l_in;
	  }

	  d = in.x * out.x + in.y * out.y;

	  /* shift only if turn is less than ~160 degrees */
	  if ( d > -15.f/16.f )
	  {
	    d = d + 1.f;

	    /* shift components along lateral bisector in proper orientation */
	    shift.x = in.y + out.y;
	    shift.y = in.x + out.x;

	    if ( orientation_negative )
	      shift.x = -shift.x;
	    else
	      shift.y = -shift.y;

	    /* restrict shift magnitude to better handle collapsing segments */
	    q = out.x * in.y - out.y * in.x;
	    if ( orientation_negative )
	      q = -q;

	    l = hb_min (l_in, l_out);

	    /* non-strict inequalities avoid divide-by-zero when q == l == 0 */
	    if (x_strength * q <= l * d)
	      shift.x = shift.x * x_strength / d;
	    else
	      shift.x = shift.x * l / q;


	    if (y_strength * q <= l * d)
	      shift.y = shift.y * y_strength / d;
	    else
	      shift.y = shift.y * l / q;
	  }
	  else
	    shift.x = shift.y = 0;

	  for ( ;
		i != j;
		i = i < last ? i + 1 : first )
	  {
	    points[i].x += x_strength + shift.x;
	    points[i].y += y_strength + shift.y;
	  }
	}
	else
	  i = j;

	in   = out;
	l_in = l_out;
      }

      first = last + 1;
    }
  }

  hb_draw_funcs_t *pen;
  void *pen_data;
  float x_strength, y_strength;
};

static inline void free_static_draw_embolden_funcs ();

static struct hb_draw_embolden_funcs_lazy_loader_t : hb_draw_funcs_lazy_loader_t<hb_draw_embolden_funcs_lazy_loader_t>
{
  static hb_draw_funcs_t *create ()
  {
    hb_draw_funcs_t *funcs = hb_draw_funcs_create ();

    using t = hb_draw_embolden_context_t;

    hb_draw_funcs_set_move_to_func (funcs, hb_filter_outline_move_to<t>, nullptr, nullptr);
    hb_draw_funcs_set_line_to_func (funcs, hb_filter_outline_line_to<t>, nullptr, nullptr);
    hb_draw_funcs_set_quadratic_to_func (funcs, hb_filter_outline_quadratic_to<t>, nullptr, nullptr);
    hb_draw_funcs_set_cubic_to_func (funcs, hb_filter_outline_cubic_to<t>, nullptr, nullptr);
    hb_draw_funcs_set_close_path_func (funcs, hb_filter_outline_close_path<t>, nullptr, nullptr);

    hb_draw_funcs_make_immutable (funcs);

    hb_atexit (free_static_draw_embolden_funcs);

    return funcs;
  }
} static_draw_embolden_funcs;

static inline
void free_static_draw_embolden_funcs ()
{
  static_draw_embolden_funcs.free_instance ();
}

static hb_draw_funcs_t *
hb_draw_embolden_get_funcs ()
{
  return static_draw_embolden_funcs.get_unconst ();
}


#endif /* HB_DRAW_EMBOLDEN_HH */
