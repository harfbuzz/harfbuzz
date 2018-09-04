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

#include "hb.hh"
#include "hb-cff-interp-cs-common.hh"

namespace CFF {

using namespace OT;

struct BlendArg : Number
{
  inline void init (void)
  {
    Number::init ();
    deltas.init ();
  }

  inline void fini (void)
  {
    Number::fini ();

    for (unsigned int i = 0; i < deltas.len; i++)
      deltas[i].fini ();
    deltas.fini ();
  }

  inline void set_int (int v) { reset_blends (); Number::set_int (v); }
  inline void set_fixed (int32_t v) { reset_blends (); Number::set_fixed (v); }
  inline void set_real (float v) { reset_blends (); Number::set_real (v); }

  inline void set_blends (unsigned int numValues_, unsigned int valueIndex_,
                          unsigned int numBlends, const BlendArg *blends_)
  {
    numValues = numValues_;
    valueIndex = valueIndex_;
    deltas.resize (numBlends);
    for (unsigned int i = 0; i < numBlends; i++)
      deltas[i] = blends_[i];
  }

  inline bool blended (void) const { return deltas.len > 0; }
  inline void reset_blends (void)
  {
    numValues = valueIndex = 0;
    deltas.resize (0);
  }

  unsigned int numValues;
  unsigned int valueIndex;
  hb_vector_t<Number> deltas;
};

typedef InterpEnv<BlendArg> BlendInterpEnv;

struct CFF2CSInterpEnv : CSInterpEnv<BlendArg, CFF2Subrs>
{
  template <typename ACC>
  inline void init (const ByteStr &str, ACC &acc, unsigned int fd)
  {
    SUPER::init (str, *acc.globalSubrs, *acc.privateDicts[fd].localSubrs);
    set_region_count (acc.region_count);
    set_vsindex (acc.privateDicts[fd].vsindex);
  }

  inline bool fetch_op (OpCode &op)
  {
    if (unlikely (this->substr.avail ()))
      return SUPER::fetch_op (op);

    /* make up return or endchar op */
    if (this->callStack.check_underflow ())
      op = OpCode_return;
    else
      op = OpCode_endchar;
    return true;
  }

  inline void process_vsindex (void)
  {
    unsigned int  index;
    if (likely (argStack.check_pop_uint (index)))
      set_vsindex (argStack.check_pop_uint (index));
  }

  inline unsigned int get_region_count (void) const { return region_count; }
  inline void         set_region_count (unsigned int region_count_) { region_count = region_count_; }
  inline unsigned int get_vsindex (void) const { return vsindex; }
  inline void         set_vsindex (unsigned int vsindex_) { vsindex = vsindex_; }

  protected:
  unsigned int  region_count;
  unsigned int  vsindex;

  typedef CSInterpEnv<BlendArg, CFF2Subrs> SUPER;
};

template <typename OPSET, typename PARAM>
struct CFF2CSOpSet : CSOpSet<BlendArg, OPSET, CFF2CSInterpEnv, PARAM>
{
  static inline bool process_op (OpCode op, CFF2CSInterpEnv &env, PARAM& param)
  {
    switch (op) {
      case OpCode_callsubr:
      case OpCode_callgsubr:
        /* a subroutine number shoudln't be a blended value */
        return (!env.argStack.peek ().blended () &&
                SUPER::process_op (op, env, param));

      case OpCode_blendcs:
        return OPSET::process_blend (env, param);

      case OpCode_vsindexcs:
        if (unlikely (env.argStack.peek ().blended ()))
          return false;
        OPSET::process_vsindex (env, param);
        break;

      default:
        return SUPER::process_op (op, env, param);
    }
    return true;
  }

  static inline bool process_blend (CFF2CSInterpEnv &env, PARAM& param)
  {
    unsigned int n, k;

    k = env.get_region_count ();
    if (unlikely (!env.argStack.check_pop_uint (n) ||
                  (k+1) * n > env.argStack.get_count ()))
      return false;
    /* copy the blend values into blend array of the default values */
    unsigned int start = env.argStack.get_count () - ((k+1) * n);
    for (unsigned int i = 0; i < n; i++)
      env.argStack.elements[start + i].set_blends (n, i, k, &env.argStack.elements[start + n + (i * k)]);

    /* pop off blend values leaving default values now adorned with blend values */
    env.argStack.count -= k * n;
    return true;
  }

  static inline void process_vsindex (CFF2CSInterpEnv &env, PARAM& param)
  {
    env.process_vsindex ();
    OPSET::flush_n_args_and_op (OpCode_vsindexcs, 1, env, param);
  }

  private:
  typedef CSOpSet<BlendArg, OPSET, CFF2CSInterpEnv, PARAM>  SUPER;
};

template <typename OPSET, typename PARAM>
struct CFF2CSInterpreter : CSInterpreter<CFF2CSInterpEnv, OPSET, PARAM> {};

} /* namespace CFF */

#endif /* HB_CFF2_INTERP_CS_HH */
