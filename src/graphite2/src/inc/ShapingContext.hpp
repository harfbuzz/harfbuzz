/*  GRAPHITE2 LICENSING

    Copyright 2020, SIL International
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
#pragma once
#include <cstddef>
#include "inc/SlotBuffer.h"
#include "inc/vector.hpp"

namespace graphite2 {

class Segment;
class json;

class ShapingContext 
{
public:
    static constexpr size_t MAX_SLOTS=64;
 
    using slotref = SlotBuffer::iterator;
    using const_slotref = SlotBuffer::iterator;
    using map_t = vector<const_slotref>;


    ShapingContext(Segment & seg, uint8_t direction, size_t maxSize);

    size_t      context() const;
    void        reset(SlotBuffer::iterator & slot, unsigned short max_pre_context);

    void        pushSlot(slotref slot);
    void        collectGarbage(slotref & aSlot);

    const_slotref highwater() const;
    void        highwater(const_slotref s);
    bool        highpassed() const;
    void        highpassed(bool v);

    int         decMax();

    Segment   & segment;
    json      * const dbgout;
    map_t       map;
    SlotBuffer  in;
    uint8_t const dir;

private:
    const_slotref   _highwater;
    int             _max_size;
    unsigned short  _precontext;
    bool            _highpassed;
};


inline
size_t ShapingContext::context() const {
  return _precontext;
}

inline
auto ShapingContext::highwater() const -> const_slotref {
    return _highwater; 
}

inline
void ShapingContext::highwater(const_slotref s) {
    _highwater = s; _highpassed = false; 
}

inline
bool ShapingContext::highpassed() const {
    return _highpassed; 
}

inline
void ShapingContext::highpassed(bool v) {
    _highpassed = v; 
}

inline
int ShapingContext::decMax() {
    return --_max_size;
}

inline
void ShapingContext::pushSlot(slotref slot)
{
  map.push_back(slot);
}


} // namespace graphite2
