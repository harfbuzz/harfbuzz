// SPDX-License-Identifier: MIT
// Copyright 2010, SIL International, All rights reserved.

#include "inc/UtfCodec.h"
#include <cstring>
#include <cstdlib>

#include "inc/bits.h"
#include "inc/Segment.h"
#include "graphite2/Font.h"
#include "inc/CharInfo.h"
#include "inc/debug.h"
#include "inc/Font.h"
#include "inc/Slot.h"
#include "inc/Main.h"
#include "inc/CmapCache.h"
#include "inc/Collider.h"
#include "graphite2/Segment.h"


using namespace graphite2;

Segment::Segment(size_t numchars, const Face* face, uint32_t script, int textDir)
: //m_srope(face->chooseSilf(script)->numUser(), face->chooseSilf(script)->numJustLevels()),
  m_charinfo(new CharInfo[numchars]),
  m_collisions(NULL),
  m_face(face),
  m_silf(face->chooseSilf(script)),
  m_bufSize(numchars + 10),
  m_numGlyphs(numchars),
  m_numCharinfo(numchars),
  m_defaultOriginal(0),
  m_dir(textDir),
  m_flags(((m_silf->flags() & 0x20) != 0) << 1),
  m_passBits(m_silf->aPassBits() ? -1 : 0)
{
    m_bufSize = log_binary(numchars)+1;
}

Segment::~Segment()
{
    delete[] m_charinfo;
    free(m_collisions);
}

void Segment::appendSlot(int id, int cid, int gid, int iFeats, size_t coffset)
{
    auto const glyph = m_face->glyphs().glyphSafe(gid);
    
    m_charinfo[id].init(cid);
    m_charinfo[id].feats(iFeats);
    m_charinfo[id].base(coffset);
    m_charinfo[id].breakWeight(glyph ? glyph->attrs()[m_silf->aBreak()] : 0);

    auto & aSlot = slots().emplace_back(numAttrs());
    aSlot.glyph(*this, gid, glyph);
    aSlot.original(id);
    aSlot.before(id);
    aSlot.after(id);
    aSlot.generation() = slots().size();
    aSlot.clusterhead(true);

    if (glyph && m_silf->aPassBits())
        m_passBits &= glyph->attrs()[m_silf->aPassBits()]
                    | (m_silf->numPasses() > 16 ? (glyph->attrs()[m_silf->aPassBits() + 1] << 16) : 0);
}

SlotBuffer::iterator Segment::newSlot()
{
    if (m_numGlyphs > m_numCharinfo * MAX_SEG_GROWTH_FACTOR)
        return m_srope.end();
    
    return m_srope.newSlot();
}

void Segment::freeSlot(SlotBuffer::iterator i)
{
    m_srope.freeSlot(i);
}

// reverse the slots but keep diacritics in their same position after their bases
void Segment::reverseSlots()
{
    m_dir = m_dir ^ 64;                 // invert the reverse flag
    if (slots().empty()) return;      // skip 0 or 1 glyph runs

    // Ensure any unkown bidi classes are set for the reversal algorithm.
    for (auto & slot: m_srope) {
        if (slot.bidiClass() == -1)
            slot.bidiClass(int8_t(glyphAttr(slot.gid(), m_silf->aBidi())));
    }

    m_srope.reverse();
}

void Segment::linkClusters()
{
#if 0
    if (slots().empty())  return;

    auto ls = &slots().front();
    if (m_dir & 1)
    {
        for (auto &&s: slots())
        {
            if (!s.isBase()) continue;

            s.sibling(ls);
            ls = &s;
        }
    }
    else
    {
        for (auto && s: slots())
        {
            if (!s.isBase()) continue;

            ls->sibling(&s);
            ls = &s;
        }
    }
#endif
}

Position Segment::positionSlots(Font const * font, SlotBuffer::iterator first, SlotBuffer::iterator last, bool isRtl, bool isFinal)
{
    assert(first.is_valid() || first == slots().end());
    if (slots().empty())   // only true for empty segments
        return Position(0.,0.);

    bool reorder = (currdir() != isRtl);

    if (reorder)
    {
        reverseSlots();
        --last; std::swap(first, last); ++last; 
    }

    // Reset all the positions.
    for (auto slot = first; slot != last; ++slot)
        slot->origin() = Position();

    // Phase one collect ranges.
    for (auto slot = first; slot != last; ++slot)
    {
        auto const base = slot->base();
        auto & clsb = const_cast<float &>(base->origin().x);
        auto & crsb = const_cast<float &>(base->origin().y);
        slot->update_cluster_metric(*this, isRtl, isFinal, clsb, crsb);
    }


    // Position each cluster either.
    Position offset = {0.f, 0.f};
    if (isRtl)
    {
#if 0
        Slot * base = nullptr;
        float clsb = 0.f, crsb = 0.f;
        auto cluster = --last;
        for (auto slot = last, end=--first; slot != end; --slot)
        {
            slot->reset_origin();
            auto const slot_base = slot->base();
            if (base !=  slot_base)
            {
                offset.x += -clsb;
                for (auto s = cluster; s != slot; --s)
                {
                    s->origin(offset + s->origin());
                    if (s->origin().x < 0) 
                    {
                        offset.x += -s->origin().x;
                        s->position_shift({-s->origin().x, 0.f});
                    }
                }
                offset.x += crsb;
                base = slot_base;
                cluster = slot;
                clsb = std::numeric_limits<float>::infinity();
                crsb = 0;
            }
            slot->update_cluster_metric(*this, isRtl, isFinal, clsb, crsb);
        }
        offset.x += -clsb;
        for (auto s = cluster; s != first; --s)
            s->position_shift(offset);
        offset.x += crsb;
        ++last; ++first;
#else
        // For the first visual cluster ensure initial x positions are
        //  never negative.
        float clsb = 0.f;
        for (auto slot = --last, end=--first; slot != end; --slot)
        {
            if (slot->isBase())
                clsb = slot->origin().x;
            if (-slot->origin().x > offset.x)
                offset.x = -slot->origin().x;
            if (slot->clusterhead()) break;
        }
        offset.x += clsb;

        // Adjust all cluster bases
        for (auto slot = last, end=first; slot != end; --slot)
        {
            if (!slot->isBase()) continue;

            auto const clsb = slot->origin().x;
            auto const crsb = slot->origin().y;
            auto const shifts = slot->collision_shift(*this);
            offset.x += -clsb;
            // Subtract the slot shift as this is RtL.
            slot->origin() = offset + shifts - slot->shift();
            offset.x += crsb + shifts.x - slot->shift().x;
        }
        ++last; ++first;
#endif
    }
    else
    {
        // For the first visual cluster ensure initial x positions are
        //  never negative.
        for (auto slot = first; slot != last; ++slot)
        {
            if (-slot->origin().x > offset.x)
                offset.x = -slot->origin().x;
            if (slot->clusterhead()) break;
        }

        // Adjust all cluster bases
        for (auto slot = first; slot != last; ++slot)
        {
            if (!slot->isBase()) continue;

            auto const clsb = slot->origin().x;
            auto const crsb = slot->origin().y;
            auto const shifts = slot->collision_shift(*this);
            offset.x += -clsb;
            slot->origin() = offset + shifts + slot->shift();
            offset.x += crsb + shifts.x;
        }
    }

    // Shift all attached slots
    for (auto slot = first; slot != last; ++slot)
    {
        if (slot->isBase()) continue;
        auto base = slot->base();
        slot->position_shift({base->origin().x, 0});
    }

    if (font && font->scale() != 1)
    {
        auto const scale = font->scale();
        for (auto slot = first; slot != last; ++slot)
            slot->scale_by(scale);
        offset *= scale;
    }

    if (reorder)
        reverseSlots();

    return offset;
}


void Segment::associateChars(int offset, size_t numChars)
{
    int i = 0, j = 0;
    CharInfo *c, *cend;
    for (c = m_charinfo + offset, cend = m_charinfo + offset + numChars; c != cend; ++c)
    {
        c->before(-1);
        c->after(-1);
    }
    for (auto & s: slots())
    {
        j = s.before();
        if (j >= 0)  {
            for (const int after = s.after(); j <= after; ++j)
            {
                c = charinfo(j);
                if (c->before() == -1 || i < c->before())   c->before(i);
                if (c->after() < i)                         c->after(i);
            }
        }
        s.index(i++);
    }
    for (auto & s: slots())
    {
        int a;
        for (a = s.after() + 1; a < offset + int(numChars) && charinfo(a)->after() < 0; ++a)
            charinfo(a)->after(s.index());
        s.after(--a);

        for (a = s.before() - 1; a >= offset && charinfo(a)->before() < 0; --a)
            charinfo(a)->before(s.index());
        s.before(++a);
    }
}


template <typename utf_iter>
inline void process_utf_data(Segment & seg, const Face & face, const int fid, utf_iter c, size_t n_chars)
{
    const Cmap    & cmap = face.cmap();
    int slotid = 0;

    const typename utf_iter::codeunit_type * const base = c;
    for (; n_chars; --n_chars, ++c, ++slotid)
    {
        const uint32_t usv = *c;
        uint16_t gid = cmap[usv];
        if (!gid)   gid = face.findPseudo(usv);
        seg.appendSlot(slotid, usv, gid, fid, c - base);
    }
}


bool Segment::read_text(const Face *face, const Features* pFeats/*must not be NULL*/, gr_encform enc, const void* pStart, size_t nChars)
{
    assert(face);
    assert(pFeats);
    if (!m_charinfo) return false;

    // utf iterator is self recovering so we don't care about the error state of the iterator.
    switch (enc)
    {
    case gr_utf8:   process_utf_data(*this, *face, addFeatures(*pFeats), utf8::const_iterator(pStart), nChars); break;
    case gr_utf16:  process_utf_data(*this, *face, addFeatures(*pFeats), utf16::const_iterator(pStart), nChars); break;
    case gr_utf32:  process_utf_data(*this, *face, addFeatures(*pFeats), utf32::const_iterator(pStart), nChars); break;
    }
    return true;
}

void Segment::doMirror(uint16_t aMirror)
{
    for (auto & s: slots())
    {
        unsigned short g = glyphAttr(s.gid(), aMirror);
        if (g && (!(dir() & 4) || !glyphAttr(s.gid(), aMirror + 1)))
            s.glyph(*this, g);
    }
}

bool Segment::initCollisions()
{
    m_collisions = grzeroalloc<SlotCollision>(slotCount());
    if (!m_collisions) return false;

    for (auto & p: slots())
        if (p.index() < slotCount())
            ::new (collisionInfo(p)) SlotCollision(*this, p);
        else
            return false;
    return true;
}
