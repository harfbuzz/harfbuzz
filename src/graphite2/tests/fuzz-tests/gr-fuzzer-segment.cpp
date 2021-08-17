/*  GRAPHITE2 LICENSING

    Copyright 2018, SIL International
    All rights reserved.

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should also have received a copy of the GNU Lesser General Public
    License along with this library in the file named "LICENSE".
    If not, write to the Free Software Foundation, 51 Franklin Street,
    Suite 500, Boston, MA 02110-1335, USA or visit their web page on the
    internet at http://www.fsf.org/licenses/lgpl.html.

Alternatively, the contents of this file may be used under the terms of the
Mozilla Public License (http://mozilla.org/MPL) or the GNU General Public
License, as published by the Free Software Foundation, either version 2
of the License or (at your option) any later version.
*/
#include <graphite2/Segment.h>

#include "graphite-fuzzer.hpp"

#pragma pack(push,1)
struct segment_parameters : public common_parameters
{
  uint8_t   direction = 0;
  uint8_t   test_text[128] = "Hello World!စက္ခုန္ဒ";
};
#pragma pack(pop)


// This fuzzer target tests Segment API, and by necesity parts of the Font API
// This includes the entire gr_cinfo_* and gr_slot_* family of functions

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  test_case<segment_parameters> test(data, size);
  auto face = gr_make_face_with_ops(&test.ttf, &test.ttf, gr_face_default);
  auto font = gr_make_font(12, face);

  const size_t n_chars = gr_count_unicode_characters(gr_encform(test.params.encoding),
                            test.params.test_text,
                            test.params.test_text + sizeof test.params.test_text,
                            nullptr);

  auto seg = gr_make_seg(font, face, test.params.script_tag, nullptr,
                gr_encform(test.params.encoding),
                test.params.test_text, n_chars,
                test.params.direction);
  if (seg)
  {
    // Test gr_seg_* family
    gr_seg_advance_X(seg);
    gr_seg_advance_Y(seg);

    // Test gr_char_info API here.
    const auto n_cinfos = gr_seg_n_cinfo(seg);
    for (auto i = 0u; i < n_cinfos; ++i)
    {
      const auto ci = gr_seg_cinfo(seg, i);
      gr_cinfo_unicode_char(ci);
      gr_cinfo_base(ci);
      gr_cinfo_break_weight(ci);
      gr_cinfo_after(ci);
      gr_cinfo_before(ci);
    }

    const auto n_slots = gr_seg_n_slots(seg);
    for (auto s = gr_seg_first_slot(seg); s; s = gr_slot_next_in_segment(s))
    {
       gr_slot_attached_to(s);
       gr_slot_first_attachment(s);
       gr_slot_next_sibling_attachment(s);
       gr_slot_gid(s);
       gr_slot_origin_X(s);
       gr_slot_origin_Y(s);
       gr_slot_advance_X(s, face, font);
       gr_slot_advance_Y(s, face, font);
       gr_slot_before(s);
       gr_slot_after(s);
       gr_slot_index(s);
       gr_slot_can_insert_before(s);
       gr_slot_original(s);
    }

    auto params = test.get_slot_attr_parameters(n_slots);
    if (params)
    {
      auto n_params = n_slots;
      for (auto s = gr_seg_first_slot(seg); s && n_params; s = gr_slot_next_in_segment(s))
      {
        auto && p = params[--n_params];
        gr_slot_attr(s, seg, gr_attrCode(p.index), p.subindex);
      }
    }

#if 0
    if (n_slots >= 16)
    {
        auto s = gr_seg_last_slot(seg);
        for (auto n = n_slots/2; n && s; --n, s = gr_slot_prev_in_segment(s));

        auto sp = gr_slot_prev_in_segment(s);
        gr_slot_linebreak_before(const_cast<gr_slot *>(s));
        gr_seg_justify(seg, gr_seg_first_slot(seg), font, 1000,
                        gr_justCompleteLine, nullptr, sp);
        gr_seg_justify(seg, s, font, 1000,
                        gr_justCompleteLine, s, nullptr);
    }
#endif
  }

  gr_seg_destroy(seg);
  gr_font_destroy(font);
  gr_face_destroy(face);

  return 0;
}
