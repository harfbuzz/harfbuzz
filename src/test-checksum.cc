/*
 * Copyright © 2026  Ebrahim Byagowi
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

#include "hb.hh"
#include "hb-open-type.hh"

static void
test_calc_table_checksum ()
{
  static const uint8_t data[] = {'a', 'b', 'c', 'd', 'e'};
  const OT::HBUINT32 *words = (const OT::HBUINT32 *) data;

  hb_always_assert (OT::CheckSum::CalcTableChecksum (words, 0) == 0u);
  hb_always_assert (OT::CheckSum::CalcTableChecksum (words, 1) == 0x61000000u);
  hb_always_assert (OT::CheckSum::CalcTableChecksum (words, 2) == 0x61620000u);
  hb_always_assert (OT::CheckSum::CalcTableChecksum (words, 3) == 0x61626300u);
  hb_always_assert (OT::CheckSum::CalcTableChecksum (words, 4) == 0x61626364u);
  hb_always_assert (OT::CheckSum::CalcTableChecksum (words, 5) == 0xC6626364u);

  /* The data pointer is deliberately unaligned. */
  hb_always_assert (OT::CheckSum::CalcTableChecksum (
			(const OT::HBUINT32 *) (data + 1), 4) == 0x62636465u);
  hb_always_assert (OT::CheckSum::CalcTableChecksum (
			(const OT::HBUINT32 *) (data + 1), 3) == 0x62636400u);

  OT::CheckSum checksum;
  for (unsigned length = 0; length <= 5; length++)
  {
    checksum.set_for_data (data, length);
    hb_always_assert ((uint32_t) checksum ==
		      OT::CheckSum::CalcTableChecksum (words, length));
  }

  checksum.set_for_data (data + 1, 4);
  hb_always_assert ((uint32_t) checksum == 0x62636465u);
}

int
main ()
{
  test_calc_table_checksum ();
  return 0;
}
