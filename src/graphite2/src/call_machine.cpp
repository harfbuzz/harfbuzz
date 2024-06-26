// SPDX-License-Identifier: MIT
// Copyright 2010, SIL International, All rights reserved.

// This call threaded interpreter implmentation for machine.h
// Author: Tim Eves

// Build either this interpreter or the direct_machine implementation.
// The call threaded interpreter is portable across compilers and
// architectures as well as being useful to debug (you can set breakpoints on
// opcodes) but is slower that the direct threaded interpreter by a factor of 2

#include <cassert>
#include <cstring>
#include <graphite2/Segment.h>
#include "inc/Machine.h"
#include "inc/Segment.h"
#include "inc/ShapingContext.hpp"
#include "inc/Slot.h"
#include "inc/Rule.h"

#define registers           vm::Machine::regbank & reg

// These are required by opcodes.h and should not be changed
#define STARTOP(name)       bool name(registers) REGPARM(4);\
                            bool name(registers) {
#define ENDOP                   return (reg.sp - reg.sb)/Machine::STACK_MAX==0; \
                            }

#define EXIT(status)        { push(status); return false; }

// This is required by opcode_table.h
#define do_(name)           instr(name)


using namespace graphite2;
using namespace vm;

typedef bool        (* ip_t)(registers);

// Pull in the opcode definitions
// We pull these into a private namespace so these otherwise common names dont
// pollute the toplevel namespace.
namespace { 
#include "inc/opcodes.h" 
}

Machine::stack_t  Machine::run(const instr   * program,
                               const byte    * data,
                               const_slotref *& slot_in,
                               slotref       & slot_out)

{
    assert(program != nullptr);

    // Declare virtual machine registers
    regbank reg = {
        program-1,                          // reg.ip
        data,                               // reg.dp
        _stack + Machine::STACK_GUARD,      // reg.sp
        _stack + Machine::STACK_GUARD,      // reg.sb
        _status,                            // reg.status
        _ctxt,                              // reg.ctxt
        _ctxt.segment,                      // reg.seg
        slot_in,                            // reg.is
        _ctxt.map.begin()+_ctxt.context(),  // reg.isb
        slot_out,                           // reg.os
        0,                                  // reg.flags
    };

    // Run the program
    while ((reinterpret_cast<ip_t>(*++reg.ip))(reg)) {}
    const stack_t ret = reg.sp == _stack+STACK_GUARD+1 ? *reg.sp-- : 0;

    check_final_stack(reg.sp);
    slot_in = reg.is;
    slot_out = reg.os;
    return ret;
}

// Pull in the opcode table
namespace {
#include "inc/opcode_table.h"
}

const opcode_t * Machine::getOpcodeTable() throw()
{
    return opcode_table;
}
