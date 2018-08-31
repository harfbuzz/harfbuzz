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
#ifndef HB_CFF1_INTERP_CS_HH
#define HB_CFF1_INTERP_CS_HH

#include "hb.hh"
#include "hb-cff-interp-cs-common.hh"

namespace CFF {

using namespace OT;

struct CFF1CSInterpEnv : CSInterpEnv<Number, CFF1Subrs>
{
  inline void init (const ByteStr &str, const CFF1Subrs &globalSubrs, const CFF1Subrs &localSubrs)
  {
    SUPER::init (str, globalSubrs, localSubrs);
    processed_width = false;
    has_width = false;
    for (unsigned int i = 0; i < kTransientArraySize; i++)
      transient_array[i].set_int (0);
  }

  bool check_transient_array_index (unsigned int i) const
  { return i < kTransientArraySize; }

  inline void check_width (void)
  {
    if (!processed_width)
    {
      if ((SUPER::argStack.count & 1) != 0)
      {
        width = SUPER::argStack.elements[0];
        has_width = true;
      }
      processed_width = true;
    }
  }

  bool          processed_width;
  bool          has_width;
  Number        width;

  static const unsigned int kTransientArraySize = 32;
  Number  transient_array[kTransientArraySize];

  private:
  typedef CSInterpEnv<Number, CFF1Subrs> SUPER;
};

template <typename OPSET, typename PARAM>
struct CFF1CSOpSet : CSOpSet<Number, OPSET, CFF1CSInterpEnv, PARAM>
{
  static inline bool process_op (OpCode op, CFF1CSInterpEnv &env, PARAM& param)
  {
    Number  n1, n2;

    switch (op) {

      case OpCode_and:
        if (unlikely (!env.argStack.check_pop_num2 (n1, n2))) return false;
        env.argStack.push_int ((n1.to_real() != 0.0f) && (n2.to_real() != 0.0f));
        break;
      case OpCode_or:
        if (unlikely (!env.argStack.check_pop_num2 (n1, n2))) return false;
        env.argStack.push_int ((n1.to_real() != 0.0f) || (n2.to_real() != 0.0f));
        break;
      case OpCode_not:
        if (unlikely (!env.argStack.check_pop_num (n1))) return false;
        env.argStack.push_int (n1.to_real() == 0.0f);
        break;
      case OpCode_abs:
        if (unlikely (!env.argStack.check_pop_num (n1)))  return false;
        env.argStack.push_real (fabs(n1.to_real ()));
        break;
      case OpCode_add:
        if (unlikely (!env.argStack.check_pop_num2 (n1, n2))) return false;
        env.argStack.push_real (n1.to_real() + n2.to_real());
        break;
      case OpCode_sub:
        if (unlikely (!env.argStack.check_pop_num2 (n1, n2))) return false;
        env.argStack.push_real (n1.to_real() - n2.to_real());
        break;
      case OpCode_div:
        if (unlikely (!env.argStack.check_pop_num2 (n1, n2))) return false;
        if (unlikely (n2.to_real() == 0.0f))
          env.argStack.push_int (0);
        else
          env.argStack.push_real (n1.to_real() / n2.to_real());
        break;
      case OpCode_neg:
        if (unlikely (!env.argStack.check_pop_num (n1))) return false;
        env.argStack.push_real (-n1.to_real ());
        break;
      case OpCode_eq:
        if (unlikely (!env.argStack.check_pop_num2 (n1, n2))) return false;
        env.argStack.push_int (n1.to_real() == n2.to_real());
        break;
      case OpCode_drop:
        if (unlikely (!env.argStack.check_pop_num (n1))) return false;
        break;
      case OpCode_put:
        if (unlikely (!env.argStack.check_pop_num2 (n1, n2) ||
                      !env.check_transient_array_index (n2.to_int ()))) return false;
        env.transient_array[n2.to_int ()] = n1;
        break;
      case OpCode_get:
        if (unlikely (!env.argStack.check_pop_num (n1) ||
                      !env.check_transient_array_index (n1.to_int ()))) return false;
        env.argStack.push (env.transient_array[n1.to_int ()]);
        break;
      case OpCode_ifelse:
        {
          if (unlikely (!env.argStack.check_pop_num2 (n1, n2))) return false;
          bool  test = n1.to_real () <= n2.to_real ();
          if (unlikely (!env.argStack.check_pop_num2 (n1, n2))) return false;
          env.argStack.push (test? n1: n2);
        }
        break;
      case OpCode_random:
          if (unlikely (!env.argStack.check_overflow (1))) return false;
          env.argStack.push_int (1);  /* we can't deal with random behavior; make it constant */
      case OpCode_mul:
        if (unlikely (!env.argStack.check_pop_num2 (n1, n2))) return false;
        env.argStack.push_real (n1.to_real() * n2.to_real());
        break;
      case OpCode_sqrt:
        if (unlikely (!env.argStack.check_pop_num (n1))) return false;
        env.argStack.push_real ((float)sqrt (n1.to_real ()));
        break;
      case OpCode_dup:
        if (unlikely (!env.argStack.check_pop_num (n1))) return false;
        env.argStack.push (n1);
        env.argStack.push (n1);
        break;
      case OpCode_exch:
        if (unlikely (!env.argStack.check_pop_num2 (n1, n2))) return false;
        env.argStack.push (n2);
        env.argStack.push (n1);
        break;
      case OpCode_index:
        {
          if (unlikely (!env.argStack.check_pop_num (n1))) return false;
          int i = n1.to_int ();
          if (i < 0) i = 0;
          if (unlikely (i >= env.argStack.count || !env.argStack.check_overflow (1))) return false;
          env.argStack.push (env.argStack.elements[env.argStack.count - i - 1]);
        }
        break;
      case OpCode_roll:
        {
          if (unlikely (!env.argStack.check_pop_num2 (n1, n2))) return false;
          int n = n1.to_int ();
          int j = n2.to_int ();
          if (unlikely (n < 0 || n > env.argStack.count)) return false;
          if (likely (n > 0))
          {
            if (j < 0)
              j = n - (-j % n);
            j %= n;
            unsigned int top = env.argStack.count - 1;
            unsigned int bot = top - n + 1;
            env.argStack.reverse_range (top - j + 1, top);
            env.argStack.reverse_range (bot, top - j);
            env.argStack.reverse_range (bot, top);
          }
        }
        break;
      default:
        if (unlikely (!SUPER::process_op (op, env, param)))
          return false;
        break;
    }
    return true;
  }

  static inline void flush_args (CFF1CSInterpEnv &env, PARAM& param)
  {
    env.check_width ();
    SUPER::flush_args (env, param);
  }

  private:
  typedef CSOpSet<Number, OPSET, CFF1CSInterpEnv, PARAM>  SUPER;
};

template <typename OPSET, typename PARAM>
struct CFF1CSInterpreter : CSInterpreter<CFF1CSInterpEnv, OPSET, PARAM> {};

} /* namespace CFF */

#endif /* HB_CFF1_INTERP_CS_HH */
