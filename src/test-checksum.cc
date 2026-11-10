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

/* Expected values generated independently with Python's int.from_bytes. */
static void
test_python_generated_checksums ()
{
/* BEGIN GENERATED CHECKSUM VECTORS */
  static const uint8_t data[] = {
    0x94, 0x26, 0x60, 0xCD, 0xAD, 0x32, 0xF7, 0x18, 0xEF, 0xE1,
    0xC3, 0x9B, 0xC1, 0xE9, 0x65, 0xA4, 0x27, 0xE8, 0x28, 0x4E,
    0x5E, 0x87, 0x80, 0xF2, 0x20, 0x43, 0x86, 0xCC, 0x7D, 0x99,
    0x37, 0x2F, 0xA0, 0xC5, 0xCB, 0x47, 0x22, 0x44, 0x9C, 0x36,
  };
  static const struct
  {
    unsigned offset;
    unsigned length;
    uint32_t checksum;
  } cases[] = {
    {0, 0, 0x00000000u},
    {0, 1, 0x94000000u},
    {0, 2, 0x94260000u},
    {0, 3, 0x94266000u},
    {0, 4, 0x942660CDu},
    {0, 5, 0x412660CDu},
    {0, 6, 0x415860CDu},
    {0, 7, 0x415957CDu},
    {0, 8, 0x415957E5u},
    {0, 9, 0x305957E5u},
    {0, 11, 0x313B1AE5u},
    {0, 13, 0xF23B1B80u},
    {0, 16, 0xF3248124u},
    {0, 19, 0x1B0CA924u},
    {0, 23, 0x79942972u},
    {0, 27, 0x99D7B064u},
    {0, 31, 0x1770E830u},
    {0, 32, 0x1770E85Fu},
    {1, 0, 0x00000000u},
    {1, 1, 0x26000000u},
    {1, 2, 0x26600000u},
    {1, 3, 0x2660CD00u},
    {1, 4, 0x2660CDADu},
    {1, 5, 0x5860CDADu},
    {1, 6, 0x5957CDADu},
    {1, 7, 0x5957E5ADu},
    {1, 8, 0x5957E69Cu},
    {1, 9, 0x3A57E69Cu},
    {1, 11, 0x3B1B819Cu},
    {1, 13, 0x241B825Du},
    {1, 16, 0x24812684u},
    {1, 19, 0x0CA97484u},
    {1, 23, 0x942A66E2u},
    {1, 27, 0xD7B13302u},
    {1, 31, 0x70E8627Fu},
    {1, 32, 0x70E8631Fu},
    {2, 0, 0x00000000u},
    {2, 1, 0x60000000u},
    {2, 2, 0x60CD0000u},
    {2, 3, 0x60CDAD00u},
    {2, 4, 0x60CDAD32u},
    {2, 5, 0x57CDAD32u},
    {2, 6, 0x57E5AD32u},
    {2, 7, 0x57E69C32u},
    {2, 8, 0x57E69D13u},
    {2, 9, 0x1AE69D13u},
    {2, 11, 0x1B825E13u},
    {2, 13, 0x80825EFCu},
    {2, 16, 0x812686E4u},
    {2, 19, 0xA974E4E4u},
    {2, 23, 0x2A67056Bu},
    {2, 27, 0xB13382AEu},
    {2, 31, 0xE8632347u},
    {2, 32, 0xE863240Cu},
    {3, 0, 0x00000000u},
    {3, 1, 0xCD000000u},
    {3, 2, 0xCDAD0000u},
    {3, 3, 0xCDAD3200u},
    {3, 4, 0xCDAD32F7u},
    {3, 5, 0xE5AD32F7u},
    {3, 6, 0xE69C32F7u},
    {3, 7, 0xE69D13F7u},
    {3, 8, 0xE69D14BAu},
    {3, 9, 0x819D14BAu},
    {3, 11, 0x825EFDBAu},
    {3, 13, 0x265EFE1Fu},
    {3, 16, 0x2686E647u},
    {3, 19, 0x74E56D47u},
    {3, 23, 0x6705B0C7u},
    {3, 27, 0x33834A4Du},
    {3, 31, 0x63240F84u},
    {3, 32, 0x6324104Fu},
  };
/* END GENERATED CHECKSUM VECTORS */

  for (const auto &c : cases)
  {
    const uint8_t *p = data + c.offset;
    hb_always_assert (OT::CheckSum::CalcTableChecksum ((const OT::HBUINT32 *) p,
						       c.length) == c.checksum);
    OT::CheckSum checksum;
    checksum.set_for_data (p, c.length);
    hb_always_assert ((uint32_t) checksum == c.checksum);
  }
}

int
main ()
{
  test_calc_table_checksum ();
  test_python_generated_checksums ();
  return 0;
}
