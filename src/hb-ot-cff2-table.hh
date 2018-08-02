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

#ifndef HB_OT_CFF2_TABLE_HH
#define HB_OT_CFF2_TABLE_HH

#include "hb-ot-cff-common-private.hh"
#include "hb-subset-cff2.hh"

namespace CFF {

/*
 * CFF2 -- Compact Font Format (CFF) Version 2
 * https://docs.microsoft.com/en-us/typography/opentype/spec/cff2
 */
#define HB_OT_TAG_cff2 HB_TAG('C','F','F','2')

struct CFF2VariationStore
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this)) && varStore.sanitize (c));
  }

  inline bool serialize (hb_serialize_context_t *c, const CFF2VariationStore *varStore)
  {
    TRACE_SANITIZE (this);
    unsigned int size_ = varStore->get_size ();
    CFF2VariationStore *dest = c->allocate_size<CFF2VariationStore> (size_);
    if (unlikely (dest == nullptr)) return_trace (false);
    memcpy (dest, varStore, size_);
    return_trace (true);
  }

  inline unsigned int get_size (void) const { return HBUINT16::static_size + size; }

  HBUINT16        size;
  VariationStore  varStore;

  DEFINE_SIZE_MIN (2 + VariationStore::min_size);
};

struct CFF2TopDictValues : DictValues
{
  inline void init (void)
  {
    DictValues::init ();
    charStringsOffset.set (0);
    vstoreOffset.set (0);
    FDArrayOffset.set (0);
    FDSelectOffset.set (0);
  }

  inline void fini (void)
  {
    DictValues::fini ();
    FontMatrix[0] = FontMatrix[3] = 0.001f;
    FontMatrix[1] = FontMatrix[2] = FontMatrix[4] = FontMatrix[5] = 0.0f;
  }

  inline unsigned int calculate_serialized_size (void) const
  {
    unsigned int size = 0;
    for (unsigned int i = 0; i < opStrs.len; i++)
    {
      OpCode op = opStrs[i].op;
      if (op == OpCode_FontMatrix)
        size += opStrs[i].str.len;
      else
        size += OpCode_Size (OpCode_longint) + 4 + OpCode_Size (op);
    }
    return size;
  }

  float     FontMatrix[6];
  LOffsetTo<CharStrings>        charStringsOffset;
  LOffsetTo<CFF2VariationStore> vstoreOffset;
  LOffsetTo<FDArray>            FDArrayOffset;
  LOffsetTo<FDSelect>           FDSelectOffset;
};

struct CFF2TopDictOpSet
{
  static inline bool process_op (const ByteStr& str, unsigned int& offset, OpCode op, Stack& stack, CFF2TopDictValues& val)
  {
    switch (op) {
      case OpCode_CharStrings:
        if (unlikely (!check_pop_offset (stack, val.charStringsOffset)))
          return false;
        val.pushOpStr (op, str, offset + 1);
        break;
      case OpCode_vstore:
        if (unlikely (!check_pop_offset (stack, val.vstoreOffset)))
          return false;
        val.pushOpStr (op, str, offset + 1);
        break;
      case OpCode_FDArray:
        if (unlikely (!check_pop_offset (stack, val.FDArrayOffset)))
          return false;
        val.pushOpStr (op, str, offset + 1);
        break;
      case OpCode_FDSelect:
        if (unlikely (!check_pop_offset (stack, val.FDSelectOffset)))
          return false;
        val.pushOpStr (op, str, offset + 1);
        break;
      case OpCode_FontMatrix:
        if (unlikely (!stack.check_underflow (6)))
          return false;
        for (int i = 0; i < 6; i++)
          val.FontMatrix[i] = stack.pop ().to_real ();
        val.pushOpStr (op, str, offset + 1);
        break;
      case OpCode_longint:  /* 5-byte integer */
        if (unlikely (!str.check_limit (offset, 5) || !stack.check_overflow (1)))
          return false;
        stack.push ((int32_t)*(const HBUINT32*)&str[offset + 1]);
        offset += 4;
        break;
      
      case OpCode_BCD:  /* real number */
        float v;
        if (unlikely (stack.check_overflow (1) || !parse_bcd (str, offset, v)))
          return false;
        stack.push (v);
        break;
    
      default:
        /* XXX: invalid */
        stack.clear ();
        return false;
    }

    return true;
  }
};

struct CFF2FontDictValues : DictValues
{
  inline void init (void)
  {
    DictValues::init ();
    privateDictSize = 0;
    privateDictOffset.set (0);
  }

  inline void fini (void)
  {
    DictValues::fini ();
  }

  unsigned int            privateDictSize;
  LOffsetTo<PrivateDict>  privateDictOffset;
};

struct CFF2FontDictOpSet
{
  static inline bool process_op (const ByteStr& str, unsigned int& offset, OpCode op, Stack& stack, CFF2FontDictValues& val)
  {
    switch (op) {
      case OpCode_Private:
        if (unlikely (!check_pop_offset (stack, val.privateDictOffset)))
          return false;
        if (unlikely (!stack.check_pop_uint (val.privateDictSize)))
          return false;
        val.pushOpStr (op, str, offset + 1);
        break;
      case OpCode_longint:  /* 5-byte integer */
        if (unlikely (!str.check_limit (offset, 5) || !stack.check_overflow (1)))
          return false;
        stack.push ((int32_t)((str[offset + 1] << 24) | ((uint32_t)str[offset + 2] << 16) | ((uint32_t)str[offset + 3] << 8) | str[offset + 4]));
        offset += 4;
        break;
      case OpCode_BCD:  /* real number */
        float v;
        if (unlikely (stack.check_overflow (1) || !parse_bcd (str, offset, v)))
          return false;
        stack.push (v);
        break;
    
      default:
        /* XXX: invalid */
        stack.clear ();
        return false;
    }

    return true;
  }
};

struct CFF2PrivateDictValues : DictValues
{
  inline void init (void)
  {
    DictValues::init ();

    languageGroup = 0;
    expansionFactor = 0.06f;
    vsIndex = 0;
    subrsOffset.set (0);
    blueScale = 0.039625f;
    blueShift = 7.0f;
    blueFuzz = 1.0f;
    stdHW = UNSET_REAL_VALUE;
    stdVW = UNSET_REAL_VALUE;
    subrsOffset.set (0);
    blueValues.init ();
    otherBlues.init ();
    familyBlues.init ();
    familyOtherBlues.init ();
    stemSnapH.init ();
    stemSnapV.init ();

    localSubrs = &Null(Subrs);
  }

  inline void fini (void)
  {
    blueValues.fini ();
    otherBlues.fini ();
    familyBlues.fini ();
    familyOtherBlues.fini ();
    stemSnapH.fini ();
    stemSnapV.fini ();

    DictValues::fini ();
  }

  inline unsigned int calculate_serialized_size (void) const
  {
    unsigned int size = 0;
    for (unsigned int i = 0; i < opStrs.len; i++)
      if (opStrs[i].op == OpCode_Subrs)
        size += OpCode_Size (OpCode_shortint) + 2 + OpCode_Size (OpCode_Subrs);
      else
        size += opStrs[i].str.len;
    return size;
  }

  int       languageGroup;
  float     expansionFactor;
  int       vsIndex;
  OffsetTo<Subrs>  subrsOffset;
  float     blueScale;
  float     blueShift;
  float     blueFuzz;
  float     stdHW;
  float     stdVW;
  hb_vector_t <float> blueValues;
  hb_vector_t <float> otherBlues;
  hb_vector_t <float> familyBlues;
  hb_vector_t <float> familyOtherBlues;
  hb_vector_t <float> stemSnapH;
  hb_vector_t <float> stemSnapV;

  const Subrs *localSubrs;
};

struct CFF2PrivateDictOpSet
{
  static inline bool process_op (const ByteStr& str, unsigned int& offset, OpCode op, Stack& stack, CFF2PrivateDictValues& val)
  {
    switch (op) {
      case OpCode_BlueValues:
        if (unlikely (!stack.check_pop_delta (val.blueValues)))
          return false;
        break;
      case OpCode_OtherBlues:
        if (unlikely (!stack.check_pop_delta (val.otherBlues)))
          return false;
        break;
      case OpCode_FamilyBlues:
        if (unlikely (!stack.check_pop_delta (val.familyBlues)))
          return false;
        break;
      case OpCode_FamilyOtherBlues:
        if (unlikely (!stack.check_pop_delta (val.familyOtherBlues)))
          return false;
        break;
      case OpCode_StdHW:
        if (unlikely (!stack.check_pop_real (val.stdHW)))
          return false;
        break;
      case OpCode_StdVW:
        if (unlikely (!stack.check_pop_real (val.stdVW)))
          return false;
        break;
      case OpCode_BlueScale:
        if (unlikely (!stack.check_pop_real (val.blueScale)))
          return false;
        break;
      case OpCode_BlueShift:
        if (unlikely (!stack.check_pop_real (val.blueShift)))
          return false;
        break;
      case OpCode_BlueFuzz:
        if (unlikely (!stack.check_pop_real (val.blueFuzz)))
          return false;
        break;
      case OpCode_StemSnapH:
        if (unlikely (!stack.check_pop_delta (val.stemSnapH)))
          return false;
        break;
      case OpCode_StemSnapV:
        if (unlikely (!stack.check_pop_delta (val.stemSnapV)))
          return false;
        break;
      case OpCode_LanguageGroup:
        if (unlikely (!stack.check_pop_int (val.languageGroup)))
          return false;
        break;
      case OpCode_ExpansionFactor:
        if (unlikely (!stack.check_pop_real (val.expansionFactor)))
          return false;
        break;
      case OpCode_Subrs:
        if (unlikely (!check_pop_offset (stack, val.subrsOffset)))
          return false;
        break;
      case OpCode_blend:
        // XXX: TODO
        break;
      case OpCode_longint:  /* 5-byte integer */
        if (unlikely (!str.check_limit (offset, 5) || !stack.check_overflow (1)))
          return false;
        stack.push ((int32_t)((str[offset + 1] << 24) | (str[offset + 2] << 16) || (str[offset + 3] << 8) || str[offset + 4]));
        offset += 4;
        break;
      case OpCode_BCD:  /* real number */
        float v;
        if (unlikely (!stack.check_overflow (1) || !parse_bcd (str, offset, v)))
          return false;
        stack.push (v);
        break;

      default:
        return false;
    }

    if (op != OpCode_blend)
      val.pushOpStr (op, str, offset + 1);

    return true;
  }
};

typedef Interpreter<CFF2TopDictOpSet, CFF2TopDictValues> CFF2TopDict_Interpreter;
typedef Interpreter<CFF2FontDictOpSet, CFF2FontDictValues> CFF2FontDict_Interpreter;
typedef Interpreter<CFF2PrivateDictOpSet, CFF2PrivateDictValues> CFF2PrivateDict_Interpreter;

}; /* namespace CFF */

namespace OT {

using namespace CFF;

struct cff2
{
  static const hb_tag_t tableTag        = HB_OT_TAG_cff2;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
                  likely (version.major == 2));
  }

  struct accelerator_t
  {
    inline void init (hb_face_t *face)
    {
      fontDicts.init ();
      privateDicts.init ();
      
      this->blob = sc.reference_table<cff2> (face);

      /* setup for run-time santization */
      sc.init (this->blob);
      sc.start_processing ();
      
      const OT::cff2 *cff2 = this->blob->as<OT::cff2> ();

      if (cff2 == &Null(OT::cff2))
      {
        fini ();
        return;
      }

      { /* parse top dict */
        ByteStr topDictStr (cff2 + cff2->topDict, cff2->topDictSize);
        CFF2TopDict_Interpreter top_interp;
        if (unlikely (!topDictStr.sanitize (&sc) ||
                      !top_interp.interpret (topDictStr, top)))
        {
          fini ();
          return;
        }
      }
      
      globalSubrs = &OT::StructAtOffset<Subrs> (cff2, cff2->topDict + cff2->topDictSize);
      varStore = &top.vstoreOffset (cff2);
      charStrings = &top.charStringsOffset (cff2);
      fdArray = &top.FDArrayOffset (cff2);
      fdSelect = &top.FDSelectOffset (cff2);
      
      if (((varStore != &Null(CFF2VariationStore)) && unlikely (!varStore->sanitize (&sc))) ||
          ((charStrings == &Null(CharStrings)) || unlikely (!charStrings->sanitize (&sc))) ||
          ((fdArray == &Null(FDArray)) || unlikely (!fdArray->sanitize (&sc))) ||
          ((fdSelect != &Null(FDSelect)) && unlikely (!fdSelect->sanitize (&sc))))
      {
        fini ();
        return;
      }

      num_glyphs = charStrings->count;
      if (num_glyphs != sc.get_num_glyphs ())
      {
        fini ();
        return;
      }

      privateDicts.resize (fdArray->count);

      // parse font dicts and gather private dicts
      for (unsigned int i = 0; i < fdArray->count; i++)
      {
        const ByteStr fontDictStr = (*fdArray)[i];
        CFF2FontDictValues  *font;
        CFF2FontDict_Interpreter font_interp;
        font = fontDicts.push ();
        if (unlikely (!fontDictStr.sanitize (&sc) ||
                      !font_interp.interpret (fontDictStr, *font)))
        {
          fini ();
          return;
        }

        const ByteStr privDictStr (font->privateDictOffset (cff2), font->privateDictSize);
        CFF2PrivateDict_Interpreter priv_interp;
        if (unlikely (!privDictStr.sanitize (&sc) ||
                      !priv_interp.interpret (privDictStr, privateDicts[i])))
        {
          fini ();
          return;
        }

        privateDicts[i].localSubrs = &privateDicts[i].subrsOffset (cff2);
      }
    }

    inline void fini (void)
    {
      sc.end_processing ();
      fontDicts.fini ();
      privateDicts.fini ();
      hb_blob_destroy (blob);
      blob = nullptr;
    }

    inline bool is_valid (void) const { return blob != nullptr; }

    inline bool get_extents (hb_codepoint_t glyph,
           hb_glyph_extents_t *extents) const
    {
      // XXX: TODO
      if (glyph >= num_glyphs)
        return false;
      
      return true;
    }

    private:
    hb_blob_t               *blob;
    hb_sanitize_context_t   sc;

    public:
    CFF2TopDictValues         top;
    const Subrs               *globalSubrs;
    const CFF2VariationStore  *varStore;
    const CharStrings         *charStrings;
    const FDArray             *fdArray;
    const FDSelect            *fdSelect;

    hb_vector_t<CFF2FontDictValues>     fontDicts;
    hb_vector_t<CFF2PrivateDictValues>  privateDicts;

    unsigned int            num_glyphs;
  };

  inline bool subset (hb_subset_plan_t *plan) const
  {
    hb_blob_t *cff2_prime = nullptr;

    bool success = true;
    if (hb_subset_cff2 (plan, &cff2_prime)) {
      success = success && plan->add_table (HB_OT_TAG_cff2, cff2_prime);
    } else {
      success = false;
    }
    hb_blob_destroy (cff2_prime);

    return success;
  }

  public:
  FixedVersion<HBUINT8> version;        /* Version of CFF2 table. set to 0x0200u */
  OffsetTo<TopDict, HBUINT8> topDict;   /* headerSize = Offset to Top DICT. */
  HBUINT16       topDictSize;           /* Top DICT size */

  public:
  DEFINE_SIZE_STATIC (5);
};

} /* namespace OT */

#endif /* HB_OT_CFF2_TABLE_HH */
