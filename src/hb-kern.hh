/*
 * Copyright © 2017  Google, Inc.
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
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_KERN_HH
#define HB_KERN_HH

#include "hb-open-type.hh"
#include "hb-aat-layout-common.hh"
#include "hb-ot-layout-gpos-table.hh"


namespace OT {


template <typename Driver>
struct hb_kern_machine_t
{
  hb_kern_machine_t (const Driver &driver_,
		     bool crossStream_ = false) :
		       driver (driver_),
		       crossStream (crossStream_) {}

  void kern (hb_font_t   *font,
	     hb_buffer_t *buffer,
	     hb_mask_t    kern_mask,
	     bool         scale = true) const
  {
    if (!buffer->message (font, "start kern"))
      return;

    buffer->unsafe_to_concat ();
    OT::hb_ot_apply_context_t c (1, font, buffer, hb_blob_get_empty ());
    c.set_lookup_mask (kern_mask);
    c.set_lookup_props (OT::LookupFlag::IgnoreMarks);
    auto &skippy_iter = c.iter_input;

    bool horizontal = HB_DIRECTION_IS_HORIZONTAL (buffer->props.direction);
    unsigned int count = buffer->len;
    hb_glyph_info_t *info = buffer->info;
    hb_glyph_position_t *pos = buffer->pos;
    for (unsigned int idx = 0; idx < count;)
    {
      if (!(info[idx].mask & kern_mask))
      {
	idx++;
	continue;
      }

      unsigned int i = idx;
      unsigned int j = i + 1;
      /* An enabled neighbor is an immediate match unless IgnoreMarks or
       * default-ignorable handling requires the skipping iterator. */
      if (unlikely (j == count || !(info[j].mask & kern_mask) ||
		    _hb_glyph_info_is_mark (&info[j]) ||
		    _hb_glyph_info_is_default_ignorable (&info[j])))
      {
	skippy_iter.reset_fast (i);
	unsigned unsafe_to;
	if (unlikely (!skippy_iter.next (&unsafe_to)))
	{
	  idx = unsafe_to;
	  continue;
	}
	j = skippy_iter.idx;
      }

      hb_position_t kern = driver.get_kerning (info[i].codepoint,
					       info[j].codepoint);


      if (likely (!kern))
	goto skip;

      if (crossStream && kern == -0x8000)
      {
	pos[j].attach_type() = OT::Layout::GPOS_impl::ATTACH_TYPE_NONE;
	pos[j].attach_chain() = 0;
	if (horizontal)
	  pos[j].y_offset = 0;
	else
	  pos[j].x_offset = 0;
      }
      else if (horizontal)
      {
	if (crossStream)
	{
	  /* Cross-stream kerning shifts perpendicular to the run, so scale
	   * it by the perpendicular (vertical) axis, not the advance axis. */
	  if (scale)
	    kern = font->em_scale_y (kern);
	  pos[j].y_offset = kern;
	  buffer->scratch_flags |= HB_BUFFER_SCRATCH_FLAG_HAS_GPOS_ATTACHMENT;
	}
	else
	{
	  if (scale)
	    kern = font->em_scale_x (kern);
	  hb_position_t kern1 = kern >> 1;
	  hb_position_t kern2 = kern - kern1;
	  pos[i].x_advance = hb_saturate_add (pos[i].x_advance, kern1);
	  pos[j].x_advance = hb_saturate_add (pos[j].x_advance, kern2);
	  pos[j].x_offset = hb_saturate_add (pos[j].x_offset, kern2);
	}
      }
      else
      {
	if (crossStream)
	{
	  /* Cross-stream kerning shifts perpendicular to the run, so scale
	   * it by the perpendicular (horizontal) axis, not the advance axis. */
	  if (scale)
	    kern = font->em_scale_x (kern);
	  pos[j].x_offset = kern;
	  buffer->scratch_flags |= HB_BUFFER_SCRATCH_FLAG_HAS_GPOS_ATTACHMENT;
	}
	else
	{
	  if (scale)
	    kern = font->em_scale_y (kern);
	  hb_position_t kern1 = kern >> 1;
	  hb_position_t kern2 = kern - kern1;
	  /* A positive value moves the glyphs apart.  Vertical advances
	   * are negative, so the value is subtracted. */
	  pos[i].y_advance = hb_saturate_sub (pos[i].y_advance, kern1);
	  pos[j].y_advance = hb_saturate_sub (pos[j].y_advance, kern2);
	  pos[j].y_offset = hb_saturate_sub (pos[j].y_offset, kern2);
	}
      }

      buffer->unsafe_to_break (i, j + 1);

    skip:
      idx = j;
    }

    (void) buffer->message (font, "end kern");
  }

  const Driver &driver;
  bool crossStream;
};


} /* namespace OT */


#endif /* HB_KERN_HH */
