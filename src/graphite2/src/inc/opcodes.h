// SPDX-License-Identifier: MIT
// Copyright 2010, SIL International, All rights reserved.

#pragma once
// This file will be pulled into and integrated into a machine implmentation
// DO NOT build directly and under no circumstances ever #include headers in
// here or you will break the direct_machine.
//
// Implementers' notes
// ==================
// You have access to a few primitives and the full C++ code:
//    declare_params(n) Tells the interpreter how many bytes of parameter
//                      space to claim for this instruction uses and
//                      initialises the param pointer.  You *must* before the
//                      first use of param.
//    use_params(n)     Claim n extra bytes of param space beyond what was
//                      claimed using delcare_param.
//    param             A const byte pointer for the parameter space claimed by
//                      this instruction.
//    binop(op)         Implement a binary operation on the stack using the
//                      specified C++ operator.
//    NOT_IMPLEMENTED   Any instruction body containing this will exit the
//                      program with an assertion error.  Instructions that are
//                      not implemented should also be marked NILOP in the
//                      opcodes tables this will cause the code class to spot
//                      them in a live code stream and throw a runtime_error
//                      instead.
//    push(n)           Push the value n onto the stack.
//    pop()             Pop the top most value and return it.
//
//    You have access to the following named fast 'registers':
//        sp        = The pointer to the current top of stack, the last value
//                    pushed.
//        seg       = A reference to the Segment this code is running over.
//        is        = The current slot index
//        isb       = The original base slot index at the start of this rule
//        isf       = The first positioned slot
//        isl       = The last positioned slot
//        ip        = The current instruction pointer
//        endPos    = Position of advance of last cluster
//        dir       = writing system directionality of the font


// #define NOT_IMPLEMENTED     assert(false)
// #define NOT_IMPLEMENTED

#define binop(op)           const uint32_t a = pop(); *reg.sp = uint32_t(*reg.sp) op a
#define sbinop(op)          const int32_t a = pop(); *reg.sp = int32_t(*reg.sp) op a
#define use_params(n)       reg.dp += (n)

#define declare_params(n)   const byte * param = reg.dp; \
                            use_params(n);

#define push(n)             { *++reg.sp = n; }
#define pop()               (*reg.sp--)
#define slotat(x)           (reg.is[(x)])
#define DIE                 { reg.os=reg.seg.slots().end(); reg.status = Machine::died_early; EXIT(1); }
//#define next_slot(x, op)        { op x; while(x -> deleted()) { op x;}; }
// TODO: Find out if Pass::runFSM can be made smarter when finishing building the map to avoid the need for this check.
// TODO: Move more common code into an opcodes_preamble.hxx, to avoid macros as functions.
#define position_context(slat) { \
    if (!reg.positioned && (slat == gr_slatPosX || slat == gr_slatPosY)) \
    { \
        auto last = reg.ctxt.map.back(); \
        if (last != reg.seg.slots().end())  ++last; \
        reg.seg.positionSlots(nullptr, reg.ctxt.map.front(), last, reg.seg.currdir()); \
        reg.positioned = true; \
    } \
}

STARTOP(nop)
    do {} while (0);
ENDOP

STARTOP(push_byte)
    declare_params(1);
    push(int8_t(*param));
ENDOP

STARTOP(push_byte_u)
    declare_params(1);
    push(uint8_t(*param));
ENDOP

STARTOP(push_short)
    declare_params(2);
    const int16_t r   = int16_t(param[0]) << 8
                    | uint8_t(param[1]);
    push(r);
ENDOP

STARTOP(push_short_u)
    declare_params(2);
    const uint16_t r  = uint16_t(param[0]) << 8
                    | uint8_t(param[1]);
    push(r);
ENDOP

STARTOP(push_long)
    declare_params(4);
    const  int32_t r  = int32_t(param[0]) << 24
                    | uint32_t(param[1]) << 16
                    | uint32_t(param[2]) << 8
                    | uint8_t(param[3]);
    push(r);
ENDOP

STARTOP(add)
    binop(+);
ENDOP

STARTOP(sub)
    binop(-);
ENDOP

STARTOP(mul)
    binop(*);
ENDOP

STARTOP(div_)
    const int32_t b = pop();
    const int32_t a = int32_t(*reg.sp);
    if (b == 0 || (a == std::numeric_limits<int32_t>::min() && b == -1)) DIE;
    *reg.sp = int32_t(*reg.sp) / b;
ENDOP

STARTOP(min_)
    const int32_t a = pop(), b = *reg.sp;
    if (a < b) *reg.sp = a;
ENDOP

STARTOP(max_)
    const int32_t a = pop(), b = *reg.sp;
    if (a > b) *reg.sp = a;
ENDOP

STARTOP(neg)
    *reg.sp = uint32_t(-int32_t(*reg.sp));
ENDOP

STARTOP(trunc8)
    *reg.sp = uint8_t(*reg.sp);
ENDOP

STARTOP(trunc16)
    *reg.sp = uint16_t(*reg.sp);
ENDOP

STARTOP(cond)
    const uint32_t f = pop(), t = pop(), c = pop();
    push(c ? t : f);
ENDOP

STARTOP(and_)
    binop(&&);
ENDOP

STARTOP(or_)
    binop(||);
ENDOP

STARTOP(not_)
    *reg.sp = !*reg.sp;
ENDOP

STARTOP(equal)
    binop(==);
ENDOP

STARTOP(not_eq_)
    binop(!=);
ENDOP

STARTOP(less)
    sbinop(<);
ENDOP

STARTOP(gtr)
    sbinop(>);
ENDOP

STARTOP(less_eq)
    sbinop(<=);
ENDOP

STARTOP(gtr_eq)
    sbinop(>=);
ENDOP

STARTOP(next)
    if (reg.is - &reg.ctxt.map[0] >= int(reg.ctxt.map.size())) DIE
    if (reg.os != reg.seg.slots().end())
    {
        if (reg.os == reg.ctxt.highwater())
            reg.ctxt.highpassed(true);
        ++reg.os;
    }
    ++reg.is;
ENDOP

//STARTOP(next_n)
//    use_params(1);
//    NOT_IMPLEMENTED;
    //declare_params(1);
    //const size_t num = uint8_t(*param);
//ENDOP

//STARTOP(copy_next)
//     if (reg.os) next_slot(reg.os,++);
//     next_slot(reg.is, ++);
// ENDOP

STARTOP(put_glyph_8bit_obs)
    declare_params(1);
    const unsigned int output_class = uint8_t(*param);
    reg.os->glyph(reg.seg, reg.seg.getClassGlyph(output_class, 0));
ENDOP

STARTOP(put_subs_8bit_obs)
    declare_params(3);
    const int           slot_ref     = int8_t(param[0]);
    const unsigned int  input_class  = uint8_t(param[1]),
                        output_class = uint8_t(param[2]);
    uint16_t index;
    slotref slot = slotat(slot_ref);
    if (slot != reg.seg.slots().end())
    {
        index = reg.seg.findClassIndex(input_class, slot->gid());
        reg.os->glyph(reg.seg, reg.seg.getClassGlyph(output_class, index));
    }
ENDOP

STARTOP(put_copy)
    declare_params(1);
    const int  slot_ref = int8_t(*param);
    if (reg.os != reg.seg.slots().end() && !reg.os->deleted())
    {
        auto ref = slotat(slot_ref);
        if (ref != reg.seg.slots().end() && ref != reg.os)
        {
            if (!reg.os->isBase() || reg.os->isParent()) DIE

            auto g = reg.os->generation();
            *reg.os = *ref;
            reg.os->generation() = g;
            if (!reg.os->isBase())
                reg.os->attachedTo()->add_child(&*reg.os);
        }
        reg.os->copied(false);
        reg.os->deleted(false);
    }
ENDOP

STARTOP(insert)
    if (reg.ctxt.decMax() <= 0) DIE;
    auto iss = reg.os;
    while (iss != reg.seg.slots().end() && iss->deleted()) ++iss;

    auto slot = reg.seg.slots().emplace(iss++, reg.seg.numAttrs());
    if (slot == reg.seg.slots().end()) DIE;

    slot->generation() += reg.seg.slots().size();
    slot->clusterhead(true);
    switch ((slot == reg.seg.slots().begin()) << 1 | (iss == reg.seg.slots().end()))
    {
        case 0: // Medial insertion
            slot->before(std::prev(slot)->after());           
            slot->original(iss->original());
            slot->after(iss->before());
            break;
        case 1: // Final insertion
            slot->before(std::prev(slot)->before());
            slot->original(std::prev(slot)->original());
            slot->after(std::prev(slot)->after());
            break;
        case 2: // Initial insertion
            slot->before(iss->before());
            slot->original(iss->original());
            slot->after(iss->before());
            break;
        default: // Singleton insertion
            slot->original(reg.seg.defaultOriginal());
            break;
    }

    if (reg.os == reg.ctxt.highwater())
        reg.ctxt.highpassed(false);
    reg.os = slot;
    reg.seg.extendLength(1);

    for (auto i = reg.is; i != reg.ctxt.map.end(); ++i) ++*i;
    if (reg.is >= reg.ctxt.map.begin())
        --reg.is;
ENDOP

STARTOP(delete_)
    if (reg.os == reg.seg.slots().end() || reg.os->deleted()) DIE
    reg.seg.extendLength(-1);
    reg.os->deleted(true);
    if (reg.os == reg.ctxt.highwater())
        reg.ctxt.highwater(std::next(reg.os));

    if (!(*reg.is)->copied()) {
        *reg.is = slotref::from(new Slot(std::move(*reg.os)));
        (*reg.is)->copied(true);
    }
    else { **reg.is = std::move(*reg.os); }

    reg.os = reg.seg.slots().erase(reg.os);
    // if (reg.os != reg.seg.slots().begin())
        --reg.os;

    for (auto i = reg.is+1; i != reg.ctxt.map.end(); ++i) --*i;
ENDOP

STARTOP(assoc)
    declare_params(1);
    unsigned int  num = uint8_t(*param);
    const int8_t *  assocs = reinterpret_cast<const int8_t *>(param+1);
    use_params(num);
    int max = -1;
    int min = -1;

    while (num-- > 0)
    {
        int sr = *assocs++;
        slotref ts = slotat(sr);
        if (ts != reg.seg.slots().end() && (min == -1 || ts->before() < min)) min = ts->before();
        if (ts != reg.seg.slots().end() && ts->after() > max) max = ts->after();
    }
    if (min > -1)   // implies max > -1
    {
        reg.os->before(min);
        reg.os->after(max);
    }
ENDOP

STARTOP(cntxt_item)
    // It turns out this is a cunningly disguised condition forward jump.
    declare_params(3);
    const int       is_arg = int8_t(param[0]);
    const size_t    iskip  = uint8_t(param[1]),
                    dskip  = uint8_t(param[2]);

    if (reg.isb + is_arg != reg.is)
    {
        reg.ip += iskip;
        reg.dp += dskip;
        push(true);
    }
ENDOP

STARTOP(attr_set)
    declare_params(1);
    auto const  slat = Slot::attrCode(uint8_t(*param));
    int const   val  = pop();
    reg.os->setAttr(reg.seg, slat, 0, val, reg.ctxt);
ENDOP

STARTOP(attr_add)
    declare_params(1);
    auto const      slat = Slot::attrCode(uint8_t(*param));
    uint32_t const  val  = pop();
    position_context(slat)
    uint32_t res = uint32_t(reg.os->getAttr(reg.seg, slat, 0));
    reg.os->setAttr(reg.seg, slat, 0, int32_t(val + res), reg.ctxt);
ENDOP

STARTOP(attr_sub)
    declare_params(1);
    auto const      slat = Slot::attrCode(uint8_t(*param));
    uint32_t const  val  = pop();
    position_context(slat)
    uint32_t res = uint32_t(reg.os->getAttr(reg.seg, slat, 0));
    reg.os->setAttr(reg.seg, slat, 0, int32_t(res - val), reg.ctxt);
ENDOP

STARTOP(attr_set_slot)
    declare_params(1);
    auto const  slat   = Slot::attrCode(uint8_t(*param));
    int const   offset = int(reg.is - reg.ctxt.map.begin())*int(slat == gr_slatAttTo);
    int const   val    = pop()  + offset;
    reg.os->setAttr(reg.seg, slat, offset, val, reg.ctxt);
ENDOP

STARTOP(iattr_set_slot)
    declare_params(2);
    auto const  slat = Slot::attrCode(uint8_t(param[0]));
    uint8_t const idx  = uint8_t(param[1]);
    int const   val  = int(pop()  + (reg.is - reg.ctxt.map.begin())*int(slat == gr_slatAttTo));
    reg.os->setAttr(reg.seg, slat, idx, val, reg.ctxt);
ENDOP

STARTOP(push_slot_attr)
    declare_params(2);
    auto const  slat     = Slot::attrCode(uint8_t(param[0]));
    int const   slot_ref = int8_t(param[1]);
    position_context(slat)
    slotref slot = slotat(slot_ref);
    if (slot != reg.seg.slots().end())
    {
        int res = slot->getAttr(reg.seg, slat, 0);
        push(res);
    }
ENDOP

STARTOP(push_glyph_attr_obs)
    declare_params(2);
    unsigned int const  glyph_attr = uint8_t(param[0]);
    int const           slot_ref   = int8_t(param[1]);
    slotref slot = slotat(slot_ref);
    if (slot != reg.seg.slots().end())
        push(int32_t(reg.seg.glyphAttr(slot->gid(), glyph_attr)));
ENDOP

STARTOP(push_glyph_metric)
    declare_params(3);
    const auto          glyph_attr  = metrics(param[0]);
    const int           slot_ref    = int8_t(param[1]);
    const signed int    attr_level  = uint8_t(param[2]);
    slotref slot = slotat(slot_ref);
    if (slot != reg.seg.slots().end())
        push(reg.seg.getGlyphMetric(&*slot, glyph_attr, attr_level, reg.ctxt.dir));
ENDOP

STARTOP(push_feat)
    declare_params(2);
    const unsigned int  feat        = uint8_t(param[0]);
    const int           slot_ref    = int8_t(param[1]);
    slotref slot = slotat(slot_ref);
    if (slot != reg.seg.slots().end())
    {
        uint8_t fid = reg.seg.charinfo(slot->original())->fid();
        push(reg.seg.getFeature(fid, feat));
    }
ENDOP

STARTOP(push_att_to_gattr_obs)
    declare_params(2);
    const unsigned int  glyph_attr  = uint8_t(param[0]);
    const int           slot_ref    = int8_t(param[1]);
    slotref slot = slotat(slot_ref);
    if (slot != reg.seg.slots().end())
    {
        auto att = slot->attachedTo();
        auto & ref = att ? *att : *slot;
        push(int32_t(reg.seg.glyphAttr(ref.gid(), glyph_attr)));
    }
ENDOP

STARTOP(push_att_to_glyph_metric)
    declare_params(3);
    const auto          glyph_attr  = metrics(param[0]);
    const int           slot_ref    = int8_t(param[1]);
    const signed int    attr_level  = uint8_t(param[2]);
    slotref slot = slotat(slot_ref);
    if (slot != reg.seg.slots().end())
    {
        auto parent = slot->attachedTo();
        if (!parent) parent = &*slot;
        push(int32_t(reg.seg.getGlyphMetric(parent, glyph_attr, attr_level, reg.ctxt.dir)));
    }
ENDOP

STARTOP(push_islot_attr)
    declare_params(3);
    auto const  slat     = Slot::attrCode(uint8_t(param[0]));
    int const   slot_ref = int8_t(param[1]),
                idx      = uint8_t(param[2]);
    position_context(slat)
    slotref slot = slotat(slot_ref);
    if (slot != reg.seg.slots().end())
    {
        int res = slot->getAttr(reg.seg, slat, idx);
        push(res);
    }
ENDOP

#if 0
STARTOP(push_iglyph_attr) // not implemented
    NOT_IMPLEMENTED;
ENDOP
#endif

STARTOP(pop_ret)
    const uint32_t ret = pop();
    EXIT(ret);
ENDOP

STARTOP(ret_zero)
    EXIT(0);
ENDOP

STARTOP(ret_true)
    EXIT(1);
ENDOP

STARTOP(iattr_set)
    declare_params(2);
    auto const  slat = Slot::attrCode(uint8_t(param[0]));
    uint8_t const idx  = uint8_t(param[1]);
    int const   val  = pop();
    reg.os->setAttr(reg.seg, slat, idx, val, reg.ctxt);
ENDOP

STARTOP(iattr_add)
    declare_params(2);
    auto const      slat = Slot::attrCode(uint8_t(param[0]));
    uint8_t const     idx  = uint8_t(param[1]);
    uint32_t const  val  = pop();
    position_context(slat)
    uint32_t res = uint32_t(reg.os->getAttr(reg.seg, slat, idx));
    reg.os->setAttr(reg.seg, slat, idx, int32_t(val + res), reg.ctxt);
ENDOP

STARTOP(iattr_sub)
    declare_params(2);
    auto const      slat = Slot::attrCode(uint8_t(param[0]));
    uint8_t const     idx  = uint8_t(param[1]);
    uint32_t const  val  = pop();
    position_context(slat)
    uint32_t res = uint32_t(reg.os->getAttr(reg.seg, slat, idx));
    reg.os->setAttr(reg.seg, slat, idx, int32_t(res - val), reg.ctxt);
ENDOP

STARTOP(push_proc_state)
    use_params(1);
    push(1);
ENDOP

STARTOP(push_version)
    push(0x00030000);
ENDOP

STARTOP(put_subs)
    declare_params(5);
    const int        slot_ref     = int8_t(param[0]);
    const unsigned int  input_class  = uint8_t(param[1]) << 8
                                     | uint8_t(param[2]);
    const unsigned int  output_class = uint8_t(param[3]) << 8
                                     | uint8_t(param[4]);
    slotref slot = slotat(slot_ref);
    if (slot != reg.seg.slots().end())
    {
        int index = reg.seg.findClassIndex(input_class, slot->gid());
        reg.os->glyph(reg.seg, reg.seg.getClassGlyph(output_class, index));
    }
ENDOP

#if 0
STARTOP(put_subs2) // not implemented
    NOT_IMPLEMENTED;
ENDOP

STARTOP(put_subs3) // not implemented
    NOT_IMPLEMENTED;
ENDOP
#endif

STARTOP(put_glyph)
    declare_params(2);
    const unsigned int output_class  = uint8_t(param[0]) << 8
                                     | uint8_t(param[1]);
    reg.os->glyph(reg.seg, reg.seg.getClassGlyph(output_class, 0));
ENDOP

STARTOP(push_glyph_attr)
    declare_params(3);
    const unsigned int  glyph_attr  = uint8_t(param[0]) << 8
                                    | uint8_t(param[1]);
    const int           slot_ref    = int8_t(param[2]);
    slotref slot = slotat(slot_ref);
    if (slot != reg.seg.slots().end())
        push(int32_t(reg.seg.glyphAttr(slot->gid(), glyph_attr)));
ENDOP

STARTOP(push_att_to_glyph_attr)
    declare_params(3);
    const unsigned int  glyph_attr  = uint8_t(param[0]) << 8
                                    | uint8_t(param[1]);
    const int           slot_ref    = int8_t(param[2]);
    slotref slot = slotat(slot_ref);
    if (slot != reg.seg.slots().end())
    {
        auto att = slot->attachedTo();
        auto & ref = att ? *att : *slot;
        push(int32_t(reg.seg.glyphAttr(ref.gid(), glyph_attr)));
    }
ENDOP

STARTOP(temp_copy)
#if 0
    reg.seg.slots().push_back(Slot());
    auto slot = --reg.seg.slots().end();
    int16_t *tempUserAttrs = slot->userAttrs();
    memcpy(&reg.seg.slots().back(), &*reg.os, sizeof(Slot));
    memcpy(tempUserAttrs, reg.os->userAttrs(), reg.seg.numAttrs() * sizeof(uint16_t));
    slot->userAttrs(tempUserAttrs);
    slot->copied(true);
    reg.seg.slots().erase(slot);
    *reg.is = slot;
#else
    auto slot = reg.seg.newSlot();
    if (slot == reg.seg.slots().end() || reg.os == reg.seg.slots().end()) DIE;
    // copy slot reg.os into new slot
    *slot = *reg.os;
    slot->copied(true);
    // TODO: remove this once we're using gr::list methods. This is the
    // hack that, that enables the hack, that enables debug output.
    // slot.prev(std::prev(reg.os));
    // slot.next(std::next(reg.os));
    *reg.is = slot;
#endif
ENDOP

STARTOP(band)
    binop(&);
ENDOP

STARTOP(bor)
    binop(|);
ENDOP

STARTOP(bnot)
    *reg.sp = ~*reg.sp;
ENDOP

STARTOP(setbits)
    declare_params(4);
    const uint16_t m  = uint16_t(param[0]) << 8
                    | uint8_t(param[1]);
    const uint16_t v  = uint16_t(param[2]) << 8
                    | uint8_t(param[3]);
    *reg.sp = ((*reg.sp) & ~m) | v;
ENDOP

STARTOP(set_feat)
    declare_params(2);
    const unsigned int  feat        = uint8_t(param[0]);
    const int           slot_ref    = int8_t(param[1]);
    slotref slot = slotat(slot_ref);
    if (slot != reg.seg.slots().end())
    {
        uint8_t fid = reg.seg.charinfo(slot->original())->fid();
        reg.seg.setFeature(fid, feat, pop());
    }
ENDOP
