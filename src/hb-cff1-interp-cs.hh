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
  template <typename ACC>
  inline void init (const ByteStr &str, ACC &acc, unsigned int fd)
  {
    SUPER::init (str, *acc.globalSubrs, *acc.privateDicts[fd].localSubrs);
    processed_width = false;
    has_width = false;
  }

  inline void fini (void)
  {
    SUPER::fini ();
  }

  inline unsigned int check_width (void)
  {
    unsigned int arg_start = 0;
    if (!processed_width)
    {
      if ((SUPER::argStack.get_count () & 1) != 0)
      {
        width = SUPER::argStack[0];
        has_width = true;
        arg_start = 1;
      }
      processed_width = true;
    }
    return arg_start;
  }

  bool          processed_width;
  bool          has_width;
  Number        width;

  private:
  typedef CSInterpEnv<Number, CFF1Subrs> SUPER;
};

template <typename OPSET, typename PARAM, typename PATH=PathProcsNull<CFF1CSInterpEnv, PARAM> >
struct CFF1CSOpSet : CSOpSet<Number, OPSET, CFF1CSInterpEnv, PARAM, PATH>
{
  /* PostScript-originated legacy opcodes (OpCode_add etc) are unsupported */

  static inline void flush_args (CFF1CSInterpEnv &env, PARAM& param, unsigned int start_arg = 0)
  {
    start_arg = env.check_width ();
    SUPER::flush_args (env, param, start_arg);
    env.clear_args ();  /* pop off width */
  }

  private:
  typedef CSOpSet<Number, OPSET, CFF1CSInterpEnv, PARAM, PATH>  SUPER;
};

template <typename OPSET, typename PARAM>
struct CFF1CSInterpreter : CSInterpreter<CFF1CSInterpEnv, OPSET, PARAM> {};

} /* namespace CFF */

#endif /* HB_CFF1_INTERP_CS_HH */
