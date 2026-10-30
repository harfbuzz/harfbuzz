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
 * Google Author(s): Garret Rieger
 */

#include "hb.hh"
#include "hb-aat-layout-common.hh"

int
main ()
{
  /* Two lookup values must each occupy at least one byte. */
  for (unsigned size = 0; size <= 5; size++)
  {
    const unsigned char data[] = {
      0, 10, 0, (unsigned char) size, 0, 3, 0, 2,
      0, 1, 0, 2, 0, 3, 0, 4, 0, 5,
    };
    unsigned length = 8 + 2 * size;
    auto *lookup = reinterpret_cast<const AAT::Lookup<OT::HBUINT16> *> (data);
    hb_sanitize_context_t c ((const char *) data, (const char *) data + length);
    c.start_processing ();
    hb_always_assert (lookup->sanitize (&c) == (size >= 1 && size <= 4));
    if (size && size <= 4)
    {
      hb_sanitize_context_t truncated ((const char *) data, (const char *) data + length - 1);
      truncated.start_processing ();
      hb_always_assert (!lookup->sanitize (&truncated));
    }
  }
}
