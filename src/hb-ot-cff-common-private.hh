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

/* utility macro */
template<typename Type>
static inline const Type& StructAtOffsetOrNull(const void *P, unsigned int offset)
{ return offset? (* reinterpret_cast<const Type*> ((const char *) P + offset)): Null(Type); }

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

struct Number
{
  inline Number (void) { set_int (0); }

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

  inline bool serialize (hb_serialize_context_t *c, const ByteStr &src)
  {
    TRACE_SANITIZE (this);
    HBUINT8 *dest = c->allocate_size<HBUINT8> (src.len);
    if (unlikely (dest == nullptr))
      return_trace (false);
    memcpy (dest, src.str, src.len);
    return_trace (true);
  }

  inline unsigned int get_size (void) const { return len; }

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
template <typename COUNT>
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
    TRACE_SERIALIZE (this);
    unsigned int size = src.get_size ();
    Index *dest = c->allocate_size<Index> (size);
    if (unlikely (dest == nullptr)) return_trace (false);
    memcpy (dest, &src, size);
    return_trace (true);
  }

  inline bool serialize (hb_serialize_context_t *c,
                         unsigned int offSize_,
                         const hb_vector_t<ByteStr> &byteArray)
  {
    TRACE_SERIALIZE (this);
    /* serialize Index header */
    if (unlikely (!c->extend_min (*this))) return_trace (false);
    this->count.set (byteArray.len);
    this->offSize.set (offSize_);
    if (!unlikely (c->allocate_size<HBUINT8> (offSize_ * (byteArray.len + 1))))
      return_trace (false);
  
    /* serialize indices */
    unsigned int  offset = 1;
    unsigned int  i = 0;
    for (; i < byteArray.len; i++)
    {
      set_offset_at (i, offset);
      offset += byteArray[i].get_size ();
    }
    set_offset_at (i, offset);

    /* serialize data */
    for (unsigned int i = 0; i < byteArray.len; i++)
    {
      ByteStr  *dest = c->start_embed<ByteStr> ();
      if (unlikely (dest == nullptr ||
                    !dest->serialize (c, byteArray[i])))
        return_trace (false);
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
    assert (index <= count);
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
  { return (const char *)this + min_size + offset_array_size (); }

  inline unsigned int data_size (void) const
  { return HBINT8::static_size; };

  ByteStr operator [] (unsigned int index) const
  {
    if (likely (index < count))
      return ByteStr (data_base () + offset_at (index) - 1, offset_at (index + 1) - offset_at (index));
    else
      return Null(ByteStr);
  }

  inline unsigned int get_size (void) const
  {
    if (this != &Null(Index))
    {
      if (count > 0)
        return min_size + offset_array_size () + (offset_at (count) - 1);
      else
        return count.static_size;  /* empty Index contains count only */
    }
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
  COUNT     count;        /* Number of object data. Note there are (count+1) offsets */
  HBUINT8   offSize;      /* The byte size of each offset in the offsets array. */
  HBUINT8   offsets[VAR]; /* The array of (count + 1) offsets into objects array (1-base). */
  /* HBUINT8 data[VAR];      Object data */
  public:
  DEFINE_SIZE_ARRAY (COUNT::static_size + HBUINT8::static_size, offsets);
};

template <typename COUNT, typename TYPE>
struct IndexOf : Index<COUNT>
{
  inline const ByteStr operator [] (unsigned int index) const
  {
    if (likely (index < Index<COUNT>::count))
      return ByteStr (Index<COUNT>::data_base () + Index<COUNT>::offset_at (index) - 1, Index<COUNT>::length_at (index));
    return Null(ByteStr);
  }

  template <typename DATA, typename PARAM1, typename PARAM2>
  inline bool serialize (hb_serialize_context_t *c,
                         unsigned int offSize_,
                         const hb_vector_t<DATA> &dataArray,
                         const hb_vector_t<unsigned int> &dataSizeArray,
                         const PARAM1 &param1,
                         const PARAM2 &param2)
  {
    TRACE_SERIALIZE (this);
    /* serialize Index header */
    if (unlikely (!c->extend_min (*this))) return_trace (false);
    this->count.set (dataArray.len);
    this->offSize.set (offSize_);
    if (!unlikely (c->allocate_size<HBUINT8> (offSize_ * (dataArray.len + 1))))
      return_trace (false);
  
    /* serialize indices */
    unsigned int  offset = 1;
    unsigned int  i = 0;
    for (; i < dataArray.len; i++)
    {
      Index<COUNT>::set_offset_at (i, offset);
      offset += dataSizeArray[i];
    }
    Index<COUNT>::set_offset_at (i, offset);

    /* serialize data */
    for (unsigned int i = 0; i < dataArray.len; i++)
    {
      TYPE  *dest = c->start_embed<TYPE> ();
      if (unlikely (dest == nullptr ||
                    !dest->serialize (c, dataArray[i], param1, param2)))
        return_trace (false);
    }
    return_trace (true);
  }

  /* in parallel to above */
  template <typename DATA, typename PARAM>
  inline static unsigned int calculate_serialized_size (unsigned int &offSize_ /* OUT */,
                                                        const hb_vector_t<DATA> &dataArray,
                                                        hb_vector_t<unsigned int> &dataSizeArray, /* OUT */
                                                        const PARAM &param)
  {
    /* determine offset size */
    unsigned int  totalDataSize = 0;
    for (unsigned int i = 0; i < dataArray.len; i++)
    {
      unsigned int dataSize = TYPE::calculate_serialized_size (dataArray[i], param);
      dataSizeArray[i] = dataSize;
      totalDataSize += dataSize;
    }
    offSize_ = calcOffSize (totalDataSize);

    return Index<COUNT>::calculate_serialized_size (offSize_, dataArray.len, totalDataSize);
  }
};

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

/* operand stack */
struct Stack
{
  inline void init (void) { size = 0; }
  inline void fini (void) { }

  inline void push (const Number &v)
  {
    if (likely (size < kSizeLimit))
      numbers[size++] = v;
  }

  inline void push_int (int v)
  {
    Number n;
    n.set_int (v);
    push (n);
  }

  inline void push_real (float v)
  {
    Number n;
    n.set_real (v);
    push (n);
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

  inline bool check_pop_num (Number& n)
  {
    if (unlikely (!this->check_underflow (1)))
      return false;
    n = this->pop ();
    return true;
  }

  inline bool check_pop_uint (unsigned int& v)
  {
    uint32_t  i;
    if (unlikely (!this->check_underflow (1)))
      return false;
    i = this->pop ().to_int ();
    if (unlikely (i < 0))
      return false;
    v = (uint32_t)i;
    return true;
  }

  inline bool check_pop_delta (hb_vector_t<Number>& vec, bool even=false)
  {
    if (even && unlikely ((this->size & 1) != 0))
      return false;

    float val = 0.0f;
    for (unsigned int i = 0; i < size; i++) {
      val += numbers[i].to_real ();
      Number *n = vec.push ();
      n->set_real (val);
    }
    return true;
  }

  inline bool push_longint_from_str (const ByteStr& str, unsigned int& offset)
  {
    if (unlikely (!str.check_limit (offset, 5) || !check_overflow (1)))
      return false;
    push_int ((int32_t)*(const HBUINT32*)&str[offset + 1]);
    offset += 4;
    return true;
  }

  inline void clear (void) { size = 0; }

  inline bool check_overflow (unsigned int count) const { return (count <= kSizeLimit) && (count + size <= kSizeLimit); }
  inline bool check_underflow (unsigned int count) const { return (count <= size); }

  inline unsigned int get_size (void) const { return size; }
  inline bool is_empty (void) const { return size == 0; }

  static const unsigned int kSizeLimit = 513;

  unsigned int size;
  Number numbers[kSizeLimit];
};

/* an operator prefixed by its operands in a byte string */
struct OpStr
{
  inline void init (void) {}

  OpCode  op;
  ByteStr str;
};

/* an opstr and the parsed out dict value(s) */
struct DictVal : OpStr
{
  inline void init (void)
  {
    single_val.set_int (0);
    multi_val.init ();
  }

  inline void fini (void)
  {
    multi_val.fini ();
  }

  Number              single_val;
  hb_vector_t<Number> multi_val;
};

template <typename VAL>
struct DictValues
{
  inline void init (void)
  {
    opStart = 0;
    values.init ();
  }

  inline void fini (void)
  {
    values.fini ();
  }

  inline void pushVal (OpCode op, const ByteStr& str, unsigned int offset)
  {
    VAL *val = values.push ();
    val->op = op;
    val->str = ByteStr (str, opStart, offset - opStart);
    opStart = offset;
  }

  inline void pushVal (OpCode op, const ByteStr& str, unsigned int offset, const VAL &v)
  {
    VAL *val = values.push (v);
    val->op = op;
    val->str = ByteStr (str, opStart, offset - opStart);
    opStart = offset;
  }

  unsigned int       opStart;
  hb_vector_t<VAL>   values;
};

/* Top Dict, Font Dict, Private Dict */
struct Dict : UnsizedByteStr
{
  template <typename DICTVAL, typename OP_SERIALIZER, typename PARAM>
  inline bool serialize (hb_serialize_context_t *c,
                        const DICTVAL &dictval,
                        OP_SERIALIZER& opszr,
                        PARAM& param)
  {
    TRACE_SERIALIZE (this);
    for (unsigned int i = 0; i < dictval.values.len; i++)
    {
      if (unlikely (!opszr.serialize (c, dictval.values[i], param)))
        return_trace (false);
    }
    return_trace (true);
  }

  /* in parallel to above */
  template <typename DICTVAL, typename OP_SERIALIZER>
  inline static unsigned int calculate_serialized_size (const DICTVAL &dictval,
                                                        OP_SERIALIZER& opszr)
  {
    unsigned int size = 0;
    for (unsigned int i = 0; i < dictval.values.len; i++)
      size += opszr.calculate_serialized_size (dictval.values[i]);
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

struct TableInfo
{
  void init (void) { offset = size = 0; }

  unsigned int    offset;
  unsigned int    size;
};

template <typename COUNT>
struct FDArray : IndexOf<COUNT, FontDict>
{
  template <typename DICTVAL, typename OP_SERIALIZER>
  inline bool serialize (hb_serialize_context_t *c,
                        unsigned int offSize_,
                        const hb_vector_t<DICTVAL> &fontDicts,
                        unsigned int fdCount,
                        const hb_vector_t<hb_codepoint_t> &fdmap,
                        OP_SERIALIZER& opszr,
                        const hb_vector_t<TableInfo> &privateInfos)
  {
    TRACE_SERIALIZE (this);
    if (unlikely (!c->extend_min (*this))) return_trace (false);
    this->count.set (fdCount);
    this->offSize.set (offSize_);
    if (!unlikely (c->allocate_size<HBUINT8> (offSize_ * (fdCount + 1))))
      return_trace (false);
    
    /* serialize font dict offsets */
    unsigned int  offset = 1;
    unsigned int  fid = 0;
    for (unsigned i = 0; i < fontDicts.len; i++)
      if (!fdmap.len || fdmap[i] != HB_SET_VALUE_INVALID)
      {
        IndexOf<COUNT, FontDict>::set_offset_at (fid++, offset);
        offset += FontDict::calculate_serialized_size (fontDicts[i], opszr);
      }
    IndexOf<COUNT, FontDict>::set_offset_at (fid, offset);

    /* serialize font dicts */
    for (unsigned int i = 0; i < fontDicts.len; i++)
      if (fdmap[i] != HB_SET_VALUE_INVALID)
      {
        FontDict *dict = c->start_embed<FontDict> ();
        if (unlikely (!dict->serialize (c, fontDicts[i], opszr, privateInfos[i])))
          return_trace (false);
      }
    return_trace (true);
  }
  
  /* in parallel to above */
  template <typename OP_SERIALIZER, typename DICTVAL>
  inline static unsigned int calculate_serialized_size (unsigned int &offSize_ /* OUT */,
                                                        const hb_vector_t<DICTVAL> &fontDicts,
                                                        unsigned int fdCount,
                                                        const hb_vector_t<hb_codepoint_t> &fdmap,
                                                        OP_SERIALIZER& opszr)
  {
    unsigned int dictsSize = 0;
    for (unsigned int i = 0; i < fontDicts.len; i++)
      if (!fdmap.len || fdmap[i] != HB_SET_VALUE_INVALID)
        dictsSize += FontDict::calculate_serialized_size (fontDicts[i], opszr);

    offSize_ = calcOffSize (dictsSize + 1);
    return Index<COUNT>::calculate_serialized_size (offSize_, fdCount, dictsSize);
  }
};

/* FDSelect */
struct FDSelect0 {
  inline bool sanitize (hb_sanitize_context_t *c, unsigned int fdcount) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!(c->check_struct (this))))
      return_trace (false);
    for (unsigned int i = 0; i < c->get_num_glyphs (); i++)
      if (unlikely (!fds[i].sanitize (c)))
        return_trace (false);

    return_trace (true);
  }

  inline hb_codepoint_t get_fd (hb_codepoint_t glyph) const
  {
    return (hb_codepoint_t)fds[glyph];
  }

  inline unsigned int get_size (unsigned int num_glyphs) const
  { return HBUINT8::static_size * num_glyphs; }

  HBUINT8     fds[VAR];

  DEFINE_SIZE_MIN (1);
};

template <typename GID_TYPE, typename FD_TYPE>
struct FDSelect3_4_Range {
  inline bool sanitize (hb_sanitize_context_t *c, unsigned int fdcount) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) && (first < c->get_num_glyphs ()) && (fd < fdcount)));
  }

  GID_TYPE    first;
  FD_TYPE     fd;

  DEFINE_SIZE_STATIC (GID_TYPE::static_size + FD_TYPE::static_size);
};

template <typename GID_TYPE, typename FD_TYPE>
struct FDSelect3_4 {
  inline unsigned int get_size (void) const
  { return GID_TYPE::static_size * 2 + FDSelect3_4_Range<GID_TYPE, FD_TYPE>::static_size * nRanges; }

  inline bool sanitize (hb_sanitize_context_t *c, unsigned int fdcount) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!(c->check_struct (this) && (nRanges > 0) && (ranges[0].first == 0))))
      return_trace (false);

    for (unsigned int i = 0; i < nRanges; i++)
    {
      if (unlikely (!ranges[i].sanitize (c, fdcount)))
        return_trace (false);
      if ((0 < i) && unlikely (ranges[i - 1].first >= ranges[i].first))
        return_trace (false);
    }
    if (unlikely (!sentinel().sanitize (c) || (sentinel() != c->get_num_glyphs ())))
      return_trace (false);

    return_trace (true);
  }

  inline hb_codepoint_t get_fd (hb_codepoint_t glyph) const
  {
    for (unsigned int i = 1; i < nRanges; i++)
      if (glyph < ranges[i].first)
        return (hb_codepoint_t)ranges[i - 1].fd;

    assert (false);
  }

  inline GID_TYPE &sentinel (void)  { return StructAfter<GID_TYPE> (ranges[nRanges - 1]); }
  inline const GID_TYPE &sentinel (void) const  { return StructAfter<GID_TYPE> (ranges[nRanges - 1]); }

  GID_TYPE         nRanges;
  FDSelect3_4_Range<GID_TYPE, FD_TYPE>  ranges[VAR];
  /* GID_TYPE sentinel */

  DEFINE_SIZE_ARRAY (GID_TYPE::static_size * 2, ranges);
};

typedef FDSelect3_4<HBUINT16, HBUINT8> FDSelect3;
typedef FDSelect3_4_Range<HBUINT16, HBUINT8> FDSelect3_Range;

struct FDSelect {
  inline bool sanitize (hb_sanitize_context_t *c, unsigned int fdcount) const
  {
    TRACE_SANITIZE (this);

    return_trace (likely (c->check_struct (this) && (format == 0 || format == 3) &&
                          (format == 0)?
                          u.format0.sanitize (c, fdcount):
                          u.format3.sanitize (c, fdcount)));
  }

  inline bool serialize (hb_serialize_context_t *c, const FDSelect &src, unsigned int num_glyphs)
  {
    TRACE_SERIALIZE (this);
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
    else
      size += u.format3.get_size ();
    return size;
  }

  inline hb_codepoint_t get_fd (hb_codepoint_t glyph) const
  {
    if (format == 0)
      return u.format0.get_fd (glyph);
    else
      return u.format3.get_fd (glyph);
  }

  HBUINT8       format;
  union {
    FDSelect0   format0;
    FDSelect3   format3;
  } u;

  DEFINE_SIZE_MIN (1);
};

struct TopDictValues : DictValues<OpStr>
{
  inline void init (void)
  {
    DictValues<OpStr>::init ();
    charStringsOffset = 0;
    FDArrayOffset = 0;
  }

  inline void fini (void)
  {
    DictValues<OpStr>::fini ();
  }

  inline unsigned int calculate_serialized_op_size (const OpStr& opstr) const
  {
    switch (opstr.op)
    {
      case OpCode_CharStrings:
      case OpCode_FDArray:
        return OpCode_Size (OpCode_longint) + 4 + OpCode_Size (opstr.op);

      default:
        return opstr.str.len;
    }
  }

  unsigned int  charStringsOffset;
  unsigned int  FDArrayOffset;
};

struct TopDictOpSet
{
  static inline bool process_op (const ByteStr& str, unsigned int& offset, OpCode op, Stack& stack, TopDictValues& dictval)
  {
    switch (op) {
      case OpCode_CharStrings:
        if (unlikely (!stack.check_pop_uint (dictval.charStringsOffset)))
          return false;
        stack.clear ();
        break;
      case OpCode_FDArray:
        if (unlikely (!stack.check_pop_uint (dictval.FDArrayOffset)))
          return false;
        stack.clear ();
        break;
      case OpCode_longint:  /* 5-byte integer */
        return stack.push_longint_from_str (str, offset);
      
      case OpCode_BCD:  /* real number */
        float v;
        if (unlikely (stack.check_overflow (1) || !parse_bcd (str, offset, v)))
          return false;
        stack.push_real (v);
        return true;
    
      default:
        /* XXX: invalid */
        stack.clear ();
        return false;
    }

    return true;
  }
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
        stack.push_int ((int16_t)((op - OpCode_TwoBytePosInt0) * 256 + str[offset + 1] + 108));
        offset++;
        break;
      
      case OpCode_TwoByteNegInt0: case OpCode_TwoByteNegInt1:
      case OpCode_TwoByteNegInt2: case OpCode_TwoByteNegInt3:
        if (unlikely (!str.check_limit (offset, 2) || !stack.check_overflow (1)))
          return false;
        stack.push_int ((int16_t)(-(op - OpCode_TwoByteNegInt0) * 256 - str[offset + 1] - 108));
        offset++;
        break;
      
      case OpCode_shortint: /* 3-byte integer */
        if (unlikely (!str.check_limit (offset, 3) || !stack.check_overflow (1)))
          return false;
        stack.push_int ((int16_t)*(const HBUINT16*)&str[offset + 1]);
        offset += 2;
        break;
      
      default:
        /* 1-byte integer */
        if (likely ((OpCode_OneByteIntFirst <= op) && (op <= OpCode_OneByteIntLast)) &&
            likely (stack.check_overflow (1)))
        {
          stack.push_int ((int)op - 139);
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

} /* namespace CFF */

#endif /* HB_OT_CFF_COMMON_PRIVATE_HH */

