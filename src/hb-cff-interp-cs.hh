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
#ifndef HB_CFF_INTERP_CS_HH
#define HB_CFF_INTERP_CS_HH

#include "hb-private.hh"
#include "hb-cff-interp-cs.hh"

namespace CFF {

using namespace OT;

struct CFFCSInterpEnv : CSInterpEnv<CFFSubrs>
{
  inline void init (const ByteStr &str, const CFFSubrs &globalSubrs, const CFFSubrs &localSubrs)
  {
    CSInterpEnv<CFFSubrs>::init (str, globalSubrs, localSubrs);
    seen_width = false;
    seen_moveto = true;
    seen_hintmask = false;
    hstem_count = 0;
    vstem_count = 0;
    for (unsigned int i = 0; i < kTransientArraySize; i++)
      transient_array[i].set_int (0);
  }

  bool check_transient_array_index (unsigned int i) const
  { return i < kTransientArraySize; }

  inline void determine_hintmask_size (void)
  {
    if (!seen_hintmask)
    {
      vstem_count += argStack.size / 2;
      hintmask_size = (hstem_count + vstem_count + 7) >> 3;
      seen_hintmask = true;
    }
    clear_stack ();
  }

  inline void process_moveto (void)
  {
    clear_stack ();

    if (!seen_moveto)
    {
      determine_hintmask_size ();
      seen_moveto = true;
    }
  }

  inline void clear_stack (void)
  {
    seen_width = true;
    argStack.clear ();
  }

  inline void process_width (void)
  {
    if (!seen_width && (argStack.size > 0))
    {
      assert (argStack.size == 1);
      width = argStack.pop ();
      seen_width = true;
    }
  }

  bool          seen_width;
  Number        width;
  bool          seen_moveto;
  bool          seen_hintmask;
  unsigned int  hintmask_size;
  unsigned int  hstem_count;
  unsigned int  vstem_count;

  static const unsigned int kTransientArraySize = 32;
  Number  transient_array[kTransientArraySize];
};

template <typename PARAM>
struct CFFCSOpSet : CSOpSet<CFFSubrs, PARAM>
{
  static inline bool process_op (OpCode op, CFFCSInterpEnv &env, PARAM& param)
  {
    Number  n1, n2;

    switch (op) {

      case OpCode_return:
        return env.returnFromSubr ();
      case OpCode_endchar:
        env.set_endchar (true);
        return true;
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
          env.argStack.push_real (((float)rand() + 1) / ((float)RAND_MAX + 1));
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
          if (unlikely (i >= env.argStack.size || !env.argStack.check_overflow (1))) return false;
          env.argStack.push (env.argStack.elements[env.argStack.size - i - 1]);
        }
        break;
      case OpCode_roll:
        {
          if (unlikely (!env.argStack.check_pop_num2 (n1, n2))) return false;
          int n = n1.to_int ();
          int j = n2.to_int ();
          if (unlikely (n < 0 || n > env.argStack.size)) return false;
          if (likely (n > 0))
          {
            if (j < 0)
              j = n - (-j % n);
            j %= n;
            unsigned int top = env.argStack.size - 1;
            unsigned int bot = top - n + 1;
            env.argStack.reverse_range (top - j + 1, top);
            env.argStack.reverse_range (bot, top - j);
            env.argStack.reverse_range (bot, top);
          }
        }
        break;
      case OpCode_hstem:
      case OpCode_vstem:
        env.clear_stack ();
        break;
      case OpCode_hstemhm:
        env.hstem_count += env.argStack.size / 2;
        env.clear_stack ();
        break;
      case OpCode_vstemhm:
        env.vstem_count += env.argStack.size / 2;
        env.clear_stack ();
        break;
      case OpCode_hintmask:
      case OpCode_cntrmask:
        env.determine_hintmask_size ();
        if (unlikely (!env.substr.avail (env.hintmask_size)))
          return false;
        env.substr.inc (env.hintmask_size);
        break;
      
      case OpCode_vmoveto:
      case OpCode_rlineto:
      case OpCode_hlineto:
      case OpCode_vlineto:
      case OpCode_rmoveto:
      case OpCode_hmoveto:
        env.process_moveto ();
        break;
      case OpCode_rrcurveto:
      case OpCode_rcurveline:
      case OpCode_rlinecurve:
      case OpCode_vvcurveto:
      case OpCode_hhcurveto:
      case OpCode_vhcurveto:
      case OpCode_hvcurveto:
      case OpCode_hflex:
      case OpCode_flex:
      case OpCode_hflex1:
      case OpCode_flex1:
        env.clear_stack ();
        break;
      default:
        typedef CSOpSet<CFFSubrs, PARAM>  SUPER;
        if (unlikely (!SUPER::process_op (op, env, param)))
          return false;
        env.process_width ();
        break;
    }
    return true;
  }
};

template <typename OPSET, typename PARAM>
struct CFFCSInterpreter : CSInterpreter<CFFCSInterpEnv, OPSET, PARAM> {};

} /* namespace CFF */

#endif /* HB_CFF_INTERP_CS_HH */
