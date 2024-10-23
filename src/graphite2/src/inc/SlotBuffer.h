/*  GRAPHITE2 LICENSING

    Copyright 2019, SIL International
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

#include <iterator>
#include "inc/vector.hpp"

#include "inc/Slot.h"


namespace graphite2 {

class SlotBuffer
{
    using _storage = vector<Slot>;
public:
    using value_type = _storage::value_type;
    using difference_type = _storage::difference_type;
    using size_type = _storage::size_type;
    using reference = _storage::reference;
    using const_reference = _storage::const_reference;
    using pointer = _storage::pointer;

private:
    _storage            _values;

    template<typename T> class _iterator;

    SlotBuffer(SlotBuffer && rhs);
    SlotBuffer & operator=(SlotBuffer && rhs);

    CLASS_NEW_DELETE;

public:
    using iterator = _iterator<value_type>;
    using const_iterator = _iterator<const value_type>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;


    // TODO: Retire
    iterator newSlot();
    void     freeSlot(iterator i);

    SlotBuffer() : _values{Slot(Slot::sentinal())} {}
    ~SlotBuffer()   { _values.clear(); }

    void reserve(size_t size) { _values.reserve(size+1); }
    
    // size_type       num_user_attrs() const noexcept { return _attrs_size; }
    // size_type       num_just_levels() const noexcept { return _justs_size; }

    iterator        begin() noexcept;
    const_iterator  begin() const noexcept;
    const_iterator  cbegin() const noexcept;

    iterator        end() noexcept;
    const_iterator  end() const noexcept;
    const_iterator  cend() const noexcept;

    // iterator        rbegin()         { return reverse_iterator(_head._next); }
    // const_iterator  rbegin() const   { return cbegin(); }
    // const_iterator  crbegin() const  { return const_reverse_iterator(_head._next); }
    // iterator        rend()           { return reverse_iterator(&_head); }
    // const_iterator  rend() const     { return cend(); }
    // const_iterator  crend() const    { return const_reverse_iterator(&_head); }

    reference       front()         { assert(!empty()); return _values.front(); }
    const_reference front() const   { assert(!empty()); return _values.front(); }
    reference       back()          { assert(!empty()); return _values[_values.size()-2]; }
    const_reference back() const    { assert(!empty()); return _values[_values.size()-2]; }

    bool empty() const noexcept { return size() == 0; }
    size_t size() const noexcept { return _values.size()-1; }

    iterator insert(const_iterator pos, value_type const &);
    iterator insert(const_iterator pos, value_type &&);
    template <typename... Args>
    iterator emplace(const_iterator pos, Args &&... args);

    void push_back(value_type const &v);
    void push_back(value_type &&v);

    template <typename... Args>
    reference emplace_back(Args &&... args) { emplace(end(), std::forward<Args>(args)...); return back(); }

    void pop_back();

    iterator erase(const_iterator first);
    iterator erase(const_iterator first, const_iterator last);

    void clear();

    void reverse();

    void swap(SlotBuffer & rhs);

    void splice(const_iterator pos, SlotBuffer & other, const_iterator first, const_iterator last);
    void splice(const_iterator pos, SlotBuffer & other);  
    void splice(const_iterator pos, SlotBuffer & other, const_iterator it);
};

template <typename T>
class SlotBuffer::_iterator
{
    SlotBuffer::_storage::iterator _i;

    friend SlotBuffer;

    constexpr _iterator(std::nullptr_t)  noexcept: _i{nullptr} {}
    _iterator(_storage::iterator i) : _i{i} {}
    // operator _storage::const_iterator() const noexcept { return _i; }

public:
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = ptrdiff_t;
    using value_type = T;
    using pointer = value_type*;
    using reference = value_type&;
    using opaque_type = gr_slot const *;

    static _iterator<T> from(T * r) noexcept {  
        return _iterator<T>(r);
    }

    void to_base() noexcept {
        _i = _i->base();
    }

    _iterator(): _iterator{nullptr} {}
    _iterator(_storage::const_iterator i) : _i{const_cast<_storage::iterator>(i)} {}
    _iterator(opaque_type p): _iterator{reinterpret_cast<T *>(const_cast<gr_slot*>(p))} {}

    bool operator==(_iterator<T const> const & rhs) const noexcept { return _i == rhs._i; }
    bool operator!=(_iterator<T const> const & rhs) const noexcept { return !operator==(rhs); }

    reference operator*() const noexcept { return *_i; }
    pointer operator->() const  noexcept { return &operator*(); }

    _iterator<T> &  operator++()     noexcept { assert(_i); ++_i; return *this;}
    _iterator<T>    operator++(int)  noexcept { _iterator<T> tmp(*this); operator++(); return tmp; }

    _iterator<T> &  operator--() noexcept    { assert(_i); --_i; return *this; }
    _iterator<T>    operator--(int) noexcept { _iterator<T> tmp(*this); operator--(); return tmp; }

    // difference_type operator - (_iterator<T const> rhs) const noexcept { return std::distance(*this._i, rhs._i); }

    bool        is_valid() const noexcept { return _i != nullptr; }

    opaque_type handle() const noexcept {
        if (_i == nullptr || (_i->last() && _i->gid() == 0xffff && _i->deleted()))
            return nullptr;
        return reinterpret_cast<opaque_type>(_i);
    }
    
    // operator bool() const         noexcept { return _p != nullptr; }
    // operator pointer() const      noexcept { return operator->(); }
    operator _iterator<T const> const &() const noexcept { return *reinterpret_cast<_iterator<T const> const *>(this); }
};

template <typename... Args>
inline
auto SlotBuffer::emplace(const_iterator pos, Args &&... args) -> iterator {
    return _values.emplace(pos._i, std::forward<Args>(args)...);
}

inline
void SlotBuffer::swap(SlotBuffer & rhs) {
    SlotBuffer tmp{std::move(rhs)};
    rhs = std::move(*this);
    *this = std::move(tmp);
}

inline 
auto SlotBuffer::newSlot() -> iterator { 
    return iterator::from(new Slot()); 
}

inline 
void SlotBuffer::freeSlot(iterator i) { 
    delete i._i; 
}

inline 
auto SlotBuffer::begin() noexcept -> iterator {
     return _values.begin(); 
}
inline auto SlotBuffer::begin() const noexcept -> const_iterator { return cbegin(); }

inline 
auto SlotBuffer::cbegin() const noexcept -> const_iterator {
     return _values.cbegin(); 
}


inline 
auto SlotBuffer::end() noexcept -> iterator {
     return _values.end()-1; 
}
inline auto SlotBuffer::end() const noexcept -> const_iterator { return cend(); }

inline 
auto SlotBuffer::cend() const noexcept -> const_iterator {
     return _values.cend()-1; 
}


inline void SlotBuffer::pop_back() { erase(end()); }
inline void SlotBuffer::clear() { erase(begin(), end()); }


inline 
void SlotBuffer::splice(const_iterator pos, SlotBuffer & other) {
     splice(pos, other, other.begin(), other.end()); 
} 

inline 
void SlotBuffer::splice(const_iterator pos, SlotBuffer & other, const_iterator it) {
     splice(pos, other, it, std::next(it)); 
} 

} // namespace graphite2
