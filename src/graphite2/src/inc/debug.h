// SPDX-License-Identifier: MIT
// Copyright 2011, SIL International, All rights reserved.

//  debug.h
//
//  Created on: 22 Dec 2011
//      Author: tim

#pragma once

#if !defined GRAPHITE2_NTRACING

#include <utility>
#include "inc/json.h"
#include "inc/Position.h"
#include "inc/SlotBuffer.h"

namespace graphite2
{

class CharInfo;
class Segment;
class Slot;

typedef std::pair<Segment const * const, Slot const * const>    dslot;
struct objectid
{
    char name[16];
    objectid(SlotBuffer::const_iterator const s) noexcept;
    objectid(Segment const & seg)  noexcept { set_name(&seg, 0); }
private:
    void set_name(void const * addr, uint16_t generation) noexcept;
};

inline
objectid::objectid(SlotBuffer::const_iterator const s) noexcept
{
    void const * o = s.handle() ? reinterpret_cast<void const *>(s->original()+1) : nullptr;
    set_name(0, o ? s->generation() : 0);
}


json & operator << (json & j, const Position &) throw();
json & operator << (json & j, const Rect &) throw();
json & operator << (json & j, const CharInfo &) throw();
json & operator << (json & j, const dslot &) throw();
json & operator << (json & j, const objectid &) throw();
json & operator << (json & j, const telemetry &) throw();

inline
json & operator << (json & j, const Position & p) throw()
{
    return j << json::flat << json::array << p.x << p.y << json::close;
}


inline
json & operator << (json & j, const Rect & p) throw()
{
    return j << json::flat << json::array << p.bl.x << p.bl.y << p.tr.x << p.tr.y << json::close;
}


inline
json & operator << (json & j, const objectid & sid) throw()
{
    return j << sid.name;
}


} // namespace graphite2

#endif //!defined GRAPHITE2_NTRACING

