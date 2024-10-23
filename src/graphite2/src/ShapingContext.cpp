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

#include "inc/Segment.h"
#include "inc/ShapingContext.hpp"

using namespace graphite2;

ShapingContext::ShapingContext(Segment & seg, uint8_t direction, size_t maxSize)
: segment(seg),
  dbgout(seg.getFace() ? seg.getFace()->logger() : nullptr),
  //in(seg.slots().num_user_attrs(), seg.slots().num_just_levels()),
  dir(direction),
  _max_size(int(maxSize)),
  _precontext(0),
  _highpassed(false)
{
    map.reserve(MAX_SLOTS);
}

void ShapingContext::reset(SlotBuffer::iterator & slot, short unsigned int max_pre_ctxt)
{
    int pre_ctxt = 0;
    for (auto const end = segment.slots().begin(); pre_ctxt != max_pre_ctxt && slot != end; ++pre_ctxt, --slot);
    _precontext = pre_ctxt; 
    map.clear();
    in.clear();
}

void ShapingContext::collectGarbage(slotref &)
{
    auto const end = map.begin()-1;
    auto si = map.end()-1;
    for(;si != end; --si) {
        auto slot = *si;
        if (slot != segment.slots().end() 
        &&  slot->copied()) 
            segment.freeSlot(slot);
    }
}
