// SPDX-License-Identifier: MIT
// Copyright 2010, SIL International, All rights reserved.

// This general interpreter interface.
// Author: Tim Eves

// Build one of direct_machine.cpp or call_machine.cpp to implement this
// interface.

#pragma once
#include <cstring>
#include <limits>
#include <graphite2/Types.h>

#include "inc/Main.h"
#include "inc/ShapingContext.hpp"
#include "inc/SlotBuffer.h"

#if defined(__GNUC__)
#if defined(__clang__) || (__GNUC__ * 100 + __GNUC_MINOR__ * 10) < 430
#define     HOT
#if defined(__x86_64)
#define     REGPARM(n)      __attribute__((regparm(n)))
#else
#define     REGPARM(n)
#endif
#else
#define     HOT             __attribute__((hot))
#if defined(__x86_64)
#define     REGPARM(n)      __attribute__((hot, regparm(n)))
#else
#define     REGPARM(n)
#endif
#endif
#else
#define     HOT
#define     REGPARM(n)
#endif

#if defined(__MINGW32__)
// MinGW's <limits> at some point includes winnt.h which #define's a
// DELETE macro, which conflicts with enum opcode below, so we undefine
// it here.
#undef DELETE
#endif

namespace graphite2 {

// Forward declarations
class Segment;
class Slot;
class ShapingContext;


namespace vm
{


using instr = void *;
using slotref = SlotBuffer::iterator;
using const_slotref = SlotBuffer::iterator;

enum {VARARGS = 0xff, MAX_NAME_LEN=32};

enum opcode {
    NOP = 0,

    PUSH_BYTE,      PUSH_BYTEU,     PUSH_SHORT,     PUSH_SHORTU,    PUSH_LONG,

    ADD,            SUB,            MUL,            DIV,
    MIN_,           MAX_,
    NEG,
    TRUNC8,         TRUNC16,

    COND,

    AND,            OR,             NOT,
    EQUAL,          NOT_EQ,
    LESS,           GTR,            LESS_EQ,        GTR_EQ,

    NEXT,           NEXT_N,         COPY_NEXT,
    PUT_GLYPH_8BIT_OBS,              PUT_SUBS_8BIT_OBS,   PUT_COPY,
    INSERT,         DELETE,
    ASSOC,
    CNTXT_ITEM,

    ATTR_SET,       ATTR_ADD,       ATTR_SUB,
    ATTR_SET_SLOT,
    IATTR_SET_SLOT,
    PUSH_SLOT_ATTR,                 PUSH_GLYPH_ATTR_OBS,
    PUSH_GLYPH_METRIC,              PUSH_FEAT,
    PUSH_ATT_TO_GATTR_OBS,          PUSH_ATT_TO_GLYPH_METRIC,
    PUSH_ISLOT_ATTR,

    PUSH_IGLYPH_ATTR,    // not implemented

    POP_RET,                        RET_ZERO,           RET_TRUE,
    IATTR_SET,                      IATTR_ADD,          IATTR_SUB,
    PUSH_PROC_STATE,                PUSH_VERSION,
    PUT_SUBS,                       PUT_SUBS2,          PUT_SUBS3,
    PUT_GLYPH,                      PUSH_GLYPH_ATTR,    PUSH_ATT_TO_GLYPH_ATTR,
    BITOR,                          BITAND,             BITNOT,
    BITSET,                         SET_FEAT,
    MAX_OPCODE,
    // private opcodes for internal use only, comes after all other on disk opcodes
    TEMP_COPY = MAX_OPCODE
};

struct opcode_t
{
    instr           impl[2];
    uint8_t           param_sz;
    char            name[MAX_NAME_LEN];
};


class Machine
{
public:
    struct regbank;
    typedef int32_t  stack_t;
    static size_t const STACK_ORDER  = 10,
                        STACK_MAX    = 1 << STACK_ORDER,
                        STACK_GUARD  = 2;

    class Code;

    enum status_t {
        finished = 0,
        stack_underflow,
        stack_not_empty,
        stack_overflow,
        slot_offset_out_bounds,
        died_early
    };

    Machine(ShapingContext &) throw();
    static const opcode_t *   getOpcodeTable() throw();

    CLASS_NEW_DELETE;

    ShapingContext   & shaping_context() const throw();
    status_t    status() const throw();
//    operator bool () const throw();


private:
    void    check_final_stack(const stack_t * const sp);
    stack_t run(const instr * program, const byte * data,
                const_slotref *& slot_in,
                slotref & slot_out) HOT;

    ShapingContext    & _ctxt;
    stack_t             _stack[STACK_MAX + 2*STACK_GUARD];
    status_t            _status;
};

struct Machine::regbank  {
    instr const           * ip;
    byte const            * dp;
    Machine::stack_t      * sp,
                    * const sb;
    Machine::status_t     & status;
    ShapingContext        & ctxt;
    Segment               & seg;
    slotref *               is;
    slotref *         const isb;
    slotref                 os;
    int8_t                    positioned;
};

inline Machine::Machine(ShapingContext & ctxt) throw()
: _ctxt(ctxt), _status(finished)
{
    // Initialise stack guard +1 entries as the stack pointer points to the
    //  current top of stack, hence the first push will never write entry 0.
    // Initialising the guard space like this is unnecessary and is only
    //  done to keep valgrind happy during fuzz testing.  Hopefully loop
    //  unrolling will flatten this.
    for (size_t n = STACK_GUARD + 1; n; --n)  _stack[n-1] = 0;
}

inline ShapingContext& Machine::shaping_context() const throw()
{
    return _ctxt;
}

inline Machine::status_t Machine::status() const throw()
{
    return _status;
}

inline void Machine::check_final_stack(const stack_t * const sp)
{
    if (_status != finished) return;

    stack_t const * const base  = _stack + STACK_GUARD,
                  * const limit = base + STACK_MAX;
    if      (sp <  base)    _status = stack_underflow;       // This should be impossible now.
    else if (sp >= limit)   _status = stack_overflow;        // So should this.
    else if (sp != base)    _status = stack_not_empty;
}

} // namespace vm
} // namespace graphite2
