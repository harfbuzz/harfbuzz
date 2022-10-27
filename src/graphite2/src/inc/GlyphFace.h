// SPDX-License-Identifier: MIT
// Copyright 2010, SIL International, All rights reserved.

#pragma once

#include "inc/Main.h"
#include "inc/Position.h"
#include "inc/Sparse.h"

namespace graphite2 {

enum metrics: uint8_t {
    kgmetLsb = 0, kgmetRsb,
    kgmetBbTop, kgmetBbBottom, kgmetBbLeft, kgmetBbRight,
    kgmetBbHeight, kgmetBbWidth,
    kgmetAdvWidth, kgmetAdvHeight,
    kgmetAscent, kgmetDescent
};


class GlyphFace
{
public:
    GlyphFace();
    template<typename I>
    GlyphFace(const Rect & bbox, const Position & adv, I first, const I last);

    const Position    & theAdvance() const;
    const Rect        & theBBox() const { return m_bbox; }
    const sparse      & attrs() const { return m_attrs; }
    int32_t               getMetric(uint8_t metric) const;

    CLASS_NEW_DELETE;
private:
    Rect     m_bbox;        // bounding box metrics in design units
    Position m_advance;     // Advance width and height in design units
    sparse   m_attrs;
};


// Inlines: class GlyphFace
//
inline
GlyphFace::GlyphFace()
{}

template<typename I>
GlyphFace::GlyphFace(const Rect & bbox, const Position & adv, I first, const I last)
: m_bbox(bbox),
  m_advance(adv),
  m_attrs(first, last)
{
}

inline
const Position & GlyphFace::theAdvance() const {
    return m_advance;
}

} // namespace graphite2
