/*
 * CFF CharString Specializer
 *
 * Optimizes CharString bytecode by using specialized operators
 * (hlineto, vlineto, hhcurveto, etc.) to save bytes and respects
 * CFF1 stack limit (48 values).
 *
 * Based on fontTools.cffLib.specializer
 */

#ifndef HB_CFF_SPECIALIZER_HH
#define HB_CFF_SPECIALIZER_HH

#include "hb.hh"
#include "hb-cff-interp-cs-common.hh"

namespace CFF {

/* CharString command representation - forward declared in hb-subset-cff-common.hh */

/* Check if a value is effectively zero */
static inline bool
is_zero (const number_t &n)
{
  return n.to_int () == 0;
}

/* Specialize CharString commands to optimize bytecode size
 *
 * Performs key optimizations:
 * 1. Convert rmoveto/rlineto to h/v variants when dx or dy is zero
 * 2. Combine adjacent compatible operators
 * 3. Respect maxstack limit (default 48 for CFF1)
 *
 * For now, implements essential optimizations. Full specialization
 * (curve variants, peephole opts, etc.) can be added later.
 */
static void
specialize_commands (hb_vector_t<cs_command_t> &commands,
                     unsigned maxstack = 48)
{
  if (commands.length == 0) return;

  /* Pass 1: Specialize rmoveto/rlineto into h/v variants */
  for (unsigned i = 0; i < commands.length; i++)
  {
    auto &cmd = commands[i];

    if ((cmd.op == OpCode_rmoveto || cmd.op == OpCode_rlineto) &&
        cmd.args.length == 2)
    {
      bool dx_zero = is_zero (cmd.args[0]);
      bool dy_zero = is_zero (cmd.args[1]);

      if (dx_zero && !dy_zero)
      {
        /* Horizontal: keep only dy */
        cmd.op = (cmd.op == OpCode_rmoveto) ? OpCode_hmoveto : OpCode_hlineto;
        /* Shift dy to position 0 */
        cmd.args[0] = cmd.args[1];
        cmd.args.resize (1);
      }
      else if (!dx_zero && dy_zero)
      {
        /* Vertical: keep only dx */
        cmd.op = (cmd.op == OpCode_rmoveto) ? OpCode_vmoveto : OpCode_vlineto;
        cmd.args.resize (1);  /* Keep only dx */
      }
      /* else: both zero or both non-zero, keep as rmoveto/rlineto */
    }
  }

  /* Pass 2: Combine adjacent hlineto/vlineto operators
   * hlineto can take multiple args alternating with vlineto
   * This saves operator bytes */
  for (int i = (int)commands.length - 1; i > 0; i--)
  {
    auto &cmd = commands[i];
    auto &prev = commands[i-1];

    /* Combine adjacent hlineto + vlineto or vlineto + hlineto */
    if ((prev.op == OpCode_hlineto && cmd.op == OpCode_vlineto) ||
        (prev.op == OpCode_vlineto && cmd.op == OpCode_hlineto))
    {
      /* Check stack depth */
      unsigned combined_args = prev.args.length + cmd.args.length;
      if (combined_args < maxstack)
      {
        /* Merge into first command, keep its operator */
        for (unsigned j = 0; j < cmd.args.length; j++)
          prev.args.push (cmd.args[j]);
        commands.remove_ordered (i);
        i++;  /* Adjust for removed element */
      }
    }
  }

  /* Pass 3: Combine adjacent identical operators */
  for (int i = (int)commands.length - 1; i > 0; i--)
  {
    auto &cmd = commands[i];
    auto &prev = commands[i-1];

    /* Combine same operators (e.g., rlineto + rlineto) */
    if (prev.op == cmd.op &&
        (cmd.op == OpCode_rlineto || cmd.op == OpCode_hlineto ||
         cmd.op == OpCode_vlineto || cmd.op == OpCode_rrcurveto))
    {
      /* Check stack depth */
      unsigned combined_args = prev.args.length + cmd.args.length;
      if (combined_args < maxstack)
      {
        /* Merge args */
        for (unsigned j = 0; j < cmd.args.length; j++)
          prev.args.push (cmd.args[j]);
        commands.remove_ordered (i);
        i++;  /* Adjust for removed element */
      }
    }
  }
}

/* Encode commands back to binary CharString */
static bool
encode_commands (const hb_vector_t<cs_command_t> &commands,
                 str_buff_t &output)
{
  for (const auto &cmd : commands)
  {
    str_encoder_t encoder (output);

    /* Encode arguments */
    for (const auto &arg : cmd.args)
      encoder.encode_num_cs (arg);

    /* Encode operator */
    if (cmd.op != OpCode_Invalid)
      encoder.encode_op (cmd.op);

    if (encoder.in_error ())
      return false;
  }

  return true;
}

} /* namespace CFF */

#endif /* HB_CFF_SPECIALIZER_HH */
