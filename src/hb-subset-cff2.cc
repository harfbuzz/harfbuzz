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

#include "hb-open-type.hh"
#include "hb-ot-cff2-table.hh"
#include "hb-set.h"
#include "hb-subset-cff2.hh"
#include "hb-subset-plan.hh"
#include "hb-subset-cff-common.hh"
#include "hb-cff2-interp-cs.hh"

using namespace CFF;

struct CFF2SubTableOffsets : CFFSubTableOffsets
{
  inline CFF2SubTableOffsets (void)
    : CFFSubTableOffsets (),
      varStoreOffset (0)
  {}

  unsigned int  varStoreOffset;
};

struct CFF2TopDict_OpSerializer : CFFTopDict_OpSerializer<>
{
  inline bool serialize (hb_serialize_context_t *c,
                         const OpStr &opstr,
                         const CFF2SubTableOffsets &offsets) const
  {
    TRACE_SERIALIZE (this);

    switch (opstr.op)
    {
      case OpCode_vstore:
        return_trace (FontDict::serialize_offset4_op(c, opstr.op, offsets.varStoreOffset));

      default:
        return_trace (CFFTopDict_OpSerializer<>::serialize (c, opstr, offsets));
    }
  }

  inline unsigned int calculate_serialized_size (const OpStr &opstr) const
  {
    switch (opstr.op)
    {
      case OpCode_vstore:
        return OpCode_Size (OpCode_longintdict) + 4 + OpCode_Size (opstr.op);
    
      default:
        return CFFTopDict_OpSerializer<>::calculate_serialized_size (opstr);
    }
  }
};

struct CFF2CSOpSet_Flatten : CFF2CSOpSet<CFF2CSOpSet_Flatten, FlattenParam>
{
  static inline void flush_args_and_op (OpCode op, CFF2CSInterpEnv &env, FlattenParam& param)
  {
    switch (op)
    {
      case OpCode_return:
      case OpCode_endchar:
        /* dummy opcodes in CFF2. ignore */
        break;
    
      case OpCode_hstem:
      case OpCode_hstemhm:
      case OpCode_vstem:
      case OpCode_vstemhm:
      case OpCode_hintmask:
      case OpCode_cntrmask:
        if (param.drop_hints)
        {
          env.clear_args ();
          return;
        }
        HB_FALLTHROUGH;
        
      default:
        SUPER::flush_args_and_op (op, env, param);
        break;
    }
  }

  static inline void flush_args (CFF2CSInterpEnv &env, FlattenParam& param)
  {
    for (unsigned int i = 0; i < env.argStack.get_count ();)
    {
      const BlendArg &arg = env.argStack[i];
      if (arg.blending ())
      {
        assert ((arg.numValues > 0) && (env.argStack.get_count () >= arg.numValues));
        flatten_blends (arg, i, env, param);
        i += arg.numValues;
      }
      else
      {
        StrEncoder  encoder (param.flatStr);
        encoder.encode_num (arg);
        i++;
      }
    }
    SUPER::flush_args (env, param);
  }

  static inline void flatten_blends (const BlendArg &arg, unsigned int i, CFF2CSInterpEnv &env, FlattenParam& param)
  {
    /* flatten the default values */
    StrEncoder  encoder (param.flatStr);
    for (unsigned int j = 0; j < arg.numValues; j++)
    {
      const BlendArg &arg1 = env.argStack[i + j];
      assert (arg1.blending () && (arg.numValues == arg1.numValues) && (arg1.valueIndex == j) &&
              (arg1.deltas.len == env.get_region_count ()));
      encoder.encode_num (arg1);
    }
    /* flatten deltas for each value */
    for (unsigned int j = 0; j < arg.numValues; j++)
    {
      const BlendArg &arg1 = env.argStack[i + j];
      for (unsigned int k = 0; k < arg1.deltas.len; k++)
        encoder.encode_num (arg1.deltas[k]);
    }
    /* flatten the number of values followed by blend operator */
    encoder.encode_int (arg.numValues);
    encoder.encode_op (OpCode_blendcs);
  }
 
  static inline void flush_op (OpCode op, CFF2CSInterpEnv &env, FlattenParam& param)
  {
    switch (op)
    {
      case OpCode_return:
      case OpCode_endchar:
        return;
      default:
        StrEncoder  encoder (param.flatStr);
        encoder.encode_op (op);
    }
  }

  private:
  typedef CFF2CSOpSet<CFF2CSOpSet_Flatten, FlattenParam> SUPER;
  typedef CSOpSet<BlendArg, CFF2CSOpSet_Flatten, CFF2CSOpSet_Flatten, CFF2CSInterpEnv, FlattenParam> CSOPSET;
};

struct cff2_subset_plan {
  inline cff2_subset_plan (void)
    : final_size (0),
      orig_fdcount (0),
      subset_fdcount(1),
      subset_fdselect_format (0),
      drop_hints (false),
      desubroutinize (false)
  {
    subset_fdselect_ranges.init ();
    fdmap.init ();
    subset_charstrings.init ();
    flat_charstrings.init ();
    privateDictInfos.init ();
  }

  inline ~cff2_subset_plan (void)
  {
    subset_fdselect_ranges.fini ();
    fdmap.fini ();
    subset_charstrings.fini ();
    flat_charstrings.fini ();
    privateDictInfos.fini ();
  }

  inline bool create (const OT::cff2::accelerator_subset_t &acc,
              hb_subset_plan_t *plan)
  {
    final_size = 0;
    orig_fdcount = acc.fdArray->count;

    drop_hints = plan->drop_hints;
    desubroutinize = plan->desubroutinize;

    /* CFF2 header */
    final_size += OT::cff2::static_size;
    
    /* top dict */
    {
      CFF2TopDict_OpSerializer topSzr;
      offsets.topDictInfo.size = TopDict::calculate_serialized_size (acc.topDict, topSzr);
      final_size += offsets.topDictInfo.size;
    }

    {
      /* Flatten global & local subrs */
      SubrFlattener<const OT::cff2::accelerator_subset_t, CFF2CSInterpEnv, CFF2CSOpSet_Flatten>
                    flattener(acc, plan->glyphs, plan->drop_hints);
      if (!flattener.flatten (flat_charstrings))
        return false;
      
      /* no global/local subroutines */
      offsets.globalSubrsInfo.size = HBUINT32::static_size; /* count 0 only */
    }
    
    /* global subrs */
    offsets.globalSubrsInfo.offset = final_size;
    final_size += offsets.globalSubrsInfo.size;

    /* variation store */
    if (acc.varStore != &Null(CFF2VariationStore))
    {
      offsets.varStoreOffset = final_size;
      final_size += acc.varStore->get_size ();
    }

    /* FDSelect */
    if (acc.fdSelect != &Null(CFF2FDSelect))
    {
      offsets.FDSelectInfo.offset = final_size;
      if (unlikely (!hb_plan_subset_cff_fdselect (plan->glyphs,
                                  orig_fdcount,
                                  *(const FDSelect *)acc.fdSelect,
                                  subset_fdcount,
                                  offsets.FDSelectInfo.size,
                                  subset_fdselect_format,
                                  subset_fdselect_ranges,
                                  fdmap)))
        return false;
      
      if (!is_fds_subsetted ())
        offsets.FDSelectInfo.size = acc.fdSelect->calculate_serialized_size (acc.num_glyphs);
      final_size += offsets.FDSelectInfo.size;
    }

    /* FDArray (FDIndex) */
    {
      offsets.FDArrayInfo.offset = final_size;
      CFFFontDict_OpSerializer fontSzr;
      final_size += CFF2FDArray::calculate_serialized_size(offsets.FDArrayInfo.offSize/*OUT*/, acc.fontDicts, subset_fdcount, fdmap, fontSzr);
    }

    /* CharStrings */
    {
      offsets.charStringsInfo.offset = final_size;
      unsigned int dataSize = 0;
      for (unsigned int i = 0; i < plan->glyphs.len; i++)
      {
        StrBuff &flatstr = flat_charstrings[i];
        ByteStr str (&flatstr[0], flatstr.len);
        subset_charstrings.push (str);
        dataSize += flatstr.len;
      }
      offsets.charStringsInfo.offSize = calcOffSize (dataSize);
      final_size += CFF2CharStrings::calculate_serialized_size (offsets.charStringsInfo.offSize, plan->glyphs.len, dataSize);
    }

    /* private dicts & local subrs */
    offsets.privateDictsOffset = final_size;
    for (unsigned int i = 0; i < orig_fdcount; i++)
    {
      if (!fdmap.excludes (i))
      {
        unsigned int priv_size;
        CFFPrivateDict_OpSerializer privSzr (desubroutinize, drop_hints);
        priv_size = PrivateDict::calculate_serialized_size (acc.privateDicts[i], privSzr);
        TableInfo  privInfo = { final_size, priv_size, 0 };
        privateDictInfos.push (privInfo);
        final_size += privInfo.size;
      }
    }

    return true;
  }

  inline unsigned int get_final_size (void) const  { return final_size; }

  unsigned int        final_size;
  CFF2SubTableOffsets offsets;

  unsigned int    orig_fdcount;
  unsigned int    subset_fdcount;
  inline bool     is_fds_subsetted (void) const { return subset_fdcount < orig_fdcount; }
  unsigned int    subset_fdselect_format;
  hb_vector_t<code_pair>   subset_fdselect_ranges;

  Remap   fdmap;

  ByteStrArray            subset_charstrings;
  StrBuffArray            flat_charstrings;
  hb_vector_t<TableInfo>  privateDictInfos;

  bool            drop_hints;
  bool            desubroutinize;
};

static inline bool _write_cff2 (const cff2_subset_plan &plan,
                                const OT::cff2::accelerator_subset_t  &acc,
                                const hb_vector_t<hb_codepoint_t>& glyphs,
                                unsigned int dest_sz,
                                void *dest)
{
  hb_serialize_context_t c (dest, dest_sz);

  OT::cff2 *cff2 = c.start_serialize<OT::cff2> ();
  if (unlikely (!c.extend_min (*cff2)))
    return false;

  /* header */
  cff2->version.major.set (0x02);
  cff2->version.minor.set (0x00);
  cff2->topDict.set (OT::cff2::static_size);

  /* top dict */
  {
    assert (cff2->topDict == c.head - c.start);
    cff2->topDictSize.set (plan.offsets.topDictInfo.size);
    TopDict &dict = cff2 + cff2->topDict;
    CFF2TopDict_OpSerializer topSzr;
    if (unlikely (!dict.serialize (&c, acc.topDict, topSzr, plan.offsets)))
    {
      DEBUG_MSG (SUBSET, nullptr, "failed to serialize CFF2 top dict");
      return false;
    }
  }

  /* global subrs */
  {
    assert (cff2->topDict + plan.offsets.topDictInfo.size == c.head - c.start);
    CFF2Subrs *dest = c.allocate_size <CFF2Subrs> (HBUINT32::static_size);
    if (unlikely (dest == nullptr)) return false;
    dest->count.set (0);
  }
  
  /* variation store */
  if (acc.varStore != &Null(CFF2VariationStore))
  {
    assert (plan.offsets.varStoreOffset == c.head - c.start);
    CFF2VariationStore *dest = c.start_embed<CFF2VariationStore> ();
    if (unlikely (!dest->serialize (&c, acc.varStore)))
    {
      DEBUG_MSG (SUBSET, nullptr, "failed to serialize CFF2 Variation Store");
      return false;
    }
  }

  /* FDSelect */
  if (acc.fdSelect != &Null(CFF2FDSelect))
  {
    assert (plan.offsets.FDSelectInfo.offset == c.head - c.start);
    
    if (plan.is_fds_subsetted ())
    {
      if (unlikely (!hb_serialize_cff_fdselect (&c, glyphs.len, *(const FDSelect *)acc.fdSelect, acc.fdArray->count,
                                                plan.subset_fdselect_format, plan.offsets.FDSelectInfo.size,
                                                plan.subset_fdselect_ranges,
                                                plan.fdmap)))
      {
        DEBUG_MSG (SUBSET, nullptr, "failed to serialize CFF2 subset FDSelect");
        return false;
      }
    }
    else
    {
      CFF2FDSelect *dest = c.start_embed<CFF2FDSelect> ();
      if (unlikely (!dest->serialize (&c, *acc.fdSelect, acc.num_glyphs)))
      {
        DEBUG_MSG (SUBSET, nullptr, "failed to serialize CFF2 FDSelect");
        return false;
      }
    }
  }

  /* FDArray (FD Index) */
  {
    assert (plan.offsets.FDArrayInfo.offset == c.head - c.start);
    CFF2FDArray  *fda = c.start_embed<CFF2FDArray> ();
    if (unlikely (fda == nullptr)) return false;
    CFFFontDict_OpSerializer  fontSzr;
    if (unlikely (!fda->serialize (&c, plan.offsets.FDArrayInfo.offSize,
                                   acc.fontDicts, plan.subset_fdcount, plan.fdmap,
                                   fontSzr, plan.privateDictInfos)))
    {
      DEBUG_MSG (SUBSET, nullptr, "failed to serialize CFF2 FDArray");
      return false;
    }
  }

  /* CharStrings */
  {
    assert (plan.offsets.charStringsInfo.offset == c.head - c.start);
    CFF2CharStrings  *cs = c.start_embed<CFF2CharStrings> ();
    if (unlikely (cs == nullptr)) return false;
    if (unlikely (!cs->serialize (&c, plan.offsets.charStringsInfo.offSize, plan.subset_charstrings)))
    {
      DEBUG_MSG (SUBSET, nullptr, "failed to serialize CFF2 CharStrings");
      return false;
    }
  }

  /* private dicts & local subrs */
  assert (plan.offsets.privateDictsOffset == c.head - c.start);
  for (unsigned int i = 0; i < acc.privateDicts.len; i++)
  {
    if (!plan.fdmap.excludes (i))
    {
      PrivateDict  *pd = c.start_embed<PrivateDict> ();
      if (unlikely (pd == nullptr)) return false;
      unsigned int priv_size = plan.privateDictInfos[plan.fdmap[i]].size;
      bool result;
      CFFPrivateDict_OpSerializer privSzr (plan.desubroutinize, plan.drop_hints);
      result = pd->serialize (&c, acc.privateDicts[i], privSzr, priv_size);
      if (unlikely (!result))
      {
        DEBUG_MSG (SUBSET, nullptr, "failed to serialize CFF2 Private Dict[%d]", i);
        return false;
      }
    }
  }

  assert (c.head == c.end);
  c.end_serialize ();

  return true;
}

static bool
_hb_subset_cff2 (const OT::cff2::accelerator_subset_t  &acc,
                const char                      *data,
                hb_subset_plan_t                *plan,
                hb_blob_t                       **prime /* OUT */)
{
  cff2_subset_plan cff2_plan;

  if (unlikely (!cff2_plan.create (acc, plan)))
  {
    DEBUG_MSG(SUBSET, nullptr, "Failed to generate a cff2 subsetting plan.");
    return false;
  }

  unsigned int  cff2_prime_size = cff2_plan.get_final_size ();
  char *cff2_prime_data = (char *) calloc (1, cff2_prime_size);

  if (unlikely (!_write_cff2 (cff2_plan, acc, plan->glyphs,
                              cff2_prime_size, cff2_prime_data))) {
    DEBUG_MSG(SUBSET, nullptr, "Failed to write a subset cff2.");
    free (cff2_prime_data);
    return false;
  }

  *prime = hb_blob_create (cff2_prime_data,
                                cff2_prime_size,
                                HB_MEMORY_MODE_READONLY,
                                cff2_prime_data,
                                free);
  return true;
}

/**
 * hb_subset_cff2:
 * Subsets the CFF2 table according to a provided plan.
 *
 * Return value: subsetted cff2 table.
 **/
bool
hb_subset_cff2 (hb_subset_plan_t *plan,
                hb_blob_t       **prime /* OUT */)
{
  hb_blob_t *cff2_blob = hb_sanitize_context_t().reference_table<CFF::cff2> (plan->source);
  const char *data = hb_blob_get_data(cff2_blob, nullptr);

  OT::cff2::accelerator_subset_t acc;
  acc.init(plan->source);
  bool result = likely (acc.is_valid ()) &&
                _hb_subset_cff2 (acc, data, plan, prime);

  hb_blob_destroy (cff2_blob);
  acc.fini ();

  return result;
}
