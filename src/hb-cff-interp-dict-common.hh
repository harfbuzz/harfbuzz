/*
 * Copyright © 2018 Adobe Inc.
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
 * Adobe Author(s): Michiharu Ariza
 */
#ifndef HB_CFF_INTERP_DICT_COMMON_HH
#define HB_CFF_INTERP_DICT_COMMON_HH

#include "hb-cff-interp-common.hh"
#include <math.h>
#include <float.h>

namespace CFF {

using namespace OT;

/* an opstr and the parsed out dict value(s) */
struct dict_val_t : op_str_t
{
  void init () { single_val.set_int (0); }
  void fini () {}

  number_t	      single_val;
};

typedef dict_val_t num_dict_val_t;

template <typename VAL> struct dict_values_t : parsed_values_t<VAL> {};

template <typename OPSTR=op_str_t>
struct top_dict_values_t : dict_values_t<OPSTR>
{
  void init ()
  {
    dict_values_t<OPSTR>::init ();
    charStringsOffset = 0;
    FDArrayOffset = 0;
  }
  void fini () { dict_values_t<OPSTR>::fini (); }

  unsigned int calculate_serialized_op_size (const OPSTR& opstr) const
  {
    switch (opstr.op)
    {
      case OpCode_CharStrings:
      case OpCode_FDArray:
	return OpCode_Size (OpCode_longintdict) + 4 + OpCode_Size (opstr.op);

      default:
	return opstr.str.length;
    }
  }

  unsigned int  charStringsOffset;
  unsigned int  FDArrayOffset;
};

/* Compile time calculating 10^n for n = 2^i */
constexpr double
pow10_of_2i (unsigned int n)
{
  return n == 1 ? 10. : pow10_of_2i (n >> 1) * pow10_of_2i (n >> 1);
}

static const double powers_of_10[] =
{
  pow10_of_2i (0x100),
  pow10_of_2i (0x80),
  pow10_of_2i (0x40),
  pow10_of_2i (0x20),
  pow10_of_2i (0x10),
  pow10_of_2i (0x8),
  pow10_of_2i (0x4),
  pow10_of_2i (0x2),
  pow10_of_2i (0x1),
};

struct float_parser_t
{
  const uint64_t MAX_FRACT = 0xFFFFFFFFFFFFFull; /* 1^52-1 */
  const uint32_t MAX_EXP = 0x7FFu; /* 1^11-1 */

  enum Part { INT_PART=0, FRAC_PART, EXP_PART };

  float_parser_t () : part (INT_PART), value (0), neg (false), int_part (0),
		      frac_part (0), frac_count (0), exp_neg (0), exp_part (0),
		      exp_overflow (false), i (0), in_error (false), exp_start (true) {}

  protected:
  char part;
  double value;
  bool neg;
  double int_part;
  uint64_t frac_part;
  uint32_t frac_count;
  bool exp_neg;
  uint32_t exp_part;
  bool exp_overflow;
  unsigned int i;
  bool in_error;
  bool exp_start;

  /* Works for x < 512 */
  double
  pow10 (unsigned int x)
  {
    unsigned int mask = 0x100; /* Should be same with the first element  */
    unsigned long result = 1;
    const double *power = powers_of_10;
    for (; mask; ++power, mask >>= 1) if (mask & x) result *= *power;
    return result;
  }

  public:
  bool is_in_error () { return in_error; }
  void set_in_error () { in_error = true; }

  void consume (char c)
  {
    switch (c)
    {
    case '-':
      if (i == 0) neg = true;
      else if (exp_start) exp_neg = true;
      else in_error = true;
      break;

    case '.':
      if (part != INT_PART) in_error = true;
      part = FRAC_PART;
      break;

    case 'e':
      if (part == EXP_PART) in_error = true;
      part = EXP_PART;
      exp_start = true;
      break;

    default:
      int d = c - '0';
      if (d < 0 || d > 9)
      {
	in_error = true;
	break;
      }
      switch (part) {
      default:
      case INT_PART:
	int_part = (int_part * 10) + d;
	break;

      case FRAC_PART:
	if (likely (frac_part <= MAX_FRACT / 10))
	{
	  frac_part = (frac_part * 10) + (unsigned)d;
	  frac_count++;
	}
	break;

      case EXP_PART:
	if (likely (exp_part * 10 + d <= MAX_EXP))
	  exp_part = (exp_part * 10) + d;
	else
	  exp_overflow = true;
	break;
      }
      break;
    }

    /* if the current isn't 'e', we have passed exp start already */
    if (c != 'e') exp_start = false;
  }

  double end ()
  {
    if (in_error) return .0;
    value = neg ? -int_part : int_part;
    if (frac_count > 0)
    {
      double frac = frac_part / pow10 (frac_count);
      if (neg) frac = -frac;
	value += frac;
    }
    if (unlikely (exp_overflow))
    {
      if (value == .0)
	return value;
      if (exp_neg)
	return neg ? -DBL_MIN : DBL_MIN;
      else
	return neg ? -DBL_MAX : DBL_MAX;
    }
    if (exp_part != 0)
    {
      if (exp_neg)
	value /= pow10 (exp_part);
      else
	value *= pow10 (exp_part);
    }
    return value;
  }
};

struct dict_opset_t : opset_t<number_t>
{
  static void process_op (op_code_t op, interp_env_t<number_t>& env)
  {
    switch (op) {
      case OpCode_longintdict:  /* 5-byte integer */
	env.argStack.push_longint_from_substr (env.str_ref);
	break;

      case OpCode_BCD:  /* real number */
	env.argStack.push_real (parse_bcd (env.str_ref));
	break;

      default:
	opset_t<number_t>::process_op (op, env);
	break;
    }
  }

  static double parse_bcd (byte_str_ref_t& str_ref)
  {
    float_parser_t parser;

    enum Nibble { DECIMAL=10, EXP_POS, EXP_NEG, RESERVED, NEG, END };

    unsigned char byte = 0;
    for (uint32_t i = 0;; i++)
    {
      char nibble;
      if ((i & 1) == 0)
      {
	if (!str_ref.avail ())
	{
	  str_ref.set_error ();
	  return 0.0;
	}
	byte = str_ref[0];
	str_ref.inc ();
	nibble = byte >> 4;
      }
      else
	nibble = byte & 0x0F;

      switch (nibble)
      {
      case RESERVED:
	parser.set_in_error ();
	break;

      case END:
	goto end;

      case NEG:
	parser.consume ('-');
	break;

      case DECIMAL:
	parser.consume ('.');
	break;

      case EXP_POS:
	parser.consume ('e');
	break;

      case EXP_NEG:
	parser.consume ('e');
	parser.consume ('-');
	break;

      default:
	parser.consume ('0' + nibble);
	break;
      }

      if (parser.is_in_error ()) break;
    }

end:
    if (parser.is_in_error ()) str_ref.set_error ();
    return parser.end ();
  }

  static bool is_hint_op (op_code_t op)
  {
    switch (op)
    {
      case OpCode_BlueValues:
      case OpCode_OtherBlues:
      case OpCode_FamilyBlues:
      case OpCode_FamilyOtherBlues:
      case OpCode_StemSnapH:
      case OpCode_StemSnapV:
      case OpCode_StdHW:
      case OpCode_StdVW:
      case OpCode_BlueScale:
      case OpCode_BlueShift:
      case OpCode_BlueFuzz:
      case OpCode_ForceBold:
      case OpCode_LanguageGroup:
      case OpCode_ExpansionFactor:
	return true;
      default:
	return false;
    }
  }
};

template <typename VAL=op_str_t>
struct top_dict_opset_t : dict_opset_t
{
  static void process_op (op_code_t op, interp_env_t<number_t>& env, top_dict_values_t<VAL> & dictval)
  {
    switch (op) {
      case OpCode_CharStrings:
	dictval.charStringsOffset = env.argStack.pop_uint ();
	env.clear_args ();
	break;
      case OpCode_FDArray:
	dictval.FDArrayOffset = env.argStack.pop_uint ();
	env.clear_args ();
	break;
      case OpCode_FontMatrix:
	env.clear_args ();
	break;
      default:
	dict_opset_t::process_op (op, env);
	break;
    }
  }
};

template <typename OPSET, typename PARAM, typename ENV=num_interp_env_t>
struct dict_interpreter_t : interpreter_t<ENV>
{
  bool interpret (PARAM& param)
  {
    param.init ();
    while (SUPER::env.str_ref.avail ())
    {
      OPSET::process_op (SUPER::env.fetch_op (), SUPER::env, param);
      if (unlikely (SUPER::env.in_error ()))
	return false;
    }

    return true;
  }

  private:
  typedef interpreter_t<ENV> SUPER;
};

} /* namespace CFF */

#endif /* HB_CFF_INTERP_DICT_COMMON_HH */
