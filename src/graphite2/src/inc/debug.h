/*  GRAPHITE2 LICENSING

    Copyright 2011, SIL International
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

