/*
 * Copyright © 2022 Behdad Esfahbod
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

#ifndef HB_PAINT_BOUNDED_HH
#define HB_PAINT_BOUNDED_HH

#include "hb.hh"
#include "hb-paint.h"

#include "hb-geometry.hh"


typedef struct  hb_paint_bounded_context_t hb_paint_bounded_context_t;

struct hb_paint_bounded_context_t
{
  void clear ()
  {
    clips.clear ();
    groups.clear ();

    clips.push (false);
    groups.push (true);
  }

  hb_paint_bounded_context_t ()
  {
    clear ();
  }

  bool is_bounded ()
  {
    return groups.tail();
  }

  void push_clip ()
  {
    clips.push (true);
  }

  void pop_clip ()
  {
    clips.pop ();
  }

  void push_group ()
  {
    groups.push (true);
  }

  void pop_group (hb_paint_composite_mode_t mode)
  {
    const bool src_bounded = groups.pop ();
    bool &backdrop_bounded = groups.tail ();

    // https://learn.microsoft.com/en-us/typography/opentype/spec/colr#format-32-paintcomposite
    switch ((int) mode)
    {
      case HB_PAINT_COMPOSITE_MODE_CLEAR:
	backdrop_bounded = true;
	break;
      case HB_PAINT_COMPOSITE_MODE_SRC:
      case HB_PAINT_COMPOSITE_MODE_SRC_OUT:
	backdrop_bounded = src_bounded;
	break;
      case HB_PAINT_COMPOSITE_MODE_DEST:
      case HB_PAINT_COMPOSITE_MODE_DEST_OUT:
	break;
      case HB_PAINT_COMPOSITE_MODE_SRC_IN:
      case HB_PAINT_COMPOSITE_MODE_DEST_IN:
	backdrop_bounded = backdrop_bounded && src_bounded;
	break;
      default:
	backdrop_bounded = backdrop_bounded || src_bounded;
	break;
     }
  }

  void paint ()
  {
    const bool &clip = clips.tail ();
    bool &group = groups.tail ();

    if (!clip)
      group = false;
  }

  protected:
  hb_vector_t<bool> clips; // true if clipped
  hb_vector_t<bool> groups; // true if bounded
};

HB_INTERNAL hb_paint_funcs_t *
hb_paint_bounded_get_funcs ();


#endif /* HB_PAINT_BOUNDED_HH */
