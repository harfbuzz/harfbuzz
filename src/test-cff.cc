/*
 * Copyright © 2024  David Corbett
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
#include "hb-ot-cff-common.hh"
#include "hb-ot-cff1-table.hh"
#include "hb-subset-cff-common.hh"
#include "hb-cff-specializer.hh"
#include "hb-cff-interp-dict-common.hh"
#include "hb-cff-interp-common.hh"

static void
test_specialized_line_geometry ()
{
  /* Reconstruct every point after specializing horizontal/vertical line
   * sequences, including repeated directions and merged alternating runs. */
  for (unsigned n = 1; n <= 8; n++)
    for (unsigned mask = 0; mask < (1u << n); mask++)
      for (unsigned maxstack : {4u, 48u})
      {
        hb_vector_t<CFF::cs_command_t> commands;
        for (unsigned i = 0; i < n; i++)
        {
          bool horizontal = mask & (1u << i);
          int delta = (i & 1) ? -(int) i - 1 : (int) i + 1;
          CFF::cs_command_t cmd (OpCode_rlineto);
          CFF::number_t dx, dy;
          dx.set_int (horizontal ? delta : 0);
          dy.set_int (horizontal ? 0 : delta);
          cmd.args.push (dx);
          cmd.args.push (dy);
          commands.push (std::move (cmd));
        }
        CFF::specialize_commands (commands, maxstack);
        for (const auto &cmd : commands)
          hb_always_assert (cmd.args.length <= maxstack);
        CFF::str_buff_t encoded;
        hb_always_assert (CFF::encode_commands (commands, encoded));
        CFF::generalize_commands (commands);
        hb_always_assert (commands.length == n);
        int x = 0, y = 0, expected_x = 0, expected_y = 0;
        for (unsigned i = 0; i < n; i++)
        {
          bool horizontal = mask & (1u << i);
          int delta = (i & 1) ? -(int) i - 1 : (int) i + 1;
          expected_x += horizontal ? delta : 0;
          expected_y += horizontal ? 0 : delta;
          x += commands[i].args[0].to_int ();
          y += commands[i].args[1].to_int ();
          hb_always_assert (x == expected_x && y == expected_y);
        }
      }
}

static void
test_specialized_fractional_geometry ()
{
  const double deltas[][2] = {
    {0.5, 12}, {-0.5, 12}, {12, 0.5}, {12, -0.5},
    {0.25, -0.75}, {0, 12}, {12, 0},
  };
  for (CFF::op_code_t op : {OpCode_rmoveto, OpCode_rlineto})
    for (const auto &delta : deltas)
    {
      hb_vector_t<CFF::cs_command_t> commands;
      CFF::cs_command_t cmd (op);
      CFF::number_t dx, dy;
      dx.set_real (delta[0]);
      dy.set_real (delta[1]);
      cmd.args.push (dx);
      cmd.args.push (dy);
      commands.push (std::move (cmd));
      CFF::specialize_commands (commands);
      CFF::str_buff_t encoded;
      hb_always_assert (CFF::encode_commands (commands, encoded));
      CFF::generalize_commands (commands);
      hb_always_assert (commands.length == 1);
      hb_always_assert (commands[0].op == op);
      hb_always_assert (commands[0].args.length == 2);
      hb_always_assert (commands[0].args[0].to_real () == delta[0]);
      hb_always_assert (commands[0].args[1].to_real () == delta[1]);
    }
}

static void
test_charset1_splits_long_ranges ()
{
  hb_vector_t<CFF::code_pair_t> ranges;
  ranges.push (CFF::code_pair_t {100, 511});
  ranges.push (CFF::code_pair_t {1000, 0});

  char buffer[10];
  hb_serialize_context_t c (buffer, sizeof (buffer));
  auto *charset = c.start_serialize<CFF::Charset> ();
  hb_always_assert (charset->serialize (&c, 1, 514, ranges));
  c.end_serialize ();
  hb_always_assert (!c.in_error ());

  const unsigned char expected[] = {
    1, 0, 100, 255, 1, 100, 255, 3, 232, 0,
  };
  hb_bytes_t bytes = c.copy_bytes ();
  hb_always_assert (bytes.length == sizeof (expected));
  hb_always_assert (!hb_memcmp (bytes.arrayZ, expected, sizeof (expected)));
  bytes.fini ();

  for (unsigned int i = 0; i < 10; i++)
    ranges.push (CFF::code_pair_t {2000 + i * 2, 0});
  hb_always_assert (CFF::Charset::get_size_for_format1 (ranges) <
		    CFF::Charset2::get_size_for_ranges (ranges.length));
}

template <typename GID, typename FD>
static void
test_fd_select_sentinel ()
{
  unsigned char data[3 * GID::static_size + FD::static_size] = {};
  auto *select = reinterpret_cast<CFF::FDSelect3_4<GID, FD> *> (data);
  select->ranges.len = 1;
  select->ranges.arrayZ[0].first = 0;
  select->ranges.arrayZ[0].fd = 0;
  hb_sanitize_context_t c ((const char *) data, (const char *) data + sizeof (data));
  c.set_num_glyphs (4);
  c.start_processing ();

  select->sentinel () = 3;
  hb_always_assert (select->sanitize (&c, 1, 3));
  hb_always_assert (!select->sanitize (&c, 1));
  select->sentinel () = 4;
  hb_always_assert (select->sanitize (&c, 1, 3));
  hb_always_assert (select->sanitize (&c, 1));
  select->sentinel () = 2;
  hb_always_assert (!select->sanitize (&c, 1, 3));
  select->sentinel () = 5;
  hb_always_assert (!select->sanitize (&c, 1, 3));
  c.end_processing ();
}

static void
test_index_count_limits ()
{
  char buffer[65550];
  for (unsigned count : {0u, 65535u, 65536u, 65537u})
  {
    auto items = hb_range (count) | hb_map ([] (unsigned) { return hb_bytes_t (); });
    hb_serialize_context_t c (buffer, sizeof (buffer));
    auto *index = c.start_serialize<OT::CFF1Index> ();
    bool success = index->serialize_header (&c, +items, 0);
    hb_always_assert (success == (count <= 65535u));
    if (success)
      hb_always_assert (index->count == count);
    else
      hb_always_assert (c.in_error ());

    hb_serialize_context_t c2 (buffer, sizeof (buffer));
    auto *index2 = c2.start_serialize<OT::CFF2Index> ();
    hb_always_assert (index2->serialize_header (&c2, +items, 0));
    hb_always_assert (index2->count == count);
  }
}

int
main (int argc, char **argv)
{
  test_index_count_limits ();
  test_specialized_line_geometry ();
  test_specialized_fractional_geometry ();
  test_charset1_splits_long_ranges ();
  test_fd_select_sentinel<OT::HBUINT16, OT::HBUINT8> ();
  test_fd_select_sentinel<OT::HBUINT32, OT::HBUINT16> ();

  /* Test encode_num_tp */
  {
    CFF::str_buff_t buff;
    CFF::str_encoder_t encoder (buff);
    CFF::number_t number;
    struct num_tp_test {
      double input;
      unsigned length;
      unsigned char output[8];
    };
    struct num_tp_test num_tp_tests[] = {
      { -9.399999999999999, 4, { OpCode_BCD, 0xE9, 0xA4, 0xFF } }, // -9.4
      { 9.399999999999999999, 3, { OpCode_BCD, 0x9A, 0x4F } }, // 9.4
      { 456.8, 4, { OpCode_BCD, 0x45, 0x6A, 0x8F } }, // 456.8
      { 98765.37e2, 5, { OpCode_longintdict, 0x00, 0x96, 0xB4, 0x39 } }, // 9876537
      { 1234567890.0, 5, { OpCode_longintdict, 0x49, 0x96, 0x02, 0xD2 } }, // 1234567900
      { 1234567900.0, 5, { OpCode_longintdict, 0x49, 0x96, 0x02, 0xDC } }, // 1234567900
      { 9.876537e-4, 7, { OpCode_BCD, 0x98, 0x76, 0x53, 0x7C, 0x10, 0xFF } }, // 9876537E-10
      { 9.876537e4, 6, { OpCode_BCD, 0x98, 0x76, 0x5A, 0x37, 0xFF } }, // 98765.37
      { 1e8, 5, { OpCode_longintdict, 0x05, 0xF5, 0xE1, 0x00 } }, // 1E8
      { 1e-5, 3, { OpCode_BCD, 0x1C, 0x5F } }, // 1E-5
      { 1.2e8, 5, { OpCode_longintdict, 0x07, 0x27, 0x0E, 0x00 } }, // 12E7
      { 1.2345e-5, 5, { OpCode_BCD, 0x12, 0x34, 0x5C, 0x9F } }, // 12345E-9
      { 9.0987654e8, 5, { OpCode_longintdict, 0x36, 0x3B, 0x9D, 0x3C } }, // 909876540
      { 0.1, 3, { OpCode_BCD, 0xA1, 0xFF } }, // .1
      { -0.1, 3, { OpCode_BCD, 0xEA, 0x1F } }, // -.1
      { 0.01, 3, { OpCode_BCD, 0x1C, 0x2F } }, // 1E-2
      { -0.01, 4, { OpCode_BCD, 0xE1, 0xC2, 0xFF } }, // -1E-2
      { 0.0123, 4, { OpCode_BCD, 0x12, 0x3C, 0x4F } }, // 123E-4
      { -0.0123, 5, { OpCode_BCD, 0xE1, 0x23, 0xC4, 0xFF } }, // -123E-4
      { 0.0012, 4, { OpCode_BCD, 0x12, 0xC4, 0xFF } },   // 12E-4
      { 1.23e-5, 4, { OpCode_BCD, 0x12, 0x3C, 0x7F } },   // 123E-7
      { 123456780.0, 5, { OpCode_longintdict, 0x07, 0x5B, 0xCD, 0x0C } },  // 123456780
      { 0.000123, 4, { OpCode_BCD, 0x12, 0x3C, 0x6F } },   // 123E-6
      { -0.000123, 5, { OpCode_BCD, 0xE1, 0x23, 0xC6, 0xFF } },  // -123E-6
      /* Test cases for trailing zeros fix */
      { 12340000.0, 5, { OpCode_longintdict, 0x00, 0xBC, 0x4B, 0x20 } }, // 1234E4
      { 1000000.0, 5, { OpCode_longintdict, 0x00, 0x0F, 0x42, 0x40 } }, // 1E6
      { 5000.0, 3, { OpCode_shortint, 0x13, 0x88 } }, // 5000 (shortint, roundtrip via parse_shortint)
      { 0.0, 1, { 0x8B } },
      { -0.0, 1, { 0x8B } },
      { 107.0, 1, { 0xF6 } },  // max positive one-byte
      { -107.0, 1, { 0x20 } }, // min negative one-byte

      { -108.0, 2, { OpCode_TwoByteNegInt0, 0x00 } },  // min two-byte negative
      { 364.0,  2, { OpCode_TwoBytePosInt1, 0x00 } },  // 364 (first Int1)
      { 620.0,  2, { OpCode_TwoBytePosInt2, 0x00 } },  // 620 (first Int2)
      { 876.0,  2, { OpCode_TwoBytePosInt3, 0x00 } },  // 876 (first Int3)
      { 1131.0, 2, { OpCode_TwoBytePosInt3, 0xFF } },  // 1131 (max)

      /* Two-byte negative: symmetric coverage. */
      { -108.0,  2, { OpCode_TwoByteNegInt0, 0x00 } },
      { -364.0,  2, { OpCode_TwoByteNegInt1, 0x00 } },
      { -620.0,  2, { OpCode_TwoByteNegInt2, 0x00 } },
      { -876.0,  2, { OpCode_TwoByteNegInt3, 0x00 } },
      { -1131.0, 2, { OpCode_TwoByteNegInt3, 0xFF } },

      { 1132.0, 3, { OpCode_shortint, 0x04, 0x6C } },  // 1132 = 0x046C
      { -1132.0, 3, { OpCode_shortint, 0xFB, 0x94 } }, // -1132 = 0xFB94
      { 32767.0, 3, { OpCode_shortint, 0x7F, 0xFF } },   // INT16_MAX
      { -32768.0, 3, { OpCode_shortint, 0x80, 0x00 } },  // INT16_MIN

      /* Test cases for int32 encoding (new in this version) */
      { 32768.0,       5, { OpCode_longintdict, 0x00, 0x00, 0x80, 0x00 } },    // INT16_MAX + 1
      { 100000.0,      5, { OpCode_longintdict, 0x00, 0x01, 0x86, 0xA0 } },    // spec Table 4
      { -100000.0,     5, { OpCode_longintdict, 0xFF, 0xFE, 0x79, 0x60 } },    // spec Table 4
      { 2147483647.0,  5, { OpCode_longintdict, 0x7F, 0xFF, 0xFF, 0xFF } },    // INT32_MAX
      { -2147483648.0, 5, { OpCode_longintdict, 0x80, 0x00, 0x00, 0x00 } },    // INT32_MIN

      { -4.9406565e-324, 8, { OpCode_BCD, 0xE4, 0x94, 0x06, 0x56, 0x5C, 0x33, 0x1F } },
      { 1.7976931348623157e308,  8, { OpCode_BCD, 0x17, 0x97, 0x69, 0x31, 0xB3, 0x01, 0xFF } }, // DBL_MAX
      { -1.7976931348623157e308, 8, { OpCode_BCD, 0xE1, 0x79, 0x76, 0x93, 0x1B, 0x30, 0x1F } }, // -DBL_MAX
      { 2.2250738585072014e-308, 8, { OpCode_BCD, 0x22, 0x25, 0x07, 0x39, 0xC3, 0x15, 0xFF } }, // DBL_MIN
      { 4.9406564584124654e-324, 8, { OpCode_BCD, 0x49, 0x40, 0x65, 0x65, 0xC3, 0x31, 0xFF } }, // DBL_TRUE_MIN positive
      { 1e200,  4, { OpCode_BCD, 0x1B, 0x20, 0x0F } }, // 1E+200
      { 1e-200, 4, { OpCode_BCD, 0x1C, 0x20, 0x0F } }, // 1E-200
      { 1e300,  4, { OpCode_BCD, 0x1B, 0x30, 0x0F } }, // 1E+300
      { 1e-300, 4, { OpCode_BCD, 0x1C, 0x30, 0x0F } }, // 1E-300
      { 1e-320, 8, { OpCode_BCD, 0x99, 0x99, 0x88, 0x87, 0xC3, 0x28, 0xFF } }, // 9.9998887E-321
    };
    for (auto &num_tp_test : num_tp_tests)
    {
      number.set_real (num_tp_test.input);
      encoder.encode_num_tp (number);
      hb_always_assert (buff.length == num_tp_test.length);
      for (unsigned i = 0; i < buff.length; i++)
	hb_always_assert (buff[i] == num_tp_test.output[i]);
      using opset_t = CFF::cff1_font_dict_opset_t::opset_t;
      if (opset_t::is_one_byte_integer (buff[0]))
      {
        hb_always_assert (buff.length == 1);
        hb_always_assert (opset_t::parse_one_byte_integer (buff[0]) == (int) num_tp_test.input);
      }
      else if (opset_t::is_two_bytes_positive_integer (buff[0]))
      {
        hb_always_assert (buff.length == 2);
        hb_always_assert (opset_t::parse_two_bytes_positive_integer (buff[0], buff[1]) == (int) num_tp_test.input);
      }
      else if (opset_t::is_two_bytes_negative_integer (buff[0]))
      {
        hb_always_assert (buff.length == 2);
        hb_always_assert (opset_t::parse_two_bytes_negative_integer (buff[0], buff[1]) == (int) num_tp_test.input);
      }
      else if (opset_t::is_shortint (buff[0]))
      {
        hb_always_assert (buff.length == 3);
        hb_always_assert (opset_t::parse_shortint (buff[1], buff[2]) == (int) num_tp_test.input);
      }
      else if (buff[0] == OpCode_longintdict)
      {
        hb_always_assert (buff.length == 5);
        hb_always_assert (CFF::arg_stack_t<>::parse_longint (buff[1], buff[2], buff[3], buff[4]) == (int) num_tp_test.input);
      }
      else if (buff[0] == OpCode_BCD)
      {
        hb_always_assert (buff.length > 1);
	hb_ubytes_t bytes (buff.arrayZ + 1, buff.length - 1);
	CFF::byte_str_ref_t str_ref (bytes);
	double parsed = CFF::dict_opset_t::parse_bcd (str_ref);
        bool almost_equal = fabs (parsed - num_tp_test.input) <= fabs (num_tp_test.input) * 1e-7;
	hb_always_assert (almost_equal);
      }
      else hb_always_assert (false); /* Unsupported opcode */
      encoder.reset ();
    }
  }

  return 0;
}
