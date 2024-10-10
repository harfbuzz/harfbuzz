/*  GRAPHITE2 LICENSING

    Copyright 2012, SIL International
    All rights reserved.

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should also have received a copy of the GNU Lesser General Public
    License along with this library in the file named "LICENSE".
    If not, write to the Free Software Foundation, 51 Franklin Street,
    Suite 500, Boston, MA 02110-1335, USA or visit their web page on the
    internet at http://www.fsf.org/licenses/lgpl.html.

Alternatively, the contents of this file may be used under the terms of the
Mozilla Public License (http://mozilla.org/MPL) or the GNU General Public
License, as published by the Free Software Foundation, either version 2
of the License or (at your option) any later version.
*/

#include "inc/Segment.h"
#include "graphite2/Font.h"
#include "inc/debug.h"
#include "inc/CharInfo.h"
#include "inc/Font.h"
#include "inc/Slot.h"
#include "inc/Main.h"
#include <cmath>

using namespace graphite2;

template<typename Callable>
typename std::result_of<Callable()>::type
Segment::subsegment(SlotBuffer::const_iterator first, SlotBuffer::const_iterator last, Callable body)
{
    // SlotBuffer sub_slots/*(m_srope.num_user_attrs(), m_srope.num_just_levels())*/;
    // sub_slots.splice(sub_slots.end(), m_srope, first, last);
    // m_srope.swap(sub_slots);

    auto retval = body();

    // m_srope.swap(sub_slots);
    // m_srope.splice(last, sub_slots);

    return retval;
}

class JustifyTotal {
public:
    JustifyTotal() : m_numGlyphs(0), m_tStretch(0), m_tShrink(0), m_tStep(0), m_tWeight(0) {}
    void accumulate(Slot const &s, Segment const &seg, int level);
    int weight() const { return m_tWeight; }

    CLASS_NEW_DELETE

private:
    int m_numGlyphs;
    int m_tStretch;
    int m_tShrink;
    int m_tStep;
    int m_tWeight;
};

void JustifyTotal::accumulate(Slot const &s, Segment const &seg, int level)
{
    ++m_numGlyphs;
    m_tStretch += s.getJustify(seg, level, 0);
    m_tShrink  += s.getJustify(seg, level, 1);
    m_tStep    += s.getJustify(seg, level, 2);
    m_tWeight  += s.getJustify(seg, level, 3);
}

float Segment::justify(SlotBuffer::iterator pSlot, const Font *font, float width, 
                       GR_MAYBE_UNUSED justFlags jflags, 
                       SlotBuffer::iterator pFirst, 
                       SlotBuffer::iterator pLast)
{
    float currWidth = 0.0;
    const float scale = font ? font->scale() : 1.0f;
    Position res;

    if (width < 0 && !(silf()->flags()))
        return width;
    --pLast;
    if ((m_dir & 1) != m_silf->dir() && m_silf->bidiPass() != m_silf->numPasses())
    {
        reverseSlots();
        std::swap(pFirst, pLast);
    }
    pFirst.to_base();
    pLast.to_base();
    const float base = pFirst->origin().x / scale;
    width = width / scale;
    if ((jflags & gr_justEndInline) == 0)
    {
        while (pLast != pFirst && pLast != slots().end())
        {
            Rect bbox = theGlyphBBoxTemporary(pLast->glyph());
            if (bbox.bl.x != 0.f || bbox.bl.y != 0.f || bbox.tr.x != 0.f || bbox.tr.y == 0.f)
                break;
            --pLast;
        }
    }
    
    int icount = 0;
    int numLevels = silf()->numJustLevels();
    if (!numLevels)
    {
        for (auto s = pSlot->children(); s != pSlot->end(); ++s)
        {
            CharInfo *c = charinfo(s->before());
            if (isWhitespace(c->unicodeChar()))
            {
                s->setJustify(*this, 0, 3, 1);
                s->setJustify(*this, 0, 2, 1);
                s->setJustify(*this, 0, 0, -1);
                ++icount;
            }
        }
        if (!icount)
        {
            for (auto s = pSlot->children(); s != pSlot->end(); ++s)
            {
                s->setJustify(*this, 0, 3, 1);
                s->setJustify(*this, 0, 2, 1);
                s->setJustify(*this, 0, 0, -1);
            }
        }
        ++numLevels;
    }

    vector<JustifyTotal> stats(numLevels);
    for (auto s = pFirst->children(); s != pFirst->end(); ++s)
    {
        float w = s->origin().x / scale + s->advance() - base;
        if (w > currWidth) currWidth = w;
        for (int j = 0; j < numLevels; ++j)
            stats[j].accumulate(*s, *this, j);
        s->just(0);
    }

    for (int i = (width < 0.0f) ? -1 : numLevels - 1; i >= 0; --i)
    {
        float diff;
        float error = 0.;
        float diffpw;
        int tWeight = stats[i].weight();
        if (tWeight == 0) continue;

        do {
            error = 0.;
            diff = width - currWidth;
            diffpw = diff / tWeight;
            tWeight = 0;
            for (auto s = pFirst->children(); s != pFirst->end(); ++s) // don't include final glyph
            {
                int w = s->getJustify(*this, i, 3);
                float pref = diffpw * w + error;
                int step = s->getJustify(*this, i, 2);
                if (!step) step = 1;        // handle lazy font developers
                if (pref > 0)
                {
                    float max = uint16_t(s->getJustify(*this, i, 0));
                    if (i == 0) max -= s->just();
                    if (pref > max) pref = max;
                    else tWeight += w;
                }
                else
                {
                    float max = uint16_t(s->getJustify(*this, i, 1));
                    if (i == 0) max += s->just();
                    if (-pref > max) pref = -max;
                    else tWeight += w;
                }
                int actual = int(pref / step) * step;

                if (actual)
                {
                    error += diffpw * w - actual;
                    if (i == 0)
                        s->just(s->just() + actual);
                    else
                        s->setJustify(*this, i, 4, actual);
                }
            }
            currWidth += diff - error;
        } while (i == 0 && int(std::abs(error)) > 0 && tWeight);
    }

    // Temporarily swap rope buffer.
    auto rv = subsegment(pSlot, std::next(pLast), [&]()->float
    {
        if (silf()->flags() & 1)
        {
            if (addLineEnd(slots().begin()) == slots().end()
                || addLineEnd(slots().end()) == slots().end())
                return -1.0;
        }

        // run justification passes here
    #if !defined GRAPHITE2_NTRACING
        json * const dbgout = m_face->logger();
        if (dbgout)
            *dbgout << json::object
                        << "justifies"  << objectid(*this)
                        << "passes"     << json::array;
    #endif

        if (m_silf->justificationPass() != m_silf->positionPass() && (width >= 0.f || (silf()->flags() & 1)))
            m_silf->runGraphite(*this, m_silf->justificationPass(), m_silf->positionPass());

    #if !defined GRAPHITE2_NTRACING
        // FIX ME if (dbgout)
        // {
        //     *dbgout     << json::item << json::close; // Close up the passes array
        //     positionSlots(nullptr, pSlot, std::next(pLast), m_dir);
        //     auto lEnd = end ? decltype(slots().cend())::from(end) : slots().cend();
        //     *dbgout << "output" << json::array;
        //     for(auto t = pSlot; t != lEnd; ++t)
        //         *dbgout     << dslot(this, &*t);
        //     *dbgout         << json::close << json::close;
        // }
    #endif

        return positionSlots(font, pSlot, std::next(pLast), m_dir).x;
    });

    if ((m_dir & 1) != m_silf->dir() && m_silf->bidiPass() != m_silf->numPasses())
        reverseSlots();
    return rv;
}

SlotBuffer::iterator Segment::addLineEnd(SlotBuffer::iterator pos)
{
    auto eSlot = slots().insert(pos, Slot(numAttrs()));
    if (eSlot == slots().end()) return eSlot;

    const uint16_t gid = silf()->endLineGlyphid();
    const GlyphFace * theGlyph = m_face->glyphs().glyphSafe(gid);
    eSlot->glyph(*this, gid, theGlyph);

    if (pos != slots().end())
    {
        eSlot->before(pos->before());
        if (eSlot != slots().begin())
            eSlot->after(std::prev(eSlot)->after());
        else
            eSlot->after(pos->before());
    }
    else
    {
        pos = --slots().end();
        eSlot->after(std::prev(eSlot)->after());
        eSlot->before(pos->after());
    }

    return eSlot;
}

void Segment::delLineEnd(SlotBuffer::iterator s)
{
    slots().erase(s);
}
