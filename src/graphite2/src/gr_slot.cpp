// SPDX-License-Identifier: MIT
// Copyright 2010, SIL International, All rights reserved.

#include "graphite2/Segment.h"
#include "inc/Segment.h"
#include "inc/Slot.h"
#include "inc/Font.h"


extern "C" {


const gr_slot* gr_slot_next_in_segment(const gr_slot* h/*not NULL*/)
{
    assert(h);
    graphite2::SlotBuffer::const_iterator p = h;
    return p->index() == -1u ? nullptr : (++p).handle();
}

const gr_slot* gr_slot_prev_in_segment(const gr_slot* h/*not NULL*/)
{
    assert(h);
    graphite2::SlotBuffer::const_iterator p = h;
    return p->index() == 0 ? nullptr : (--p).handle();
}

const gr_slot* gr_slot_attached_to(const gr_slot* h/*not NULL*/)        //returns NULL iff base. If called repeatedly on result, will get to a base
{
    assert(h);
    graphite2::SlotBuffer::const_iterator p = h;   
    auto slot = p->attachedTo();
    return slot ? decltype(p)::from(slot).handle() : nullptr;
}


const gr_slot* gr_slot_first_attachment(const gr_slot* h/*not NULL*/)        //returns NULL iff no attachments.
{        //if slot_first_attachment(p) is not NULL, then slot_attached_to(slot_first_attachment(p))==p.
    assert(h);
    graphite2::SlotBuffer::const_iterator p = h;   
    return p->children().handle();
}


const gr_slot* gr_slot_next_sibling_attachment(const gr_slot* h/*not NULL*/)        //returns NULL iff no more attachments.
{        //if slot_next_sibling_attachment(p) is not NULL, then slot_attached_to(slot_next_sibling_attachment(p))==slot_attached_to(p).
    assert(h);
    graphite2::Slot::const_child_iterator p = h;
    return (++p).handle();
}


unsigned short gr_slot_gid(const gr_slot* h/*not NULL*/)
{
    assert(h);
    graphite2::SlotBuffer::const_iterator p = h;   
    return p->glyph();
}


float gr_slot_origin_X(const gr_slot* h/*not NULL*/)
{
    assert(h);
    graphite2::SlotBuffer::const_iterator p = h;   
    return p->origin().x;
}


float gr_slot_origin_Y(const gr_slot* h/*not NULL*/)
{
    assert(h);
    graphite2::SlotBuffer::const_iterator p = h;   
    return p->origin().y;
}


float gr_slot_advance_X(const gr_slot* h/*not NULL*/, const gr_face *face, const gr_font *font)
{
    assert(h);
    graphite2::SlotBuffer::const_iterator p = h;   
    float scale = 1.0;
    float res = p->advance();
    if (font)
    {
        scale = font->scale();
        int gid = p->glyph();
        if (face && font->isHinted() && gid < face->glyphs().numGlyphs())
            res = (res - face->glyphs().glyph(gid)->theAdvance().x) * scale + font->advance(gid);
        else
            res = res * scale;
    }
    return res;
}

float gr_slot_advance_Y(const gr_slot *h/*not NULL*/, GR_MAYBE_UNUSED const gr_face *face, const gr_font *font)
{
    assert(h);
    graphite2::SlotBuffer::const_iterator p = h;   
    float res = p->advancePos().y;
    if (font)
        return res * font->scale();
    else
        return res;
}

int gr_slot_before(const gr_slot* h/*not NULL*/)
{
    assert(h);
    graphite2::SlotBuffer::const_iterator p = h;   
    return p->before();
}


int gr_slot_after(const gr_slot* h/*not NULL*/)
{
    assert(h);
    graphite2::SlotBuffer::const_iterator p = h;   
    return p->after();
}

unsigned int gr_slot_index(const gr_slot *h/*not NULL*/)
{
    assert(h);
    graphite2::SlotBuffer::const_iterator p = h;
    return p->index();
}

int gr_slot_attr(const gr_slot* h/*not NULL*/, const gr_segment* pSeg/*not NULL*/, gr_attrCode index, uint8_t subindex)
{
    assert(h);
    assert(pSeg);
    graphite2::SlotBuffer::const_iterator p = h;
    return p->getAttr(*pSeg, index, subindex);
}


int gr_slot_can_insert_before(const gr_slot* h/*not NULL*/)
{
    assert(h);
    graphite2::SlotBuffer::const_iterator p = h;
    return (p->insertBefore())? 1 : 0;
}


int gr_slot_original(const gr_slot* h/*not NULL*/)
{
    assert(h);
    graphite2::SlotBuffer::const_iterator p = h;
    return p->original();
}

void gr_slot_linebreak_before(gr_slot* h/*not NULL*/)
{
    assert(h);
    graphite2::SlotBuffer::iterator p = h;
    p->last(true);
}

#if 0       //what should this be
size_t id(const gr_slot* p/*not NULL*/)
{
    return (size_t)p->id();
}
#endif


} // extern "C"
