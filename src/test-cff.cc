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
test_specialized_many_commands ()
{
  /* Combining lines must not get slower with the number of commands that
   * follow them, and must leave those commands alone. */
  const unsigned n = 100000;
  hb_vector_t<CFF::cs_command_t> commands;
  for (unsigned i = 0; i < n; i++)
  {
    CFF::cs_command_t cmd (OpCode_rlineto);
    CFF::number_t dx, dy;
    dx.set_int ((i & 1) ? 0 : 1);
    dy.set_int ((i & 1) ? 1 : 0);
    cmd.args.push (dx);
    cmd.args.push (dy);
    commands.push (std::move (cmd));
  }
  for (unsigned i = 0; i < n; i++)
    commands.push (CFF::cs_command_t (OpCode_hhcurveto));
  CFF::specialize_commands (commands);
  unsigned num_lines = (n + 46) / 47;
  hb_always_assert (commands.length == num_lines + n);
  unsigned num_args = 0;
  for (unsigned i = 0; i < num_lines; i++)
  {
    hb_always_assert (commands[i].op == ((num_args & 1) ? OpCode_vlineto : OpCode_hlineto));
    num_args += commands[i].args.length;
  }
  hb_always_assert (num_args == n);
  for (unsigned i = num_lines; i < commands.length; i++)
    hb_always_assert (commands[i].op == OpCode_hhcurveto && !commands[i].args.length);
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
  /* The last range ends at the sentinel. */
  hb_always_assert (select->get_fd_range (2).second == 4);
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
  test_specialized_many_commands ();
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
      unsigned char output[7];
    };
    struct num_tp_test num_tp_tests[] = {
      { -9.399999999999999, 4, { 0x1E, 0xE9, 0xA4, 0xFF } }, // -9.4
      { 9.399999999999999999, 3, { 0x1E, 0x9A, 0x4F } }, // 9.4
      { 456.8, 4, { 0x1E, 0x45, 0x6A, 0x8F } }, // 456.8
      { 98765.37e2, 5, { 0x1E, 0x98, 0x76, 0x53, 0x7F } }, // 9876537
      { 1234567890.0, 7, { 0x1E, 0x12, 0x34, 0x56, 0x79, 0xB2, 0xFF } }, // 12345679E2
      { 9.876537e-4, 7, { 0x1E, 0x98, 0x76, 0x53, 0x7C, 0x10, 0xFF } }, // 9876537E-10
      { 9.876537e4, 6, { 0x1E, 0x98, 0x76, 0x5A, 0x37, 0xFF } }, // 98765.37
      { 1e8, 3, { 0x1E, 0x1B, 0x8F } }, // 1E8
      { 1e-5, 3, { 0x1E, 0x1C, 0x5F } }, // 1E-5
      { 1.2e8, 4, { 0x1E, 0x12, 0xB7, 0xFF } }, // 12E7
      { 1.2345e-5, 5, { 0x1E, 0x12, 0x34, 0x5C, 0x9F } }, // 12345E-9
      { 9.0987654e8, 6, { 0x1E, 0x90, 0x98, 0x76, 0x54, 0x0F } }, // 909876540
      { 0.1, 3, { 0x1E, 0xA1, 0xFF } }, // .1
      { -0.1, 3, { 0x1E, 0xEA, 0x1F } }, // -.1
      { 0.01, 3, { 0x1E, 0x1C, 0x2F } }, // 1E-2
      { -0.01, 4, { 0x1E, 0xE1, 0xC2, 0xFF } }, // -1E-2
      { 0.0123, 4, { 0x1E, 0x12, 0x3C, 0x4F } }, // 123E-4
      { -0.0123, 5, { 0x1E, 0xE1, 0x23, 0xC4, 0xFF } }, // -123E-4
    };
    for (size_t t = 0; t < sizeof num_tp_tests / sizeof num_tp_tests[0]; t++)
    {
      struct num_tp_test num_tp_test = num_tp_tests[t];
      number.set_real (num_tp_test.input);
      encoder.encode_num_tp (number);
      hb_always_assert (buff.length == num_tp_test.length);
      for (unsigned i = 0; i < buff.length; i++)
	hb_always_assert (buff[i] == num_tp_test.output[i]);
      encoder.reset ();
    }
  }

  return 0;
}
