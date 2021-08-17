// /*  GRAPHITE2 LICENSING

//     Copyright 2010, SIL International
//     All rights reserved.

//     This library is free software; you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published
//     by the Free Software Foundation; either version 2.1 of License, or
//     (at your option) any later version.

//     This program is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//     Lesser General Public License for more details.

//     You should also have received a copy of the GNU Lesser General Public
//     License along with this library in the file named "LICENSE".
//     If not, write to the Free Software Foundation, 51 Franklin Street,
//     Suite 500, Boston, MA 02110-1335, USA or visit their web page on the
//     internet at http://www.fsf.org/licenses/lgpl.html.

// Alternatively, the contents of this file may be used under the terms of the
// Mozilla Public License (http://mozilla.org/MPL) or the GNU General Public
// License, as published by the Free Software Foundation, either version 2
// of the License or (at your option) any later version.
// */
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>

#include "inc/SlotBuffer.h"


using namespace graphite2;

namespace 
{
    // @DEBUG_ATTRS: The number of extra per-slot user attributes beyond the
    // number declared in by the font. Used to store persistent debug
    // information that is not zeroed by SlotBuffer::_free_node(). 
// #if defined(GRAPHITE2_NTRACING)
    constexpr size_t const DEBUG_ATTRS = 0;
// #else
//     constexpr size_t const DEBUG_ATTRS = 1;
// #endif
}

SlotBuffer::SlotBuffer(SlotBuffer && rhs)
: _values{std::move(rhs._values)}
//   _attrs_size{rhs._attrs_size}
{
}

SlotBuffer & SlotBuffer::operator = (SlotBuffer && rhs) {
    _values = std::move(rhs._values);
    return *this;
}

SlotBuffer::iterator SlotBuffer::insert(const_iterator pos, value_type const & slot)
{
    return _values.insert(pos._i, slot);
}

SlotBuffer::iterator SlotBuffer::insert(const_iterator pos, value_type && slot)
{
    return _values.insert(pos._i, std::forward<value_type>(slot));
}

void SlotBuffer::splice(const_iterator pos, SlotBuffer &other, const_iterator first, const_iterator last)
{
    if (first != last)
    {
        auto l = std::distance(first, last);
        auto i = _values.insert(pos._i, l, Slot());
        while (first != last) *i = std::move(*first++);
        other.erase(first, last);
    }
}

auto SlotBuffer::erase(const_iterator pos) -> iterator
{
    return _values.erase(pos._i);
}

auto SlotBuffer::erase(const_iterator first, const_iterator const last) -> iterator
{
    return _values.erase(first._i, last._i);
}

namespace {
    constexpr int8_t BIDI_MARK = 0x10;

    template <class It>
    inline It skip_bidi_mark(It first, It const last) {
        while (first != last && first->bidiClass() == BIDI_MARK) ++first;
        return first;
    }

}

// reverse the clusters, but keep diacritics in their original order w.r.t their base character.
void SlotBuffer::reverse()
{
    assert(!empty());
    _storage out;
    out.reserve(_values.size());

    auto s = skip_bidi_mark(cbegin(), cend());
    if (s == cend()) return;

    while (s != cend())
    {
        auto const c = s;
        s = skip_bidi_mark(++s, cend());
        out.insert(out.cbegin(), c, s);
    }
    out.emplace_back(Slot(Slot::sentinal()));
    _values = std::move(out);
}

