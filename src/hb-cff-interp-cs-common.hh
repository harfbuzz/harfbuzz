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

struct Point
{
  inline void init (void)
  {
    x.init ();
    y.init ();
  }

  inline void move_x (const Number &dx) { x += dx; }
  inline void move_y (const Number &dy) { y += dy; }
  inline void move (const Number &dx, const Number &dy) { move_x (dx); move_y (dy); }

  Number  x;
  Number  y;
};

template <typename ARG, typename SUBRS>
struct CSInterpEnv : InterpEnv<ARG>
{
  inline void init (const ByteStr &str, const SUBRS &globalSubrs_, const SUBRS &localSubrs_)
  {
    InterpEnv<ARG>::init (str);

    seen_moveto = true;
    seen_hintmask = false;
    hstem_count = 0;
    vstem_count = 0;
    pt.init ();
    callStack.init ();
    globalSubrs.init (globalSubrs_);
    localSubrs.init (localSubrs_);
  }
  inline void fini (void)
  {
    InterpEnv<ARG>::fini ();

    callStack.fini ();
    globalSubrs.fini ();
    localSubrs.fini ();
  }

  inline bool popSubrNum (const BiasedSubrs<SUBRS>& biasedSubrs, unsigned int &subr_num)
  {
    int n;
    if (unlikely ((!callStack.check_overflow (1) ||
                   !SUPER::argStack.check_pop_int (n))))
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
    callStack.push (SUPER::substr);
    SUPER::substr = (*biasedSubrs.subrs)[subr_num];

    return true;
  }

  inline bool returnFromSubr (void)
  {
    if (unlikely (!callStack.check_underflow ()))
      return false;

    SUPER::substr = callStack.pop ();
    return true;
  }

  inline void determine_hintmask_size (void)
  {
    if (!seen_hintmask)
    {
      vstem_count += SUPER::argStack.get_count() / 2;
      hintmask_size = (hstem_count + vstem_count + 7) >> 3;
      seen_hintmask = true;
    }
  }

  inline void set_endchar (bool endchar_flag_) { endchar_flag = endchar_flag_; }
  inline bool is_endchar (void) const { return endchar_flag; }

  inline const Number &get_x (void) const { return pt.x; }
  inline const Number &get_y (void) const { return pt.y; }
  inline const Point &get_pt (void) const { return pt; }

  inline void moveto (const Point &pt_ ) { pt = pt_; }

  inline unsigned int move_x_with_arg (unsigned int i)
  {
    pt.move_x (SUPER::argStack[i]);
    return i + 1;
  }

  inline unsigned int move_y_with_arg (unsigned int i)
  {
    pt.move_y (SUPER::argStack[i]);
    return i + 1;
  }

  inline unsigned int move_xy_with_arg (unsigned int i)
  {
    pt.move (SUPER::argStack[i], SUPER::argStack[i+1]);
    return i + 2;
  }

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

  private:
  Point         pt;

  typedef InterpEnv<ARG> SUPER;
};

template <typename ENV, typename PARAM>
struct PathProcsNull
{
  static inline void rmoveto (ENV &env, PARAM& param) {}
  static inline void hmoveto (ENV &env, PARAM& param) {}
  static inline void vmoveto (ENV &env, PARAM& param) {}
  static inline void rlineto (ENV &env, PARAM& param) {}
  static inline void hlineto (ENV &env, PARAM& param) {}
  static inline void vlineto (ENV &env, PARAM& param) {}
  static inline void rrcurveto (ENV &env, PARAM& param) {}
  static inline void rcurveline (ENV &env, PARAM& param) {}
  static inline void rlinecurve (ENV &env, PARAM& param) {}
  static inline void vvcurveto (ENV &env, PARAM& param) {}
  static inline void hhcurveto (ENV &env, PARAM& param) {}
  static inline void vhcurveto (ENV &env, PARAM& param) {}
  static inline void hvcurveto (ENV &env, PARAM& param) {}
  static inline void moveto (ENV &env, PARAM& param, const Point &pt) {}
  static inline void line (ENV &env, PARAM& param, const Point &pt1) {}
  static inline void curve (ENV &env, PARAM& param, const Point &pt1, const Point &pt2, const Point &pt3) {}
};

template <typename ARG, typename OPSET, typename ENV, typename PARAM, typename PATH=PathProcsNull<ENV, PARAM> >
struct CSOpSet : OpSet<ARG>
{
  static inline bool process_op (OpCode op, ENV &env, PARAM& param)
  {  
    switch (op) {

      case OpCode_return:
        return env.returnFromSubr ();
      case OpCode_endchar:
        env.set_endchar (true);
        OPSET::flush_args_and_op (op, env, param);
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
      case OpCode_rmoveto:
        PATH::rmoveto (env, param);
        process_post_move (op, env, param);
        break;
      case OpCode_hmoveto:
        PATH::hmoveto (env, param);
        process_post_move (op, env, param);
        break;
      case OpCode_vmoveto:
        PATH::vmoveto (env, param);
        process_post_move (op, env, param);
        break;
      case OpCode_rlineto:
        PATH::rlineto (env, param);
        process_post_path (op, env, param);
        break;
      case OpCode_hlineto:
        PATH::hlineto (env, param);
        process_post_path (op, env, param);
        break;
      case OpCode_vlineto:
        PATH::vlineto (env, param);
        process_post_path (op, env, param);
        break;
      case OpCode_rrcurveto:
        PATH::rrcurveto (env, param);
        process_post_path (op, env, param);
        break;
      case OpCode_rcurveline:
        PATH::rcurveline (env, param);
        process_post_path (op, env, param);
        break;
      case OpCode_rlinecurve:
        PATH::rlinecurve (env, param);
        process_post_path (op, env, param);
        break;
      case OpCode_vvcurveto:
        PATH::vvcurveto (env, param);
        process_post_path (op, env, param);
        break;
      case OpCode_hhcurveto:
        PATH::hhcurveto (env, param);
        process_post_path (op, env, param);
        break;
      case OpCode_vhcurveto:
        PATH::vhcurveto (env, param);
        process_post_path (op, env, param);
        break;
      case OpCode_hvcurveto:
        PATH::hvcurveto (env, param);
        process_post_path (op, env, param);
        break;

      case OpCode_hflex:
      case OpCode_flex:
      case OpCode_hflex1:
      case OpCode_flex1:
        OPSET::process_flex (op, env, param);
        break;

      default:
        return SUPER::process_op (op, env);
    }
    return true;
  }

  static inline void process_hstem (OpCode op, ENV &env, PARAM& param)
  {
    env.hstem_count += env.argStack.get_count () / 2;
    OPSET::flush_args_and_op (op, env, param);
  }

  static inline void process_vstem (OpCode op, ENV &env, PARAM& param)
  {
    env.vstem_count += env.argStack.get_count () / 2;
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

  static inline void process_post_move (OpCode op, ENV &env, PARAM& param)
  {
    if (!env.seen_moveto)
    {
      env.determine_hintmask_size ();
      env.seen_moveto = true;
    }
    OPSET::flush_args_and_op (op, env, param);
  }

  static inline void process_post_path (OpCode op, ENV &env, PARAM& param)
  {
    OPSET::flush_args_and_op (op, env, param);
  }

  static inline void flush_args_and_op (OpCode op, ENV &env, PARAM& param, unsigned int start_arg = 0)
  {
    OPSET::flush_args (env, param, start_arg);
    OPSET::flush_op (op, env, param);
  }

  static inline void flush_args (ENV &env, PARAM& param, unsigned int start_arg = 0)
  {
    env.pop_n_args (env.argStack.get_count () - start_arg);
  }

  static inline void flush_op (OpCode op, ENV &env, PARAM& param)
  {
  }

  static inline void flush_hintmask (OpCode op, ENV &env, PARAM& param)
  {
    OPSET::flush_args_and_op (op, env, param);
  }

  protected:
  typedef OpSet<ARG>  SUPER;
};

template <typename PATH, typename ENV, typename PARAM>
struct PathProcs
{
  static inline void rmoveto (ENV &env, PARAM& param)
  {
    Point pt1 = env.get_pt ();
    const Number &dy = env.argStack.pop ();
    const Number &dx = env.argStack.pop ();
    pt1.move (dx, dy);
    PATH::moveto (env, param, pt1);
  }

  static inline void hmoveto (ENV &env, PARAM& param)
  {
    Point pt1 = env.get_pt ();
    pt1.move_x (env.argStack.pop ());
    PATH::moveto (env, param, pt1);
  }

  static inline void vmoveto (ENV &env, PARAM& param)
  {
    Point pt1 = env.get_pt ();
    pt1.move_y (env.argStack.pop ());
    PATH::moveto (env, param, pt1);
  }

  static inline void rlineto (ENV &env, PARAM& param)
  {
    for (unsigned int i = 0; i + 2 <= env.argStack.get_count (); i += 2)
    {
      Point pt1 = env.get_pt ();
      pt1.move (env.argStack[i], env.argStack[i+1]);
      PATH::line (env, param, pt1);
    }
  }
  
  static inline void hlineto (ENV &env, PARAM& param)
  {
    Point pt1;
    unsigned int i = 0;
    for (; i + 2 <= env.argStack.get_count (); i += 2)
    {
      pt1 = env.get_pt ();
      pt1.move_x (env.argStack[i]);
      PATH::line (env, param, pt1);
      pt1.move_y (env.argStack[i+1]);
      PATH::line (env, param, pt1);
    }
    if (i < env.argStack.get_count ())
    {
      pt1 = env.get_pt ();
      pt1.move_x (env.argStack[i]);
      PATH::line (env, param, pt1);
    }
  }

  static inline void vlineto (ENV &env, PARAM& param)
  {
    Point pt1;
    unsigned int i = 0;
    for (; i + 2 <= env.argStack.get_count (); i += 2)
    {
      pt1 = env.get_pt ();
      pt1.move_y (env.argStack[i]);
      PATH::line (env, param, pt1);
      pt1.move_x (env.argStack[i+1]);
      PATH::line (env, param, pt1);
    }
    if (i < env.argStack.get_count ())
    {
      pt1 = env.get_pt ();
      pt1.move_y (env.argStack[i]);
      PATH::line (env, param, pt1);
    }
  }

  static inline void rrcurveto (ENV &env, PARAM& param)
  {
    for (unsigned int i = 0; i + 6 <= env.argStack.get_count (); i += 6)
    {
      Point pt1 = env.get_pt ();
      pt1.move (env.argStack[i], env.argStack[i+1]);
      Point pt2 = pt1;
      pt2.move (env.argStack[i+2], env.argStack[i+3]);
      Point pt3 = pt2;
      pt3.move (env.argStack[i+4], env.argStack[i+5]);
      PATH::curve (env, param, pt1, pt2, pt3);
    }
  }

  static inline void rcurveline (ENV &env, PARAM& param)
  {
    unsigned int i = 0;
    for (; i + 6 <= env.argStack.get_count (); i += 6)
    {
      Point pt1 = env.get_pt ();
      pt1.move (env.argStack[i], env.argStack[i+1]);
      Point pt2 = pt1;
      pt2.move (env.argStack[i+2], env.argStack[i+3]);
      Point pt3 = pt2;
      pt3.move (env.argStack[i+4], env.argStack[i+5]);
      PATH::curve (env, param, pt1, pt2, pt3);
    }
    for (; i + 2 <= env.argStack.get_count (); i += 2)
    {
      Point pt1 = env.get_pt ();
      pt1.move (env.argStack[i], env.argStack[i+1]);
      PATH::line (env, param, pt1);
    }
  }

  static inline void rlinecurve (ENV &env, PARAM& param)
  {
    unsigned int i = 0;
    unsigned int line_limit = (env.argStack.get_count () % 6);
    for (; i + 2 <= line_limit; i += 2)
    {
      Point pt1 = env.get_pt ();
      pt1.move (env.argStack[i], env.argStack[i+1]);
      PATH::line (env, param, pt1);
    }
    for (; i + 6 <= env.argStack.get_count (); i += 6)
    {
      Point pt1 = env.get_pt ();
      pt1.move (env.argStack[i], env.argStack[i+1]);
      Point pt2 = pt1;
      pt2.move (env.argStack[i+2], env.argStack[i+3]);
      Point pt3 = pt2;
      pt3.move (env.argStack[i+4], env.argStack[i+5]);
      PATH::curve (env, param, pt1, pt2, pt3);
    }
  }

  static inline void vvcurveto (ENV &env, PARAM& param)
  {
    unsigned int i = 0;
    Point pt1 = env.get_pt ();
    if ((env.argStack.get_count () & 1) != 0)
      pt1.move_x (env.argStack[i++]);
    for (; i + 4 <= env.argStack.get_count (); i += 4)
    {
      pt1.move_y (env.argStack[i]);
      Point pt2 = pt1;
      pt2.move (env.argStack[i+1], env.argStack[i+2]);
      Point pt3 = pt2;
      pt3.move_y (env.argStack[i+3]);
      PATH::curve (env, param, pt1, pt2, pt3);
      pt1 = env.get_pt ();
    }
  }

  static inline void hhcurveto (ENV &env, PARAM& param)
  {
    unsigned int i = 0;
    Point pt1 = env.get_pt ();
    if ((env.argStack.get_count () & 1) != 0)
      pt1.move_y (env.argStack[i++]);
    for (; i + 4 <= env.argStack.get_count (); i += 4)
    {
      pt1.move_x (env.argStack[i]);
      Point pt2 = pt1;
      pt2.move (env.argStack[i+1], env.argStack[i+2]);
      Point pt3 = pt2;
      pt3.move_x (env.argStack[i+3]);
      PATH::curve (env, param, pt1, pt2, pt3);
      pt1 = env.get_pt ();
    }
  }

  static inline void vhcurveto (ENV &env, PARAM& param)
  {
    Point pt1, pt2, pt3;
    unsigned int i = 0;
    if ((env.argStack.get_count () % 8) >= 4)
    {
      Point pt1 = env.get_pt ();
      pt1.move_y (env.argStack[i]);
      Point pt2 = pt1;
      pt2.move (env.argStack[i+1], env.argStack[i+2]);
      Point pt3 = pt2;
      pt3.move_x (env.argStack[i+3]);
      i += 4;

      for (; i + 8 <= env.argStack.get_count (); i += 8)
      {
        PATH::curve (env, param, pt1, pt2, pt3);
        pt1 = env.get_pt ();
        pt1.move_x (env.argStack[i]);
        pt2 = pt1;
        pt2.move (env.argStack[i+1], env.argStack[i+2]);
        pt3 = pt2;
        pt3.move_y (env.argStack[i+3]);
        PATH::curve (env, param, pt1, pt2, pt3);

        pt1 = pt3;
        pt1.move_y (env.argStack[i+4]);
        pt2 = pt1;
        pt2.move (env.argStack[i+5], env.argStack[i+6]);
        pt3 = pt2;
        pt3.move_x (env.argStack[i+7]);
      }
      if (i < env.argStack.get_count ())
        pt3.move_y (env.argStack[i]);
      PATH::curve (env, param, pt1, pt2, pt3);
    }
    else
    {
      for (; i + 8 <= env.argStack.get_count (); i += 8)
      {
        pt1 = env.get_pt ();
        pt1.move_y (env.argStack[i]);
        pt2 = pt1;
        pt2.move (env.argStack[i+1], env.argStack[i+2]);
        pt3 = pt2;
        pt3.move_x (env.argStack[i+3]);
        PATH::curve (env, param, pt1, pt2, pt3);

        pt1 = pt3;
        pt1.move_x (env.argStack[i+4]);
        pt2 = pt1;
        pt2.move (env.argStack[i+5], env.argStack[i+6]);
        pt3 = pt2;
        pt3.move_y (env.argStack[i+7]);
        if ((env.argStack.get_count () - i < 16) && ((env.argStack.get_count () & 1) != 0))
          pt3.move_x (env.argStack[i+8]);
        PATH::curve (env, param, pt1, pt2, pt3);
      }
    }
  }

  static inline void hvcurveto (ENV &env, PARAM& param)
  {
    Point pt1, pt2, pt3;
    unsigned int i = 0;
    if ((env.argStack.get_count () % 8) >= 4)
    {
      Point pt1 = env.get_pt ();
      pt1.move_x (env.argStack[i]);
      Point pt2 = pt1;
      pt2.move (env.argStack[i+1], env.argStack[i+2]);
      Point pt3 = pt2;
      pt3.move_y (env.argStack[i+3]);
      i += 4;

      for (; i + 8 <= env.argStack.get_count (); i += 8)
      {
        PATH::curve (env, param, pt1, pt2, pt3);
        pt1 = env.get_pt ();
        pt1.move_y (env.argStack[i]);
        pt2 = pt1;
        pt2.move (env.argStack[i+1], env.argStack[i+2]);
        pt3 = pt2;
        pt3.move_x (env.argStack[i+3]);
        PATH::curve (env, param, pt1, pt2, pt3);

        pt1 = pt3;
        pt1.move_x (env.argStack[i+4]);
        pt2 = pt1;
        pt2.move (env.argStack[i+5], env.argStack[i+6]);
        pt3 = pt2;
        pt3.move_y (env.argStack[i+7]);
      }
      if (i < env.argStack.get_count ())
        pt3.move_x (env.argStack[i]);
      PATH::curve (env, param, pt1, pt2, pt3);
    }
    else
    {
      for (; i + 8 <= env.argStack.get_count (); i += 8)
      {
        pt1 = env.get_pt ();
        pt1.move_x (env.argStack[i]);
        pt2 = pt1;
        pt2.move (env.argStack[i+1], env.argStack[i+2]);
        pt3 = pt2;
        pt3.move_y (env.argStack[i+3]);
        PATH::curve (env, param, pt1, pt2, pt3);

        pt1 = pt3;
        pt1.move_y (env.argStack[i+4]);
        pt2 = pt1;
        pt2.move (env.argStack[i+5], env.argStack[i+6]);
        pt3 = pt2;
        pt3.move_x (env.argStack[i+7]);
        if ((env.argStack.get_count () - i < 16) && ((env.argStack.get_count () & 1) != 0))
          pt3.move_y (env.argStack[i+8]);
        PATH::curve (env, param, pt1, pt2, pt3);
      }
    }
  }

  /* default actions to be overridden */
  static inline void moveto (ENV &env, PARAM& param, const Point &pt)
  { env.moveto (pt); }

  static inline void line (ENV &env, PARAM& param, const Point &pt1)
  { PATH::moveto (env, param, pt1); }

  static inline void curve (ENV &env, PARAM& param, const Point &pt1, const Point &pt2, const Point &pt3)
  { PATH::moveto (env, param, pt3); }
};

template <typename ENV, typename OPSET, typename PARAM>
struct CSInterpreter : Interpreter<ENV>
{
  inline bool interpret (PARAM& param)
  {
    SUPER::env.set_endchar (false);

    for (;;) {
      OpCode op;
      if (unlikely (!SUPER::env.fetch_op (op) ||
                    !OPSET::process_op (op, SUPER::env, param)))
        return false;
      if (SUPER::env.is_endchar ())
        break;
    }
    
    return true;
  }

  private:
  typedef Interpreter<ENV> SUPER;
};

} /* namespace CFF */

#endif /* HB_CFF_INTERP_CS_COMMON_HH */
