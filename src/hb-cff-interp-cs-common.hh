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
#ifndef HB_CFF_INTERP_CS_COMMON_HH
#define HB_CFF_INTERP_CS_COMMON_HH

#include "hb.hh"
#include "hb-cff-interp-common.hh"

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

    seen_moveto = true;
    seen_hintmask = false;
    hstem_count = 0;
    vstem_count = 0;
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
    if (unlikely (!callStack.check_underflow ()))
      return false;

    substr = callStack.pop ();
    return true;
  }

  inline void determine_hintmask_size (void)
  {
    if (!seen_hintmask)
    {
      vstem_count += argStack.get_count() / 2;
      hintmask_size = (hstem_count + vstem_count + 7) >> 3;
      seen_hintmask = true;
    }
  }

  inline void set_endchar (bool endchar_flag_) { endchar_flag = endchar_flag_; }
  inline bool is_endchar (void) const { return endchar_flag; }

  public:
  bool          endchar_flag;
  bool          seen_moveto;
  bool          seen_hintmask;

  unsigned int  hstem_count;
  unsigned int  vstem_count;
  unsigned int  hintmask_size;
  CallStack            callStack;
  BiasedSubrs<SUBRS>   globalSubrs;
  BiasedSubrs<SUBRS>   localSubrs;
};

template <typename OPSET, typename ENV, typename PARAM>
struct CSOpSet : OpSet
{
  static inline bool process_op (OpCode op, ENV &env, PARAM& param)
  {  
    switch (op) {

      case OpCode_return:
        return env.returnFromSubr ();
      case OpCode_endchar:
        env.set_endchar (true);
        OPSET::flush_op (op, env, param);
        break;

      case OpCode_fixedcs:
        return env.argStack.push_fixed_from_substr (env.substr);

      case OpCode_callsubr:
        return env.callSubr (env.localSubrs);
      
      case OpCode_callgsubr:
        return env.callSubr (env.globalSubrs);

      case OpCode_hstem:
      case OpCode_hstemhm:
        OPSET::process_hstem (op, env, param);
        break;
      case OpCode_vstem:
      case OpCode_vstemhm:
        OPSET::process_vstem (op, env, param);
        break;
      case OpCode_hintmask:
      case OpCode_cntrmask:
        OPSET::process_hintmask (op, env, param);
        break;
      
      case OpCode_vmoveto:
      case OpCode_rlineto:
      case OpCode_hlineto:
      case OpCode_vlineto:
      case OpCode_rmoveto:
      case OpCode_hmoveto:
        OPSET::process_moveto (op, env, param);
        break;
      case OpCode_rrcurveto:
      case OpCode_rcurveline:
      case OpCode_rlinecurve:
      case OpCode_vvcurveto:
      case OpCode_hhcurveto:
      case OpCode_vhcurveto:
      case OpCode_hvcurveto:
        OPSET::process_path (op, env, param);
        break;

      case OpCode_hflex:
      case OpCode_flex:
      case OpCode_hflex1:
      case OpCode_flex1:
        OPSET::process_flex (op, env, param);
        break;

      default:
        return OpSet::process_op (op, env);
    }
    return true;
  }

  static inline void process_hstem (OpCode op, ENV &env, PARAM& param)
  {
    env.hstem_count += env.argStack.count / 2;
    OPSET::flush_args_and_op (op, env, param);
  }

  static inline void process_vstem (OpCode op, ENV &env, PARAM& param)
  {
    env.vstem_count += env.argStack.count / 2;
    OPSET::flush_args_and_op (op, env, param);
  }

  static inline void process_hintmask (OpCode op, ENV &env, PARAM& param)
  {
    env.determine_hintmask_size ();
    if (likely (env.substr.avail (env.hintmask_size)))
    {
      OPSET::flush_hintmask (op, env, param);
      env.substr.inc (env.hintmask_size);
    }
  }

  static inline void process_flex (OpCode op, ENV &env, PARAM& param)
  {
    OPSET::flush_args_and_op (op, env, param);
  }

  static inline void process_moveto (OpCode op, ENV &env, PARAM& param)
  {
    if (!env.seen_moveto)
    {
      env.determine_hintmask_size ();
      env.seen_moveto = true;
    }
    OPSET::flush_args_and_op (op, env, param);
  }

  static inline void process_path (OpCode op, ENV &env, PARAM& param)
  {
    OPSET::flush_args_and_op (op, env, param);
  }

  static inline void flush_args_and_op (OpCode op, ENV &env, PARAM& param)
  {
    OPSET::flush_n_args_and_op (op, env.argStack.count, env, param);
  }

  static inline void flush_n_args_and_op (OpCode op, unsigned int n, ENV &env, PARAM& param)
  {
    OPSET::flush_n_args (n, env, param);
    OPSET::flush_op (op, env, param);
  }

  static inline void flush_args (ENV &env, PARAM& param)
  {
    OPSET::flush_n_args (env.argStack.count, env, param);
  }

  static inline void flush_n_args (unsigned int n, ENV &env, PARAM& param)
  {
    env.pop_n_args (n);
  }

  static inline void flush_op (OpCode op, ENV &env, PARAM& param)
  {
  }

  static inline void flush_hintmask (OpCode op, ENV &env, PARAM& param)
  {
    OPSET::flush_args_and_op (op, env, param);
  }

  static inline bool is_subr_op (OpCode op)
  {
    switch (op)
    {
      case OpCode_callsubr:
      case OpCode_callgsubr:
      case OpCode_return:
        return true;
      default:
        return false;
    }
  }
};

template <typename ENV, typename OPSET, typename PARAM>
struct CSInterpreter : Interpreter<ENV>
{
  inline bool interpret (PARAM& param)
  {
    Interpreter<ENV> &super = *this;
    super.env.set_endchar (false);

    for (;;) {
      OpCode op;
      if (unlikely (!super.env.fetch_op (op) ||
                    !OPSET::process_op (op, super.env, param)))
        return false;
      if (super.env.is_endchar ())
        break;
    }
    
    return true;
  }
};

} /* namespace CFF */

#endif /* HB_CFF_INTERP_CS_COMMON_HH */
