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

#include "hb-open-type-private.hh"
#include "hb-ot-cff-table.hh"
#include "hb-set.h"
#include "hb-subset-cff.hh"
#include "hb-subset-plan.hh"
#include "hb-subset-cff-common-private.hh"
#include "hb-cff-interp-cs.hh"

using namespace CFF;

struct CFFSubTableOffsets {
  inline CFFSubTableOffsets (void)
  {
    memset (this, 0, sizeof(*this));
    localSubrsInfos.init ();
  }

  inline ~CFFSubTableOffsets (void)
  {
    localSubrsInfos.fini ();
  }

  unsigned int  nameIndexOffset;
  TableInfo     topDictInfo;
  unsigned int  stringIndexOffset;
  TableInfo     globalSubrsInfo;
  unsigned int  encodingOffset;
  unsigned int  charsetOffset;
  TableInfo     FDSelectInfo;
  TableInfo     FDArrayInfo;
  TableInfo     charStringsInfo;
  TableInfo     privateDictInfo;
  hb_vector_t<TableInfo>  localSubrsInfos;
};

struct CFFTopDict_OpSerializer : OpSerializer
{
  inline bool serialize (hb_serialize_context_t *c,
                         const OpStr &opstr,
                         const CFFSubTableOffsets &offsets) const
  {
    TRACE_SERIALIZE (this);

    switch (opstr.op)
    {
      case OpCode_charset:
        return_trace (FontDict::serialize_offset4_op(c, opstr.op, offsets.charsetOffset));

      case OpCode_Encoding:
        return_trace (FontDict::serialize_offset4_op(c, opstr.op, offsets.encodingOffset));

      case OpCode_CharStrings:
        return_trace (FontDict::serialize_offset4_op(c, opstr.op, offsets.charStringsInfo.offset));

      case OpCode_FDArray:
        return_trace (FontDict::serialize_offset4_op(c, opstr.op, offsets.FDArrayInfo.offset));

      case OpCode_FDSelect:
        return_trace (FontDict::serialize_offset4_op(c, opstr.op, offsets.FDSelectInfo.offset));

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
        return_trace (copy_opstr (c, opstr));
    }
    return_trace (true);
  }

  inline unsigned int calculate_serialized_size (const OpStr &opstr) const
  {
    switch (opstr.op)
    {
      case OpCode_charset:
      case OpCode_Encoding:
      case OpCode_CharStrings:
      case OpCode_FDArray:
      case OpCode_FDSelect:
        return OpCode_Size (OpCode_longintdict) + 4 + OpCode_Size (opstr.op);
    
      case OpCode_Private:
        return OpCode_Size (OpCode_longintdict) + 4 + OpCode_Size (OpCode_shortint) + 2 + OpCode_Size (OpCode_Private);
    
      default:
        return opstr.str.len;
    }
  }
};

struct CFFFontDict_OpSerializer : OpSerializer
{
  inline bool serialize (hb_serialize_context_t *c,
                         const OpStr &opstr,
                         const TableInfo &privateDictInfo) const
  {
    TRACE_SERIALIZE (this);

    if (opstr.op == OpCode_Private)
    {
      /* serialize the private dict size as a 2-byte integer */
      if (unlikely (!UnsizedByteStr::serialize_int2 (c, privateDictInfo.size)))
        return_trace (false);

      /* serialize the private dict offset as a 4-byte integer */
      if (unlikely (!UnsizedByteStr::serialize_int4 (c, privateDictInfo.offset)))
        return_trace (false);

      /* serialize the opcode */
      HBUINT8 *p = c->allocate_size<HBUINT8> (1);
      if (unlikely (p == nullptr)) return_trace (false);
      p->set (OpCode_Private);

      return_trace (true);
    }
    else
    {
      HBUINT8 *d = c->allocate_size<HBUINT8> (opstr.str.len);
      if (unlikely (d == nullptr)) return_trace (false);
      memcpy (d, &opstr.str.str[0], opstr.str.len);
    }
    return_trace (true);
  }

  inline unsigned int calculate_serialized_size (const OpStr &opstr) const
  {
    if (opstr.op == OpCode_Private)
      return OpCode_Size (OpCode_longintdict) + 4 + OpCode_Size (OpCode_shortint) + 2 + OpCode_Size (OpCode_Private);
    else
      return opstr.str.len;
  }
};

struct CFFPrivateDict_OpSerializer : OpSerializer
{
  inline bool serialize (hb_serialize_context_t *c,
                         const OpStr &opstr,
                         const unsigned int subrsOffset) const
  {
    TRACE_SERIALIZE (this);

    if (opstr.op == OpCode_Subrs)
      return_trace (FontDict::serialize_offset2_op(c, OpCode_Subrs, subrsOffset));
    else
      return_trace (copy_opstr (c, opstr));
  }

  inline unsigned int calculate_serialized_size (const OpStr &opstr) const
  {
    if (opstr.op == OpCode_Subrs)
      return OpCode_Size (OpCode_shortint) + 2 + OpCode_Size (OpCode_Subrs);
    else
      return opstr.str.len;
  }
};

struct CFFCSOpSet_SubrSubset : CFFCSOpSet<SubrRefMapPair>
{
  static inline bool process_op (OpCode op, CFFCSInterpEnv &env, SubrRefMapPair& refMapPair)
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
    return CFFCSOpSet<SubrRefMapPair>::process_op (op, env, refMapPair);
  }
};

struct cff_subset_plan {
  inline cff_subset_plan (void)
    : final_size (0),
      orig_fdcount (0),
      subst_fdcount(1),
      subst_fdselect_format (0),
      offsets()
  {
    topdict_sizes.init ();
    topdict_sizes.resize (1);
    subst_fdselect_first_glyphs.init ();
    fdmap.init ();
    subset_charstrings.init ();
    privateDictInfos.init ();
  }

  inline ~cff_subset_plan (void)
  {
    topdict_sizes.fini ();
    subst_fdselect_first_glyphs.fini ();
    fdmap.fini ();
    subset_charstrings.fini ();
    privateDictInfos.fini ();
    subrRefMaps.fini ();
  }

  inline bool create (const OT::cff::accelerator_subset_t &acc,
                      hb_subset_plan_t *plan)
  {
    final_size = 0;
    orig_fdcount = acc.fdCount;

    /* CFF header */
    final_size += OT::cff::static_size;
    
    /* Name INDEX */
    offsets.nameIndexOffset = final_size;
    final_size += acc.nameIndex->get_size ();
    
    /* top dict INDEX */
    {
      offsets.topDictInfo.offset = final_size;
      CFFTopDict_OpSerializer topSzr;
      unsigned int topDictSize = TopDict::calculate_serialized_size (acc.topDicts[0], topSzr);
      offsets.topDictInfo.offSize = calcOffSize(topDictSize);
      final_size += CFFIndexOf<TopDict>::calculate_serialized_size<CFFTopDictValues> (offsets.topDictInfo.offSize, acc.topDicts, topdict_sizes, topSzr);
    }

    /* String INDEX */
    offsets.stringIndexOffset = final_size;
    final_size += acc.stringIndex->get_size ();
    
    /* Subset global & local subrs */
    {
      SubrSubsetter<const OT::cff::accelerator_subset_t, CFFCSInterpEnv, CFFCSOpSet_SubrSubset> subsetter(acc, plan->glyphs);
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
    if (acc.fdSelect != &Null(CFFFDSelect))
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
    if (acc.fdArray != &Null(CFFFDArray)) {
      offsets.FDArrayInfo.offset = final_size;
      CFFFontDict_OpSerializer fontSzr;
      final_size += CFFFDArray::calculate_serialized_size(offsets.FDArrayInfo.offSize/*OUT*/, acc.fontDicts, subst_fdcount, fdmap, fontSzr);
    }

    /* CharStrings */
    {
      offsets.charStringsInfo.offset = final_size;
      unsigned int dataSize = 0;
      for (unsigned int i = 0; i < plan->glyphs.len; i++)
      {
        const ByteStr str = (*acc.charStrings)[plan->glyphs[i]];
        subset_charstrings.push (str);
        dataSize += str.len;
      }
      offsets.charStringsInfo.offSize = calcOffSize (dataSize + 1);
      final_size += CFFCharStrings::calculate_serialized_size (offsets.charStringsInfo.offSize, plan->glyphs.len, dataSize);
    }

    /* private dicts & local subrs */
    offsets.privateDictInfo.offset = final_size;
    for (unsigned int i = 0; i < orig_fdcount; i++)
    {
      if (!fdmap.excludes (i))
      {
        CFFPrivateDict_OpSerializer privSzr;
        TableInfo  privInfo = { final_size, PrivateDict::calculate_serialized_size (acc.privateDicts[i], privSzr), 0 };
        privateDictInfos.push (privInfo);
        final_size += privInfo.size + offsets.localSubrsInfos[i].size;
      }
    }

    if (!acc.is_CID ())
      offsets.privateDictInfo = privateDictInfos[0];

    return true;
  }

  inline unsigned int get_final_size (void) const  { return final_size; }

  unsigned int        final_size;
  hb_vector_t<unsigned int> topdict_sizes;
  CFFSubTableOffsets  offsets;

  unsigned int    orig_fdcount;
  unsigned int    subst_fdcount;
  inline bool     is_fds_subsetted (void) const { return subst_fdcount < orig_fdcount; }
  unsigned int    subst_fdselect_format;
  hb_vector_t<hb_codepoint_t>   subst_fdselect_first_glyphs;

  /* font dict index remap table from fullset FDArray to subset FDArray.
   * set to HB_SET_VALUE_INVALID if excluded from subset */
  FDMap   fdmap;

  hb_vector_t<ByteStr> subset_charstrings;
  hb_vector_t<TableInfo> privateDictInfos;

  SubrRefMaps             subrRefMaps;
};

static inline bool _write_cff (const cff_subset_plan &plan,
                                const OT::cff::accelerator_subset_t  &acc,
                                const hb_vector_t<hb_codepoint_t>& glyphs,
                                unsigned int dest_sz,
                                void *dest)
{
  hb_serialize_context_t c (dest, dest_sz);

  char RETURN_OP[1] = { OpCode_return };
  const ByteStr NULL_SUBR (RETURN_OP, 1);

  OT::cff *cff = c.start_serialize<OT::cff> ();
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
    NameIndex *dest = c.start_embed<NameIndex> ();
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
    CFFIndexOf<TopDict> *dest = c.start_embed< CFFIndexOf<TopDict> > ();
    if (dest == nullptr) return false;
    CFFTopDict_OpSerializer topSzr;
    if (unlikely (!dest->serialize (&c, plan.offsets.topDictInfo.offSize, acc.topDicts, plan.topdict_sizes, topSzr, plan.offsets)))
    {
      DEBUG_MSG (SUBSET, nullptr, "failed to serialize CFF top dict");
      return false;
    }
  }

  /* String INDEX */
  {
    assert (plan.offsets.stringIndexOffset == c.head - c.start);
    StringIndex *dest = c.start_embed<StringIndex> ();
    if (unlikely (dest == nullptr)) return false;
    if (unlikely (!dest->serialize (&c, *acc.stringIndex)))
    {
      DEBUG_MSG (SUBSET, nullptr, "failed to serialize CFF string INDEX");
      return false;
    }
  }

  /* global subrs */
  {
    assert (plan.offsets.globalSubrsInfo.offset == c.head - c.start);
    CFFSubrs *dest = c.start_embed<CFFSubrs> ();
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
  if (acc.fdSelect != &Null(CFFFDSelect))
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
      CFFFDSelect *dest = c.start_embed<CFFFDSelect> ();
      if (unlikely (!dest->serialize (&c, *acc.fdSelect, acc.num_glyphs)))
      {
        DEBUG_MSG (SUBSET, nullptr, "failed to serialize CFF FDSelect");
        return false;
      }
    }
  }

  /* FDArray (FD Index) */
  if (acc.fdArray != &Null(CFFFDArray))
  {
    assert (plan.offsets.FDArrayInfo.offset == c.head - c.start);
    CFFFDArray  *fda = c.start_embed<CFFFDArray> ();
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
    CFFCharStrings  *cs = c.start_embed<CFFCharStrings> ();
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
      CFFPrivateDict_OpSerializer privSzr;
      /* N.B. local subrs immediately follows its corresponding private dict. i.e., subr offset == private dict size */
      if (unlikely (!pd->serialize (&c, acc.privateDicts[i], privSzr, plan.privateDictInfos[plan.fdmap[i]].size)))
      {
        DEBUG_MSG (SUBSET, nullptr, "failed to serialize CFF Private Dict[%d]", i);
        return false;
      }
      if (acc.privateDicts[i].subrsOffset != 0)
      {
        CFFSubrs *subrs = c.start_embed<CFFSubrs> ();
        if (unlikely (subrs == nullptr) || acc.privateDicts[i].localSubrs == &Null(CFFSubrs))
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

  c.end_serialize ();

  return true;
}

static bool
_hb_subset_cff (const OT::cff::accelerator_subset_t  &acc,
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

  if (unlikely (!_write_cff (cff_plan, acc, plan->glyphs,
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
 * hb_subset_cff:
 * Subsets the CFF table according to a provided plan.
 *
 * Return value: subsetted cff table.
 **/
bool
hb_subset_cff (hb_subset_plan_t *plan,
                hb_blob_t       **prime /* OUT */)
{
  hb_blob_t *cff_blob = hb_sanitize_context_t().reference_table<CFF::cff> (plan->source);
  const char *data = hb_blob_get_data(cff_blob, nullptr);

  OT::cff::accelerator_subset_t acc;
  acc.init(plan->source);
  bool result = likely (acc.is_valid ()) &&
                        _hb_subset_cff (acc, data, plan, prime);
  hb_blob_destroy (cff_blob);
  acc.fini ();

  return result;
}
