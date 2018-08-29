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

#ifndef HB_SUBSET_CFF_COMMON_HH
#define HB_SUBSET_CFF_COMMON_HH

#include "hb.hh"

#include "hb-subset-plan.hh"
#include "hb-cff-interp-cs-common.hh"

namespace CFF {

/* Used for writing a temporary charstring */
struct ByteStrBuff : hb_vector_t<char, 1>
{
  inline bool encode_byte (unsigned char b)
  {
    return (push ((const char)b) != &Crap(char));
  }

  inline bool encode_num (const Number& n)
  {
    if (n.in_int_range ())
    {
      int v = n.to_int ();
      if ((-1131 <= v) && (v <= 1131))
      {
        if ((-107 <= v) && (v <= 107))
          return encode_byte (v + 139);
        else if (v > 0)
        {
          v -= 108;
          return encode_byte ((v >> 8) + OpCode_TwoBytePosInt0) && encode_byte (v & 0xFF);
        }
        else
        {
          v = -v - 108;
          return encode_byte ((v >> 8) + OpCode_TwoByteNegInt0) && encode_byte (v & 0xFF);
        }
      }
      assert ((v & ~0xFFFF) == 0);
      return encode_byte (OpCode_shortint) &&
             encode_byte ((v >> 8) & 0xFF) &&
             encode_byte (v & 0xFF);
    }
    else
    {
      int32_t v = n.to_fixed ();
      return encode_byte (OpCode_fixedcs) &&
             encode_byte ((v >> 24) & 0xFF) &&
             encode_byte ((v >> 16) & 0xFF) &&
             encode_byte ((v >> 8) & 0xFF) &&
             encode_byte (v & 0xFF);
    }
  }

  inline bool encode_op (OpCode op)
  {
    if (Is_OpCode_ESC (op))
      return encode_byte (OpCode_escape) &&
             encode_byte (Unmake_OpCode_ESC (op));
    else
      return encode_byte (op);
  }
};

struct ByteStrBuffArray : hb_vector_t<ByteStrBuff, 1>
{
  inline void fini (void)
  {
    for (unsigned int i = 0; i < len; i++)
      hb_vector_t<ByteStrBuff, 1>::operator[] (i).fini ();
    hb_vector_t<ByteStrBuff, 1>::fini ();
  }
};


template <typename ACCESSOR, typename ENV, typename OPSET>
struct SubrFlattener
{
  inline SubrFlattener (const ACCESSOR &acc_, const hb_vector_t<hb_codepoint_t> &glyphs_)
    : acc (acc_),
      glyphs (glyphs_)
  {}

  inline bool flatten (ByteStrBuffArray &flat_charstrings)
  {
    if (!flat_charstrings.resize (glyphs.len))
      return false;
    for (unsigned int i = 0; i < glyphs.len; i++)
      flat_charstrings[i].init ();
    for (unsigned int i = 0; i < glyphs.len; i++)
    {
      hb_codepoint_t  glyph = glyphs[i];
      const ByteStr str = (*acc.charStrings)[glyph];
      unsigned int fd = acc.fdSelect->get_fd (glyph);
      CSInterpreter<ENV, OPSET, ByteStrBuff> interp;
      interp.env.init (str, *acc.globalSubrs, *acc.privateDicts[fd].localSubrs);
      if (unlikely (!interp.interpret (flat_charstrings[i])))
        return false;
    }
    return true;
  }
  
  const ACCESSOR &acc;
  const hb_vector_t<hb_codepoint_t> &glyphs;
};

struct SubrRefMaps
{
  inline void init (void)
  {
    valid = false;
    global_map = nullptr;
    local_maps.init ();
  }

  inline void init (unsigned int fd_count)
  {
    valid = true;
    global_map = hb_set_create ();
    if (global_map == hb_set_get_empty ())
      valid = false;
    if (!local_maps.resize (fd_count))
      valid = false;

    for (unsigned int i = 0; i < local_maps.len; i++)
    {
      local_maps[i] = hb_set_create ();
      if (local_maps[i] == hb_set_get_empty ())
        valid = false;
    }
  }

  inline void fini (void)
  {
    hb_set_destroy (global_map);
    for (unsigned int i = 0; i < local_maps.len; i++)
      hb_set_destroy (local_maps[i]);
    local_maps.fini ();
  }

  bool is_valid (void) const { return valid; }
  bool  valid;
  hb_set_t  *global_map;
  hb_vector_t<hb_set_t *> local_maps;
};

struct SubrRefMapPair
{
  inline void init (void) {}

  hb_set_t  *global_map;
  hb_set_t  *local_map;
};

template <typename ACCESSOR, typename ENV, typename OPSET>
struct SubrSubsetter
{
  inline SubrSubsetter (const ACCESSOR &acc_, const hb_vector_t<hb_codepoint_t> &glyphs_)
    : acc (acc_),
      glyphs (glyphs_)
  {}
  
  bool collect_refs (SubrRefMaps& refmaps /*OUT*/)
  {
    refmaps.init (acc.fdCount);
    if (unlikely (!refmaps.valid)) return false;
    for (unsigned int i = 0; i < glyphs.len; i++)
    {
      hb_codepoint_t  glyph = glyphs[i];
      const ByteStr str = (*acc.charStrings)[glyph];
      unsigned int fd = acc.fdSelect->get_fd (glyph);
      SubrRefMapPair  pair = { refmaps.global_map, refmaps.local_maps[fd] };
      CSInterpreter<ENV, OPSET, SubrRefMapPair> interp;
      interp.env.init (str, *acc.globalSubrs, *acc.privateDicts[fd].localSubrs);
      if (unlikely (!interp.interpret (pair)))
        return false;
    }
    return true;
  }

  const ACCESSOR &acc;
  const hb_vector_t<hb_codepoint_t> &glyphs;
};

};  /* namespace CFF */

HB_INTERNAL bool
hb_plan_subset_cff_fdselect (const hb_vector_t<hb_codepoint_t> &glyphs,
                            unsigned int fdCount,
                            const CFF::FDSelect &src, /* IN */
                            unsigned int &subset_fd_count /* OUT */,
                            unsigned int &subst_fdselect_size /* OUT */,
                            unsigned int &subst_fdselect_format /* OUT */,
                            hb_vector_t<hb_codepoint_t> &subst_first_glyphs /* OUT */,
                            CFF::FDMap &fdmap /* OUT */);

HB_INTERNAL bool
hb_serialize_cff_fdselect (hb_serialize_context_t *c,
                          const hb_vector_t<hb_codepoint_t> &glyphs,
                          const CFF::FDSelect &src,
                          unsigned int fd_count,
                          unsigned int fdselect_format,
                          unsigned int size,
                          const hb_vector_t<hb_codepoint_t> &first_glyphs,
                          const CFF::FDMap &fdmap);

#endif /* HB_SUBSET_CFF_COMMON_HH */
