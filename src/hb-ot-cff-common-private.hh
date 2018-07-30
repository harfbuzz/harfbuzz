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

inline OpCode Make_OpCode_ESC(unsigned char byte2)  { return (OpCode)(OpCode_ESC_Base + byte2); }

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

  inline unsigned int offset_array_size (void) const
  { return offSize * (count + 1); }

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

  inline hb_bytes_t operator [] (unsigned int index) const
  {
    if (likely (index < count))
    {
      hb_bytes_t  b;
      b.bytes = data_base () + offset_at (index) - 1;
      b.len = offset_at (index + 1) - offset_at (index);
      return b;
    }
    return Null(hb_bytes_t);
  }

  inline unsigned int get_size (void) const
  { return count.static_size + offSize.static_size + offset_array_size () + (offset_at (count) - 1); }

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
  DEFINE_SIZE_MIN (6);
};

template <typename Type>
struct IndexOf : Index
{
  inline const Type& operator [] (unsigned int index) const
  {
    if (likely (index < count))
      return StructAtOffset<Type>(data_base (), offset_at (index) - 1);
    return Null(Type);
  }
};

struct UnsizedByteStr : UnsizedArrayOf <HBUINT8> {};

struct ByteStr {
  ByteStr (const UnsizedByteStr& s, unsigned int l)
    : str (s), len (l) {}

  inline bool sanitize (hb_sanitize_context_t *c) const { return str.sanitize (c, len); }

  inline const HBUINT8& operator [] (unsigned int i) const { return str[i]; }

  inline bool check_limit (unsigned int offset, unsigned int count) const
  { return (offset + count <= len); }

  const UnsizedByteStr& str;
  const unsigned int len;
};

/* Top Dict, Font Dict, Private Dict */
typedef UnsizedByteStr Dict;

typedef Index CharStrings;
typedef Index Subrs;
typedef IndexOf<Dict> FDArray;

/* FDSelect */
struct FDSelect0 {
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) && fds[c->get_num_glyphs () - 1].sanitize (c)));
  }

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

  HBUINT8       format;
  union {
    FDSelect0   format0;
    FDSelect3   format3;
  } u;

  DEFINE_SIZE_MIN (2);
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

  inline void init ()
  {
    stack.init ();
  }
  
  inline void fini ()
  {
    stack.fini ();
  }

  inline bool interpret (const ByteStr& str, Param& param)
  {
    param.init();

    for (unsigned int i = 0; i < str.len; i++)
    {
      unsigned char byte = str[i];
      if ((OpCode_shortint == byte) ||
          (OpCode_OneByteIntFirst <= byte && OpCode_TwoByteNegInt3 >= byte))
      {
        if (unlikely (!process_intop (str, i, byte)))
          return false;
      } else {
        if (byte == OpCode_escape) {
          if (unlikely (!str.check_limit (i, 1)))
            return false;
          byte = Make_OpCode_ESC(str[++i]);
        }

        if (unlikely (!OpSet::process_op (str, i, byte, stack, param)))
          return false;
      }
    }
    return true;
  }

  inline bool process_intop (const ByteStr& str, unsigned int& offset, unsigned char byte)
  {
    switch (byte) {
      case OpCode_TwoBytePosInt0: case OpCode_TwoBytePosInt1:
      case OpCode_TwoBytePosInt2: case OpCode_TwoBytePosInt3:
        if (unlikely (str.check_limit (offset, 2) || !stack.check_overflow (1)))
          return false;
        stack.push ((int16_t)((byte - OpCode_TwoBytePosInt0) * 256 + str[offset + 1] + 108));
        break;
      
      case OpCode_TwoByteNegInt0: case OpCode_TwoByteNegInt1:
      case OpCode_TwoByteNegInt2: case OpCode_TwoByteNegInt3:
        if (unlikely (!str.check_limit (offset, 2) || !stack.check_overflow (1)))
          return false;
        stack.push ((int16_t)(-(byte - OpCode_TwoByteNegInt0) * 256 - str[offset + 1] - 108));
        break;
      
      case OpCode_shortint: /* 3-byte integer */
        if (unlikely (!str.check_limit (offset, 3) || !stack.check_overflow (1)))
          return false;
        stack.push ((int16_t)((str[offset + 1] << 8) | str[offset + 2]));
        break;
      
      default:
        /* 1-byte integer */
        if (likely ((OpCode_OneByteIntFirst <= byte) && (byte <= OpCode_OneByteIntLast)) &&
            likely (stack.check_overflow (1)))
        {
          stack.push ((int)byte - 139);
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

