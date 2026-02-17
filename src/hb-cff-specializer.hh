/*
 * CFF CharString Specializer
 *
 * Optimizes CharString bytecode by using specialized operators
 * (hlineto, vlineto, hhcurveto, etc.) to save bytes.
 *
 * Based on fontTools.cffLib.specializer
 */

#ifndef HB_CFF_SPECIALIZER_HH
#define HB_CFF_SPECIALIZER_HH

#include "hb.hh"
#include "hb-cff-interp-cs-common.hh"

namespace CFF {

/* CharString command representation - forward declared in hb-subset-cff-common.hh */

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

/* Specialize CharString commands to optimize bytecode size
 *
 * This performs several optimization passes:
 * 1. Combine successive rmoveto operations
 * 2. Specialize rmoveto/rlineto/rrcurveto into h/v variants
 * 3. Merge or delete redundant operations
 * 4. Peephole optimization
 * 5. Combine adjacent operators (respecting maxstack)
 * 6. Resolve pseudo-operators to real operators
 */
static void
specialize_commands (hb_vector_t<cs_command_t> &commands,
                     bool preserve_topology = false,
                     unsigned maxstack = 48)
{
  if (commands.length == 0) return;

  /* 1. Combine successive rmoveto operations */
  for (int i = commands.length - 1; i > 0; i--)
  {
    if (commands[i].op == "rmoveto" && commands[i-1].op == "rmoveto")
    {
      auto &args1 = commands[i-1].args;
      auto &args2 = commands[i].args;
      if (args1.length == 2 && args2.length == 2)
      {
        args1[0] += args2[0];
        args1[1] += args2[1];
        commands.remove (i);
      }
    }
  }

  /* 2. Specialize rmoveto/rlineto/rrcurveto into h/v/r/0 variants */
  for (unsigned i = 0; i < commands.length; i++)
  {
    auto &cmd = commands[i];

    if (cmd.op == "rmoveto" || cmd.op == "rlineto")
    {
      if (cmd.args.length == 2)
      {
        direction_t d = categorize_vector (cmd.args[0], cmd.args[1]);
        switch (d)
        {
          case direction_t::ZERO:
            cmd.op = cmd.op == "rmoveto" ? "0moveto" : "0lineto";
            cmd.args.resize (1);
            cmd.args[0] = 0;
            break;
          case direction_t::H:
            cmd.op = cmd.op == "rmoveto" ? "hmoveto" : "hlineto";
            cmd.args.remove (0);  /* keep dy only */
            break;
          case direction_t::V:
            cmd.op = cmd.op == "rmoveto" ? "vmoveto" : "vlineto";
            cmd.args.resize (1);  /* keep dx only */
            break;
          case direction_t::R:
            /* already rmoveto/rlineto */
            break;
        }
      }
    }
    else if (cmd.op == "rrcurveto")
    {
      if (cmd.args.length == 6)
      {
        direction_t d1 = categorize_vector (cmd.args[0], cmd.args[1]);
        direction_t d2 = categorize_vector (cmd.args[4], cmd.args[5]);

        /* Build specialized curve operator name: d1 + d2 + "curveto" */
        char op_name[16];
        const char *d1_str = "";
        const char *d2_str = "";

        switch (d1) {
          case direction_t::ZERO: d1_str = "0"; break;
          case direction_t::H: d1_str = "h"; break;
          case direction_t::V: d1_str = "v"; break;
          case direction_t::R: d1_str = "r"; break;
        }

        switch (d2) {
          case direction_t::ZERO: d2_str = "0"; break;
          case direction_t::H: d2_str = "h"; break;
          case direction_t::V: d2_str = "v"; break;
          case direction_t::R: d2_str = "r"; break;
        }

        snprintf (op_name, sizeof(op_name), "%s%scurveto", d1_str, d2_str);
        cmd.op = op_name;

        /* Adjust args based on direction */
        hb_vector_t<double> new_args;
        new_args.alloc (6);

        if (d1 != direction_t::R)
        {
          if (d1 == direction_t::H)
            new_args.push (cmd.args[1]);  /* dy0 only */
          else if (d1 == direction_t::V)
            new_args.push (cmd.args[0]);  /* dx0 only */
          else  /* ZERO */
            new_args.push (0);
        }
        else
        {
          new_args.push (cmd.args[0]);
          new_args.push (cmd.args[1]);
        }

        new_args.push (cmd.args[2]);
        new_args.push (cmd.args[3]);

        if (d2 != direction_t::R)
        {
          if (d2 == direction_t::H)
            new_args.push (cmd.args[5]);  /* dy3 only */
          else if (d2 == direction_t::V)
            new_args.push (cmd.args[4]);  /* dx3 only */
          else  /* ZERO */
            new_args.push (0);
        }
        else
        {
          new_args.push (cmd.args[4]);
          new_args.push (cmd.args[5]);
        }

        cmd.args = hb_move (new_args);
      }
    }
  }

  /* 3. Merge or delete redundant operations */
  if (!preserve_topology)
  {
    for (int i = commands.length - 1; i >= 0; i--)
    {
      auto &cmd = commands[i];

      /* 00curveto is demoted to a lineto */
      if (cmd.op == "00curveto" && cmd.args.length == 4)
      {
        direction_t d = categorize_vector (cmd.args[1], cmd.args[2]);
        switch (d)
        {
          case direction_t::ZERO:
            cmd.op = "0lineto";
            cmd.args.resize (1);
            cmd.args[0] = 0;
            break;
          case direction_t::H:
            cmd.op = "hlineto";
            cmd.args.resize (1);
            cmd.args[0] = cmd.args[2];
            break;
          case direction_t::V:
            cmd.op = "vlineto";
            cmd.args.resize (1);
            cmd.args[0] = cmd.args[1];
            break;
          case direction_t::R:
            cmd.op = "rlineto";
            cmd.args.resize (2);
            cmd.args[0] = cmd.args[1];
            cmd.args[1] = cmd.args[2];
            break;
        }
      }

      /* 0lineto can be deleted */
      if (cmd.op == "0lineto")
      {
        commands.remove (i);
        continue;
      }

      /* Merge adjacent hlineto's and vlineto's */
      if (i > 0 && (cmd.op == "hlineto" || cmd.op == "vlineto") &&
          cmd.op == commands[i-1].op)
      {
        auto &prev_args = commands[i-1].args;
        if (cmd.args.length == 1 && prev_args.length == 1)
        {
          prev_args[0] += cmd.args[0];
          commands.remove (i);
          continue;
        }
      }
    }
  }

  /* 4. Peephole optimization - revert h/v back to r if it saves bytes */
  for (unsigned i = 1; i + 1 < commands.length; i++)
  {
    auto &cmd = commands[i];
    const auto &prv = commands[i-1].op;
    const auto &nxt = commands[i+1].op;

    if ((cmd.op == "0lineto" || cmd.op == "hlineto" || cmd.op == "vlineto") &&
        prv == "rlineto" && nxt == "rlineto")
    {
      if (cmd.args.length == 1)
      {
        hb_vector_t<double> new_args;
        new_args.alloc (2);
        if (cmd.op[0] == 'v')
        {
          new_args.push (0);
          new_args.push (cmd.args[0]);
        }
        else
        {
          new_args.push (cmd.args[0]);
          new_args.push (0);
        }
        cmd.args = hb_move (new_args);
        cmd.op = "rlineto";
      }
    }
  }

  /* 5. Combine adjacent operators (respecting maxstack) */
  unsigned stack_use = commands.length > 0 ? args_stack_use (commands[commands.length-1].args) : 0;

  for (int i = commands.length - 1; i > 0; i--)
  {
    auto &cmd1 = commands[i-1];
    auto &cmd2 = commands[i];
    bool can_merge = false;

    /* Merge rlineto + rlineto */
    if (cmd1.op == "rlineto" && cmd2.op == "rlineto")
    {
      can_merge = true;
    }
    /* Merge rrcurveto + rrcurveto */
    else if (cmd1.op == "rrcurveto" && cmd2.op == "rrcurveto")
    {
      can_merge = true;
    }
    /* Merge hlineto + vlineto or vlineto + hlineto */
    else if ((cmd1.op == "hlineto" && cmd2.op == "vlineto") ||
             (cmd1.op == "vlineto" && cmd2.op == "hlineto"))
    {
      can_merge = true;
    }

    if (can_merge)
    {
      unsigned args1_stack_use = args_stack_use (cmd1.args);
      unsigned combined_stack_use = hb_max (args1_stack_use, cmd1.args.length + stack_use);

      if (combined_stack_use < maxstack)
      {
        /* Merge args */
        for (unsigned j = 0; j < cmd2.args.length; j++)
          cmd1.args.push (cmd2.args[j]);

        commands.remove (i);
        stack_use = combined_stack_use;
      }
      else
      {
        stack_use = args1_stack_use;
      }
    }
    else
    {
      stack_use = args_stack_use (cmd1.args);
    }
  }

  /* 6. Resolve pseudo-operators to real operators */
  for (unsigned i = 0; i < commands.length; i++)
  {
    auto &cmd = commands[i];

    /* 0moveto -> hmoveto */
    if (cmd.op == "0moveto")
    {
      cmd.op = "hmoveto";
    }
    /* 0lineto -> hlineto */
    else if (cmd.op == "0lineto")
    {
      cmd.op = "hlineto";
    }
    /* Resolve specialized curve operators */
    else if (cmd.op.ends_with ("curveto") && cmd.op.length > 8)
    {
      const char *op_str = cmd.op.c_str ();
      if (strncmp (op_str, "rr", 2) != 0 &&
          strncmp (op_str, "hh", 2) != 0 &&
          strncmp (op_str, "vv", 2) != 0 &&
          strncmp (op_str, "hv", 2) != 0 &&
          strncmp (op_str, "vh", 2) != 0)
      {
        /* Resolve complex curve operators to hh/vv/hv/vh */
        char d0 = op_str[0];
        char d1 = op_str[1];

        if (d0 == '0') d0 = 'h';
        if (d1 == '0') d1 = 'h';

        if (d0 == 'r') d0 = d1;
        if (d1 == 'r') d1 = (d0 == 'h') ? 'v' : 'h';

        char new_op[16];
        snprintf (new_op, sizeof(new_op), "%c%ccurveto", d0, d1);
        cmd.op = new_op;
      }
    }
  }
}

} /* namespace CFF */

#endif /* HB_CFF_SPECIALIZER_HH */
