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
#include "hb-ot-cff1-table.hh"
#include "hb-set.h"
#include "hb-subset-cff1.hh"
#include "hb-subset-plan.hh"
#include "hb-subset-cff-common.hh"
#include "hb-cff1-interp-cs.hh"

using namespace CFF;

struct CFF1SubTableOffsets : CFFSubTableOffsets
{
  inline CFF1SubTableOffsets (void)
    : CFFSubTableOffsets (),
      nameIndexOffset (0),
      stringIndexOffset (0),
      encodingOffset (0),
      charsetOffset (0)
  {
    privateDictInfo.init ();
  }

  unsigned int  nameIndexOffset;
  unsigned int  stringIndexOffset;
  unsigned int  encodingOffset;
  unsigned int  charsetOffset;
  TableInfo     privateDictInfo;
};

struct CFF1TopDict_OpSerializer : CFFTopDict_OpSerializer
{
  inline bool serialize (hb_serialize_context_t *c,
                         const OpStr &opstr,
                         const CFF1SubTableOffsets &offsets) const
  {
    TRACE_SERIALIZE (this);

    switch (opstr.op)
    {
      case OpCode_charset:
        return_trace (FontDict::serialize_offset4_op(c, opstr.op, offsets.charsetOffset));

      case OpCode_Encoding:
        return_trace (FontDict::serialize_offset4_op(c, opstr.op, offsets.encodingOffset));

      case OpCode_Private:
        {
          if (unlikely (!UnsizedByteStr::serialize_int2 (c, offsets.privateDictInfo.size)))
            return_trace (false);
          if (unlikely (!UnsizedByteStr::serialize_int4 (c, offsets.privateDictInfo.offset)))
            return_trace (false);
          HBUINT8 *p = c->allocate_size<HBUINT8> (1);
          if (unlikely (p == nullptr)) return_trace (false);
          p->set (OpCode_Private);
        }
        break;

      default:
        return_trace (CFFTopDict_OpSerializer::serialize (c, opstr, offsets));
    }
    return_trace (true);
  }

  inline unsigned int calculate_serialized_size (const OpStr &opstr) const
  {
    switch (opstr.op)
    {
      case OpCode_charset:
      case OpCode_Encoding:
        return OpCode_Size (OpCode_longintdict) + 4 + OpCode_Size (opstr.op);
    
      case OpCode_Private:
        return OpCode_Size (OpCode_longintdict) + 4 + OpCode_Size (OpCode_shortint) + 2 + OpCode_Size (OpCode_Private);
    
      default:
        return CFFTopDict_OpSerializer::calculate_serialized_size (opstr);
    }
  }
};

struct CFF1CSOpSet_Flatten : CFF1CSOpSet<CFF1CSOpSet_Flatten, FlattenParam>
{
  static inline bool process_op (OpCode op, CFF1CSInterpEnv &env, FlattenParam& param)
  {
    if (param.drop_hints && CSOPSET::is_hint_op (op))
    {
      env.clear_stack ();
      return true;
    }
    if (unlikely (!SUPER::process_op (op, env, param)))
      return false;
    switch (op)
    {
      case OpCode_hintmask:
      case OpCode_cntrmask:
        if (param.drop_hints)
        {
          env.clear_stack ();
          return true;
        }
        if (unlikely (!param.flatStr.encode_op (op)))
          return false;
        for (int i = -env.hintmask_size; i < 0; i++)
          if (unlikely (!param.flatStr.encode_byte (env.substr[i])))
            return false;
        break;
      default:
        if (!CSOPSET::is_subr_op (op) &&
            !CSOPSET::is_arg_op (op))
          return param.flatStr.encode_op (op);
    }
    return true;
  }

  static inline void flush_stack (CFF1CSInterpEnv &env, FlattenParam& param)
  {
    for (unsigned int i = 0; i < env.argStack.size; i++)
      param.flatStr.encode_num (env.argStack.elements[i]);
    SUPER::flush_stack (env, param);
  }

  private:
  typedef CFF1CSOpSet<CFF1CSOpSet_Flatten, FlattenParam> SUPER;
  typedef CSOpSet<CFF1CSOpSet_Flatten, CFF1CSInterpEnv, FlattenParam> CSOPSET;
};

struct CFF1CSOpSet_SubsetSubrs : CFF1CSOpSet<CFF1CSOpSet_SubsetSubrs, SubrRefMapPair>
{
  static inline bool process_op (OpCode op, CFF1CSInterpEnv &env, SubrRefMapPair& refMapPair)
  {
    unsigned int  subr_num;
    switch (op) {
      case OpCode_callsubr:
        if (!unlikely (env.popSubrNum(env.localSubrs, subr_num)))
          return false;
        env.argStack.unpop ();
        refMapPair.local_map->add (subr_num);
        break;
      case OpCode_callgsubr:
        if (!unlikely (env.popSubrNum(env.globalSubrs, subr_num)))
          return false;
        env.argStack.unpop ();
        refMapPair.global_map->add (subr_num);
        break;
      default:
        break;
    }
    return CFF1CSOpSet<CFF1CSOpSet_SubsetSubrs, SubrRefMapPair>::process_op (op, env, refMapPair);
  }
};

struct cff_subset_plan {
  inline cff_subset_plan (void)
    : final_size (0),
      orig_fdcount (0),
      subst_fdcount (1),
      subst_fdselect_format (0),
      offsets (),
      flatten_subrs (true),
      drop_hints (false)
  {
    topdict_sizes.init ();
    topdict_sizes.resize (1);
    subst_fdselect_first_glyphs.init ();
    fdmap.init ();
    subset_charstrings.init ();
    flat_charstrings.init ();
    privateDictInfos.init ();
    subrRefMaps.init ();
  }

  inline ~cff_subset_plan (void)
  {
    topdict_sizes.fini ();
    subst_fdselect_first_glyphs.fini ();
    fdmap.fini ();
    subset_charstrings.fini ();
    flat_charstrings.fini ();
    privateDictInfos.fini ();
    subrRefMaps.fini ();
  }

  inline bool create (const OT::cff1::accelerator_subset_t &acc,
                      hb_subset_plan_t *plan)
  {
    final_size = 0;
    orig_fdcount = acc.fdCount;
    drop_hints = plan->drop_hints;

    /* CFF header */
    final_size += OT::cff1::static_size;
    
    /* Name INDEX */
    offsets.nameIndexOffset = final_size;
    final_size += acc.nameIndex->get_size ();
    
    /* top dict INDEX */
    {
      offsets.topDictInfo.offset = final_size;
      CFF1TopDict_OpSerializer topSzr;
      unsigned int topDictSize = TopDict::calculate_serialized_size (acc.topDicts[0], topSzr);
      offsets.topDictInfo.offSize = calcOffSize(topDictSize);
      final_size += CFF1IndexOf<TopDict>::calculate_serialized_size<CFF1TopDictValues> (offsets.topDictInfo.offSize, acc.topDicts, topdict_sizes, topSzr);
    }

    /* String INDEX */
    offsets.stringIndexOffset = final_size;
    final_size += acc.stringIndex->get_size ();
    
    if (flatten_subrs)
    {
      /* Flatten global & local subrs */
      SubrFlattener<const OT::cff1::accelerator_subset_t, CFF1CSInterpEnv, CFF1CSOpSet_Flatten>
                    flattener(acc, plan->glyphs, plan->drop_hints);
      if (!flattener.flatten (flat_charstrings))
        return false;
      
      /* no global/local subroutines */
      offsets.globalSubrsInfo.size = HBUINT16::static_size; /* count 0 only */
    }
    else
    {
      /* Subset global & local subrs */
      SubrSubsetter<const OT::cff1::accelerator_subset_t, CFF1CSInterpEnv, CFF1CSOpSet_SubsetSubrs> subsetter(acc, plan->glyphs);
      if (!subsetter.collect_refs (subrRefMaps))
        return false;
      
      offsets.globalSubrsInfo.size = acc.globalSubrs->calculate_serialized_size (offsets.globalSubrsInfo.offSize, subrRefMaps.global_map, 1);
      if (!offsets.localSubrsInfos.resize (orig_fdcount))
        return false;
      for (unsigned int i = 0; i < orig_fdcount; i++)
        offsets.localSubrsInfos[i].size = acc.privateDicts[i].localSubrs->calculate_serialized_size (offsets.localSubrsInfos[i].offSize, subrRefMaps.local_maps[i], 1);
    }
    /* global subrs */
    offsets.globalSubrsInfo.offset = final_size;
    final_size += offsets.globalSubrsInfo.size;

    /* Encoding */
    offsets.encodingOffset = final_size;
    if (acc.encoding != &Null(Encoding))
      final_size += acc.encoding->get_size ();

    /* Charset */
    offsets.charsetOffset = final_size;
    if (acc.charset != &Null(Charset))
      final_size += acc.charset->get_size (acc.num_glyphs);

    /* FDSelect */
    if (acc.fdSelect != &Null(CFF1FDSelect))
    {
      offsets.FDSelectInfo.offset = final_size;
      if (unlikely (!hb_plan_subset_cff_fdselect (plan->glyphs,
                                  orig_fdcount,
                                  *acc.fdSelect,
                                  subst_fdcount,
                                  offsets.FDSelectInfo.size,
                                  subst_fdselect_format,
                                  subst_fdselect_first_glyphs,
                                  fdmap)))
        return false;
      
      if (!is_fds_subsetted ())
        offsets.FDSelectInfo.size = acc.fdSelect->calculate_serialized_size (acc.num_glyphs);
      final_size += offsets.FDSelectInfo.size;
    }

    /* FDArray (FDIndex) */
    if (acc.fdArray != &Null(CFF1FDArray)) {
      offsets.FDArrayInfo.offset = final_size;
      CFFFontDict_OpSerializer fontSzr;
      final_size += CFF1FDArray::calculate_serialized_size(offsets.FDArrayInfo.offSize/*OUT*/, acc.fontDicts, subst_fdcount, fdmap, fontSzr);
    }

    /* CharStrings */
    {
      offsets.charStringsInfo.offset = final_size;
      unsigned int dataSize = 0;
      for (unsigned int i = 0; i < plan->glyphs.len; i++)
      {
        if (flatten_subrs)
        {
          ByteStrBuff &flatstr = flat_charstrings[i];
          ByteStr str (&flatstr[0], flatstr.len);
          subset_charstrings.push (str);
          dataSize += flatstr.len;
        }
        else
        {
          const ByteStr str = (*acc.charStrings)[plan->glyphs[i]];
          subset_charstrings.push (str);
          dataSize += str.len;
        }
      }
      offsets.charStringsInfo.offSize = calcOffSize (dataSize + 1);
      final_size += CFF1CharStrings::calculate_serialized_size (offsets.charStringsInfo.offSize, plan->glyphs.len, dataSize);
    }

    /* private dicts & local subrs */
    offsets.privateDictInfo.offset = final_size;
    for (unsigned int i = 0; i < orig_fdcount; i++)
    {
      if (!fdmap.excludes (i))
      {
        unsigned int  priv_size;
        CFFPrivateDict_OpSerializer privSzr (plan->drop_hints, flatten_subrs);
        priv_size = PrivateDict::calculate_serialized_size (acc.privateDicts[i], privSzr);
        TableInfo  privInfo = { final_size, priv_size, 0 };
        privateDictInfos.push (privInfo);
        final_size += privInfo.size;
        if (!flatten_subrs)
          final_size += offsets.localSubrsInfos[i].size;
      }
    }

    if (!acc.is_CID ())
      offsets.privateDictInfo = privateDictInfos[0];

    return ((subset_charstrings.len == plan->glyphs.len) &&
            (privateDictInfos.len == subst_fdcount));
  }

  inline unsigned int get_final_size (void) const  { return final_size; }

  unsigned int        final_size;
  hb_vector_t<unsigned int> topdict_sizes;
  CFF1SubTableOffsets  offsets;

  unsigned int    orig_fdcount;
  unsigned int    subst_fdcount;
  inline bool     is_fds_subsetted (void) const { return subst_fdcount < orig_fdcount; }
  unsigned int    subst_fdselect_format;
  hb_vector_t<hb_codepoint_t>   subst_fdselect_first_glyphs;

  /* font dict index remap table from fullset FDArray to subset FDArray.
   * set to HB_SET_VALUE_INVALID if excluded from subset */
  FDMap   fdmap;

  hb_vector_t<ByteStr>    subset_charstrings;
  ByteStrBuffArray        flat_charstrings;
  hb_vector_t<TableInfo>  privateDictInfos;

  SubrRefMaps             subrRefMaps;

  bool            flatten_subrs;
  bool            drop_hints;
};

static inline bool _write_cff1 (const cff_subset_plan &plan,
                                const OT::cff1::accelerator_subset_t  &acc,
                                const hb_vector_t<hb_codepoint_t>& glyphs,
                                unsigned int dest_sz,
                                void *dest)
{
  hb_serialize_context_t c (dest, dest_sz);

  char RETURN_OP[1] = { OpCode_return };
  const ByteStr NULL_SUBR (RETURN_OP, 1);

  OT::cff1 *cff = c.start_serialize<OT::cff1> ();
  if (unlikely (!c.extend_min (*cff)))
    return false;

  /* header */
  cff->version.major.set (0x01);
  cff->version.minor.set (0x00);
  cff->nameIndex.set (cff->min_size);
  cff->offSize.set (4); /* unused? */

  /* name INDEX */
  {
    assert (cff->nameIndex == c.head - c.start);
    CFF1NameIndex *dest = c.start_embed<CFF1NameIndex> ();
    if (unlikely (dest == nullptr)) return false;
    if (unlikely (!dest->serialize (&c, *acc.nameIndex)))
    {
      DEBUG_MSG (SUBSET, nullptr, "failed to serialize CFF name INDEX");
      return false;
    }
  }

  /* top dict INDEX */
  {
    assert (plan.offsets.topDictInfo.offset == c.head - c.start);
    CFF1IndexOf<TopDict> *dest = c.start_embed< CFF1IndexOf<TopDict> > ();
    if (dest == nullptr) return false;
    CFF1TopDict_OpSerializer topSzr;
    if (unlikely (!dest->serialize (&c, plan.offsets.topDictInfo.offSize, acc.topDicts, plan.topdict_sizes, topSzr, plan.offsets)))
    {
      DEBUG_MSG (SUBSET, nullptr, "failed to serialize CFF top dict");
      return false;
    }
  }

  /* String INDEX */
  {
    assert (plan.offsets.stringIndexOffset == c.head - c.start);
    CFF1StringIndex *dest = c.start_embed<CFF1StringIndex> ();
    if (unlikely (dest == nullptr)) return false;
    if (unlikely (!dest->serialize (&c, *acc.stringIndex)))
    {
      DEBUG_MSG (SUBSET, nullptr, "failed to serialize CFF string INDEX");
      return false;
    }
  }

  /* global subrs */
  {
    assert (plan.offsets.globalSubrsInfo.offset != 0);
    assert (plan.offsets.globalSubrsInfo.offset == c.head - c.start);
    CFF1Subrs *dest = c.start_embed<CFF1Subrs> ();
    if (unlikely (dest == nullptr)) return false;
    if (unlikely (!dest->serialize (&c, *acc.globalSubrs, plan.offsets.globalSubrsInfo.offSize, plan.subrRefMaps.global_map, NULL_SUBR)))
    {
      DEBUG_MSG (SUBSET, nullptr, "failed to serialize CFF global subrs");
      return false;
    }
  }

  /* Encoding */
  if (acc.encoding != &Null(Encoding)){
    assert (plan.offsets.encodingOffset == c.head - c.start);
    Encoding *dest = c.start_embed<Encoding> ();
    if (unlikely (dest == nullptr)) return false;
    if (unlikely (!dest->serialize (&c, *acc.encoding, acc.num_glyphs)))  // XXX: TODO
    {
      DEBUG_MSG (SUBSET, nullptr, "failed to serialize Encoding");
      return false;
    }
  }

  /* Charset */
  if (acc.charset != &Null(Charset))
  {
    assert (plan.offsets.charsetOffset == c.head - c.start);
    Charset *dest = c.start_embed<Charset> ();
    if (unlikely (dest == nullptr)) return false;
    if (unlikely (!dest->serialize (&c, *acc.charset, acc.num_glyphs)))  // XXX: TODO
    {
      DEBUG_MSG (SUBSET, nullptr, "failed to serialize Charset");
      return false;
    }
  }

  /* FDSelect */
  if (acc.fdSelect != &Null(CFF1FDSelect))
  {
    assert (plan.offsets.FDSelectInfo.offset == c.head - c.start);
    
    if (plan.is_fds_subsetted ())
    {
      if (unlikely (!hb_serialize_cff_fdselect (&c, glyphs, *acc.fdSelect, acc.fdCount,
                                                plan.subst_fdselect_format, plan.offsets.FDSelectInfo.size,
                                                plan.subst_fdselect_first_glyphs,
                                                plan.fdmap)))
      {
        DEBUG_MSG (SUBSET, nullptr, "failed to serialize CFF subset FDSelect");
        return false;
      }
    }
    else
    {
      CFF1FDSelect *dest = c.start_embed<CFF1FDSelect> ();
      if (unlikely (!dest->serialize (&c, *acc.fdSelect, acc.num_glyphs)))
      {
        DEBUG_MSG (SUBSET, nullptr, "failed to serialize CFF FDSelect");
        return false;
      }
    }
  }

  /* FDArray (FD Index) */
  if (acc.fdArray != &Null(CFF1FDArray))
  {
    assert (plan.offsets.FDArrayInfo.offset == c.head - c.start);
    CFF1FDArray  *fda = c.start_embed<CFF1FDArray> ();
    if (unlikely (fda == nullptr)) return false;
    CFFFontDict_OpSerializer  fontSzr;
    if (unlikely (!fda->serialize (&c, plan.offsets.FDArrayInfo.offSize,
                                   acc.fontDicts, plan.subst_fdcount, plan.fdmap,
                                   fontSzr, plan.privateDictInfos)))
    {
      DEBUG_MSG (SUBSET, nullptr, "failed to serialize CFF FDArray");
      return false;
    }
  }

  /* CharStrings */
  {
    assert (plan.offsets.charStringsInfo.offset == c.head - c.start);
    CFF1CharStrings  *cs = c.start_embed<CFF1CharStrings> ();
    if (unlikely (cs == nullptr)) return false;
    if (unlikely (!cs->serialize (&c, plan.offsets.charStringsInfo.offSize, plan.subset_charstrings)))
    {
      DEBUG_MSG (SUBSET, nullptr, "failed to serialize CFF CharStrings");
      return false;
    }
  }

  /* private dicts & local subrs */
  assert (plan.offsets.privateDictInfo.offset == c.head - c.start);
  for (unsigned int i = 0; i < acc.privateDicts.len; i++)
  {
    if (!plan.fdmap.excludes (i))
    {
      PrivateDict  *pd = c.start_embed<PrivateDict> ();
      if (unlikely (pd == nullptr)) return false;
      unsigned int priv_size = plan.flatten_subrs? 0: plan.privateDictInfos[plan.fdmap[i]].size;
      bool result;
      CFFPrivateDict_OpSerializer privSzr (plan.drop_hints, plan.flatten_subrs);
      /* N.B. local subrs immediately follows its corresponding private dict. i.e., subr offset == private dict size */
      result = pd->serialize (&c, acc.privateDicts[i], privSzr, priv_size);
      if (unlikely (!result))
      {
        DEBUG_MSG (SUBSET, nullptr, "failed to serialize CFF Private Dict[%d]", i);
        return false;
      }
      if (!plan.flatten_subrs && (acc.privateDicts[i].subrsOffset != 0))
      {
        CFF1Subrs *subrs = c.start_embed<CFF1Subrs> ();
        if (unlikely (subrs == nullptr) || acc.privateDicts[i].localSubrs == &Null(CFF1Subrs))
        {
          DEBUG_MSG (SUBSET, nullptr, "CFF subset: local subrs unexpectedly null [%d]", i);
          return false;
        }
        if (unlikely (!subrs->serialize (&c, *acc.privateDicts[i].localSubrs, plan.offsets.localSubrsInfos[i].offSize, plan.subrRefMaps.local_maps[i], NULL_SUBR)))
        {
          DEBUG_MSG (SUBSET, nullptr, "failed to serialize CFF local subrs [%d]", i);
          return false;
        }
      }
    }
  }

  assert (c.head == c.end);
  c.end_serialize ();

  return true;
}

static bool
_hb_subset_cff1 (const OT::cff1::accelerator_subset_t  &acc,
                const char                      *data,
                hb_subset_plan_t                *plan,
                hb_blob_t                       **prime /* OUT */)
{
  cff_subset_plan cff_plan;

  if (unlikely (!cff_plan.create (acc, plan)))
  {
    DEBUG_MSG(SUBSET, nullptr, "Failed to generate a cff subsetting plan.");
    return false;
  }

  unsigned int  cff_prime_size = cff_plan.get_final_size ();
  char *cff_prime_data = (char *) calloc (1, cff_prime_size);

  if (unlikely (!_write_cff1 (cff_plan, acc, plan->glyphs,
                              cff_prime_size, cff_prime_data))) {
    DEBUG_MSG(SUBSET, nullptr, "Failed to write a subset cff.");
    free (cff_prime_data);
    return false;
  }

  *prime = hb_blob_create (cff_prime_data,
                           cff_prime_size,
                           HB_MEMORY_MODE_READONLY,
                           cff_prime_data,
                           free);
  return true;
}

/**
 * hb_subset_cff1:
 * Subsets the CFF table according to a provided plan.
 *
 * Return value: subsetted cff table.
 **/
bool
hb_subset_cff1 (hb_subset_plan_t *plan,
                hb_blob_t       **prime /* OUT */)
{
  hb_blob_t *cff_blob = hb_sanitize_context_t().reference_table<CFF::cff1> (plan->source);
  const char *data = hb_blob_get_data(cff_blob, nullptr);

  OT::cff1::accelerator_subset_t acc;
  acc.init(plan->source);
  bool result = likely (acc.is_valid ()) &&
                        _hb_subset_cff1 (acc, data, plan, prime);
  hb_blob_destroy (cff_blob);
  acc.fini ();

  return result;
}
