// SPDX-License-Identifier: MIT
// Copyright 2010, SIL International, All rights reserved.

#pragma once

#include "inc/Main.h"

#include <cassert>

#include "inc/CharInfo.h"
#include "inc/Face.h"
#include "inc/FeatureVal.h"
#include "inc/GlyphCache.h"
#include "inc/GlyphFace.h"
#include "inc/Slot.h"
#include "inc/SlotBuffer.h"
#include "inc/Position.h"
#include "inc/vector.hpp"
#include "inc/Collider.h"

#define MAX_SEG_GROWTH_FACTOR  64

namespace graphite2 {

typedef vector<Features>        FeatureList;

class Font;
class Segment;
class Silf;

enum SpliceParam {
/** sub-Segments longer than this are not cached
 * (in Unicode code points) */
    eMaxSpliceSize = 96
};

enum justFlags {
    gr_justStartInline = 1,
    gr_justEndInline = 2
};


class Segment
{
    // Prevent copying of any kind.
    Segment(const Segment&) = delete;
    Segment& operator=(const Segment&) = delete;

public:

    enum {
        SEG_INITCOLLISIONS = 1,
        SEG_HASCOLLISIONS = 2
    };

    SlotBuffer const & slots() const    { return m_srope; }
    SlotBuffer &       slots()          { return m_srope; }
    size_t slotCount() const { return m_numGlyphs; }      //one slot per glyph
    void extendLength(ptrdiff_t num) { m_numGlyphs += num; }
    Position advance() const { return m_advance; }
    bool runGraphite() { if (m_silf) return m_face->runGraphite(*this, m_silf); else return true;};
    void chooseSilf(uint32_t script) { m_silf = m_face->chooseSilf(script); }
    const Silf *silf() const { return m_silf; }
    size_t charInfoCount() const { return m_numCharinfo; }
    const CharInfo *charinfo(unsigned int index) const { return index < m_numCharinfo ? m_charinfo + index : NULL; }
    CharInfo *charinfo(unsigned int index) { return index < m_numCharinfo ? m_charinfo + index : NULL; }

    Segment(size_t numchars, const Face* face, uint32_t script, int dir);
    ~Segment();
    uint8_t flags() const { return m_flags; }
    void flags(uint8_t f) { m_flags = f; }
    void appendSlot(int i, int cid, int gid, int fid, size_t coffset);
    SlotBuffer::iterator newSlot();
    void freeSlot(SlotBuffer::iterator);

    Position positionSlots(
        Font const * font, 
        SlotBuffer::iterator first, 
        SlotBuffer::iterator last, 
        bool isRtl = false, 
        bool isFinal = true);

    void associateChars(int offset, size_t num);
    void linkClusters();
    uint16_t getClassGlyph(uint16_t cid, uint16_t offset) const { return m_silf->getClassGlyph(cid, offset); }
    uint16_t findClassIndex(uint16_t cid, uint16_t gid) const { return m_silf->findClassIndex(cid, gid); }
    int addFeatures(const Features& feats) { m_feats.push_back(feats); return int(m_feats.size()) - 1; }
    uint32_t getFeature(int index, uint8_t findex) const { const FeatureRef* pFR=m_face->theSill().theFeatureMap().featureRef(findex); if (!pFR) return 0; else return pFR->getFeatureVal(m_feats[index]); }
    void setFeature(int index, uint8_t findex, uint32_t val) {
        const FeatureRef* pFR=m_face->theSill().theFeatureMap().featureRef(findex);
        if (pFR)
        {
            if (val > pFR->maxVal()) val = pFR->maxVal();
            pFR->applyValToFeature(val, m_feats[index]);
        } }
    int8_t dir() const { return m_dir; }
    void dir(int8_t val) { m_dir = val; }
    bool currdir() const { return ((m_dir >> 6) ^ m_dir) & 1; }
    uint8_t passBits() const { return m_passBits; }
    void mergePassBits(const uint8_t val) { m_passBits &= val; }
    int16_t glyphAttr(uint16_t gid, uint16_t gattr) const { const GlyphFace * p = m_face->glyphs().glyphSafe(gid); return p ? p->attrs()[gattr] : 0; }
    int32_t getGlyphMetric(Slot const *iSlot, metrics metric, uint8_t attrLevel, bool rtl) const;
    float glyphAdvance(uint16_t gid) const { return m_face->glyphs().glyph(gid)->theAdvance().x; }
    const Rect &theGlyphBBoxTemporary(uint16_t gid) const { return m_face->glyphs().glyph(gid)->theBBox(); }   //warning value may become invalid when another glyph is accessed
    size_t numAttrs() const { return m_silf->numUser(); }
    int defaultOriginal() const { return m_defaultOriginal; }
    const Face * getFace() const { return m_face; }
    const Features & getFeatures(unsigned int /*charIndex*/) { assert(m_feats.size() == 1); return m_feats[0]; }
    void bidiPass(int paradir, uint8_t aMirror);
    void doMirror(uint16_t aMirror);
    SlotBuffer::iterator addLineEnd(SlotBuffer::iterator nSlot);
    void delLineEnd(SlotBuffer::iterator s);
    void reverseSlots();

    bool isWhitespace(const int cid) const;
    bool hasCollisionInfo() const { return (m_flags & SEG_HASCOLLISIONS) && m_collisions; }
    SlotCollision *collisionInfo(Slot const & s) const { return m_collisions ? m_collisions + s.index() : nullptr; }
    CLASS_NEW_DELETE

public:       //only used by: GrSegment* makeAndInitialize(const GrFont *font, const GrFace *face, uint32_t script, const FeaturesHandle& pFeats/*must not be IsNull*/, encform enc, const void* pStart, size_t nChars, int dir);
    bool read_text(const Face *face, const Features* pFeats/*must not be NULL*/, gr_encform enc, const void*pStart, size_t nChars);
    void finalise(const Font *font, bool reverse=false);
    float justify(SlotBuffer::iterator pSlot, const Font *font, float width, enum justFlags flags, SlotBuffer::iterator pFirst, SlotBuffer::iterator pLast);
    bool initCollisions();

    template<typename Callable>
    typename std::result_of<Callable()>::type
    subsegment(SlotBuffer::const_iterator first, SlotBuffer::const_iterator last, Callable body);

private:
    SlotBuffer      m_srope;
    Position        m_advance;          // whole segment advance
    FeatureList     m_feats;            // feature settings referenced by charinfos in this segment
    CharInfo      * m_charinfo;         // character info, one per input character
    SlotCollision * m_collisions;
    const Face    * m_face;             // GrFace
    const Silf    * m_silf;
    size_t          m_bufSize,          // how big a buffer to create when need more slots
                    m_numGlyphs,
                    m_numCharinfo;      // size of the array and number of input characters
    int             m_defaultOriginal;  // number of whitespace chars in the string
    int8_t            m_dir;
    uint8_t           m_flags,            // General purpose flags
                    m_passBits;         // if bit set then skip pass
};


inline
void Segment::finalise(const Font *font, bool reverse)
{
    if (slots().empty()) return;

    m_advance = positionSlots(font, slots().begin(), slots().end(), m_silf->dir(), true);
    //associateChars(0, m_numCharinfo);
    if (reverse && currdir() != (m_dir & 1))
        reverseSlots();
    linkClusters();
}

inline
int32_t Segment::getGlyphMetric(Slot const * iSlot, metrics metric, uint8_t attrLevel, bool rtl) const {
    if (attrLevel > 0)
    {
        auto is = iSlot->base();
        return is->clusterMetric(*this, metric, attrLevel, rtl);
    }
    else
        return m_face->getGlyphMetric(iSlot->gid(), metric);
}

inline
bool Segment::isWhitespace(const int cid) const
{
    return ((cid >= 0x0009) * (cid <= 0x000D)
         + (cid == 0x0020)
         + (cid == 0x0085)
         + (cid == 0x00A0)
         + (cid == 0x1680)
         + (cid == 0x180E)
         + (cid >= 0x2000) * (cid <= 0x200A)
         + (cid == 0x2028)
         + (cid == 0x2029)
         + (cid == 0x202F)
         + (cid == 0x205F)
         + (cid == 0x3000)) != 0;
}

} // namespace graphite2

struct gr_segment : public graphite2::Segment {};
