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
#ifndef HB_CFF2_INTERP_CS_HH
#define HB_CFF2_INTERP_CS_HH

#include "hb-private.hh"
#include "hb-cff-interp-cs-common-private.hh"

namespace CFF {

using namespace OT;

struct CFF2CSInterpEnv : CSInterpEnv<CFF2Subrs>
{
  inline void init (const ByteStr &str, const CFF2Subrs &globalSubrs_, const CFF2Subrs &localSubrs_)
  {
    CSInterpEnv<CFF2Subrs>::init (str, globalSubrs_, localSubrs_);
    ivs = 0;
  }

  inline bool fetch_op (OpCode &op)
  {
    if (unlikely (substr.avail ()))
      return CSInterpEnv<CFF2Subrs>::fetch_op (op);

    /* make up return or endchar op */
    if (callStack.check_underflow ())
      op = OpCode_return;
    else
      op = OpCode_endchar;
    return true;
  }

  inline unsigned int get_ivs (void) const { return ivs; }
  inline void         set_ivs (unsigned int ivs_) { ivs = ivs_; }

  protected:
  unsigned int  ivs;
};

template <typename PARAM>
struct CFF2CSOpSet : CSOpSet<CFF2Subrs, PARAM>
{
  static inline bool process_op (OpCode op, CFF2CSInterpEnv &env, PARAM& param)
  {
    switch (op) {

      case OpCode_blendcs:
        env.clear_stack (); // XXX: TODO
        break;
      case OpCode_vsindexcs:
        {
          unsigned int ivs;
          if (unlikely (!env.argStack.check_pop_uint (ivs))) return false;
          env.set_ivs (ivs);
          env.clear_stack ();
        }
        break;
      default:
        typedef CSOpSet<CFF2Subrs, PARAM>  SUPER;
        if (unlikely (!SUPER::process_op (op, env, param)))
          return false;
        break;
    }
    return true;
  }
};

template <typename OPSET, typename PARAM>
struct CFF2CSInterpreter : CSInterpreter<CFF2CSInterpEnv, OPSET, PARAM> {};

} /* namespace CFF */

#endif /* HB_CFF2_INTERP_CS_HH */
