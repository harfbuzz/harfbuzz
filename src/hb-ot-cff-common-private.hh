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
#ifndef HB_OT_CFF_COMMON_PRIVATE_HH
#define HB_OT_CFF_COMMON_PRIVATE_HH

#include "hb-open-type-private.hh"
#include "hb-ot-layout-common-private.hh"
#include "hb-subset-plan.hh"

namespace CFF {

using namespace OT;

const float UNSET_REAL_VALUE = -1.0f;

enum OpCode {
    /* One byte operators (0-31) */
    OpCode_version,                /* 0   CFF Top */
    OpCode_Notice,                 /* 1   CFF Top */
    OpCode_FullName,               /* 2   CFF Top */
    OpCode_FamilyName,             /* 3   CFF Top */
    OpCode_Weight,                 /* 4   CFF Top */
    OpCode_FontBBox,               /* 5   CFF Top */
    OpCode_BlueValues,             /* 6   CFF Private, CFF2 Private */
    OpCode_OtherBlues,             /* 7   CFF Private, CFF2 Private */
    OpCode_FamilyBlues,            /* 8   CFF Private, CFF2 Private */
    OpCode_FamilyOtherBlues,       /* 9   CFF Private, CFF2 Private */
    OpCode_StdHW,                  /* 10  CFF Private, CFF2 Private */
    OpCode_StdVW,                  /* 11  CFF Private, CFF2 Private */
    OpCode_escape,                 /* 12  All. Shared with CS */
    OpCode_UniqueID,               /* 13  CFF Top */
    OpCode_XUID,                   /* 14  CFF Top */
    OpCode_charset,                /* 15  CFF Top (0) */
    OpCode_Encoding,               /* 16  CFF Top (0) */
    OpCode_CharStrings,            /* 17  CFF Top, CFF2 Top */
    OpCode_Private,                /* 18  CFF Top, CFF2 FD */
    OpCode_Subrs,                  /* 19  CFF Private, CFF2 Private */
    OpCode_defaultWidthX,          /* 20  CFF Private (0) */
    OpCode_nominalWidthX,          /* 21  CFF Private (0) */
    OpCode_vsindex,                /* 22  CFF2 Private/CS */
    OpCode_blend,                  /* 23  CFF2 Private/CS  */
    OpCode_vstore,                 /* 24  CFF2 Top */
    OpCode_reserved25,             /* 25 */
    OpCode_reserved26,             /* 26 */
    OpCode_reserved27,             /* 27 */

    /* Numbers */
    OpCode_shortint,               /* 28  All */
    OpCode_longint,                /* 29  All */
    OpCode_BCD,                    /* 30  CFF2 Top/FD */
    OpCode_reserved31,             /* 31 */

    /* 1-byte integers */
    OpCode_OneByteIntFirst = 32,   /* All. beginning of the range of first byte ints */
    OpCode_OneByteIntLast = 246,   /* All. ending of the range of first byte int */

    /* 2-byte integers */
    OpCode_TwoBytePosInt0,         /* 247  All. first byte of two byte positive int (+108 to +1131) */
    OpCode_TwoBytePosInt1,
    OpCode_TwoBytePosInt2,
    OpCode_TwoBytePosInt3,

    OpCode_TwoByteNegInt0,         /* 251  All. first byte of two byte negative int (-1131 to -108) */
    OpCode_TwoByteNegInt1,
    OpCode_TwoByteNegInt2,
    OpCode_TwoByteNegInt3,

    /* Two byte escape operators 12, (0-41) */
    OpCode_ESC_Base = 32,
    OpCode_Copyright = OpCode_ESC_Base, /* OpCode_ESC(0) CFF Top */
    OpCode_isFixedPitch,           /* OpCode_ESC(1)  CFF Top (false) */
    OpCode_ItalicAngle,            /* OpCode_ESC(2)  CFF Top (0) */
    OpCode_UnderlinePosition,      /* OpCode_ESC(3)  CFF Top (-100) */
    OpCode_UnderlineThickness,     /* OpCode_ESC(4)  CFF Top (50) */
    OpCode_PaintType,              /* OpCode_ESC(5)  CFF Top (0) */
    OpCode_CharstringType,         /* OpCode_ESC(6)  CFF Top (2) */
    OpCode_FontMatrix,             /* OpCode_ESC(7)  CFF Top, CFF2 Top (.001 0 0 .001 0 0)*/
    OpCode_StrokeWidth,            /* OpCode_ESC(8)  CFF Top (0) */
    OpCode_BlueScale,              /* OpCode_ESC(9)  CFF Private, CFF2 Private (0.039625) */
    OpCode_BlueShift,              /* OpCode_ESC(10) CFF Private, CFF2 Private (7) */
    OpCode_BlueFuzz,               /* OpCode_ESC(11) CFF Private, CFF2 Private (1) */
    OpCode_StemSnapH,              /* OpCode_ESC(12) CFF Private, CFF2 Private */
    OpCode_StemSnapV,              /* OpCode_ESC(13) CFF Private, CFF2 Private */
    OpCode_ForceBold,              /* OpCode_ESC(14) CFF Private (false) */
    OpCode_reservedESC15,          /* OpCode_ESC(15) */
    OpCode_reservedESC16,          /* OpCode_ESC(16) */
    OpCode_LanguageGroup,          /* OpCode_ESC(17) CFF Private, CFF2 Private (0) */
    OpCode_ExpansionFactor,        /* OpCode_ESC(18) CFF Private, CFF2 Private (0.06) */
    OpCode_initialRandomSeed,      /* OpCode_ESC(19) CFF Private (0) */
    OpCode_SyntheticBase,          /* OpCode_ESC(20) CFF Top */
    OpCode_PostScript,             /* OpCode_ESC(21) CFF Top */
    OpCode_BaseFontName,           /* OpCode_ESC(22) CFF Top */
    OpCode_BaseFontBlend,          /* OpCode_ESC(23) CFF Top */
    OpCode_reservedESC24,          /* OpCode_ESC(24) */
    OpCode_reservedESC25,          /* OpCode_ESC(25) */
    OpCode_reservedESC26,          /* OpCode_ESC(26) */
    OpCode_reservedESC27,          /* OpCode_ESC(27) */
    OpCode_reservedESC28,          /* OpCode_ESC(28) */
    OpCode_reservedESC29,          /* OpCode_ESC(29) */
    OpCode_ROS,                    /* OpCode_ESC(30) CFF Top_CID */
    OpCode_CIDFontVersion,         /* OpCode_ESC(31) CFF Top_CID (0) */
    OpCode_CIDFontRevision,        /* OpCode_ESC(32) CFF Top_CID (0) */
    OpCode_CIDFontType,            /* OpCode_ESC(33) CFF Top_CID (0) */
    OpCode_CIDCount,               /* OpCode_ESC(34) CFF Top_CID (8720) */
    OpCode_UIDBase,                /* OpCode_ESC(35) CFF Top_CID */
    OpCode_FDArray,                /* OpCode_ESC(36) CFF Top_CID, CFF2 Top */
    OpCode_FDSelect,               /* OpCode_ESC(37) CFF Top_CID, CFF2 Top */
    OpCode_FontName,               /* OpCode_ESC(38) CFF Top_CID */

    OpCode_reserved255 = 255
};

inline OpCode Make_OpCode_ESC (unsigned char byte2)  { return (OpCode)(OpCode_ESC_Base + byte2); }
inline unsigned int OpCode_Size (OpCode op) { return (op >= OpCode_ESC_Base)? 2: 1; }

/* pair of table offset and length */
struct offset_size_pair {
  unsigned int  offset;
  unsigned int  size;
};

/* byte string */
struct UnsizedByteStr : UnsizedArrayOf <HBUINT8>
{
  // encode 2-byte int (Dict/CharString) or 4-byte int (Dict)
  template <typename INTTYPE, int minVal, int maxVal>
  inline static bool serialize_int (hb_serialize_context_t *c, OpCode intOp, int value)
  {
    TRACE_SERIALIZE (this);

    if (unlikely ((value < minVal || value > maxVal)))
      return_trace (false);

    HBUINT8 *p = c->allocate_size<HBUINT8> (1);
    if (unlikely (p == nullptr)) return_trace (false);
    p->set (intOp);

    INTTYPE *ip = c->allocate_size<INTTYPE> (INTTYPE::static_size);
    if (unlikely (ip == nullptr)) return_trace (false);
    ip->set ((unsigned int)value);

    return_trace (true);
  }
  
  inline static bool serialize_int4 (hb_serialize_context_t *c, int value)
  { return serialize_int<HBUINT32, 0, 0x7FFFFFFF> (c, OpCode_longint, value); }
  
  inline static bool serialize_int2 (hb_serialize_context_t *c, int value)
  { return serialize_int<HBUINT16, 0, 0x7FFF> (c, OpCode_shortint, value); }
};

struct ByteStr
{
  ByteStr (const UnsizedByteStr& s, unsigned int l)
    : str (&s), len (l) {}
  ByteStr (const char *s=nullptr, unsigned int l=0)
    : str ((const UnsizedByteStr *)s), len (l) {}
  /* sub-string */
  ByteStr (const ByteStr &bs, unsigned int offset, unsigned int l)
  {  
    str = (const UnsizedByteStr *)&bs.str[offset];
    len = l;
  }

  inline bool sanitize (hb_sanitize_context_t *c) const { return str->sanitize (c, len); }

  inline const HBUINT8& operator [] (unsigned int i) const {
    assert (str && (i < len));
    return (*str)[i];
  }

  inline bool check_limit (unsigned int offset, unsigned int count) const
  { return (offset + count <= len); }

  const UnsizedByteStr *str;
  unsigned int len;
};

inline unsigned int calcOffSize(unsigned int offset)
{
  unsigned int size = 1;
  while ((offset & ~0xFF) != 0)
  {
    size++;
    offset >>= 8;
  }
  assert (size <= 4);
  return size;
}

/* CFF INDEX */
struct Index
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) && offSize >= 1 && offSize <= 4 &&
                          c->check_array (offsets, offSize, count + 1) &&
                          c->check_array (data_base (), 1, max_offset () - 1)));
  }

  inline static unsigned int calculate_offset_array_size (unsigned int offSize, unsigned int count)
  { return offSize * (count + 1); }

  inline unsigned int offset_array_size (void) const
  { return calculate_offset_array_size (offSize, count); }

  inline static unsigned int calculate_serialized_size (unsigned int offSize, unsigned int count, unsigned int dataSize)
  { return min_size + calculate_offset_array_size (offSize, count) + dataSize; }

  inline bool serialize (hb_serialize_context_t *c, const Index &src)
  {
    TRACE_SANITIZE (this);
    unsigned int size = src.get_size ();
    Index *dest = c->allocate_size<Index> (size);
    if (unlikely (dest == nullptr)) return_trace (false);
    memcpy (dest, &src, size);
    return_trace (true);
  }

  inline bool serialize (hb_serialize_context_t *c,
                         unsigned int offSize,
                         const hb_vector_t<ByteStr> &bytesArray)
  {
    TRACE_SERIALIZE (this);

    /* serialize Index header */
    if (unlikely (!c->extend_min (*this))) return_trace (false);
    this->count.set (bytesArray.len);
    this->offSize.set (offSize);
    if (!unlikely (c->allocate_size<HBUINT8> (offSize * (bytesArray.len + 1))))
      return_trace (false);
  
    /* serialize indices */
    unsigned int  offset = 1;
    unsigned int  i = 0;
    for (; i < bytesArray.len; i++)
    {
      set_offset_at (i, offset);
      offset += bytesArray[i].len;
    }
    set_offset_at (i, offset);

    /* serialize data */
    for (unsigned int i = 0; i < bytesArray.len; i++)
    {
      HBUINT8 *dest = c->allocate_size<HBUINT8> (bytesArray[i].len);
      if (dest == nullptr)
        return_trace (false);
      memcpy (dest, &bytesArray[i].str[0], bytesArray[i].len);
    }
    return_trace (true);
  }

  inline void set_offset_at (unsigned int index, unsigned int offset)
  {
    HBUINT8 *p = offsets + offSize * index + offSize;
    unsigned int size = offSize;
    for (; size; size--)
    {
      --p;
      p->set (offset & 0xFF);
      offset >>= 8;
    }
  }

  inline const unsigned int offset_at (unsigned int index) const
  {
    const HBUINT8 *p = offsets + offSize * index;
    unsigned int size = offSize;
    unsigned int offset = 0;
    for (; size; size--)
      offset = (offset << 8) + *p++;
    return offset;
  }

  inline const unsigned int length_at (unsigned int index) const
  { return offset_at (index + 1) - offset_at (index); }

  inline const char *data_base (void) const
  { return (const char *)this + 5 + offset_array_size (); }

  inline unsigned int data_size (void) const
  { return HBINT8::static_size; };

  inline ByteStr operator [] (unsigned int index) const
  {
    if (likely (index < count))
      return ByteStr (data_base () + offset_at (index) - 1, offset_at (index + 1) - offset_at (index));
    else
      return Null(ByteStr);
  }

  inline unsigned int get_size (void) const
  {
    if (this != &Null(Index))
      return count.static_size + offSize.static_size + offset_array_size () + (offset_at (count) - 1);
    else
      return 0;
  }

  protected:
  inline unsigned int max_offset (void) const
  {
    unsigned int max = 0;
    for (unsigned int i = 0; i <= count; i++)
    {
      unsigned int off = offset_at (i);
      if (off > max) max = off;
    }
    return max;
  }

  public:
  HBUINT32  count;        /* Number of object data. Note there are (count+1) offsets */
  HBUINT8   offSize;      /* The byte size of each offset in the offsets array. */
  HBUINT8   offsets[VAR]; /* The array of (count + 1) offsets into objects array (1-base). */
  /* HBUINT8 data[VAR];      Object data */
  public:
  DEFINE_SIZE_ARRAY (5, offsets);
};

template <typename Type>
struct IndexOf : Index
{
  inline ByteStr operator [] (unsigned int index) const
  {
    if (likely (index < count))
      return ByteStr (data_base () + offset_at (index) - 1, length_at (index));
    return Null(ByteStr);
  }
};

/* an operator prefixed by its operands in a byte string */
struct OpStr
{
  OpCode  op;
  ByteStr str;
  bool    update;
};

typedef hb_vector_t <OpStr> OpStrs;

/* base param type for dict parsing */
struct DictValues
{
  inline void init (void)
  {
    opStart = 0;
    opStrs.init ();
  }

  inline void fini (void)
  {
    opStrs.fini ();
  }

  void pushOpStr (OpCode op, const ByteStr& str, unsigned int offset, bool update)
  {
    OpStr *opstr = opStrs.push ();
    opstr->op = op;
    opstr->str = ByteStr (str, opStart, offset - opStart);
    opstr->update = update;
    opStart = offset;
  }

  unsigned int  opStart;
  OpStrs        opStrs;
};

/* base of OP_SERIALIZER */
struct OpSerializer
{
  protected:
  inline bool copy_opstr (hb_serialize_context_t *c, const OpStr& opstr) const
  {
    TRACE_SERIALIZE (this);

    HBUINT8 *d = c->allocate_size<HBUINT8> (opstr.str.len);
    if (unlikely (d == nullptr)) return_trace (false);
    memcpy (d, &opstr.str.str[0], opstr.str.len);
    return_trace (true);
  }
};

/* Top Dict, Font Dict, Private Dict */
struct Dict : UnsizedByteStr
{
  template <typename OP_SERIALIZER, typename PARAM>
  inline bool serialize (hb_serialize_context_t *c,
                        const DictValues &values,
                        OP_SERIALIZER& opszr,
                        PARAM& param)
  {
    TRACE_SERIALIZE (this);
    for (unsigned int i = 0; i < values.opStrs.len; i++)
    {
      if (unlikely (!opszr.serialize (c, values.opStrs[i], param)))
        return_trace (false);
    }
    return_trace (true);
  }

  /* in parallel to above */
  template <typename OP_SERIALIZER>
  inline static unsigned int calculate_serialized_size (const DictValues &values,
                                                        OP_SERIALIZER& opszr)
  {
    unsigned int size = 0;
    for (unsigned int i = 0; i < values.opStrs.len; i++)
      size += opszr.calculate_serialized_size (values.opStrs[i]);
    return size;
  }

  template <typename INTTYPE, int minVal, int maxVal>
  inline static bool serialize_offset_op (hb_serialize_context_t *c, OpCode op, int value, OpCode intOp)
  {
    if (value == 0)
      return true;
    // XXX: not sure why but LLVM fails to compile the following 'unlikely' macro invocation
    if (/*unlikely*/ (!serialize_int<INTTYPE, minVal, maxVal> (c, intOp, value)))
      return false;

    TRACE_SERIALIZE (this);
    /* serialize the opcode */
    HBUINT8 *p = c->allocate_size<HBUINT8> ((op >= OpCode_ESC_Base)? 2: 1);
    if (unlikely (p == nullptr)) return_trace (false);
    if (op >= OpCode_ESC_Base)
    {
      p->set (OpCode_escape);
      op = (OpCode)(op - OpCode_ESC_Base);
      p++;
    }
    p->set (op);
    return_trace (true);
  }

  inline static bool serialize_offset4_op (hb_serialize_context_t *c, OpCode op, int value)
  { return serialize_offset_op<HBUINT32, 0, 0x7FFFFFFF> (c, op, value, OpCode_longint); }

  inline static bool serialize_offset2_op (hb_serialize_context_t *c, OpCode op, int value)
  { return serialize_offset_op<HBUINT16, 0, 0x7FFF> (c, op, value, OpCode_shortint); }
};

struct TopDict : Dict {};
struct FontDict : Dict {};
struct PrivateDict : Dict {};

struct FDArray : IndexOf<FontDict>
{
  template <typename DICTVAL, typename OP_SERIALIZER>
  inline bool serialize (hb_serialize_context_t *c,
                        unsigned int offSize,
                        const hb_vector_t<DICTVAL> &fontDicts,
                        OP_SERIALIZER& opszr,
                        const hb_vector_t<offset_size_pair> &privatePairs)
  {
    TRACE_SERIALIZE (this);
    if (unlikely (!c->extend_min (*this))) return_trace (false);
    this->count.set (fontDicts.len);
    this->offSize.set (offSize);
    if (!unlikely (c->allocate_size<HBUINT8> (offSize * (fontDicts.len + 1))))
      return_trace (false);
    
    /* serialize font dict offsets */
    unsigned int  offset = 1;
    unsigned int  i;
    for (i = 0; i < fontDicts.len; i++)
    {
      set_offset_at (i, offset);
      offset += FontDict::calculate_serialized_size (fontDicts[i], opszr);
    }
    set_offset_at (i, offset);

    /* serialize font dicts */
    for (unsigned int i = 0; i < fontDicts.len; i++)
    {
      FontDict *dict = c->start_embed<FontDict> ();
      if (unlikely (!dict->serialize (c, fontDicts[i], opszr, privatePairs[i])))
        return_trace (false);
    }
    return_trace (true);
  }
  
  /* in parallel to above */
  template <typename OP_SERIALIZER, typename DICTVAL>
  inline static unsigned int calculate_serialized_size (unsigned int &offSize /* OUT */,
                                                        const hb_vector_t<DICTVAL> &fontDicts,
                                                        OP_SERIALIZER& opszr)
  {
    unsigned int dictsSize = 0;
    for (unsigned int i = 0; i < fontDicts.len; i++)
      dictsSize += FontDict::calculate_serialized_size (fontDicts[i], opszr);

    offSize = calcOffSize (dictsSize + 1);
    return Index::calculate_serialized_size (offSize, fontDicts.len, dictsSize);
  }
};

/* FDSelect */
struct FDSelect0 {
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) && fds[c->get_num_glyphs () - 1].sanitize (c)));
  }

  inline unsigned int get_size (unsigned int num_glyphs) const
  { return HBUINT8::static_size * num_glyphs; }

  HBUINT8     fds[VAR];

  DEFINE_SIZE_MIN (1);
};

struct FDSelect3_Range {
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) && (first < c->get_num_glyphs ())));
  }

  HBUINT16    first;
  HBUINT8     fd;

  DEFINE_SIZE_STATIC (3);
};

struct FDSelect3 {
  inline unsigned int get_size (void) const
  { return HBUINT16::static_size * 2 + FDSelect3_Range::static_size * nRanges; }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) && (nRanges > 0) &&
                         (ranges[nRanges - 1].sanitize (c))));
  }

  HBUINT16         nRanges;
  FDSelect3_Range  ranges[VAR];
  /* HBUINT16 sentinel */

  DEFINE_SIZE_MIN (5);
};

struct FDSelect {
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);

    return_trace (likely (c->check_struct (this) && (format == 0 || format == 3) &&
                          (format == 0)? u.format0.sanitize (c): u.format3.sanitize (c)));
  }

  inline bool serialize (hb_serialize_context_t *c, const FDSelect &src, unsigned int num_glyphs)
  {
    TRACE_SANITIZE (this);
    unsigned int size = src.get_size (num_glyphs);
    FDSelect *dest = c->allocate_size<FDSelect> (size);
    if (unlikely (dest == nullptr)) return_trace (false);
    memcpy (dest, &src, size);
    return_trace (true);
  }

  inline unsigned int calculate_serialized_size (unsigned int num_glyphs) const
  { return get_size (num_glyphs); }

  inline unsigned int get_size (unsigned int num_glyphs) const
  {
    unsigned int size = format.static_size;
    if (format == 0)
      size += u.format0.get_size (num_glyphs);
    else if (likely (format == 3))
      size += u.format3.get_size ();
    return size;
  }

  HBUINT8       format;
  union {
    FDSelect0   format0;
    FDSelect3   format3;
  } u;

  DEFINE_SIZE_MIN (2);
};

typedef Index CharStrings;
typedef Index Subrs;

inline float parse_bcd (const ByteStr& str, unsigned int& offset, float& v)
{
  // XXX: TODO
  v = 0;
  for (;;) {
    if (++offset >= str.len)
      return false;
    unsigned char byte = str[offset];
    if (((byte & 0xF0) == 0xF0) || ((byte & 0x0F) == 0x0F))
      break;
  }
  return true;
}

struct Number
{
  Number (int v = 0) { set_int (v); }
  Number (float v) { set_real (v); }

  inline void set_int (int v)       { is_real = false; u.int_val = v; };
  inline int to_int (void) const    { return is_real? (int)u.real_val: u.int_val; }
  inline void set_real (float v)    { is_real = true; u.real_val = v; };
  inline float to_real (void) const { return is_real? u.real_val: (float)u.int_val; }

protected:
  bool is_real;
  union {
    int     int_val;
    float   real_val;
  } u;
};

struct Stack
{
  inline void init (void) { size = 0; }
  inline void fini (void) { }

  inline void push (const Number &v)
  {
    if (likely (size < kSizeLimit))
      numbers[size++] = v;
  }

  inline const Number& pop (void)
  {
    if (likely (size > 0))
      return numbers[--size];
    else
      return Null(Number);
  }

  inline bool check_push (void)
  {
    if (likely (size < kSizeLimit)) {
      size++;
      return true;
    } else
      return false;
  }

  inline bool check_pop (void)
  {
    if (likely (0 < size)) {
      size--;
      return true;
    } else
      return false;
  }

  inline bool check_pop_int (int& v)
  {
    if (unlikely (!this->check_underflow (1)))
      return false;
    v = this->pop ().to_int ();
    return true;
  }

  inline bool check_pop_uint (unsigned int& v)
  {
    uint32_t  i;
    if (unlikely (!this->check_underflow (1)))
      return false;
    i = this->pop ().to_int ();
    if (unlikely (i <= 0))
      return false;
    v = (uint32_t)i;
    return true;
  }

  inline bool check_pop_real (float& v)
  {
    if (unlikely (!this->check_underflow (1)))
      return false;
    v = this->pop ().to_real ();
    return true;
  }

  inline bool check_pop_delta (hb_vector_t<float>& vec, bool even=false)
  {
    if (even && unlikely ((this->size & 1) != 0))
      return false;

    float val = 0.0f;
    for (unsigned int i = 0; i < size; i++) {
      val += numbers[i].to_real ();
      vec.push (val);
    }
    return true;
  }

  inline void clear (void) { size = 0; }

  inline bool check_overflow (unsigned int count) { return (count <= kSizeLimit) && (count + size <= kSizeLimit); }
  inline bool check_underflow (unsigned int count) { return (count <= size); }

  static const unsigned int kSizeLimit = 513;

  unsigned int size;
  Number numbers[kSizeLimit];
};

template <typename Offset>
inline bool check_pop_offset (Stack& stack, Offset& offset)
{
  unsigned int  v;
  if (unlikely (!stack.check_pop_uint (v)))
    return false;
  offset.set (v);
  return true;
}

template <typename OpSet, typename Param>
struct Interpreter {

  inline Interpreter (void)
  {
    stack.init ();
  }
  
  inline ~Interpreter (void)
  {
    stack.fini ();
  }

  inline bool interpret (const ByteStr& str, Param& param)
  {
    param.init ();

    for (unsigned int i = 0; i < str.len; i++)
    {
      OpCode op = (OpCode)(unsigned char)str[i];
      if ((OpCode_shortint == op) ||
          (OpCode_OneByteIntFirst <= op && OpCode_TwoByteNegInt3 >= op))
      {
        if (unlikely (!process_intop (str, i, op)))
          return false;
      } else {
        if (op == OpCode_escape) {
          if (unlikely (!str.check_limit (i, 1)))
            return false;
          op = Make_OpCode_ESC(str[++i]);
        }

        if (unlikely (!OpSet::process_op (str, i, op, stack, param)))
          return false;
      }
    }
    
    return true;
  }

  inline bool process_intop (const ByteStr& str, unsigned int& offset, OpCode op)
  {
    switch (op) {
      case OpCode_TwoBytePosInt0: case OpCode_TwoBytePosInt1:
      case OpCode_TwoBytePosInt2: case OpCode_TwoBytePosInt3:
        if (unlikely (!str.check_limit (offset, 2) || !stack.check_overflow (1)))
          return false;
        stack.push ((int16_t)((op - OpCode_TwoBytePosInt0) * 256 + str[offset + 1] + 108));
        offset++;
        break;
      
      case OpCode_TwoByteNegInt0: case OpCode_TwoByteNegInt1:
      case OpCode_TwoByteNegInt2: case OpCode_TwoByteNegInt3:
        if (unlikely (!str.check_limit (offset, 2) || !stack.check_overflow (1)))
          return false;
        stack.push ((int16_t)(-(op - OpCode_TwoByteNegInt0) * 256 - str[offset + 1] - 108));
        offset++;
        break;
      
      case OpCode_shortint: /* 3-byte integer */
        if (unlikely (!str.check_limit (offset, 3) || !stack.check_overflow (1)))
          return false;
        stack.push ((int16_t)*(const HBUINT16*)&str[offset + 1]);
        offset += 2;
        break;
      
      default:
        /* 1-byte integer */
        if (likely ((OpCode_OneByteIntFirst <= op) && (op <= OpCode_OneByteIntLast)) &&
            likely (stack.check_overflow (1)))
        {
          stack.push ((int)op - 139);
        } else {
          return false;
        }
        break;
    }
    return true;
  }

  protected:
  Stack stack;
};

/* used by subsettter */
struct SubTableOffsets {
  inline SubTableOffsets (void)
  {
    memset (this, 0, sizeof(*this));
  }

  unsigned int  topDictSize;
  unsigned int  varStoreOffset;
  unsigned int  FDSelectOffset;
  unsigned int  FDArrayOffset;
  unsigned int  FDArrayOffSize;
  unsigned int  charStringsOffset;
  unsigned int  charStringsOffSize;
  unsigned int  privateDictsOffset;
};

} /* namespace CFF */

#endif /* HB_OT_CFF_COMMON_PRIVATE_HH */

