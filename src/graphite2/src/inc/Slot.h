// SPDX-License-Identifier: MIT
// Copyright 2010, SIL International, All rights reserved.

#pragma once

#include <cassert>
#include <cstring>
#include <limits>
#include <type_traits>

#include "graphite2/Segment.h"

#include "inc/GlyphFace.h"
#include "inc/Main.h"
#include "inc/Position.h"

namespace graphite2 {

class Segment;
// class Slot;
class ShapingContext;
class Font;

struct Slot_data {
    int8_t      m_parent_offset;   // index to parent we are attached to
    Position    m_position; // absolute position of glyph
    Position    m_shift;    // .shift slot attribute
    Position    m_advance;  // .advance slot attribute
    Position    m_attach;   // attachment point on us
    Position    m_with;     // attachment point position on parent
    float       m_just;     // Justification inserted space
    uint32_t      m_original; // charinfo that originated this slot (e.g. for feature values)
    uint32_t      m_before;   // charinfo index of before association
    uint32_t      m_after;    // charinfo index of after association
    uint32_t      m_index;    // slot index given to this slot during finalising
    uint16_t      m_glyphid;  // glyph id
    uint16_t      m_realglyphid;
    byte        m_attLevel;    // attachment level
    byte        m_bidiLevel;   // bidirectional level
    int8_t        m_bidiCls;     // bidirectional class
    struct {
        bool    deleted: 1,
                inserted: 1,
                copied: 1,
                positioned: 1,
                clusterhead:1,
                last:1,
                children:1;
    }        m_flags;       // holds bit flags
};


class Slot : private Slot_data
{
    static constexpr int NUMJUSTPARAMS = 5;

    union attributes {
    private:
        struct {
            #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            uint8_t n_attrs;
            #endif
            int16_t data[sizeof(uintptr_t)/sizeof(int16_t)-1];
            #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            uint8_t n_attrs;
            #endif
        }   local;
        struct { 
            uint8_t n_attrs, n_justs;
            int16_t data[1]; 
        } * external;

        bool is_inline() const { return !external || uintptr_t(external) & 0x3;}

    public:
        constexpr attributes(): external{nullptr} {}
        attributes(size_t n_attrs, size_t n_justs = 0): external{nullptr} { reserve(n_attrs, n_justs); }
        attributes(attributes const & rhs): external{nullptr} { operator = (rhs); }
        attributes(attributes && rhs) noexcept : external{rhs.external} { rhs.external = nullptr; }
        ~attributes() noexcept { if (!is_inline()) free(external); }

        void reserve(size_t target_num_attrs, size_t target_num_justs);
        attributes & operator = (attributes const & rhs);
        attributes & operator = (attributes && rhs) noexcept;

        size_t num_attrs() const { return is_inline() ? local.n_attrs : external->n_attrs; }
        size_t num_justs() const { return is_inline() ? 0 : external->n_justs; }

        int16_t       * user_attributes() { return is_inline() ? local.data : external->data; }
        int16_t const * user_attributes() const { return is_inline() ? local.data : external->data; }
        int16_t       * just_info() { return is_inline() ? nullptr : external->data + external->n_attrs; }
        int16_t const * just_info() const { return is_inline() ? nullptr : external->data + external->n_attrs; }
    };

    bool has_justify() const { return m_attrs.num_justs() != 0; };
    void init_just_infos(Segment const & seg);
    void attachTo(Slot *ap);

    attributes  m_attrs;
#if !defined GRAPHITE2_NTRACING
    uint8_t     m_gen;
#endif

    template<class S> class _cluster_iterator;
    template<class S> class _child_iterator;

public:
    using cluster_iterator = _cluster_iterator<Slot>;
    using const_cluster_iterator = _cluster_iterator<Slot const>;
    using child_iterator = _child_iterator<Slot>;
    using const_child_iterator = _child_iterator<Slot const>;
    struct sentinal {};
    using attrCode = gr_attrCode;

//     static bool is_sentinal(Slot const & s) noexcept {
//         return s.m_index == 0xffffffff 
//             && s.m_glyphid == 0xffff
//             && s.m_realglyphid == 0xffff;
//     }

//     template<class T>
    constexpr Slot(sentinal const &)
    : Slot_data{0, {}, {}, {}, {}, {}, 0, 0, 0, 0, -1u, uint16_t(-1), uint16_t(-1), 0, 0, 0, {true,false,false,false,true,true,false}},
      m_attrs{}
#if !defined GRAPHITE2_NTRACING
      , m_gen{0}
#endif
    {}

    Slot(size_t num_attrs = 0) : Slot_data{}, m_attrs{num_attrs} {}
    Slot(Slot const &);
    Slot(Slot &&) noexcept;

    Slot & operator=(Slot const &);
    Slot & operator=(Slot &&) noexcept;

    // Glyph
    uint16_t    gid() const     { return m_glyphid; }
    uint16_t    glyph() const   { return m_realglyphid ? m_realglyphid : m_glyphid; }
    void        glyph(Segment &seg, uint16_t glyphid, const GlyphFace * theGlyph = nullptr);
//     void setRealGid(uint16_t realGid) { m_realglyphid = realGid; }

    // Positioning
    Position const & origin() const { return m_position; }
    Position & origin() { return m_position; }
    Position const & shift() const { return m_shift; }
//     void adjKern(const Position &pos) { m_shift = m_shift + pos; m_advance = m_advance + pos; }
    float advance() const { return m_advance.x; }
    void advance(Position &val) { m_advance = val; }
    Position attachOffset() const { return m_attach - m_with; }
    Position const & advancePos() const { return m_advance; }
    void position_shift(Position const & p) { m_position += p; }
    void floodShift(Position adj, int depth = 0);
    void scale_by(float scale) { m_position *= scale; /*m_advance *= scale; m_shift *= scale; m_with *= scale; m_attach *= scale; */}
    Position collision_shift(Segment const & seg) const;


    // Slot ordering
    uint32_t  index() const       { return m_index; }
    void    index(uint32_t val)   { m_index = val; }
    int     before() const      { return m_before; }
    void    before(int ind)     { m_before = ind; }
    int     after() const       { return m_after; }
    void    after(int ind)      { m_after = ind; }
    int     original() const    { return m_original; }
    void    original(int ind)   { m_original = ind; }

//     // Flags
    bool deleted() const     { return m_flags.deleted; }
    void deleted(bool state) { m_flags.deleted = state; }
    bool copied() const      { return m_flags.copied; }
    void copied(bool state)  { m_flags.copied = state; }
//     bool positioned() const { return m_flags.positioned; }
//     void positioned(bool state) { m_flags.positioned = state; }
    bool last() const        { return m_flags.last; }
    void last(bool state)    { m_flags.last = state; }
    bool insertBefore() const { return !m_flags.inserted; }
    void insertBefore(bool state) { m_flags.inserted = !state; }
    void clusterhead(bool state) { m_flags.clusterhead = state; }
    bool clusterhead() const { return m_flags.clusterhead ;}

    // Bidi
    uint8_t   bidiLevel() const        { return m_bidiLevel; }
    void    bidiLevel(uint8_t level)   { m_bidiLevel = level; }
    int8_t    bidiClass() const        { return m_bidiCls; }
    void    bidiClass(int8_t cls)      { m_bidiCls = cls; }

    // Operations
    Position update_cluster_metric(Segment const & seg, bool const rtl, bool const is_final, float & clsb, float & crsb, unsigned depth=100);
    void update(int numSlots, int numCharInfo, Position &relpos);
    Position finalise(const Segment & seg, const Font* font, Position & base, Rect & bbox, uint8_t attrLevel, float & clusterMin, bool rtl, bool isFinal, int depth = 0);
    int32_t clusterMetric(Segment const & seg, metrics metric, uint8_t attrLevel, bool rtl) const;

    // Attributes
    void    setAttr(Segment & seg, attrCode ind, uint8_t subindex, int16_t val, const ShapingContext & map);
    int     getAttr(const Segment &seg, attrCode ind, uint8_t subindex) const;
    int16_t const *userAttrs() const { return m_attrs.user_attributes(); }

//     // Justification
    int getJustify(const Segment &seg, uint8_t level, uint8_t subindex) const;
    void setJustify(Segment &seg, uint8_t level, uint8_t subindex, int16_t value);
    float just() const { return m_just; }
    void just(float j) { m_just = j; }

    // Clustering
    cluster_iterator cluster();
    const_cluster_iterator cluster() const;
    child_iterator children();
    const_child_iterator children() const;
    const_cluster_iterator end() const noexcept;

    bool isBase() const             { return !m_parent_offset; }
    bool isParent() const           { return m_flags.children; }
    Slot const * base() const noexcept;
    Slot * base() noexcept { return const_cast<Slot*>(const_cast<Slot const *>(this)->base()); }

    Slot const * attachedTo() const;
    Slot * attachedTo() { return const_cast<Slot *>(const_cast<Slot const *>(this)->attachedTo());}

    bool add_child(Slot *ap);
    bool remove_child(Slot *ap);
    bool has_base(const Slot *base) const;


#if !defined GRAPHITE2_NTRACING
    uint8_t & generation() { return m_gen; }
    uint8_t   generation() const { return m_gen; }
    
#else
    uint8_t & generation() { static uint8_t _v; return _v; }
#endif
    CLASS_NEW_DELETE
};

template<class S>
class Slot::_cluster_iterator {
protected:
    S* _s;

public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = S;
    using pointer = value_type*;
    using reference = value_type&;

    constexpr _cluster_iterator() noexcept: _s{nullptr} {}
    _cluster_iterator(pointer s) noexcept : _s{s} {}

    bool operator == (_cluster_iterator<S const> const & rhs) const noexcept;
    bool operator != (_cluster_iterator<S const> const & rhs) const noexcept;

    reference operator*() const noexcept            { return *_s;  }
    pointer   operator->() const  noexcept          { return &operator*(); }

    _cluster_iterator<S> &  operator++() noexcept       { assert(_s); if ((++_s)->clusterhead()) _s = nullptr; return *this; }
    _cluster_iterator<S>    operator++(int) noexcept    { auto tmp(*this); operator++(); return tmp; }

    _cluster_iterator<S> &  operator--() noexcept       { assert(_s); --_s; return *this; }
    _cluster_iterator<S>    operator--(int) noexcept    { auto tmp(*this); operator--(); return tmp; }

    operator _cluster_iterator<S const> const &() const noexcept { return *reinterpret_cast<_cluster_iterator<S const> const *>(this); }
};

template<class S>
inline
bool Slot::_cluster_iterator<S>::operator == (_cluster_iterator<S const> const & rhs) const noexcept { 
    return _s == rhs._s; 
}

template<class S>
inline
bool Slot::_cluster_iterator<S>::operator != (_cluster_iterator<S const> const & rhs) const noexcept {
    return !rhs.operator==(*this);
}



template<class S>
class Slot::_child_iterator: public _cluster_iterator<S> {
    using base_t = Slot::_cluster_iterator<S>;

    void _next_child(base_t & (base_t::*advfn)()) noexcept { 
        assert(base_t::_s);
        auto parent = base_t::_s->attachedTo();
        do { 
            (this->*advfn)(); 
        } while (base_t::_s && base_t::_s->attachedTo() != parent);
    }

public:
    using base_t::iterator_category;
    using base_t::value_type;
    using base_t::pointer;
    using base_t::reference;

    constexpr _child_iterator(): _cluster_iterator<S>{} {}
    _child_iterator(S * s): base_t{s} {}

    _child_iterator<S> &  operator++() noexcept       { _next_child(&base_t::operator++); return *this; }
    _child_iterator<S>    operator++(int) noexcept    { auto tmp(*this); operator++(); return tmp; }

    _child_iterator<S> &  operator--() noexcept       { _next_child(&base_t::operator--); return *this; }
    _child_iterator<S>    operator--(int) noexcept    { auto tmp(*this); operator--(); return tmp; }

    gr_slot const * handle() const noexcept {
        return base_t::_s 
            ? base_t::_s->last() && base_t::_s->gid() == 0xffff && base_t::_s->deleted() 
                ? nullptr 
                : static_cast<gr_slot const *>(base_t::_s)
            : nullptr;
    }

    operator _child_iterator<S const>() const noexcept { return *reinterpret_cast<_child_iterator<S const> const *>(this); }
};


inline
auto Slot::cluster() -> cluster_iterator {
    auto r = this;
    while (!r->clusterhead()) --r;
    return const_cast<Slot *>(r);
}

inline
auto Slot::cluster() const -> const_cluster_iterator { 
    auto r = this;
    while (!r->clusterhead()) --r;
    return const_cast<Slot *>(r);
}

inline
auto Slot::children() -> child_iterator {
    for (auto ci = cluster(), end = cluster_iterator(); ci != end; ++ci)
        if (ci->attachedTo() == this) return child_iterator(&*ci);
    return child_iterator();
}

inline
auto Slot::children() const -> const_child_iterator { 
    for (auto ci = cluster(), end = const_cluster_iterator(); ci != end; ++ci)
        if (ci->attachedTo() == this) return const_child_iterator(&*ci);
    return const_child_iterator();
}

inline
auto Slot::end() const noexcept -> const_cluster_iterator {
    return const_cluster_iterator();
}


inline
Slot::Slot(Slot && rhs) noexcept
: Slot_data(std::move(rhs)),
  m_attrs(std::move(rhs.m_attrs))
#if !defined GRAPHITE2_NTRACING
  ,m_gen(rhs.m_gen)
#endif
{
  m_parent_offset = 0;
}


inline
Slot::Slot(Slot const & rhs) 
: Slot_data(rhs),
  m_attrs(rhs.m_attrs)
#if !defined GRAPHITE2_NTRACING
  ,m_gen(rhs.m_gen)
#endif
{
  m_parent_offset = 0;
}

inline
Slot const * Slot::base() const noexcept {
    auto s = this; 
    while (s->m_parent_offset) 
        s += s->m_parent_offset; 
    return s; 
}

inline 
auto Slot::attachedTo() const -> Slot const * {
    return m_parent_offset ? this + m_parent_offset : nullptr;
}

inline 
void Slot::attachTo(Slot *ap) {
    m_parent_offset = ap ? ap - this : 0;
}

} // namespace graphite2

struct gr_slot : public graphite2::Slot {};
