// SPDX-License-Identifier: MIT
// Copyright 2010, SIL International, All rights reserved.

#include "inc/Segment.h"
#include "inc/CharInfo.h"
#include "inc/Collider.h"
#include "inc/Font.h"
#include "inc/ShapingContext.hpp"
#include "inc/Slot.h"
#include "inc/Silf.h"
#include "inc/Rule.h"


using namespace graphite2;

auto Slot::attributes::operator = (attributes const & rhs) -> attributes & {
    if (this != &rhs) {
        reserve(rhs.num_attrs(), rhs.num_justs());
        if (!rhs.is_inline() && external) {
            auto const sz = external->n_attrs 
                            + external->n_justs*NUMJUSTPARAMS + 1;
            memcpy(external, rhs.external, sz*sizeof(uint16_t));
        } else local = rhs.local;
    }
    return *this;
}

auto Slot::attributes::operator = (attributes && rhs) noexcept -> attributes & {
    if (!is_inline()) free(external);
    external = rhs.external;
    rhs.external = nullptr;
    return *this;
}

void Slot::attributes::reserve(size_t target_num_attrs, size_t target_num_justs) {
    assert(target_num_attrs <  256);
    assert(target_num_justs <  256);

    if (num_attrs() >= target_num_attrs 
        && num_justs() >= target_num_justs)
        return;

    if (target_num_justs > 0 
        || target_num_attrs > sizeof local.data/sizeof *local.data)
    {
        auto const sz = target_num_attrs + target_num_justs*NUMJUSTPARAMS + 1;

        if (is_inline()) {  // Convert to non-inline form.
            auto box = reinterpret_cast<decltype(external)>(grzeroalloc<int16_t>(sz));
            if (box) {
                if (local.n_attrs) memcpy(box->data, local.data, local.n_attrs*sizeof *local.data);
                external = box;
                external->n_attrs = target_num_attrs;
                external->n_justs = target_num_justs;
            }
        } else { // Grow the existing buffer.
            external = static_cast<decltype(external)>(realloc(external, sz*sizeof *local.data));
            external->n_attrs = target_num_attrs;
            external->n_justs = target_num_justs;
        }
    }
    else local.n_attrs = target_num_attrs;
}


void Slot::init_just_infos(Segment const & seg)
{
    auto const target_num_justs = seg.silf()->numJustLevels();

    for (int i = target_num_justs - 1; i >= 0; --i)
    {
        Justinfo *justs = seg.silf()->justAttrs() + i;
        int16_t *v = m_attrs.just_info() + i * NUMJUSTPARAMS;
        v[0] = seg.glyphAttr(gid(), justs->attrStretch());
        v[1] = seg.glyphAttr(gid(), justs->attrShrink());
        v[2] = seg.glyphAttr(gid(), justs->attrStep());
        v[3] = seg.glyphAttr(gid(), justs->attrWeight());
    }
}

Slot & Slot::operator = (Slot && rhs) noexcept 
{
    if (this != &rhs)
    {
        Slot_data::operator=(std::move(rhs));
        m_parent_offset =  0;
        m_attrs = std::move(rhs.m_attrs);
#if !defined GRAPHITE2_NTRACING
        m_gen = rhs.m_gen;
#endif
    }
    return *this;
}

Slot & Slot::operator = (Slot const & rhs)
{
    if (this != &rhs)
    {
        Slot_data::operator=(rhs);
        m_parent_offset = 0;
        m_attrs = rhs.m_attrs;
#if !defined GRAPHITE2_NTRACING
        m_gen = rhs.m_gen;
#endif
    }

    return *this;
}

void Slot::update(int /*numGrSlots*/, int numCharInfo, Position &relpos)
{
    m_before += numCharInfo;
    m_after += numCharInfo;
    m_position = m_position + relpos;
}


Position Slot::collision_shift(Segment const & seg) const
{
    auto const cinfo = seg.collisionInfo(*this);
    return cinfo ? cinfo->offset() : Position{};
}


Position Slot::update_cluster_metric(Segment const & seg, bool const rtl, bool const is_final, float & clsb, float & crsb, unsigned depth)
{
    // Bail out early if the attachment chain is too deep.
    if (depth == 0) return Position();

    // Incorporate shift into positioning, and cluster rsb calculations.
    Position shift = {m_shift.x + m_just, m_shift.y};
    auto const collision_info = seg.collisionInfo(*this);
    if (is_final && collision_info) {
        // if (!(collision_info->flags() & SlotCollision::COLL_KERN) || rtl)
            shift += collision_info->offset();
    }

    // Only consider if design space advance is a non-zero whole unit.
    auto const slot_adv = m_advance.x + m_just;
    auto const parent = attachedTo();
    auto pos = shift;
    if (!parent) {
        clsb = min(0.0f, clsb);
        pos = {0,0};
        shift = {0,0};
    } else {
        auto base = parent->update_cluster_metric(seg, rtl, is_final, clsb, crsb, depth-1);
        m_position = (pos += base + m_attach - m_with);
        if (m_advance.x >= 0.5f)
            clsb = min(pos.x, clsb);
    }

    if (m_advance.x >= 0.5f)
        // We only consider shift for attached glyphs not ourselves.
        crsb = max(pos.x - shift.x + slot_adv, crsb);
    return pos;
}


Position Slot::finalise(const Segment & seg, const Font *font, Position & base, Rect & bbox, uint8_t attrLevel, float & clusterMin, bool rtl, bool isFinal, int depth)
{
    // assert(false);
    SlotCollision *coll = NULL;
    if (depth > 100 || (attrLevel && m_attLevel > attrLevel)) return Position(0, 0);
    float scale = font ? font->scale() : 1.0f;
    Position shift(m_shift.x * (rtl * -2 + 1) + m_just, m_shift.y);
    float tAdvance = m_advance.x + m_just;
    if (isFinal && (coll = seg.collisionInfo(*this)))
    {
        const Position &collshift = coll->offset();
        if (!(coll->flags() & SlotCollision::COLL_KERN) || rtl)
            shift = shift + collshift;
    }
    const GlyphFace * glyphFace = seg.getFace()->glyphs().glyphSafe(glyph());
    if (font)
    {
        scale = font->scale();
        shift *= scale;
        if (font->isHinted() && glyphFace)
            tAdvance = (m_advance.x - glyphFace->theAdvance().x + m_just) * scale + font->advance(glyph());
        else
            tAdvance *= scale;
    }
    Position res;

    m_position = base + shift;
    if (isBase())
    {
        res = base + Position(tAdvance, m_advance.y * scale);
        clusterMin = m_position.x;
    }
    else
    {
        float tAdv;
        m_position += (m_attach - m_with) * scale;
        tAdv = m_advance.x >= 0.5f ? m_position.x + tAdvance - shift.x : 0.f;
        res = Position(tAdv, 0);
        if ((m_advance.x >= 0.5f || m_position.x < 0) && m_position.x < clusterMin) clusterMin = m_position.x;
    }

    if (glyphFace)
    {
        Rect ourBbox = glyphFace->theBBox() * scale + m_position;
        bbox = bbox.widen(ourBbox);
    }

    for (auto c = children(); c != end(); ++c) 
    {
        auto tRes = c->finalise(seg, font, m_position, bbox, attrLevel, clusterMin, rtl, isFinal, depth + 1);
        if ((isBase() || m_advance.x >= 0.5f) && tRes.x > res.x) res = tRes;
    }
    if (isParent())
    {
        Position tRes = children()->finalise(seg, font, m_position, bbox, attrLevel, clusterMin, rtl, isFinal, depth + 1);
        if ((isBase() || m_advance.x >= 0.5f) && tRes.x > res.x) res = tRes;
    }

    auto sibling = child_iterator(this);
    if (!isBase() && ++sibling != end())
    {
        Position tRes = sibling->finalise(seg, font, base, bbox, attrLevel, clusterMin, rtl, isFinal, depth + 1);
        if (tRes.x > res.x) res = tRes;
    }

    if (isBase() && clusterMin < base.x)
    {
        Position adj = Position(m_position.x - clusterMin, 0.);
        res += adj;
        m_position += adj;
        if (isParent()) children()->floodShift(adj);
    }
    return res;
}

int32_t Slot::clusterMetric(const Segment & seg, metrics metric, uint8_t attrLevel, bool rtl) const
{
    if (glyph() >= seg.getFace()->glyphs().numGlyphs())
        return 0;
    Rect bbox = seg.theGlyphBBoxTemporary(glyph());
    auto & slot = const_cast<Slot &>(*this);
    float range[2] = {0, 0};
    for (auto s = cluster(); s != end(); ++s)
    {
        auto base = slot.update_cluster_metric(seg, rtl, false, range[0], range[1]);
        bbox.widen(seg.getFace()->glyphs().glyphSafe(glyph())->theBBox() + base);
    }
    Position res = {range[1], m_advance.y};
    // Position res = const_cast<Slot *>(this)->finalise(seg, NULL, base, bbox, attrLevel, clusterMin, rtl, false);

    switch (metric)
    {
    case kgmetLsb :
        return int32_t(bbox.bl.x);
    case kgmetRsb :
        return int32_t(res.x - bbox.tr.x);
    case kgmetBbTop :
        return int32_t(bbox.tr.y);
    case kgmetBbBottom :
        return int32_t(bbox.bl.y);
    case kgmetBbLeft :
        return int32_t(bbox.bl.x);
    case kgmetBbRight :
        return int32_t(bbox.tr.x);
    case kgmetBbWidth :
        return int32_t(bbox.tr.x - bbox.bl.x);
    case kgmetBbHeight :
        return int32_t(bbox.tr.y - bbox.bl.y);
    case kgmetAdvWidth :
        return int32_t(res.x);
    case kgmetAdvHeight :
        return int32_t(res.y);
    default :
        return 0;
    }
}

#define SLOTGETCOLATTR(x) { SlotCollision *c = seg.collisionInfo(*this); return c ? int(c-> x) : 0; }

int Slot::getAttr(const Segment & seg, attrCode ind, uint8_t subindex) const
{
    if (ind >= gr_slatJStretch && ind < gr_slatJStretch + 20 && ind != gr_slatJWidth)
    {
        int indx = ind - gr_slatJStretch;
        return getJustify(seg, indx / 5, indx % 5);
    }

    switch (ind)
    {
    case gr_slatAdvX :      return int(m_advance.x);
    case gr_slatAdvY :      return int(m_advance.y);
    case gr_slatAttTo :     return m_parent_offset ? 1 : 0;
    case gr_slatAttX :      return int(m_attach.x);
    case gr_slatAttY :      return int(m_attach.y);
    case gr_slatAttXOff :
    case gr_slatAttYOff :   return 0;
    case gr_slatAttWithX :  return int(m_with.x);
    case gr_slatAttWithY :  return int(m_with.y);
    case gr_slatAttWithXOff:
    case gr_slatAttWithYOff:return 0;
    case gr_slatAttLevel :  return m_attLevel;
    case gr_slatBreak :     return seg.charinfo(m_original)->breakWeight();
    case gr_slatCompRef :   return 0;
    case gr_slatDir :       return seg.dir() & 1;
    case gr_slatInsert :    return insertBefore();
    case gr_slatPosX :      return int(m_position.x); // but need to calculate it
    case gr_slatPosY :      return int(m_position.y);
    case gr_slatShiftX :    return int(m_shift.x);
    case gr_slatShiftY :    return int(m_shift.y);
    case gr_slatMeasureSol: return -1; // err what's this?
    case gr_slatMeasureEol: return -1;
    case gr_slatJWidth:     return int(m_just);
    case gr_slatUserDefnV1: subindex = 0; GR_FALLTHROUGH;
      // no break
    case gr_slatUserDefn :  return subindex < m_attrs.num_attrs() ? m_attrs.user_attributes()[subindex] : 0;
    case gr_slatSegSplit :  return seg.charinfo(m_original)->flags() & 3;
    case gr_slatBidiLevel:  return m_bidiLevel;
    case gr_slatColFlags :		{ SlotCollision *c = seg.collisionInfo(*this); return c ? c->flags() : 0; }
    case gr_slatColLimitblx:SLOTGETCOLATTR(limit().bl.x)
    case gr_slatColLimitbly:SLOTGETCOLATTR(limit().bl.y)
    case gr_slatColLimittrx:SLOTGETCOLATTR(limit().tr.x)
    case gr_slatColLimittry:SLOTGETCOLATTR(limit().tr.y)
    case gr_slatColShiftx :	SLOTGETCOLATTR(offset().x)
    case gr_slatColShifty :	SLOTGETCOLATTR(offset().y)
    case gr_slatColMargin :	SLOTGETCOLATTR(margin())
    case gr_slatColMarginWt:SLOTGETCOLATTR(marginWt())
    case gr_slatColExclGlyph:SLOTGETCOLATTR(exclGlyph())
    case gr_slatColExclOffx:SLOTGETCOLATTR(exclOffset().x)
    case gr_slatColExclOffy:SLOTGETCOLATTR(exclOffset().y)
    case gr_slatSeqClass :	SLOTGETCOLATTR(seqClass())
    case gr_slatSeqProxClass:SLOTGETCOLATTR(seqProxClass())
    case gr_slatSeqOrder :	SLOTGETCOLATTR(seqOrder())
    case gr_slatSeqAboveXoff:SLOTGETCOLATTR(seqAboveXoff())
    case gr_slatSeqAboveWt: SLOTGETCOLATTR(seqAboveWt())
    case gr_slatSeqBelowXlim:SLOTGETCOLATTR(seqBelowXlim())
    case gr_slatSeqBelowWt:	SLOTGETCOLATTR(seqBelowWt())
    case gr_slatSeqValignHt:SLOTGETCOLATTR(seqValignHt())
    case gr_slatSeqValignWt:SLOTGETCOLATTR(seqValignWt())
    default : return 0;
    }
}

#define SLOTCOLSETATTR(x) { \
        SlotCollision *c = seg.collisionInfo(*this); \
        if (c) { c-> x ; c->setFlags(c->flags() & ~SlotCollision::COLL_KNOWN); } \
        break; }
#define SLOTCOLSETCOMPLEXATTR(t, y, x) { \
        SlotCollision *c = seg.collisionInfo(*this); \
        if (c) { \
        const t &s = c-> y; \
        c-> x ; c->setFlags(c->flags() & ~SlotCollision::COLL_KNOWN); } \
        break; }

void Slot::setAttr(Segment & seg, attrCode ind, uint8_t subindex, int16_t value, const ShapingContext & ctxt)
{
    if (ind == gr_slatUserDefnV1)
    {
        ind = gr_slatUserDefn;
        subindex = 0;
        if (seg.numAttrs() == 0)
            return;
    }
    else if (ind >= gr_slatJStretch && ind < gr_slatJStretch + 20 && ind != gr_slatJWidth)
    {
        int indx = ind - gr_slatJStretch;
        return setJustify(seg, indx / 5, indx % 5, value);
    }

    switch (ind)
    {
    case gr_slatAdvX :  m_advance.x = value; break;
    case gr_slatAdvY :  m_advance.y = value; break;
    case gr_slatAttTo :
    {
        const uint16_t idx = uint16_t(value);
        if (idx < ctxt.map.size() && ctxt.map[idx].is_valid())
        {
            auto other = &*ctxt.map[idx];
            if (other == this || other == attachedTo() || other->copied()) break;
            if (!isBase()) { attachedTo()->remove_child(this); }
            auto pOther = other;
            int count = 0;
            bool foundOther = false;
            while (pOther)
            {
                ++count;
                if (pOther == this) foundOther = true;
                pOther = pOther->attachedTo();
            }
            if (count < 100 && !foundOther && other->add_child(this))
            {
                attachTo(other);
                if ((ctxt.dir != 0) ^ (idx > subindex))
                    m_with = Position(advance(), 0);
                else        // normal match to previous root
                    m_attach = Position(other->advance(), 0);
            }
        }
        break;
    }
    case gr_slatAttX :          m_attach.x = value; break;
    case gr_slatAttY :          m_attach.y = value; break;
    case gr_slatAttXOff :
    case gr_slatAttYOff :       break;
    case gr_slatAttWithX :      m_with.x = value; break;
    case gr_slatAttWithY :      m_with.y = value; break;
    case gr_slatAttWithXOff :
    case gr_slatAttWithYOff :   break;
    case gr_slatAttLevel :
        m_attLevel = byte(value);
        break;
    case gr_slatBreak :
        seg.charinfo(m_original)->breakWeight(value);
        break;
    case gr_slatCompRef :   break;      // not sure what to do here
    case gr_slatDir : break;
    case gr_slatInsert :
        insertBefore(value? true : false);
        break;
    case gr_slatPosX :      break; // can't set these here
    case gr_slatPosY :      break;
    case gr_slatShiftX :    m_shift.x = value; break;
    case gr_slatShiftY :    m_shift.y = value; break;
    case gr_slatMeasureSol :    break;
    case gr_slatMeasureEol :    break;
    case gr_slatJWidth :    just(value); break;
    case gr_slatSegSplit :  seg.charinfo(m_original)->addflags(value & 3); break;
    case gr_slatUserDefn :  assert(subindex < m_attrs.num_attrs()); m_attrs.user_attributes()[subindex] = value; break;
    case gr_slatColFlags :  {
        SlotCollision *c = seg.collisionInfo(*this);
        if (c)
            c->setFlags(value);
        break; }
    case gr_slatColLimitblx :	SLOTCOLSETCOMPLEXATTR(Rect, limit(), setLimit(Rect(Position(value, s.bl.y), s.tr)))
    case gr_slatColLimitbly :	SLOTCOLSETCOMPLEXATTR(Rect, limit(), setLimit(Rect(Position(s.bl.x, value), s.tr)))
    case gr_slatColLimittrx :	SLOTCOLSETCOMPLEXATTR(Rect, limit(), setLimit(Rect(s.bl, Position(value, s.tr.y))))
    case gr_slatColLimittry :	SLOTCOLSETCOMPLEXATTR(Rect, limit(), setLimit(Rect(s.bl, Position(s.tr.x, value))))
    case gr_slatColMargin :		SLOTCOLSETATTR(setMargin(value))
    case gr_slatColMarginWt :	SLOTCOLSETATTR(setMarginWt(value))
    case gr_slatColExclGlyph :	SLOTCOLSETATTR(setExclGlyph(value))
    case gr_slatColExclOffx :	SLOTCOLSETCOMPLEXATTR(Position, exclOffset(), setExclOffset(Position(value, s.y)))
    case gr_slatColExclOffy :	SLOTCOLSETCOMPLEXATTR(Position, exclOffset(), setExclOffset(Position(s.x, value)))
    case gr_slatSeqClass :		SLOTCOLSETATTR(setSeqClass(value))
	case gr_slatSeqProxClass :	SLOTCOLSETATTR(setSeqProxClass(value))
    case gr_slatSeqOrder :		SLOTCOLSETATTR(setSeqOrder(value))
    case gr_slatSeqAboveXoff :	SLOTCOLSETATTR(setSeqAboveXoff(value))
    case gr_slatSeqAboveWt :	SLOTCOLSETATTR(setSeqAboveWt(value))
    case gr_slatSeqBelowXlim :	SLOTCOLSETATTR(setSeqBelowXlim(value))
    case gr_slatSeqBelowWt :	SLOTCOLSETATTR(setSeqBelowWt(value))
    case gr_slatSeqValignHt :	SLOTCOLSETATTR(setSeqValignHt(value))
    case gr_slatSeqValignWt :	SLOTCOLSETATTR(setSeqValignWt(value))
    default :
        break;
    }
}

int Slot::getJustify(const Segment & seg, uint8_t level, uint8_t subindex) const
{
    if (level && level >= seg.silf()->numJustLevels()) return 0;

    if (has_justify())
        return m_attrs.just_info()[level * Slot::NUMJUSTPARAMS + subindex];

    if (level >= seg.silf()->numJustLevels()) return 0;
    Justinfo *jAttrs = seg.silf()->justAttrs() + level;

    switch (subindex) {
        case 0 : return seg.glyphAttr(gid(), jAttrs->attrStretch());
        case 1 : return seg.glyphAttr(gid(), jAttrs->attrShrink());
        case 2 : return seg.glyphAttr(gid(), jAttrs->attrStep());
        case 3 : return seg.glyphAttr(gid(), jAttrs->attrWeight());
        case 4 : return 0;      // not been set yet, so clearly 0
        default: return 0;
    }
}

void Slot::setJustify(Segment & seg, uint8_t level, uint8_t subindex, int16_t value)
{
    if (level && level >= seg.silf()->numJustLevels()) return;
    if (!has_justify()) {
        m_attrs.reserve(m_attrs.num_attrs(), std::max(1ul,seg.silf()->numJustLevels()));
        init_just_infos(seg);
    }
    
    m_attrs.just_info()[level * Slot::NUMJUSTPARAMS + subindex] = value;
}

bool Slot::add_child(Slot *ap)
{
    if (this == ap || ap->attachedTo() == this) 
        return false;
    ap->attachTo(this);
    m_flags.children = true;

    for(auto first = min(ap, this)+1, last = max(ap, this)+1; first != last; ++first)
        first->clusterhead(false);

    return true;
}

bool Slot::remove_child(Slot *ap)
{
    if (this == ap || !isParent() || !ap) return false;
    if (ap->m_parent_offset > 0 && ap->m_flags.clusterhead)
    {
        for (auto first = min(ap, this)+1, last = max(ap, this)+1; first != last; ++first)
        {
            if (first->base() != ap)
            {
                first->clusterhead(true);
                break;
            }
        }
    }
    ap->m_parent_offset = 0;
    ap->m_flags.clusterhead = true;
    m_flags.children = !m_flags.clusterhead || (children() != end());
    return true;
}

void Slot::glyph(Segment & seg, uint16_t glyphid, const GlyphFace * theGlyph)
{
    m_glyphid = glyphid;
    m_bidiCls = -1;
    if (!theGlyph)
    {
        theGlyph = seg.getFace()->glyphs().glyphSafe(glyphid);
        if (!theGlyph)
        {
            m_realglyphid = 0;
            m_advance = Position(0.,0.);
            return;
        }
    }
    m_realglyphid = theGlyph->attrs()[seg.silf()->aPseudo()];
    if (m_realglyphid > seg.getFace()->glyphs().numGlyphs())
        m_realglyphid = 0;
    const GlyphFace *aGlyph = theGlyph;
    if (m_realglyphid)
    {
        aGlyph = seg.getFace()->glyphs().glyphSafe(m_realglyphid);
        if (!aGlyph) aGlyph = theGlyph;
    }
    m_advance = Position(aGlyph->theAdvance().x, 0.);
    if (seg.silf()->aPassBits())
    {
        seg.mergePassBits(uint8_t(theGlyph->attrs()[seg.silf()->aPassBits()]));
        if (seg.silf()->numPasses() > 16)
            seg.mergePassBits(theGlyph->attrs()[seg.silf()->aPassBits()+1] << 16);
    }
}


void Slot::floodShift(Position adj, int depth)
{
    if (depth > 100)
        return;
    m_position += adj;
    for (auto c = children(); c != end(); ++c)
        c->floodShift(adj, depth + 1);
}


bool Slot::has_base(const Slot *base) const
{
    for (auto p = attachedTo(); p; p = p->attachedTo())
        if (p == base)
            return true;
    return false;
}
