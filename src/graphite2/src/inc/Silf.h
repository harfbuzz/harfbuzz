// SPDX-License-Identifier: MIT
// Copyright 2010, SIL International, All rights reserved.

#pragma once

#include "graphite2/Font.h"
#include "inc/Main.h"
#include "inc/Pass.h"

namespace graphite2 {

class Face;
class Segment;
class FeatureVal;
class VMScratch;
class Error;

class Pseudo
{
public:
    uint32_t uid;
    uint32_t gid;
    CLASS_NEW_DELETE;
};

class Justinfo
{
public:
    Justinfo(uint8_t stretch, uint8_t shrink, uint8_t step, uint8_t weight) :
        m_astretch(stretch), m_ashrink(shrink), m_astep(step),
        m_aweight(weight) {};
    uint8_t attrStretch() const { return m_astretch; }
    uint8_t attrShrink() const { return m_ashrink; }
    uint8_t attrStep() const { return m_astep; }
    uint8_t attrWeight() const { return m_aweight; }

private:
    uint8_t   m_astretch;
    uint8_t   m_ashrink;
    uint8_t   m_astep;
    uint8_t   m_aweight;
};

class Silf
{
    // Prevent copying
    Silf(const Silf&);
    Silf& operator=(const Silf&);

public:
    Silf() throw();
    ~Silf() throw();

    bool readGraphite(const byte * const pSilf, size_t lSilf, Face &face, uint32_t version);
    bool runGraphite(Segment &seg, uint8_t firstPass=0, uint8_t lastPass=0, int dobidi = 0) const;
    uint16_t findClassIndex(uint16_t cid, uint16_t gid) const;
    uint16_t getClassGlyph(uint16_t cid, unsigned int index) const;
    uint16_t findPseudo(uint32_t uid) const;
    size_t numUser() const { return m_aUser; }
    uint8_t aPseudo() const { return m_aPseudo; }
    uint8_t aBreak() const { return m_aBreak; }
    uint8_t aMirror() const {return m_aMirror; }
    uint8_t aPassBits() const { return m_aPassBits; }
    uint8_t aBidi() const { return m_aBidi; }
    uint8_t aCollision() const { return m_aCollision; }
    uint8_t substitutionPass() const { return m_sPass; }
    uint8_t positionPass() const { return m_pPass; }
    uint8_t justificationPass() const { return m_jPass; }
    uint8_t bidiPass() const { return m_bPass; }
    size_t numPasses() const { return m_numPasses; }
    uint8_t maxCompPerLig() const { return m_iMaxComp; }
    size_t numClasses() const { return m_nClass; }
    byte  flags() const { return m_flags; }
    byte  dir() const { return m_dir; }
    size_t numJustLevels() const { return m_numJusts; }
    Justinfo *justAttrs() const { return m_justs; }
    uint16_t endLineGlyphid() const { return m_gEndLine; }
    const gr_faceinfo *silfInfo() const { return &m_silfinfo; }

    CLASS_NEW_DELETE;

private:
    size_t readClassMap(const byte *p, size_t data_len, uint32_t version, Error &e);
    template<typename T> inline uint32_t readClassOffsets(const byte *&p, size_t data_len, Error &e);

    Pass          * m_passes;
    Pseudo        * m_pseudos;
    uint32_t        * m_classOffsets;
    uint16_t        * m_classData;
    Justinfo      * m_justs;
    uint8_t           m_numPasses;
    uint8_t           m_numJusts;
    uint8_t           m_sPass, m_pPass, m_jPass, m_bPass,
                    m_flags, m_dir;

    uint8_t       m_aPseudo, m_aBreak, m_aUser, m_aBidi, m_aMirror, m_aPassBits,
                m_iMaxComp, m_aCollision;
    uint16_t      m_aLig, m_numPseudo, m_nClass, m_nLinear,
                m_gEndLine;
    gr_faceinfo m_silfinfo;

    void releaseBuffers() throw();
};

} // namespace graphite2
