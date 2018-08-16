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

#ifndef HB_OT_CFF1_TABLE_HH
#define HB_OT_CFF1_TABLE_HH

#include "hb-ot-cff-common-private.hh"
#include "hb-subset-cff1.hh"

namespace CFF {

/*
 * CFF -- Compact Font Format (CFF)
 * http://www.adobe.com/content/dam/acom/en/devnet/font/pdfs/5176.CFF.pdf
 */
#define HB_OT_TAG_cff1 HB_TAG('C','F','F',' ')

typedef Index<HBUINT16>   CFF1Index;
template <typename Type> struct CFF1IndexOf : IndexOf<HBUINT16, Type> {};

typedef Index<HBUINT16>   CFF1Index;
typedef CFF1Index         CFF1CharStrings;
typedef FDArray<HBUINT16> CFF1FDArray;
typedef Subrs<HBUINT16>   CFF1Subrs;

struct CFF1FDSelect : FDSelect {};

/* Encoding */
struct Encoding0 {
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) && codes[nCodes - 1].sanitize (c));
  }

  inline bool get_code (hb_codepoint_t glyph, hb_codepoint_t &code) const
  {
    if (glyph < nCodes)
    {
      code = (hb_codepoint_t)codes[glyph];
      return true;
    }
    else
      return false;
  }

  inline unsigned int get_size (void) const
  { return HBUINT8::static_size * (nCodes + 1); }

  HBUINT8     nCodes;
  HBUINT8     codes[VAR];

  DEFINE_SIZE_ARRAY(1, codes);
};

struct Encoding1_Range {
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  HBUINT8   first;
  HBUINT8   nLeft;

  DEFINE_SIZE_STATIC (2);
};

struct Encoding1 {
  inline unsigned int get_size (void) const
  { return HBUINT8::static_size + Encoding1_Range::static_size * nRanges; }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) && ((nRanges == 0) || (ranges[nRanges - 1]).sanitize (c)));
  }

  inline bool get_code (hb_codepoint_t glyph, hb_codepoint_t &code) const
  {
    for (unsigned int i = 0; i < nRanges; i++)
    {
      if (glyph <= ranges[i].nLeft)
      {
        code = (hb_codepoint_t)ranges[i].first + glyph;
        return true;
      }
      glyph -= (ranges[i].nLeft + 1);
    }
  
    return false;
  }

  HBUINT8           nRanges;
  Encoding1_Range   ranges[VAR];

  DEFINE_SIZE_ARRAY (1, ranges);
};

struct EncSupplement {
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  HBUINT8   code;
  HBUINT16  glyph;

  DEFINE_SIZE_STATIC (3);
};

struct CFF1SuppEncData {
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) && ((nSups == 0) || (supps[nSups - 1]).sanitize (c)));
  }

  inline bool get_code (hb_codepoint_t glyph, hb_codepoint_t &code) const
  {
    for (unsigned int i = 0; i < nSups; i++)
      if (glyph == supps[i].glyph)
      {
        code = supps[i].code;
        return true;
      }
  
    return false;
  }

  inline unsigned int get_size (void) const
  { return HBUINT8::static_size + EncSupplement::static_size * nSups; }

  HBUINT8         nSups;
  EncSupplement   supps[VAR];

  DEFINE_SIZE_ARRAY (1, supps);
};

struct Encoding {
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);

    if (unlikely (!c->check_struct (this)))
      return_trace (false);
    unsigned int fmt = format & 0x7F;
    if (unlikely (fmt > 1))
      return_trace (false);
    if (unlikely (!((fmt == 0)? u.format0.sanitize (c): u.format1.sanitize (c))))
      return_trace (false);
    return_trace (((format & 0x80) == 0) || suppEncData ().sanitize (c));
  }

  inline bool serialize (hb_serialize_context_t *c, const Encoding &src, unsigned int num_glyphs)
  {
    TRACE_SERIALIZE (this);
    unsigned int size = src.get_size ();
    Encoding *dest = c->allocate_size<Encoding> (size);
    if (unlikely (dest == nullptr)) return_trace (false);
    memcpy (dest, &src, size);
    return_trace (true);
  }

  inline unsigned int calculate_serialized_size (void) const
  { return get_size (); } // XXX: TODO

  inline unsigned int get_size (void) const
  {
    unsigned int size = min_size;
    if ((format & 0x7F) == 0)
      size += u.format0.get_size ();
    else
      size += u.format1.get_size ();
    if ((format & 0x80) != 0)
      size += suppEncData ().get_size ();
    return size;
  }

  inline bool get_code (hb_codepoint_t glyph, hb_codepoint_t &code) const
  {
    // XXX: TODO
    return false;
  }

  inline const CFF1SuppEncData &suppEncData (void) const
  {
    if ((format & 0x7F) == 0)
      return StructAfter<CFF1SuppEncData> (u.format0.codes[u.format0.nCodes-1]);
    else
      return StructAfter<CFF1SuppEncData> (u.format1.ranges[u.format1.nRanges-1]);
  }

  HBUINT8       format;
  union {
    Encoding0   format0;
    Encoding1   format1;
  } u;
  /* CFF1SuppEncData  suppEncData; */

  DEFINE_SIZE_MIN (1);
};

/* Charset */
struct Charset0 {
  inline bool sanitize (hb_sanitize_context_t *c, unsigned int num_glyphs) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) && sids[num_glyphs - 1].sanitize (c));
  }

  inline unsigned int get_sid (hb_codepoint_t glyph)
  {
    if (glyph == 0)
      return 0;
    else
      return sids[glyph - 1];
  }

  inline unsigned int get_size (unsigned int num_glyphs) const
  {
    assert (num_glyphs > 0);
    return HBUINT16::static_size * (num_glyphs - 1);
  }

  HBUINT16  sids[VAR];

  DEFINE_SIZE_ARRAY(0, sids);
};

template <typename TYPE>
struct Charset_Range {
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  HBUINT8   first;
  TYPE      nLeft;

  DEFINE_SIZE_STATIC (1 + TYPE::static_size);
};

template <typename TYPE>
struct Charset1_2 {
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) && ((nRanges == 0) || (ranges[nRanges - 1]).sanitize (c)));
  }

  inline bool get_sid (hb_codepoint_t glyph, unsigned int &sid) const
  {
    for (unsigned int i = 0; i < nRanges; i++)
    {
      if (glyph <= ranges[i].nLeft)
      {
        sid = (hb_codepoint_t)ranges[i].first + glyph;
        return true;
      }
      glyph -= (ranges[i].nLeft + 1);
    }
  
    return false;
  }

  inline unsigned int get_size (void) const
  { return HBUINT8::static_size + Charset_Range<TYPE>::static_size * nRanges; }

  HBUINT8               nRanges;
  Charset_Range<TYPE>   ranges[VAR];

  DEFINE_SIZE_ARRAY (1, ranges);
};

struct Charset {
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);

    if (unlikely (!c->check_struct (this)))
      return_trace (false);
    if (format == 0)
      return_trace (u.format0.sanitize (c, c->get_num_glyphs ()));
    else if (format == 1)
      return_trace (u.format1.sanitize (c));
    else if (likely (format == 2))
      return_trace (u.format2.sanitize (c));
    else
      return_trace (false);
  }

  inline bool serialize (hb_serialize_context_t *c, const Charset &src, unsigned int num_glyphs)
  {
    TRACE_SERIALIZE (this);
    unsigned int size = src.get_size (num_glyphs);
    Charset *dest = c->allocate_size<Charset> (size);
    if (unlikely (dest == nullptr)) return_trace (false);
    memcpy (dest, &src, size);
    return_trace (true);
  }

  inline unsigned int calculate_serialized_size (unsigned int num_glyphs) const
  { return get_size (num_glyphs); } // XXX: TODO

  inline unsigned int get_size (unsigned int num_glyphs) const
  {
    unsigned int size = min_size;
    if (format == 0)
      size += u.format0.get_size (num_glyphs);
    else if (format == 1)
      size += u.format1.get_size ();
    else
      size += u.format2.get_size ();
    return size;
  }

  inline bool get_sid (hb_codepoint_t glyph, unsigned int &sid) const
  {
    // XXX: TODO
    return false;
  }

  HBUINT8       format;
  union {
    Charset0              format0;
    Charset1_2<HBUINT8>   format1;
    Charset1_2<HBUINT16>  format2;
  } u;

  DEFINE_SIZE_MIN (1);
};

struct CFF1TopDictValues : TopDictValues
{
  inline void init (void)
  {
    TopDictValues::init ();

    ros[0] = ros[1] = ros[2] = 0;
    cidCount = 8720;
    EncodingOffset = 0;
    CharsetOffset = 0;
    FDSelectOffset = 0;
    privateDictInfo.init ();
  }

  inline void fini (void)
  {
    TopDictValues::fini ();
  }

  inline bool is_CID (void) const
  { return ros[0] != 0; }

  inline unsigned int calculate_serialized_size (void) const
  {
    unsigned int size = 0;
    for (unsigned int i = 0; i < values.len; i++)
    {
      OpCode op = values[i].op;
      switch (op)
      {
        case OpCode_FDSelect:
          size += OpCode_Size (OpCode_longintdict) + 4 + OpCode_Size (op);
          break;
        default:
          size += TopDictValues::calculate_serialized_op_size (values[i]);
          break;
      }
    }
    return size;
  }

  unsigned int              ros[3]; /* registry, ordering, supplement */
  unsigned int              cidCount;

  enum EncodingID { StandardEncoding = 0, ExpertEncoding = 1 };
  unsigned int  EncodingOffset;
  unsigned int  CharsetOffset;
  unsigned int  FDSelectOffset;
  TableInfo     privateDictInfo;
};

struct CFF1TopDictOpSet : TopDictOpSet
{
  static inline bool process_op (OpCode op, InterpEnv& env, CFF1TopDictValues& dictval)
  {
  
    switch (op) {
      case OpCode_version:
      case OpCode_Notice:
      case OpCode_Copyright:
      case OpCode_FullName:
      case OpCode_FamilyName:
      case OpCode_Weight:
      case OpCode_isFixedPitch:
      case OpCode_ItalicAngle:
      case OpCode_UnderlinePosition:
      case OpCode_UnderlineThickness:
      case OpCode_PaintType:
      case OpCode_CharstringType:
      case OpCode_UniqueID:
      case OpCode_StrokeWidth:
      case OpCode_SyntheticBase:
      case OpCode_PostScript:
      case OpCode_BaseFontName:
      case OpCode_CIDFontVersion:
      case OpCode_CIDFontRevision:
      case OpCode_CIDFontType:
      case OpCode_UIDBase:
      case OpCode_FontBBox:
      case OpCode_XUID:
      case OpCode_BaseFontBlend:
        env.argStack.clear ();
        break;
        
      case OpCode_CIDCount:
        if (unlikely (!env.argStack.check_pop_uint (dictval.cidCount)))
          return false;
        env.argStack.clear ();
        break;

      case OpCode_ROS:
        if (unlikely (!env.argStack.check_pop_uint (dictval.ros[2]) ||
                      !env.argStack.check_pop_uint (dictval.ros[1]) ||
                      !env.argStack.check_pop_uint (dictval.ros[0])))
          return false;
        env.argStack.clear ();
        break;

      case OpCode_Encoding:
        if (unlikely (!env.argStack.check_pop_uint (dictval.EncodingOffset)))
          return false;
        env.argStack.clear ();
        break;

      case OpCode_charset:
        if (unlikely (!env.argStack.check_pop_uint (dictval.CharsetOffset)))
          return false;
        env.argStack.clear ();
        break;

      case OpCode_FDSelect:
        if (unlikely (!env.argStack.check_pop_uint (dictval.FDSelectOffset)))
          return false;
        env.argStack.clear ();
        break;
    
      case OpCode_Private:
        if (unlikely (!env.argStack.check_pop_uint (dictval.privateDictInfo.offset)))
          return false;
        if (unlikely (!env.argStack.check_pop_uint (dictval.privateDictInfo.size)))
          return false;
        env.argStack.clear ();
        break;
    
      default:
        if (unlikely (!TopDictOpSet::process_op (op, env, dictval)))
          return false;
        /* Record this operand below if stack is empty, otherwise done */
        if (!env.argStack.is_empty ()) return true;
        break;
    }

    dictval.pushVal (op, env.substr);
    return true;
  }
};

struct CFF1FontDictValues : DictValues<OpStr>
{
  inline void init (void)
  {
    DictValues<OpStr>::init ();
    privateDictInfo.init ();
  }

  inline void fini (void)
  {
    DictValues<OpStr>::fini ();
  }

  TableInfo   privateDictInfo;
};

struct CFF1FontDictOpSet : DictOpSet
{
  static inline bool process_op (OpCode op, InterpEnv& env, CFF1FontDictValues& dictval)
  {
    switch (op) {
      case OpCode_FontName:
      case OpCode_FontMatrix:
      case OpCode_PaintType:
        env.argStack.clear ();
        break;
      case OpCode_Private:
        if (unlikely (!env.argStack.check_pop_uint (dictval.privateDictInfo.offset)))
          return false;
        if (unlikely (!env.argStack.check_pop_uint (dictval.privateDictInfo.size)))
          return false;
        env.argStack.clear ();
        break;
    
      default:
        if (unlikely (!DictOpSet::process_op (op, env)))
          return false;
        if (!env.argStack.is_empty ()) return true;
        break;
    }

    dictval.pushVal (op, env.substr);
    return true;
  }
};

template <typename VAL>
struct CFF1PrivateDictValues_Base : DictValues<VAL>
{
  inline void init (void)
  {
    DictValues<VAL>::init ();
    subrsOffset = 0;
    localSubrs = &Null(CFF1Subrs);
  }

  inline void fini (void)
  {
    DictValues<VAL>::fini ();
  }

  inline unsigned int calculate_serialized_size (void) const
  {
    unsigned int size = 0;
    for (unsigned int i = 0; i < DictValues<VAL>::values.len; i++)
      if (DictValues<VAL>::values[i].op == OpCode_Subrs)
        size += OpCode_Size (OpCode_shortint) + 2 + OpCode_Size (OpCode_Subrs);
      else
        size += DictValues<VAL>::values[i].str.len;
    return size;
  }

  unsigned int      subrsOffset;
  const CFF1Subrs    *localSubrs;
};

typedef CFF1PrivateDictValues_Base<OpStr> CFF1PrivateDictValues_Subset;
typedef CFF1PrivateDictValues_Base<DictVal> CFF1PrivateDictValues;

struct CFF1PrivateDictOpSet : DictOpSet
{
  static inline bool process_op (OpCode op, InterpEnv& env, CFF1PrivateDictValues& dictval)
  {
    DictVal val;
    val.init ();

    switch (op) {
      case OpCode_BlueValues:
      case OpCode_OtherBlues:
      case OpCode_FamilyBlues:
      case OpCode_FamilyOtherBlues:
      case OpCode_StemSnapH:
      case OpCode_StemSnapV:
        if (unlikely (!env.argStack.check_pop_delta (val.multi_val)))
          return false;
        break;
      case OpCode_StdHW:
      case OpCode_StdVW:
      case OpCode_BlueScale:
      case OpCode_BlueShift:
      case OpCode_BlueFuzz:
      case OpCode_ForceBold:
      case OpCode_LanguageGroup:
      case OpCode_ExpansionFactor:
      case OpCode_initialRandomSeed:
      case OpCode_defaultWidthX:
      case OpCode_nominalWidthX:
        if (unlikely (!env.argStack.check_pop_num (val.single_val)))
          return false;
        env.argStack.clear ();
        break;
      case OpCode_Subrs:
        if (unlikely (!env.argStack.check_pop_uint (dictval.subrsOffset)))
          return false;
        env.argStack.clear ();
        break;

      default:
        if (unlikely (!DictOpSet::process_op (op, env)))
          return false;
        if (!env.argStack.is_empty ()) return true;
        break;
    }

    dictval.pushVal (op, env.substr, val);
    return true;
  }
};

struct CFF1PrivateDictOpSet_Subset : DictOpSet
{
  static inline bool process_op (OpCode op, InterpEnv& env, CFF1PrivateDictValues_Subset& dictval)
  {
    switch (op) {
      case OpCode_BlueValues:
      case OpCode_OtherBlues:
      case OpCode_FamilyBlues:
      case OpCode_FamilyOtherBlues:
      case OpCode_StemSnapH:
      case OpCode_StemSnapV:
      case OpCode_StdHW:
      case OpCode_StdVW:
      case OpCode_BlueScale:
      case OpCode_BlueShift:
      case OpCode_BlueFuzz:
      case OpCode_ForceBold:
      case OpCode_LanguageGroup:
      case OpCode_ExpansionFactor:
      case OpCode_initialRandomSeed:
      case OpCode_defaultWidthX:
      case OpCode_nominalWidthX:
        env.argStack.clear ();
        break;

      case OpCode_Subrs:
        if (unlikely (!env.argStack.check_pop_uint (dictval.subrsOffset)))
          return false;
        env.argStack.clear ();
        break;

      default:
        if (unlikely (!DictOpSet::process_op (op, env)))
          return false;
        if (!env.argStack.is_empty ()) return true;
        break;
    }

    dictval.pushVal (op, env.substr);
    return true;
  }
};

typedef DictInterpreter<CFF1TopDictOpSet, CFF1TopDictValues> CFF1TopDict_Interpreter;
typedef DictInterpreter<CFF1FontDictOpSet, CFF1FontDictValues> CFF1FontDict_Interpreter;
typedef DictInterpreter<CFF1PrivateDictOpSet, CFF1PrivateDictValues> CFF1PrivateDict_Interpreter;

typedef CFF1Index CFF1NameIndex;
typedef CFF1IndexOf<TopDict> CFF1TopDictIndex;
typedef CFF1Index CFF1StringIndex;

}; /* namespace CFF */

namespace OT {

using namespace CFF;

struct cff1
{
  static const hb_tag_t tableTag        = HB_OT_TAG_cff1;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
                  likely (version.major == 1));
  }

  template <typename PrivOpSet, typename PrivDictVal>
  struct accelerator_templ_t
  {
    inline void init (hb_face_t *face)
    {
      topDicts.init ();
      topDicts.resize (1);
      topDicts[0].init ();
      fontDicts.init ();
      privateDicts.init ();
      
      this->blob = sc.reference_table<cff1> (face);

      /* setup for run-time santization */
      sc.init (this->blob);
      sc.start_processing ();
      
      const OT::cff1 *cff = this->blob->template as<OT::cff1> ();

      if (cff == &Null(OT::cff1))
      { fini (); return; }

      nameIndex = &cff->nameIndex (cff);
      if ((nameIndex == &Null (CFF1NameIndex)) || !nameIndex->sanitize (&sc))
      { fini (); return; }

      topDictIndex = &StructAtOffset<CFF1TopDictIndex> (nameIndex, nameIndex->get_size ());
      if ((topDictIndex == &Null (CFF1TopDictIndex)) || !topDictIndex->sanitize (&sc) || (topDictIndex->count == 0))
      { fini (); return; }

      { /* parse top dict */
        const ByteStr topDictStr = (*topDictIndex)[0];
        if (unlikely (!topDictStr.sanitize (&sc))) { fini (); return; }
        CFF1TopDict_Interpreter top_interp;
        top_interp.env.init (topDictStr);
        if (unlikely (!top_interp.interpret (topDicts[0]))) { fini (); return; }
      }
      
      encoding = &Null(Encoding);
      charset = &StructAtOffsetOrNull<Charset> (cff, topDicts[0].CharsetOffset);

      fdCount = 1;
      if (is_CID ())
      {
        fdArray = &StructAtOffsetOrNull<CFF1FDArray> (cff, topDicts[0].FDArrayOffset);
        fdSelect = &StructAtOffsetOrNull<CFF1FDSelect> (cff, topDicts[0].FDSelectOffset);
        if (unlikely ((fdArray == &Null(CFF1FDArray)) || !fdArray->sanitize (&sc) ||
            (fdSelect == &Null(CFF1FDSelect)) || !fdSelect->sanitize (&sc, fdArray->count)))
        { fini (); return; }

        fdCount = fdArray->count;
      }
      else
      {
        fdArray = &Null(CFF1FDArray);
        fdSelect = &Null(CFF1FDSelect);
        if (topDicts[0].EncodingOffset != CFF1TopDictValues::StandardEncoding &&
            topDicts[0].EncodingOffset != CFF1TopDictValues::ExpertEncoding)
        {
          encoding = &StructAtOffsetOrNull<Encoding> (cff, topDicts[0].EncodingOffset);
          if ((encoding == &Null (Encoding)) || !encoding->sanitize (&sc))
          { fini (); return; }
        }
      }

      stringIndex = &StructAtOffset<CFF1StringIndex> (topDictIndex, topDictIndex->get_size ());
      if ((stringIndex == &Null (CFF1StringIndex)) || !stringIndex->sanitize (&sc))
      { fini (); return; }

      globalSubrs = &StructAtOffset<CFF1Subrs> (stringIndex, stringIndex->get_size ());
      if ((globalSubrs != &Null (CFF1Subrs)) && !stringIndex->sanitize (&sc))
      { fini (); return; }

      charStrings = &StructAtOffsetOrNull<CFF1CharStrings> (cff, topDicts[0].charStringsOffset);

      if ((charStrings == &Null(CFF1CharStrings)) || unlikely (!charStrings->sanitize (&sc)))
      { fini (); return; }

      num_glyphs = charStrings->count;
      if (num_glyphs != sc.get_num_glyphs ())
      { fini (); return; }

      privateDicts.resize (fdCount);
      for (unsigned int i = 0; i < fdCount; i++)
        privateDicts[i].init ();

      // parse CID font dicts and gather private dicts
      if (is_CID ())
      {
        for (unsigned int i = 0; i < fdCount; i++)
        {
          ByteStr fontDictStr = (*fdArray)[i];
          if (unlikely (!fontDictStr.sanitize (&sc))) { fini (); return; }
          CFF1FontDictValues  *font;
          CFF1FontDict_Interpreter font_interp;
          font_interp.env.init (fontDictStr);
          font = fontDicts.push ();
          if (unlikely (!font_interp.interpret (*font))) { fini (); return; }
          PrivDictVal  *priv = &privateDicts[i];
          const ByteStr privDictStr (StructAtOffset<UnsizedByteStr> (cff, font->privateDictInfo.offset), font->privateDictInfo.size);
          if (unlikely (!privDictStr.sanitize (&sc))) { fini (); return; }
          DictInterpreter<PrivOpSet, PrivDictVal> priv_interp;
          priv_interp.env.init (privDictStr);
          if (unlikely (!priv_interp.interpret (*priv))) { fini (); return; }

          priv->localSubrs = &StructAtOffsetOrNull<CFF1Subrs> (privDictStr.str, priv->subrsOffset);
          if (priv->localSubrs != &Null(CFF1Subrs) &&
              unlikely (!priv->localSubrs->sanitize (&sc)))
          { fini (); return; }
        }
      }
      else  /* non-CID */
      {
        CFF1TopDictValues  *font = &topDicts[0];
        PrivDictVal  *priv = &privateDicts[0];
        
        const ByteStr privDictStr (StructAtOffset<UnsizedByteStr> (cff, font->privateDictInfo.offset), font->privateDictInfo.size);
        if (unlikely (!privDictStr.sanitize (&sc))) { fini (); return; }
        DictInterpreter<PrivOpSet, PrivDictVal> priv_interp;
        priv_interp.env.init (privDictStr);
        if (unlikely (!priv_interp.interpret (*priv))) { fini (); return; }

        priv->localSubrs = &StructAtOffsetOrNull<CFF1Subrs> (privDictStr.str, priv->subrsOffset);
        if (priv->localSubrs != &Null(CFF1Subrs) &&
            unlikely (!priv->localSubrs->sanitize (&sc)))
        { fini (); return; }
      }
    }

    inline void fini (void)
    {
      sc.end_processing ();
      topDicts[0].fini ();
      topDicts.fini ();
      fontDicts.fini ();
      privateDicts.fini ();
      hb_blob_destroy (blob);
      blob = nullptr;
    }

    inline bool is_valid (void) const { return blob != nullptr; }
    inline bool is_CID (void) const { return topDicts[0].is_CID (); }

    inline bool get_extents (hb_codepoint_t glyph,
           hb_glyph_extents_t *extents) const
    {
      // XXX: TODO
      if (glyph >= num_glyphs)
        return false;
      
      return true;
    }

    protected:
    hb_blob_t               *blob;
    hb_sanitize_context_t   sc;

    public:
    const CFF1NameIndex     *nameIndex;
    const CFF1TopDictIndex  *topDictIndex;
    const CFF1StringIndex   *stringIndex;
    const Encoding          *encoding;
    const Charset           *charset;
    const CFF1Subrs         *globalSubrs;
    const CFF1CharStrings   *charStrings;
    const CFF1FDArray       *fdArray;
    const CFF1FDSelect      *fdSelect;
    unsigned int            fdCount;

    hb_vector_t<CFF1TopDictValues>    topDicts;
    hb_vector_t<CFF1FontDictValues>   fontDicts;
    hb_vector_t<PrivDictVal>          privateDicts;

    unsigned int            num_glyphs;
  };

  typedef accelerator_templ_t<CFF1PrivateDictOpSet, CFF1PrivateDictValues> accelerator_t;
  typedef accelerator_templ_t<CFF1PrivateDictOpSet_Subset, CFF1PrivateDictValues_Subset> accelerator_subset_t;

  inline bool subset (hb_subset_plan_t *plan) const
  {
    hb_blob_t *cff_prime = nullptr;

    bool success = true;
    if (hb_subset_cff1 (plan, &cff_prime)) {
      success = success && plan->add_table (HB_OT_TAG_cff1, cff_prime);
    } else {
      success = false;
    }
    hb_blob_destroy (cff_prime);

    return success;
  }

  public:
  FixedVersion<HBUINT8> version;          /* Version of CFF table. set to 0x0100u */
  OffsetTo<CFF1NameIndex, HBUINT8> nameIndex; /* headerSize = Offset to Name INDEX. */
  HBUINT8               offSize;          /* offset size (unused?) */

  public:
  DEFINE_SIZE_STATIC (4);
};

} /* namespace OT */

#endif /* HB_OT_CFF1_TABLE_HH */
