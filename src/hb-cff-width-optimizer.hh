/*
 * CFF Width Optimizer
 *
 * Determines optimal defaultWidthX and nominalWidthX values
 * to minimize CharString byte cost.
 *
 * Based on fontTools.cffLib.width
 */

#ifndef HB_CFF_WIDTH_OPTIMIZER_HH
#define HB_CFF_WIDTH_OPTIMIZER_HH

#include "hb.hh"

namespace CFF {

/* Calculate byte cost for encoding a width delta */
static inline unsigned
width_delta_cost (int delta)
{
  delta = abs (delta);
  if (delta <= 107) return 1;
  if (delta <= 1131) return 2;
  return 5;
}

/* Calculate total byte cost for encoding widths */
static unsigned
byte_cost (const hb_hashmap_t<unsigned, unsigned> &widths,
           unsigned default_width,
           unsigned nominal_width)
{
  unsigned cost = 0;
  for (auto kv : widths.iter ())
  {
    unsigned w = kv.first;
    unsigned freq = kv.second;

    if (w == default_width)
      continue;

    cost += freq * width_delta_cost ((int) w - (int) nominal_width);
  }
  return cost;
}

/* Optimize defaultWidthX and nominalWidthX from a list of widths
 * Brute force: O(n^2) but simple and correct */
static void
optimize_widths (const hb_vector_t<unsigned> &width_list,
                 unsigned &default_width,
                 unsigned &nominal_width)
{
  if (width_list.length == 0)
  {
    default_width = nominal_width = 0;
    return;
  }

  /* Build frequency map */
  hb_hashmap_t<unsigned, unsigned> widths;
  unsigned min_w = width_list[0];
  unsigned max_w = width_list[0];

  for (unsigned w : width_list)
  {
    widths.set (w, widths.get (w) + 1);
    min_w = hb_min (min_w, w);
    max_w = hb_max (max_w, w);
  }

  /* Brute force search */
  unsigned best_cost = (unsigned) -1;
  unsigned best_default = 0;
  unsigned best_nominal = 0;

  for (unsigned nominal = min_w; nominal <= max_w; nominal++)
  {
    for (unsigned def = min_w; def <= max_w; def++)
    {
      unsigned cost = byte_cost (widths, def, nominal);
      if (cost < best_cost)
      {
        best_cost = cost;
        best_default = def;
        best_nominal = nominal;
      }
    }
  }

  default_width = best_default;
  nominal_width = best_nominal;
}

} /* namespace CFF */

#endif /* HB_CFF_WIDTH_OPTIMIZER_HH */
