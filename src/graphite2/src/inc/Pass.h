// SPDX-License-Identifier: MIT
// Copyright 2010, SIL International, All rights reserved.

#pragma once

#include <cstdlib>
#include "inc/Code.h"
#include "inc/Rule.h"
#include "inc/SlotBuffer.h"

namespace graphite2 {

class Segment;
class Face;
class Silf;
struct State;
class FiniteStateMachine;
class Error;
class ShiftCollider;
class KernCollider;
class json;

enum passtype;

class Pass
{
public:
    Pass();
    ~Pass();

    bool readPass(const byte * pPass, size_t pass_length, size_t subtable_base, Face & face,
                  enum passtype pt, uint32_t version, Error &e);
    bool runGraphite(vm::Machine & m, ShapingContext & ctxt, bool reverse) const;
    void init(Silf *silf) { m_silf = silf; }
    byte collisionLoops() const { return m_numCollRuns; }
    bool reverseDir() const { return m_isReverseDir; }

    CLASS_NEW_DELETE
private:
    void    findNDoRule(vm::Machine &, ShapingContext & ctxt, vm::const_slotref &slot) const;
    int     doAction(const vm::Machine::Code* codeptr, SlotBuffer::iterator & slot_out, vm::Machine &) const;
    bool    testPassConstraint(vm::Machine & m) const;
    bool    testConstraint(const Rule & r, vm::Machine &) const;
    bool    readRules(const byte * rule_map, const size_t num_entries,
                     const byte *precontext, const uint16_t * sort_key,
                     const uint16_t * o_constraint, const byte *constraint_data,
                     const uint16_t * o_action, const byte * action_data,
                     Face &, enum passtype pt, Error &e);
    bool    readStates(const byte * starts, const byte * states, const byte * o_rule_map, Face &, Error &e);
    bool    readRanges(const byte * ranges, size_t num_ranges, Error &e);
    uint16_t  glyphToCol(const uint16_t gid) const;
    bool    runFSM(ShapingContext & ctxt, vm::const_slotref slot, Rules &rules) const;
    void    dumpRuleEventConsidered(ShapingContext const & ctxt, Rules::const_iterator first, Rules::const_iterator const & last) const;
    void    dumpRuleEventOutput(ShapingContext const & ctxt, Rule const & r, SlotBuffer::const_iterator const, SlotBuffer::const_iterator const) const;
    void    adjustSlot(int delta, vm::const_slotref & slot, ShapingContext &) const;
    bool    collisionShift(Segment & seg, int dir, json * const dbgout) const;
    bool    collisionKern(Segment & seg, int dir, json * const dbgout) const;
    bool    collisionFinish(Segment & seg, GR_MAYBE_UNUSED json * const dbgout) const;
    bool    resolveCollisions(Segment & seg, SlotBuffer::iterator const & slotFix, SlotBuffer::iterator start, ShiftCollider &coll, bool isRev,
                     int dir, bool &moved, bool &hasCol, json * const dbgout) const;
    float   resolveKern(Segment & seg, SlotBuffer::iterator const slotFix, GR_MAYBE_UNUSED SlotBuffer::iterator start, int dir,
    float &ymin, float &ymax, json *const dbgout) const;

    const Silf        * m_silf;
    uint16_t            * m_cols;
    Rule              * m_rules; // rules
    Rules::Entry      * m_ruleMap;
    uint16_t            * m_startStates; // prectxt length
    uint16_t            * m_transitions;
    State             * m_states;
    vm::Machine::Code * m_codes;
    byte              * m_progs;

    byte   m_numCollRuns;
    byte   m_kernColls;
    byte   m_iMaxLoop;
    uint16_t m_numGlyphs;
    uint16_t m_numRules;
    uint16_t m_numStates;
    uint16_t m_numTransition;
    uint16_t m_numSuccess;
    uint16_t m_successStart;
    uint16_t m_numColumns;
    byte m_minPreCtxt;
    byte m_maxPreCtxt;
    byte m_colThreshold;
    bool m_isReverseDir;
    vm::Machine::Code m_cPConstraint;

private:        //defensive
    Pass(const Pass&);
    Pass& operator=(const Pass&);
};

} // namespace graphite2
