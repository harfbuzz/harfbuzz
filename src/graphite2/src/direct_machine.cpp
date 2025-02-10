// SPDX-License-Identifier: MIT
// Copyright 2010, SIL International, All rights reserved.

// This direct threaded interpreter implmentation for machine.h
// Author: Tim Eves

// Build either this interpreter or the call_machine implementation.
// The direct threaded interpreter is relies upon a gcc feature called
// labels-as-values so is only portable to compilers that support the
// extension (gcc only as far as I know) however it should build on any
// architecture gcc supports.
// This is twice as fast as the call threaded model and is likely faster on
// inorder processors with short pipelines and little branch prediction such
// as the ARM and possibly Atom chips.


#include <cassert>
#include <cstring>
#include "inc/Machine.h"
#include "inc/Segment.h"
#include "inc/ShapingContext.hpp"
#include "inc/Slot.h"
#include "inc/Rule.h"

#define STARTOP(name)           name: {
#define ENDOP                   }; goto *((reg.sp - reg.sb)/Machine::STACK_MAX ? &&end : *++reg.ip);
#define EXIT(status)            { push(status); goto end; }

#define do_(name)               &&name


using namespace graphite2;
using namespace vm;

namespace {

// The GCC manual has this to say about labels as values:
//   The &&foo expressions for the same label might have different values
//   if the containing function is inlined or cloned. If a program relies
//   on them being always the same, __attribute__((__noinline__,__noclone__))
//   should be used to prevent inlining and cloning.
//
// is_return in Code.cpp relies on being able to do comparisons, so it needs
// them to be always the same.
//
// The GCC manual further adds:
//   If &&foo is used in a static variable initializer, inlining and
//   cloning is forbidden.
//
// In this file, &&foo *is* used in a static variable initializer, and it's not
// entirely clear whether this should prevent inlining of the function or not.
// In practice, though, clang 7 can end up inlining the function with ThinLTO,
// which breaks at least is_return. https://bugs.llvm.org/show_bug.cgi?id=39241
// So all in all, we need at least the __noinline__ attribute. __noclone__
// is not supported by clang.
__attribute__((__noinline__))
const void * direct_run(Machine::regbank * _reg = nullptr)
{
    // We need to define and return to opcode table from within this function
    // other inorder to take the addresses of the instruction bodies.
    #include "inc/opcode_table.h"
    if (_reg == nullptr)
        return opcode_table;

    auto & reg = *_reg;
    // start the program
    goto **reg.ip;

    // Pull in the opcode definitions
    #include "inc/opcodes.h"

    end: return nullptr;
}

}

const opcode_t * Machine::getOpcodeTable() throw()
{
    return static_cast<const opcode_t *>(direct_run());
}


Machine::stack_t  Machine::run(const instr   *   program,
                               const byte    *   data,
                               const_slotref * & slot_in,
                               slotref         & slot_out)
{
    assert(program != 0);

    // Declare virtual machine registers
    regbank reg = {
        program,                                // reg.ip
        data,                                   // reg.dp
        _stack + Machine::STACK_GUARD,          // reg.sp
        _stack + Machine::STACK_GUARD,          // reg.sb
        _status,                                // reg.status
        _ctxt,                                  // reg.ctxt
        _ctxt.segment,                          // reg.seg
        slot_in,                                     // reg.is
        _ctxt.map.begin()+_ctxt.context(),      // reg.isb
        slot_out,                                    // reg.os
        0,                                      // reg.flags
    };

    direct_run(&reg);
    slot_in = reg.is;
    slot_out = reg.os;
    const stack_t ret = reg.sp == _stack+STACK_GUARD+1 ? *reg.sp-- : 0;
    check_final_stack(reg.sp);
    return ret;
}
