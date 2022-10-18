// SPDX-License-Identifier: MIT
// Copyright 2010, SIL International, All rights reserved.


// designed to have a limited subset of the std::vector api
#pragma once

#include <cstddef>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <iterator>
#include <new>

#include "Main.h"

namespace graphite2 {

// template <typename T>
// inline
// ptrdiff_t distance(T* first, T* last) { return last-first; }


template <typename T>
class vector
{
    T * m_first, *m_last, *m_end;
public:
    using value_type = T;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using reference = T&;
    using const_reference = T const &;
    using pointer = T*;
    using const_pointer = T const *;
    using iterator = T*;
    using const_iterator = T const *;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    constexpr vector() : m_first{nullptr}, m_last{nullptr}, m_end{nullptr} {}
    vector(size_type n, const value_type& value) : vector<T>() { insert(begin(), n, value); }
    explicit vector(size_type n) : vector<T>(n, T()) {}
    template<class It>
    vector(It first, const It last) : vector<T>() { insert(begin(), first, last); }
    vector(const vector<T> &rhs) : vector<T>(rhs.begin(), rhs.end()) { }
    vector(vector<T> &&rhs) : m_first{rhs.m_first}, m_last{rhs.m_last}, m_end{rhs.m_end} { rhs.m_first = rhs.m_last = nullptr; }
    vector(std::initializer_list<T> ilist) : vector{ilist.begin(), ilist.end()} {}
    ~vector() { clear(); free(m_first); }

    iterator            begin()         { return m_first; }
    const_iterator      cbegin() const  { return m_first; }
    const_iterator      begin() const   { return m_first; }

    iterator            end()           { return m_last; }
    const_iterator      cend() const    { return m_last; }          
    const_iterator      end() const     { return m_last; }

    bool                empty() const   { return m_first == m_last; }
    size_type           size() const    { return m_last - m_first; }
    size_type           capacity() const{ return m_end - m_first; }

    void                reserve(size_type n);
    void                resize(size_type n, value_type const & v = value_type());

    reference           front()         { assert(size() > 0); return *begin(); }
    const_reference     front() const   { assert(size() > 0); return *begin(); }
    reference           back()          { assert(size() > 0); return *(end()-1); }
    const_reference     back() const    { assert(size() > 0); return *(end()-1); }

    vector<T>         & operator = (const vector<T> & rhs) { if (&rhs != this) assign(rhs.begin(), rhs.end()); return *this; }
    vector<T>         & operator = (vector<T> && rhs);
    reference           operator [] (size_type n)          { assert(size() > n); return m_first[n]; }
    const_reference     operator [] (size_type n) const    { assert(size() > n); return m_first[n]; }

    void                assign(size_type n, const value_type& u)    { clear(); insert(begin(), n, u); }
    template<class It>
    void                assign(It first, It last)      { clear(); insert(begin(), first, last); }
    void                assign(std::initializer_list<T> ilist)  { assign(ilist.begin(), ilist.end()); }
    iterator            insert(const_iterator p, const value_type & x) { auto n = _insert_default(p, 1); new (n) value_type(x); return n; }
    iterator            insert(const_iterator p, value_type && x) { auto n = _insert_default(p, 1); new (n) value_type(std::move(x)); return n; }
    iterator            insert(const_iterator p, size_type n, const T & x);
    template<class It>
    iterator            insert(const_iterator p, It first, It last);
    void                pop_back()              { assert(size() > 0); --m_last; }
    void                push_back(const value_type &v)   { if (m_last == m_end) reserve(size()+1); new (m_last++) value_type(v); }
    void                push_back(value_type && v)       { if (m_last == m_end) reserve(size()+1); new (m_last++) value_type(std::move(v)); }

    template<typename... Args>
    iterator            emplace(const_iterator p, Args &&... args) { auto n = _insert_default(p, 1); new (n) value_type(std::forward<Args>(args)...); return n; }
    template<typename... Args>
    reference           emplace_back(Args &&... args) { if (m_last == m_end) reserve(size()+1); return *new (m_last++) value_type(std::forward<Args>(args)...); }

    void                clear()                 { erase(begin(), end()); }
    iterator            erase(const_iterator p)       { return erase(p, std::next(p)); }
    iterator            erase(const_iterator first, const_iterator last);

private:
    iterator            _insert_default(const_iterator p, size_type n);
};

template <typename T>
inline
void vector<T>::reserve(size_type n)
{
    if (n > capacity())
    {
        const ptrdiff_t sz = size();
        size_t requested;
        if (checked_mul(n, sizeof(value_type), requested))  std::abort();
        m_first = static_cast<value_type*>(realloc(m_first, requested));
        if (!m_first)   std::abort();
        m_last  = m_first + sz;
        m_end   = m_first + n;
    }
}

template <typename T>
inline
void vector<T>::resize(size_type n, const value_type & v) {
    const ptrdiff_t d = n-size();
    if (d < 0)      erase(end()+d, end());
    else if (d > 0) insert(end(), d, v);
}

template<typename T>
inline
vector<T> & vector<T>::operator = (vector<T> && rhs) {
    if (&rhs != this) {
        clear();
        m_first = rhs.m_first;
        m_last  = rhs.m_last;
        m_end   = rhs.m_end;
        rhs.m_first = rhs.m_last = nullptr;
    }
    return *this;
}

template<typename T>
inline
auto vector<T>::_insert_default(const_iterator p, size_type n) -> iterator 
{
    assert(begin() <= p && p <= end());
    const ptrdiff_t d = p - begin();
    reserve(((size() + n + 7) >> 3) << 3);
    auto i = begin() + d;
    // Move tail if there is one
    if (i != end()) memmove(i + n, i, std::distance(i, end())*sizeof(value_type));
    m_last += n;
    return i;
}

template<typename T>
inline
auto vector<T>::insert(const_iterator p, size_type n, const T & x) -> iterator 
{
    auto const i = _insert_default(p, n);
    // Copy in elements
    for (auto u = i; n; --n, ++u) { new (u) value_type(x); }
    return i;
}

template<typename T>
template<class It>
inline
auto vector<T>::insert(const_iterator p, It first, It last) -> iterator
{
    auto const i = _insert_default(p, std::distance(first, last));
    // Copy in elements
    for (auto u = i;first != last; ++first, ++u) { new (u) value_type(*first); }
    return i;
}

template<typename T>
inline
auto vector<T>::erase(const_iterator first, const_iterator last) -> iterator
{
    if (first != last)
    {
        
        for (iterator e = const_cast<iterator>(first); e != last; ++e) 
            e->~value_type();
        auto const sz = std::distance(first, last);
        if (m_last != last) memmove(const_cast<iterator>(first), 
                                    last, 
                                    std::distance(last, cend())*sizeof(value_type));
        m_last -= sz;
    }
    return const_cast<iterator>(first);
}

} // namespace graphite2
