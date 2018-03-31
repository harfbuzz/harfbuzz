/*
 * Copyright © 2018  Ebrahim Byagowi
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

#ifndef HB_AAT_LCAR_TABLE_HH
#define HB_AAT_LCAR_TABLE_HH

#include "hb-aat-layout-common-private.hh"

#define HB_AAT_TAG_lcar HB_TAG('l','c','a','r')


namespace AAT {

/*
 * lcar -- Ligature caret
 * https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6lcar.html
 */

struct lcar
{
  static const hb_tag_t tableTag = HB_AAT_TAG_lcar;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) && lookup.sanitize (c, this));
  }

  protected:
  FixedVersion<>version;	/* Version number of the ligature caret table */
  HBUINT16	format;		/* Format of the ligature caret table. */
  Lookup<OffsetTo<ArrayOf<HBINT16> > >
		lookup;		/* data Lookup table associating glyphs */
  public:
  DEFINE_SIZE_MIN (8);
};

} /* namespace AAT */


#endif /* HB_AAT_LCAR_TABLE_HH */
