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

#ifndef HB_OT_COLOR_SBIX_TABLE_HH
#define HB_OT_COLOR_SBIX_TABLE_HH

#include "hb-open-type-private.hh"

#define HB_OT_TAG_SBIX HB_TAG('s','b','i','x')

namespace OT {


struct SBIXGlyph
{
  friend struct sbix;

  protected:
  HBINT16 originOffsetX;
  HBINT16 originOffsetY;
  unsigned char tag[4];
  HBUINT8 data[VAR];
  public:
  DEFINE_SIZE_STATIC (9);
};

struct ImageTable
{
  friend struct sbix;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
      c->check_array (imageOffsetsZ, sizeof (HBUINT32), c->num_glyphs) &&
      c->check_range (this, imageOffsetsZ[c->num_glyphs]));
  }

  protected:
  HBUINT16	ppem;
  HBUINT16	resolution;
  LOffsetTo<SBIXGlyph>	imageOffsetsZ[VAR]; // VAR=maxp.numGlyphs + 1
  public:
  DEFINE_SIZE_STATIC (8);
};

/*
 * sbix -- Standard Bitmap Graphics Table
 */
// It should be called with something like this so it can have
// access to num_glyph while sanitizing.
//
//   static inline const OT::sbix*
//   _get_sbix (hb_face_t *face)
//   {
//     OT::Sanitizer<OT::sbix> sanitizer;
//     sanitizer.set_num_glyphs (face->get_num_glyphs ());
//     hb_blob_t *sbix_blob = sanitizer.sanitize (face->reference_table (HB_OT_TAG_SBIX));
//     return OT::Sanitizer<OT::sbix>::lock_instance (sbix_blob);
//   }
//
struct sbix
{
  static const hb_tag_t tableTag = HB_OT_TAG_SBIX;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (!(c->check_struct (this) && imageTables.sanitize (c, this)))
      return_trace (false);

    for (unsigned int i = 0; i < imageTables.len; ++i)
      if (!(imageTables[i].sanitize (c, this)))
        return_trace (false);

    // dump (c->num_glyphs, 8);

    return_trace (true);
  }

  // inline void dump (unsigned int num_glyphs, unsigned int group) const
  // {
  //   const ImageTable &imageTable = imageTables[group](this);
  //   for (unsigned int i = 0; i < num_glyphs; ++i)
  //     if (imageTable.imageOffsetsZ[i + 1] - imageTable.imageOffsetsZ[i] > 0)
  //     {
  //       const SBIXGlyph &sbixGlyph = imageTable.imageOffsetsZ[i]((const void *) &imageTable);
  //       char outName[255];
  //       sprintf (outName, "out/%d-%d.png", group, i);
  //       FILE *f = fopen (outName, "wb");
  //       fwrite (sbixGlyph.data, 1,
  //         imageTable.imageOffsetsZ[i + 1] - imageTable.imageOffsetsZ[i] - 8, f);
  //       fclose (f);
  //     }
  // }

  protected:
  HBUINT16 version;
  HBUINT16 flags;
  ArrayOf<LOffsetTo<ImageTable>, HBUINT32> imageTables;
  public:
  DEFINE_SIZE_ARRAY (8, imageTables);
};

} /* namespace OT */

#endif /* HB_OT_COLOR_SBIX_TABLE_HH */
