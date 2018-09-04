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
#ifndef HB_CFF_INTERP_COMMON_HH
#define HB_CFF_INTERP_COMMON_HH

namespace CFF {

using namespace OT;

enum OpCode {
    /* === Dict operators === */

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
    OpCode_vsindexdict,            /* 22  CFF2 Private/CS */
    OpCode_blenddict,              /* 23  CFF2 Private/CS  */
    OpCode_vstore,                 /* 24  CFF2 Top */
    OpCode_reserved25,             /* 25 */
    OpCode_reserved26,             /* 26 */
    OpCode_reserved27,             /* 27 */

    /* Numbers */
    OpCode_shortint,               /* 28  16-bit integer, All */
    OpCode_longintdict,            /* 29  32-bit integer, All */
    OpCode_BCD,                    /* 30  real number, CFF2 Top/FD */
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
    OpCode_ESC_Base = 256,
    OpCode_Copyright = OpCode_ESC_Base, /* Make_OpCode_ESC (0) CFF Top */
    OpCode_isFixedPitch,           /* Make_OpCode_ESC (1)  CFF Top (false) */
    OpCode_ItalicAngle,            /* Make_OpCode_ESC (2)  CFF Top (0) */
    OpCode_UnderlinePosition,      /* Make_OpCode_ESC (3)  CFF Top (-100) */
    OpCode_UnderlineThickness,     /* Make_OpCode_ESC (4)  CFF Top (50) */
    OpCode_PaintType,              /* Make_OpCode_ESC (5)  CFF Top (0) */
    OpCode_CharstringType,         /* Make_OpCode_ESC (6)  CFF Top (2) */
    OpCode_FontMatrix,             /* Make_OpCode_ESC (7)  CFF Top, CFF2 Top (.001 0 0 .001 0 0)*/
    OpCode_StrokeWidth,            /* Make_OpCode_ESC (8)  CFF Top (0) */
    OpCode_BlueScale,              /* Make_OpCode_ESC (9)  CFF Private, CFF2 Private (0.039625) */
    OpCode_BlueShift,              /* Make_OpCode_ESC (10) CFF Private, CFF2 Private (7) */
    OpCode_BlueFuzz,               /* Make_OpCode_ESC (11) CFF Private, CFF2 Private (1) */
    OpCode_StemSnapH,              /* Make_OpCode_ESC (12) CFF Private, CFF2 Private */
    OpCode_StemSnapV,              /* Make_OpCode_ESC (13) CFF Private, CFF2 Private */
    OpCode_ForceBold,              /* Make_OpCode_ESC (14) CFF Private (false) */
    OpCode_reservedESC15,          /* Make_OpCode_ESC (15) */
    OpCode_reservedESC16,          /* Make_OpCode_ESC (16) */
    OpCode_LanguageGroup,          /* Make_OpCode_ESC (17) CFF Private, CFF2 Private (0) */
    OpCode_ExpansionFactor,        /* Make_OpCode_ESC (18) CFF Private, CFF2 Private (0.06) */
    OpCode_initialRandomSeed,      /* Make_OpCode_ESC (19) CFF Private (0) */
    OpCode_SyntheticBase,          /* Make_OpCode_ESC (20) CFF Top */
    OpCode_PostScript,             /* Make_OpCode_ESC (21) CFF Top */
    OpCode_BaseFontName,           /* Make_OpCode_ESC (22) CFF Top */
    OpCode_BaseFontBlend,          /* Make_OpCode_ESC (23) CFF Top */
    OpCode_reservedESC24,          /* Make_OpCode_ESC (24) */
    OpCode_reservedESC25,          /* Make_OpCode_ESC (25) */
    OpCode_reservedESC26,          /* Make_OpCode_ESC (26) */
    OpCode_reservedESC27,          /* Make_OpCode_ESC (27) */
    OpCode_reservedESC28,          /* Make_OpCode_ESC (28) */
    OpCode_reservedESC29,          /* Make_OpCode_ESC (29) */
    OpCode_ROS,                    /* Make_OpCode_ESC (30) CFF Top_CID */
    OpCode_CIDFontVersion,         /* Make_OpCode_ESC (31) CFF Top_CID (0) */
    OpCode_CIDFontRevision,        /* Make_OpCode_ESC (32) CFF Top_CID (0) */
    OpCode_CIDFontType,            /* Make_OpCode_ESC (33) CFF Top_CID (0) */
    OpCode_CIDCount,               /* Make_OpCode_ESC (34) CFF Top_CID (8720) */
    OpCode_UIDBase,                /* Make_OpCode_ESC (35) CFF Top_CID */
    OpCode_FDArray,                /* Make_OpCode_ESC (36) CFF Top_CID, CFF2 Top */
    OpCode_FDSelect,               /* Make_OpCode_ESC (37) CFF Top_CID, CFF2 Top */
    OpCode_FontName,               /* Make_OpCode_ESC (38) CFF Top_CID */

    /* === CharString operators === */

    OpCode_hstem = 1,              /* 1 CFF, CFF2 */
    OpCode_Reserved2,
    OpCode_vstem,                  /* 3 CFF, CFF2 */
    OpCode_vmoveto,                /* 4 CFF, CFF2 */
    OpCode_rlineto,                /* 5 CFF, CFF2 */
    OpCode_hlineto,                /* 6 CFF, CFF2 */
    OpCode_vlineto,                /* 7 CFF, CFF2 */
    OpCode_rrcurveto,              /* 8 CFF, CFF2 */
    OpCode_Reserved9,
    OpCode_callsubr,               /* 10 CFF, CFF2 */
    OpCode_return,                 /* 11 CFF */
 // OpCode_escape,                 /* 12 CFF, CFF2 */
    OpCode_Reserved13 = 13,
    OpCode_endchar,                /* 14 CFF */
    OpCode_vsindexcs,              /* 15 CFF2 */
    OpCode_blendcs,                /* 16 CFF2 */
    OpCode_Reserved17,
    OpCode_hstemhm,                /* 18 CFF, CFF2 */
    OpCode_hintmask,               /* 19 CFF, CFF2 */
    OpCode_cntrmask,               /* 20 CFF, CFF2 */
    OpCode_rmoveto,                /* 21 CFF, CFF2 */
    OpCode_hmoveto,                /* 22 CFF, CFF2 */
    OpCode_vstemhm,                /* 23 CFF, CFF2 */
    OpCode_rcurveline,             /* 24 CFF, CFF2 */
    OpCode_rlinecurve,             /* 25 CFF, CFF2 */
    OpCode_vvcurveto,              /* 26 CFF, CFF2 */
    OpCode_hhcurveto,              /* 27 CFF, CFF2 */
 // OpCode_shortint,               /* 28 CFF, CFF2 */
    OpCode_callgsubr = 29,         /* 29 CFF, CFF2 */
    OpCode_vhcurveto,              /* 30 CFF, CFF2 */
    OpCode_hvcurveto,              /* 31 CFF, CFF2 */

    OpCode_fixedcs = 255,          /* 32-bit fixed */

    /* Two byte escape operators 12, (0-41) */
    OpCode_ReservedESC0 = OpCode_ESC_Base, /* Make_OpCode_ESC (0) */
    OpCode_ReservedESC1,           /* Make_OpCode_ESC (1) */
    OpCode_ReservedESC2,           /* Make_OpCode_ESC (2) */
    OpCode_and,                    /* Make_OpCode_ESC (3) CFF */
    OpCode_or,                     /* Make_OpCode_ESC (4) CFF */
    OpCode_not,                    /* Make_OpCode_ESC (5) CFF */
    OpCode_ReservedESC6,           /* Make_OpCode_ESC (6) */
    OpCode_ReservedESC7,           /* Make_OpCode_ESC (7) */
    OpCode_ReservedESC8,           /* Make_OpCode_ESC (8) */
    OpCode_abs,                    /* Make_OpCode_ESC (9) CFF */
    OpCode_add,                    /* Make_OpCode_ESC (10) CFF */
    OpCode_sub,                    /* Make_OpCode_ESC (11) CFF */
    OpCode_div,                    /* Make_OpCode_ESC (12) CFF */
    OpCode_ReservedESC13,          /* Make_OpCode_ESC (13) */
    OpCode_neg,                    /* Make_OpCode_ESC (14) CFF */
    OpCode_eq,                     /* Make_OpCode_ESC (15) CFF */
    OpCode_ReservedESC16,          /* Make_OpCode_ESC (16) */
    OpCode_ReservedESC17,          /* Make_OpCode_ESC (17) */
    OpCode_drop,                   /* Make_OpCode_ESC (18) CFF */
    OpCode_ReservedESC19,          /* Make_OpCode_ESC (19) */
    OpCode_put,                    /* Make_OpCode_ESC (20) CFF */
    OpCode_get,                    /* Make_OpCode_ESC (21) CFF */
    OpCode_ifelse,                 /* Make_OpCode_ESC (22) CFF */
    OpCode_random,                 /* Make_OpCode_ESC (23) CFF */
    OpCode_mul,                    /* Make_OpCode_ESC (24) CFF */
 // OpCode_reservedESC25,          /* Make_OpCode_ESC (25) */
    OpCode_sqrt = OpCode_mul+2,    /* Make_OpCode_ESC (26) CFF */
    OpCode_dup,                    /* Make_OpCode_ESC (27) CFF */
    OpCode_exch,                   /* Make_OpCode_ESC (28) CFF */
    OpCode_index,                  /* Make_OpCode_ESC (29) CFF */
    OpCode_roll,                   /* Make_OpCode_ESC (30) CFF */
    OpCode_reservedESC31,          /* Make_OpCode_ESC (31) */
    OpCode_reservedESC32,          /* Make_OpCode_ESC (32) */
    OpCode_reservedESC33,          /* Make_OpCode_ESC (33) */
    OpCode_hflex,                  /* Make_OpCode_ESC (34) CFF, CFF2 */
    OpCode_flex,                   /* Make_OpCode_ESC (35) CFF, CFF2 */
    OpCode_hflex1,                 /* Make_OpCode_ESC (36) CFF, CFF2 */
    OpCode_flex1                   /* Make_OpCode_ESC (37) CFF, CFF2 */
};

inline OpCode Make_OpCode_ESC (unsigned char byte2)  { return (OpCode)(OpCode_ESC_Base + byte2); }
inline OpCode Unmake_OpCode_ESC (OpCode op)  { return (OpCode)(op - OpCode_ESC_Base); }
inline bool Is_OpCode_ESC (OpCode op) { return op >= OpCode_ESC_Base; }
inline unsigned int OpCode_Size (OpCode op) { return Is_OpCode_ESC (op)? 2: 1; }

struct Number
{
  inline void init (void)
  { set_int (0); }
  inline void fini (void)
  {}

  inline void set_int (int v)           { format = NumInt; u.int_val = v; };
  inline int to_int (void) const        { return is_int ()? u.int_val: (int)to_real (); }
  inline void set_fixed (int32_t v)     { format = NumFixed; u.fixed_val = v; };
  inline int32_t to_fixed (void) const
  {
    if (is_fixed ())
      return u.fixed_val;
    else if (is_real ())
      return (int32_t)(u.real_val * 65536.0);
    else
      return (int32_t)(u.int_val << 16);
  }
  inline void set_real (float v)        { format = NumReal; u.real_val = v; };
  inline float to_real (void) const
  {
    if (is_real ())
      return u.real_val;
    if (is_fixed ())
      return u.fixed_val / 65536.0;
    else
      return (float)u.int_val;
  }
  inline bool in_int_range (void) const
  {
    if (is_int ())
      return true;
    if (is_fixed () && ((u.fixed_val & 0xFFFF) == 0))
      return true;
    else
      return ((float)(int16_t)to_int () == u.real_val);
  }

protected:
  enum NumFormat {
    NumInt,
    NumFixed,
    NumReal
  };
  NumFormat  format;
  union {
    int     int_val;
    int32_t fixed_val;
    float   real_val;
  } u;

  inline bool  is_int (void) const { return format == NumInt; }
  inline bool  is_fixed (void) const { return format == NumFixed; }
  inline bool  is_real (void) const { return format == NumReal; }
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
  { return serialize_int<HBUINT32, 0, 0x7FFFFFFF> (c, OpCode_longintdict, value); }
  
  inline static bool serialize_int2 (hb_serialize_context_t *c, int value)
  { return serialize_int<HBUINT16, 0, 0x7FFF> (c, OpCode_shortint, value); }
};

struct ByteStr
{
  inline ByteStr (void)
    : str (&Null(UnsizedByteStr)), len (0) {}
  inline ByteStr (const UnsizedByteStr& s, unsigned int l)
    : str (&s), len (l) {}
  inline ByteStr (const char *s, unsigned int l=0)
    : str ((const UnsizedByteStr *)s), len (l) {}
  /* sub-string */
  inline ByteStr (const ByteStr &bs, unsigned int offset, unsigned int len_)
  {
    str = (const UnsizedByteStr *)&bs.str[offset];
    len = len_;
  }

  inline bool sanitize (hb_sanitize_context_t *c) const { return str->sanitize (c, len); }

  inline const HBUINT8& operator [] (unsigned int i) const {
    assert (str && (i < len));
    return (*str)[i];
  }

  inline bool serialize (hb_serialize_context_t *c, const ByteStr &src)
  {
    TRACE_SERIALIZE (this);
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

struct SubByteStr
{
  inline SubByteStr (void)
  { init (); }

  inline void init (void)
  {
    str = ByteStr (0);
    offset = 0;
  }

  inline void fini (void) {}

  inline SubByteStr (const ByteStr &str_, unsigned int offset_ = 0)
    : str (str_), offset (offset_) {}

  inline void reset (const ByteStr &str_, unsigned int offset_ = 0)
  {
    str = str_;
    offset = offset_;
  }

  inline const HBUINT8& operator [] (int i) const {
    return str[offset + i];
  }

  inline operator ByteStr (void) const { return ByteStr (str, offset, str.len - offset); }

  inline bool avail (unsigned int count=1) const { return str.check_limit (offset, count); }
  inline void inc (unsigned int count=1) { offset += count; assert (count <= str.len); }

  ByteStr       str;
  unsigned int  offset; /* beginning of the sub-string within str */
};

inline float parse_bcd (SubByteStr& substr, float& v)
{
  // XXX: TODO
  v = 0;
  for (;;) {
    if (!substr.avail ())
      return false;
    unsigned char byte = substr[0];
    substr.inc ();
    if (((byte & 0xF0) == 0xF0) || ((byte & 0x0F) == 0x0F))
      break;
  }
  return true;
}

/* stack */
template <typename ELEM, int LIMIT>
struct Stack
{
  inline void init (void)
  {
    count = 0;
    elements.init ();
    elements.resize (kSizeLimit);
    for (unsigned int i = 0; i < elements.len; i++)
      elements[i].init ();
  }

  inline void fini (void)
  {
    for (unsigned int i = 0; i < elements.len; i++)
      elements[i].fini ();
  }

  inline void push (const ELEM &v)
  {
    if (likely (count < elements.len))
      elements[count++] = v;
  }

  inline ELEM &push (void)
  {
    if (likely (count < elements.len))
      return elements[count++];
    else
      return Crap(ELEM);
  }

  inline const ELEM& pop (void)
  {
    if (likely (count > 0))
      return elements[--count];
    else
      return Null(ELEM);
  }

  inline const ELEM& peek (void)
  {
    if (likely (count > 0))
      return elements[count];
    else
      return Null(ELEM);
  }

  inline void unpop (void)
  {
    if (likely (count < elements.len))
      count++;
  }

  inline void clear (void) { count = 0; }

  inline bool check_overflow (unsigned int n=1) const { return (n <= kSizeLimit) && (n + count <= kSizeLimit); }
  inline bool check_underflow (unsigned int n=1) const { return (n <= count); }

  inline unsigned int get_count (void) const { return count; }
  inline bool is_empty (void) const { return count == 0; }

  static const unsigned int kSizeLimit = LIMIT;

  unsigned int count;
  hb_vector_t<ELEM, kSizeLimit> elements;
};

/* argument stack */
template <typename ARG=Number>
struct ArgStack : Stack<ARG, 513>
{
  inline void push_int (int v)
  {
    ARG &n = S::push ();
    n.set_int (v);
  }

  inline void push_fixed (int32_t v)
  {
    ARG &n = S::push ();
    n.set_fixed (v);
  }

  inline void push_real (float v)
  {
    ARG &n = S::push ();
    n.set_real (v);
  }

  inline bool check_pop_num (ARG& n)
  {
    if (unlikely (!this->check_underflow ()))
      return false;
    n = this->pop ();
    return true;
  }

  inline bool check_pop_num2 (ARG& n1, ARG& n2)
  {
    if (unlikely (!this->check_underflow (2)))
      return false;
    n2 = this->pop ();
    n1 = this->pop ();
    return true;
  }

  inline bool check_pop_int (int& v)
  {
    if (unlikely (!this->check_underflow ()))
      return false;
    v = this->pop ().to_int ();
    return true;
  }

  inline bool check_pop_uint (unsigned int& v)
  {
    int  i;
    if (unlikely (!check_pop_int (i) || i < 0))
      return false;
    v = (unsigned int)i;
    return true;
  }

  inline bool check_pop_delta (hb_vector_t<ARG>& vec, bool even=false)
  {
    if (even && unlikely ((this->count & 1) != 0))
      return false;

    float val = 0.0f;
    for (unsigned int i = 0; i < S::count; i++) {
      val += S::elements[i].to_real ();
      ARG *n = vec.push ();
      n->set_real (val);
    }
    return true;
  }

  inline bool push_longint_from_substr (SubByteStr& substr)
  {
    if (unlikely (!substr.avail (4) || !S::check_overflow (1)))
      return false;
    push_int ((int32_t)*(const HBUINT32*)&substr[0]);
    substr.inc (4);
    return true;
  }

  inline bool push_fixed_from_substr (SubByteStr& substr)
  {
    if (unlikely (!substr.avail (4) || !S::check_overflow (1)))
      return false;
    push_fixed ((int32_t)*(const HBUINT32*)&substr[0]);
    substr.inc (4);
    return true;
  }

  inline void reverse_range (int i, int j)
  {
    assert (i >= 0 && i < j);
    ARG  tmp;
    while (i < j)
    {
      tmp = S::elements[i];
      S::elements[i++] = S::elements[j];
      S::elements[j++] = tmp;
    }
  }

  private:
  typedef Stack<ARG, 513> S;
};

/* an operator prefixed by its operands in a byte string */
struct OpStr
{
  inline void init (void) {}

  OpCode  op;
  ByteStr str;
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

template <typename ARG=Number>
struct InterpEnv
{
  inline void init (const ByteStr &str_)
  {
    substr.reset (str_);
    argStack.init ();
  }

  inline void fini (void)
  {
    argStack.fini ();
  }

  inline bool fetch_op (OpCode &op)
  {
    if (unlikely (!substr.avail ()))
      return false;
    op = (OpCode)(unsigned char)substr[0];
    if (op == OpCode_escape) {
      if (unlikely (!substr.avail ()))
        return false;
      op = Make_OpCode_ESC (substr[1]);
      substr.inc ();
    }
    substr.inc ();
    return true;
  }

  inline void pop_n_args (unsigned int n)
  {
    assert (n <= argStack.count);
    argStack.count -= n;
  }

  inline void clear_args (void)
  {
    pop_n_args (argStack.count);
  }

  SubByteStr    substr;
  ArgStack<ARG> argStack;
};

typedef InterpEnv<> NumInterpEnv;

template <typename ARG=Number>
struct OpSet
{
  static inline bool process_op (OpCode op, InterpEnv<ARG>& env)
  {
    switch (op) {
      case OpCode_shortint:
        if (unlikely (!env.substr.avail (2) || !env.argStack.check_overflow (1)))
          return false;
        env.argStack.push_int ((int16_t)*(const HBUINT16*)&env.substr[0]);
        env.substr.inc (2);
        break;

      case OpCode_TwoBytePosInt0: case OpCode_TwoBytePosInt1:
      case OpCode_TwoBytePosInt2: case OpCode_TwoBytePosInt3:
        if (unlikely (!env.substr.avail () || !env.argStack.check_overflow (1)))
          return false;
        env.argStack.push_int ((int16_t)((op - OpCode_TwoBytePosInt0) * 256 + env.substr[0] + 108));
        env.substr.inc ();
        break;
      
      case OpCode_TwoByteNegInt0: case OpCode_TwoByteNegInt1:
      case OpCode_TwoByteNegInt2: case OpCode_TwoByteNegInt3:
        if (unlikely (!env.substr.avail () || !env.argStack.check_overflow (1)))
          return false;
        env.argStack.push_int ((int16_t)(-(op - OpCode_TwoByteNegInt0) * 256 - env.substr[0] - 108));
        env.substr.inc ();
        break;

      default:
        /* 1-byte integer */
        if (likely ((OpCode_OneByteIntFirst <= op) && (op <= OpCode_OneByteIntLast)) &&
            likely (env.argStack.check_overflow (1)))
        {
          env.argStack.push_int ((int)op - 139);
        } else {
          /* invalid unknown operator */
          env.clear_args ();
          return false;
        }
        break;
    }

    return true;
  }
};

template <typename ENV>
struct Interpreter {

  inline ~Interpreter(void) { fini (); }

  inline void fini (void) { env.fini (); }

  ENV env;
};

} /* namespace CFF */

#endif /* HB_CFF_INTERP_COMMON_HH */
