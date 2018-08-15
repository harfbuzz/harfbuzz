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
#ifndef HB_CFF_INTERP_CS_COMMON_PRIVATE_HH
#define HB_CFF_INTERP_CS_COMMON_PRIVATE_HH

#include "hb-private.hh"
#include "hb-cff-interp-common-private.hh"

namespace CFF {

using namespace OT;

/* call stack */
struct CallStack : Stack<SubByteStr, 10> {};

template <typename SUBRS>
struct BiasedSubrs
{
  inline void init (const SUBRS &subrs_)
  {
    subrs = &subrs_;
    unsigned int  nSubrs = subrs_.count;
    if (nSubrs < 1240)
      bias = 107;
    else if (nSubrs < 33900)
      bias = 1131;
    else
      bias = 32768;
  }

  inline void fini (void) {}

  const SUBRS   *subrs;
  unsigned int  bias;
};

template <typename SUBRS>
struct CSInterpEnv : InterpEnv
{
  inline void init (const ByteStr &str, const SUBRS &globalSubrs_, const SUBRS &localSubrs_)
  {
    InterpEnv::init (str);

    callStack.init ();
    globalSubrs.init (globalSubrs_);
    localSubrs.init (localSubrs_);
  }
  inline void fini (void)
  {
    InterpEnv::fini ();

    callStack.fini ();
    globalSubrs.fini ();
    localSubrs.fini ();
  }

  inline bool popSubrNum (const BiasedSubrs<SUBRS>& biasedSubrs, unsigned int &subr_num)
  {
    int n;
    if (unlikely ((!callStack.check_overflow (1) ||
                   !argStack.check_pop_int (n))))
      return false;
    n += biasedSubrs.bias;
    if (unlikely ((n < 0) || (n >= biasedSubrs.subrs->count)))
      return false;

    subr_num = (unsigned int)n;
    return true;
  }

  inline bool callSubr (const BiasedSubrs<SUBRS>& biasedSubrs)
  {
    unsigned int subr_num;

    if (unlikely (!popSubrNum (biasedSubrs, subr_num)))
      return false;
    callStack.push (substr);
    substr = (*biasedSubrs.subrs)[subr_num];

    return true;
  }

  inline bool returnFromSubr (void)
  {
    if (unlikely (!callStack.check_underflow (1)))
      return false;

    substr = callStack.pop ();
    return true;
  }

  inline void set_endchar (bool endchar_flag_) { endchar_flag = endchar_flag_; }
  inline bool is_endchar (void) const { return endchar_flag; }

  protected:
  bool              endchar_flag;

  public:
  CallStack            callStack;
  BiasedSubrs<SUBRS>   globalSubrs;
  BiasedSubrs<SUBRS>   localSubrs;
};

template <typename SUBRS, typename PARAM>
struct CSOpSet : OpSet
{
  static inline bool process_op (OpCode op, CSInterpEnv<SUBRS> &env, PARAM& param)
  {
    switch (op) {

      case OpCode_longintcs:
        return env.argStack.push_longint_from_substr (env.substr);

      case OpCode_callsubr:
        return env.callSubr (env.localSubrs);
      
      case OpCode_callgsubr:
        return env.callSubr (env.globalSubrs);

      default:
        return OpSet::process_op (op, env);
    }
  }
};

template <typename ENV, typename OPSET, typename PARAM>
struct CSInterpreter : Interpreter<ENV>
{
  inline bool interpret (PARAM& param)
  {
    param.init ();
    Interpreter<ENV> &super = *this;
    super.env.set_endchar (false);

    for (;;) {
      OpCode op;
      if (unlikely (!super.fetch_op (op) ||
                    !OPSET::process_op (op, super.env, param)))
        return false;
      if (super.env.is_endchar ())
        break;
      if (!super.env.substr.avail ())
        return false;
    }
    
    return true;
  }
};

} /* namespace CFF */

#endif /* HB_CFF_INTERP_CS_COMMON_PRIVATE_HH */
