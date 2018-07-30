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

struct CFF2TopDictValues
{
  inline void init ()
  {
    charStringsOffset.set (0);
    vstoreOffset.set (0);
    FDArrayOffset.set (0);
    FDSelectOffset.set (0);
    FontMatrix[0] = FontMatrix[3] = 0.001f;
    FontMatrix[1] = FontMatrix[2] = FontMatrix[4] = FontMatrix[5] = 0.0f;
  }

  LOffsetTo<CharStrings>    charStringsOffset;
  LOffsetTo<VariationStore> vstoreOffset;
  LOffsetTo<FDArray>        FDArrayOffset;
  LOffsetTo<FDSelect>       FDSelectOffset;
  float     FontMatrix[6];
};

struct CFF2TopDictOpSet
{
  static inline bool process_op (const ByteStr& str, unsigned int& offset, unsigned char byte, Stack& stack, CFF2TopDictValues& val)
  {
    switch (byte) {
      case OpCode_CharStrings:
        if (unlikely (!check_pop_offset (stack, val.charStringsOffset)))
          return false;
        break;
      case OpCode_vstore:
        if (unlikely (!check_pop_offset (stack, val.vstoreOffset)))
          return false;
        break;
      case OpCode_FDArray:
        if (unlikely (!check_pop_offset (stack, val.FDArrayOffset)))
          return false;
        break;
      case OpCode_FDSelect:
        if (unlikely (!check_pop_offset (stack, val.FDSelectOffset)))
          return false;
        break;
      case OpCode_FontMatrix:
        if (unlikely (!stack.check_underflow (6)))
          return false;
        for (int i = 0; i < 6; i++)
          val.FontMatrix[i] = stack.pop ().to_real ();
        break;
      case OpCode_longint:  /* 5-byte integer */
        if (unlikely (!str.check_limit (offset, 5) || !stack.check_overflow (1)))
          return false;
        stack.push ((int32_t)((str[offset + 1] << 24) | (str[offset + 2] << 16) | (str[offset + 3] << 8) | str[offset + 4]));
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

struct CFF2FontDictValues
{
  inline void init ()
  {
    privateDictSize = 0;
    privateDictOffset.set (0);
  }

  unsigned int    privateDictSize;
  OffsetTo<Dict>  privateDictOffset;
};

struct CFF2FontDictOpSet
{
  static inline bool process_op (const ByteStr& str, unsigned int& offset, unsigned char byte, Stack& stack, CFF2FontDictValues& val)
  {
    switch (byte) {
      case OpCode_Private:
        if (unlikely (!stack.check_pop_uint (val.privateDictSize)))
          return false;
        if (unlikely (!check_pop_offset (stack, val.privateDictOffset)))
          return false;
        break;
      case OpCode_longint:  /* 5-byte integer */
        if (unlikely (!str.check_limit (offset, 5) || !stack.check_overflow (1)))
          return false;
        stack.push ((int32_t)((str[offset + 1] << 24) | (str[offset + 2] << 16) || (str[offset + 3] << 8) || str[offset + 4]));
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

struct CFF2PrivateDictValues
{
  inline void init ()
  {
    languageGroup = 0;
    expansionFactor = 0.06f;
    vsIndex = 0;
    subrsOffset.set (0);
    blueScale = 0.039625f;
    blueShift = 7.0f;
    blueFuzz = 1.0f;
    stdHW = UNSET_REAL_VALUE;
    stdVW = UNSET_REAL_VALUE;
    blueValues.init ();
    otherBlues.init ();
    familyBlues.init ();
    familyOtherBlues.init ();
    stemSnapH.init ();
    stemSnapV.init ();
  }

  inline void fini ()
  {
    blueValues.fini ();
    otherBlues.fini ();
    familyBlues.fini ();
    familyOtherBlues.fini ();
    stemSnapH.fini ();
    stemSnapV.fini ();
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
};

struct CFF2PrivateDictOpSet
{
  static inline bool process_op (const ByteStr& str, unsigned int& offset, unsigned char byte, Stack& stack, CFF2PrivateDictValues& val)
  {
    switch (byte) {
      case OpCode_BlueValues:
        if (unlikely (!stack.check_pop_delta (val.blueValues, true)))
          return false;
        break;
      case OpCode_OtherBlues:
        if (unlikely (!stack.check_pop_delta (val.otherBlues, true)))
          return false;
        break;
      case OpCode_FamilyBlues:
        if (unlikely (!stack.check_pop_delta (val.familyBlues, true)))
          return false;
        break;
      case OpCode_FamilyOtherBlues:
        if (unlikely (!stack.check_pop_delta (val.familyOtherBlues, true)))
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

      default:
        return false;
    }
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
      hb_sanitize_context_t c;
      this->blob = c.reference_table<cff2> (face);
      const OT::cff2 *cff2 = this->blob->as<OT::cff2> ();

      if (cff2 == &Null(OT::cff2))
      {
        fini ();
        return;
      }

      { /* parse top dict */
        ByteStr topDictStr (cff2 + cff2->topDict, cff2->topDictSize);
        if (unlikely (!topDictStr.sanitize (&c)))
        {
          fini ();
          return;
        }
        CFF2TopDict_Interpreter top_interp;
        top_interp.init ();
        top_interp.interpret (topDictStr, top);
        top_interp.fini ();
      }
      
      varStore = &OT::StructAtOffset<VariationStore>(cff2, top.vstoreOffset);
      charStrings = &OT::StructAtOffset<CharStrings>(cff2, top.charStringsOffset);
      fdArray = &OT::StructAtOffset<FDArray>(cff2, top.FDArrayOffset);
      fdSelect = &OT::StructAtOffset<FDSelect>(cff2, top.FDSelectOffset);
      
      if (((varStore != &Null(VariationStore)) && unlikely (!varStore->sanitize (&c))) ||
          ((charStrings == &Null(CharStrings)) || unlikely (!charStrings->sanitize (&c))) ||
          ((fdArray == &Null(FDArray)) || unlikely (!fdArray->sanitize (&c))) ||
          ((fdSelect == &Null(FDSelect)) || unlikely (!fdSelect->sanitize (&c))))
      {
        fini ();
        return;
      }

      num_glyphs = charStrings->count;
      if (num_glyphs != c.get_num_glyphs ())
      {
        fini ();
        return;
      }
    }

    inline void fini (void)
    {
      hb_blob_destroy (blob);
      blob = nullptr;
    }

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

    CFF2TopDictValues       top;
    const VariationStore    *varStore;
    const CharStrings       *charStrings;
    const FDArray           *fdArray;
    const FDSelect          *fdSelect;

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

  protected:
  FixedVersion<HBUINT8> version;    /* Version of CFF2 table. set to 0x0200u */
  OffsetTo<Dict, HBUINT8> topDict;  /* headerSize = Offset to Top DICT. */
  HBUINT16       topDictSize;       /* Top DICT size */

  public:
  DEFINE_SIZE_STATIC (5);
};

} /* namespace OT */

#endif /* HB_OT_CFF2_TABLE_HH */
