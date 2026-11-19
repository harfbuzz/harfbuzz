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

#include "hb-ot-layout-gsubgpos.hh"

#if !defined(HB_NO_GDEF_CACHE) && !defined(HB_NO_OT_LAYOUT)
struct matcher_context_t
{
  matcher_context_t (const OT::GDEF::accelerator_t &gdef, hb_buffer_t *buffer_) :
    gdef_accel (gdef), buffer (buffer_) {}

  const OT::GDEF::accelerator_t &gdef_accel;
  hb_buffer_t *buffer;
  unsigned lookup_props = 0, lookup_mask = 1, table_index = 0;
  bool auto_zwnj = true, auto_zwj = true, per_syllable = false;
};

struct iterator_t : OT::skipping_iterator_t<matcher_context_t>
{
  const hb_bit_vector_t<> *get_mark_filter () const { return matcher.mark_filter; }
};

struct skip_context_t : matcher_context_t
{
  using matcher_context_t::matcher_context_t;
  bool check_glyph_property (const hb_glyph_info_t *info,
                            unsigned HB_UNUSED,
                            const hb_bit_vector_t<> * HB_UNUSED) const
  { return info->codepoint != 1; }
};

static void test_skip_budget (const OT::GDEF::accelerator_t &gdef)
{
  /* Skipped glyphs are charged in runs of 32. */
  const unsigned len = 66;
  hb_buffer_t *buffer = hb_buffer_create ();
  skip_context_t context (gdef, buffer);
  for (bool backward : {false, true})
    for (unsigned ending = 0; ending < 3; ending++)
      for (int budget = 0; budget <= 3; budget++)
      {
        hb_buffer_clear_contents (buffer);
        for (unsigned i = 0; i < len; i++)
          hb_buffer_add (buffer, ending == 2 || (i != 0 && i != len - 1) ? 1 : 0, i);
        for (unsigned i = 0; i < len; i++) buffer->info[i].mask = 1;
        buffer->idx = backward ? len - 1 : 0;
        buffer->out_info = buffer->info;
        buffer->out_len = buffer->len;
        if (ending == 1) buffer->info[backward ? 0 : len - 1].mask = 0;
        buffer->max_ops = budget;
        OT::skipping_iterator_t<skip_context_t> iter;
        iter.init (&context);
        if (backward) iter.reset_back (len - 1);
        else iter.reset (0);
        unsigned unsafe = UINT_MAX;
        bool matched = backward ? iter.prev (&unsafe) : iter.next (&unsafe);
        unsigned skipped = (ending == 2 ? len - 1 : len - 2) / 32;
        hb_always_assert (matched == (ending == 0));
        hb_always_assert (buffer->max_ops == budget - (int) skipped);
        hb_always_assert (buffer->successful == (budget >= (int) skipped));
        if (!matched) hb_always_assert (unsafe == (backward ? 0u : len));
      }
  hb_buffer_destroy (buffer);
}
#endif

int
main ()
{
#if !defined(HB_NO_GDEF_CACHE) && !defined(HB_NO_OT_LAYOUT)
  /* Empty, singleton, sparse, unaligned ranges, all glyphs, and spans
   * immediately below and above the exact-cache limit. */
  static const unsigned char data[] = {
    0, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14,
    0, 1, 0, 7,
    0, 0, 0, 32, 0, 0, 0, 36, 0, 0, 0, 42, 0, 0, 0, 56,
    0, 0, 0, 72, 0, 0, 0, 82, 0, 0, 0, 92,
    0, 1, 0, 0,
    0, 1, 0, 1, 0, 37,
    0, 1, 0, 5, 0, 37, 0, 63, 0, 64, 0, 65, 0, 100,
    0, 2, 0, 2, 0, 37, 0, 102, 0, 0, 0, 150, 0, 159, 0, 66,
    0, 2, 0, 1, 0, 0, 255, 255, 0, 0,
    0, 2, 0, 1, 0, 37, 2, 36, 0, 0,
    0, 2, 0, 1, 0, 37, 2, 37, 0, 0,
  };
  hb_face_t *face = hb_face_builder_create ();
  hb_blob_t *blob = hb_blob_create ((const char *) data, sizeof (data),
                                  HB_MEMORY_MODE_READONLY, nullptr, nullptr);
  hb_always_assert (hb_face_builder_add_table (face, HB_OT_TAG_GDEF, blob));
  hb_blob_destroy (blob);
  hb_ot_layout_has_glyph_classes (face);
  const auto *accel = face->table.GDEF.get_relaxed ();
  hb_always_assert (accel && accel->mark_glyph_set_bitmaps.length == 7);
  hb_buffer_t *buffer = hb_buffer_create ();
  matcher_context_t context (*accel, buffer);
  iterator_t iter;
  for (unsigned set = 0; set < 7; set++)
  {
    bool exact = set == 1 || set == 2 || set == 3 || set == 5;
    hb_always_assert (accel->mark_glyph_set_bitmaps[set].glyphs.in_error () == !exact);
    const auto *bitmap = exact ? &accel->mark_glyph_set_bitmaps[set].glyphs : nullptr;
    unsigned props = (set << 16) | OT::LookupFlag::UseMarkFilteringSet;
    context.lookup_props = props;
    iter.init (&context);
    hb_always_assert (iter.get_mark_filter () == bitmap);
    /* Overriding lookup properties must refresh or clear the cached set. */
    iter.set_lookup_props (0);
    hb_always_assert (!iter.get_mark_filter ());
    iter.set_lookup_props (props);
    hb_always_assert (iter.get_mark_filter () == bitmap);
    for (unsigned glyph = 0; glyph <= 65536; glyph++)
      hb_always_assert (accel->mark_set_covers (set, glyph) ==
                        accel->table->mark_set_covers (set, glyph));
    hb_always_assert (!accel->mark_set_covers (set, UINT_MAX));
  }
  test_skip_budget (*accel);
  hb_buffer_destroy (buffer);
  hb_face_destroy (face);
#endif
}
