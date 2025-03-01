// SPDX-License-Identifier: MIT
// Copyright 2019, SIL International, All rights reserved.

#pragma once

#include <cassert>
#include <memory>
#include <iterator>
#include <type_traits>

#include "inc/Main.h"

namespace graphite2 {

template<typename T>
class list
{
public:
    using value_type = T;
    using difference_type = ptrdiff_t;
    using size_type = size_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;

private:
    template <typename> class _iterator;

    struct _node_linkage {
        _node_linkage * _next;
        _node_linkage * _prev;

        void link(_node_linkage &);
        void unlink();
        void splice(_node_linkage & start, _node_linkage & end);
    };

    struct _node : public _node_linkage {
        value_type _value;
    };

    _node * _allocate_node();
    void    _release_node(_node * const p) { free(p); }

    _node_linkage       _head;
    size_type           _size;

public:
    using iterator = _iterator<value_type>;
    using const_iterator = _iterator<const value_type>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    list()                    :  _head{&_head,&_head}, _size{0} {}
    list(const list<T> &rhs):  list(rhs.begin(), rhs.end()) {}
    template <typename I>
    list(I first, const I last) { insert(begin(), first, last); }
    ~list()                                     { clear(); }

    reference       front()                     { assert(!empty()); return static_cast<_node*>(_head._next)->_value; }
    const_reference front()     const           { assert(!empty()); return static_cast<_node*>(_head._next)->_value; }
    reference       back()                      { assert(!empty()); return static_cast<_node*>(_head._prev)->_value; }
    const_reference back()      const           { assert(!empty()); return static_cast<_node*>(_head._prev)->_value; }

    iterator        begin()           noexcept  { return iterator(_head._next); }
    const_iterator  begin()     const noexcept  { return cbegin(); }
    const_iterator  cbegin()    const noexcept  { return const_iterator(_head._next); }

    iterator        end()             noexcept  { return iterator(&_head); }
    const_iterator  end()       const noexcept  { return cend(); }
    const_iterator  cend()      const noexcept  { return const_iterator(&_head); }

    iterator        rbegin()          noexcept  { return reverse_iterator(_head._next); }
    const_iterator  rbegin()    const noexcept  { return crbegin(); }
    const_iterator  crbegin()   const noexcept  { return const_reverse_iterator(_head._next); }
    iterator        rend()            noexcept  { return reverse_iterator(&_head); }
    const_iterator  rend()      const noexcept  { return crend(); }
    const_iterator  crend()     const noexcept  { return const_reverse_iterator(&_head); }

    bool            empty()     const noexcept  { return _head._next == &_head; }
    size_t          size()      const noexcept  { return _size; }

    void            clear()           noexcept  { erase(begin(),end()); }
    iterator        insert(const_iterator pos, value_type const &);
    iterator        insert(const_iterator pos, value_type &&);
    // iterator        insert(const_iterator pos, size_type, value_type const &);
    template <typename I>
    iterator        insert(const_iterator pos, I first, I last);

    void            push_back(value_type const & value)          { insert(cend(), value); }
    void            push_front(value_type const & value)         { insert(cbegin(), value); }

    void            pop_front()                 { erase(iterator(_head._next)); }
    void            pop_back()                  { erase(iterator(_head._prev)); }

    iterator erase(iterator pos);
    iterator erase(iterator first, iterator last);
};



template <typename L>
template <typename T>
class list<L>::_iterator
{
    _node_linkage const * _p;

    friend list<L>;

    // Find the address of the enclosing object of class S from a pointer to
    // it's contained member m.
    // This is extermely dangerous if performed on a T* that is not an
    // agreggate member of class S.
    template<class S>
    static constexpr S* container_of(void const *p, T S::* m) noexcept {
        return reinterpret_cast<S *>(
                    reinterpret_cast<uint8_t const *>(p) 
                  - reinterpret_cast<uint8_t const *>(&(((S *)0)->*m)));
    }
    
    // Cast a _node_linkage to a _node<T>
    static _node * node(_iterator<T> const &i) noexcept { 
        return const_cast<_node *>(static_cast<_node const *>(i._p)); 
    }

public:
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = ptrdiff_t;
    using value_type = T;
    using pointer = value_type*;
    using reference = value_type&;

    _iterator(std::nullptr_t): _iterator(nullptr) {}
    _iterator(_node_linkage const *p)  noexcept: _p{p} {}

    bool operator==(_iterator<T> rhs) const noexcept { return _p == rhs._p; }
    bool operator!=(_iterator<T> rhs) const noexcept { return !operator==(rhs); }

    reference operator*() const noexcept { return node(*this)->_value; }
    pointer operator->() const  noexcept { return &operator*(); }

    _iterator<T> &  operator++()     noexcept { _p = _p->_next; return *this; }
    _iterator<T>    operator++(int)  noexcept { _iterator<T> tmp(*this); operator++(); return tmp; }

    _iterator<T> &  operator--() noexcept    { _p = _p->_prev; return *this; }
    _iterator<T>    operator--(int) noexcept { _iterator<T> tmp(*this); operator--(); return tmp; }

    operator pointer() const      noexcept { return operator->(); }
    operator _iterator<T const>() noexcept { return _iterator<T const>(_p); }
};



// 
template<typename T>
auto list<T>::_node_linkage::splice(_node_linkage & start, _node_linkage & end) -> void
{
    // Unlink from current position
    start._prev->_next = end._next;
    end._next->_prev = start._prev;

    // Link in before this node.
    start._prev = this->_prev;
    end._next = this;
    start._prev->_next = &start;
    end._next->_prev = &end;
}


template<typename T>
auto list<T>::_node_linkage::link(_node_linkage & pos) -> void
{
    pos.splice(*this, *this);
}



template<typename T>
auto list<T>::_node_linkage::unlink() -> void
{
    _prev->_next = _next;
    _next->_prev = _prev;
    _prev = _next = this;
}

template <typename T>
auto list<T>::_allocate_node() -> _node * { 
    auto node = grzeroalloc<_node>(1);
    node->_next = node->_prev = node;
    return node;
}


template<typename T>
auto list<T>::insert(const_iterator pos, value_type const & value) -> iterator
{
    assert(pos._p);
    auto node = _allocate_node();
    if (!node) return end();

    node->link(*const_cast<_node_linkage *>(pos._p));
    new (&node->_value) T(value);

    ++_size;
    return iterator(node);
}

template<typename T>
auto list<T>::insert(const_iterator pos, value_type && value) -> iterator
{
    assert(pos._p);
    auto node = _allocate_node();
    if (!node) return end();

    node->link(*const_cast<_node_linkage *>(pos._p));
    new (&node->_value) T(std::move(value));

    ++_size;
    return iterator(node);
}


// template<typename T>
// auto list<T>::insert(const_iterator pos, size_type count, value_type const & value) -> iterator
// {
//     assert(pos._p);
//     auto r = iterator(--pos._p);
//     while (count--) insert(pos, value);
//     return ++r;
// }


template <typename T>
template <class I>
auto list<T>::insert(const_iterator pos, I first, I last) -> iterator
{
    assert(last._p);
    auto r = --iterator(pos._p);
    while (first != last) insert(pos, *first++);
    return ++r;
}


template<typename T>
auto list<T>::erase(iterator pos) -> iterator
{
    assert(pos._p);
    auto node = iterator::node(pos++);
    node->unlink();
    _release_node(node);
    --_size;
    return pos;
}


template<typename T>
auto list<T>::erase(iterator first, iterator const last) -> iterator
{
    assert(last._p);
    while (first != last) erase(first++);
    return first;
}


} //namespace graphite