/*
 * Copyright © 2018 Adobe Systems Incorporated.
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

namespace CFF {

using namespace OT;

/* an opstr and the parsed out dict value(s) */
template <typename ARG=Number>
struct DictVal : OpStr
{
  inline void init (void)
  {
    single_val.set_int (0);
    multi_val.init ();
  }

  inline void fini (void)
  {
    multi_val.fini ();
  }

  ARG              single_val;
  hb_vector_t<ARG> multi_val;
};

typedef DictVal<> NumDictVal;

template <typename VAL>
struct DictValues
{
  inline void init (void)
  {
    opStart = 0;
    values.init ();
  }

  inline void fini (void)
  {
    values.fini ();
  }

  inline void pushVal (OpCode op, const SubByteStr& substr)
  {
    VAL *val = values.push ();
    val->op = op;
    val->str = ByteStr (substr.str, opStart, substr.offset - opStart);
    opStart = substr.offset;
  }

  inline void pushVal (OpCode op, const SubByteStr& substr, const VAL &v)
  {
    VAL *val = values.push (v);
    val->op = op;
    val->str = ByteStr (substr.str, opStart, substr.offset - opStart);
    opStart = substr.offset;
  }

  unsigned int       opStart;
  hb_vector_t<VAL>   values;
};

struct TopDictValues : DictValues<OpStr>
{
  inline void init (void)
  {
    DictValues<OpStr>::init ();
    charStringsOffset = 0;
    FDArrayOffset = 0;
  }

  inline void fini (void)
  {
    DictValues<OpStr>::fini ();
  }

  inline unsigned int calculate_serialized_op_size (const OpStr& opstr) const
  {
    switch (opstr.op)
    {
      case OpCode_CharStrings:
      case OpCode_FDArray:
        return OpCode_Size (OpCode_longintdict) + 4 + OpCode_Size (opstr.op);

      default:
        return opstr.str.len;
    }
  }

  unsigned int  charStringsOffset;
  unsigned int  FDArrayOffset;
};

template <typename ARG=Number>
struct DictOpSet : OpSet<ARG>
{
  static inline bool process_op (OpCode op, InterpEnv<ARG>& env)
  {
    switch (op) {
      case OpCode_longintdict:  /* 5-byte integer */
        return env.argStack.push_longint_from_substr (env.substr);
      case OpCode_BCD:  /* real number */
        float v;
        if (unlikely (!env.argStack.check_overflow (1) || !parse_bcd (env.substr, v)))
          return false;
        env.argStack.push_real (v);
        return true;

      default:
        return OpSet<ARG>::process_op (op, env);
    }

    return true;
  }

  static inline bool is_hint_op (OpCode op)
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

template <typename ARG=Number>
struct TopDictOpSet : DictOpSet<ARG>
{
  static inline bool process_op (OpCode op, InterpEnv<ARG>& env, TopDictValues& dictval)
  {
    switch (op) {
      case OpCode_CharStrings:
        if (unlikely (!env.argStack.check_pop_uint (dictval.charStringsOffset)))
          return false;
        env.clear_args ();
        break;
      case OpCode_FDArray:
        if (unlikely (!env.argStack.check_pop_uint (dictval.FDArrayOffset)))
          return false;
        env.clear_args ();
        break;
      default:
        return DictOpSet<ARG>::process_op (op, env);
    }

    return true;
  }
};

template <typename OPSET, typename PARAM, typename ENV=NumInterpEnv>
struct DictInterpreter : Interpreter<ENV>
{
  inline bool interpret (PARAM& param)
  {
    param.init ();
    do
    {
      OpCode op;
      if (unlikely (!SUPER::env.fetch_op (op) ||
                    !OPSET::process_op (op, SUPER::env, param)))
        return false;
    } while (SUPER::env.substr.avail ());
    
    return true;
  }

  private:
  typedef Interpreter<ENV> SUPER;
};

} /* namespace CFF */

#endif /* HB_CFF_INTERP_DICT_COMMON_HH */
