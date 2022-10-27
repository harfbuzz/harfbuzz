// SPDX-License-Identifier: MIT
// Copyright 2010, SIL International, All rights reserved.

#include <cstdlib>
#include "graphite2/Segment.h"
#include "inc/debug.h"
#include "inc/Endian.h"
#include "inc/Silf.h"
#include "inc/Segment.h"
#include "inc/Rule.h"
#include "inc/Error.h"
#include "inc/ShapingContext.hpp"


using namespace graphite2;

namespace { static const uint32_t ERROROFFSET = 0xFFFFFFFF; }

Silf::Silf() throw()
: m_passes(0),
  m_pseudos(0),
  m_classOffsets(0),
  m_classData(0),
  m_justs(0),
  m_numPasses(0),
  m_numJusts(0),
  m_sPass(0),
  m_pPass(0),
  m_jPass(0),
  m_bPass(0),
  m_flags(0),
  m_dir(0),
  m_aPseudo(0),
  m_aBreak(0),
  m_aUser(0),
  m_aBidi(0),
  m_aMirror(0),
  m_aPassBits(0),
  m_iMaxComp(0),
  m_aCollision(0),
  m_aLig(0),
  m_numPseudo(0),
  m_nClass(0),
  m_nLinear(0),
  m_gEndLine(0)
{
    memset(&m_silfinfo, 0, sizeof m_silfinfo);
}

Silf::~Silf() throw()
{
    releaseBuffers();
}

void Silf::releaseBuffers() throw()
{
    delete [] m_passes;
    delete [] m_pseudos;
    free(m_classOffsets);
    free(m_classData);
    free(m_justs);
    m_passes= 0;
    m_pseudos = 0;
    m_classOffsets = 0;
    m_classData = 0;
    m_justs = 0;
}


bool Silf::readGraphite(const byte * const silf_start, size_t lSilf, Face& face, uint32_t version)
{
    const byte * p = silf_start,
               * const silf_end = p + lSilf;
    Error e;

    if (e.test(version >= 0x00060000, E_BADSILFVERSION))
    {
        releaseBuffers(); return face.error(e);
    }
    if (version >= 0x00030000)
    {
        if (e.test(lSilf < 28, E_BADSIZE)) { releaseBuffers(); return face.error(e); }
        be::skip<int32_t>(p);    // ruleVersion
        be::skip<uint16_t>(p,2); // passOffset & pseudosOffset
    }
    else if (e.test(lSilf < 20, E_BADSIZE)) { releaseBuffers(); return face.error(e); }
    const uint16_t maxGlyph = be::read<uint16_t>(p);
    m_silfinfo.extra_ascent = be::read<uint16_t>(p);
    m_silfinfo.extra_descent = be::read<uint16_t>(p);
    m_numPasses = be::read<uint8_t>(p);
    m_sPass     = be::read<uint8_t>(p);
    m_pPass     = be::read<uint8_t>(p);
    m_jPass     = be::read<uint8_t>(p);
    m_bPass     = be::read<uint8_t>(p);
    m_flags     = be::read<uint8_t>(p);
    be::skip<uint8_t>(p,2); //  max{Pre,Post}Context.
    m_aPseudo   = be::read<uint8_t>(p);
    m_aBreak    = be::read<uint8_t>(p);
    m_aBidi     = be::read<uint8_t>(p);
    m_aMirror   = be::read<uint8_t>(p);
    m_aPassBits = be::read<uint8_t>(p);

    // Read Justification levels.
    m_numJusts  = be::read<uint8_t>(p);
    if (e.test(maxGlyph >= face.glyphs().numGlyphs(), E_BADMAXGLYPH)
        || e.test(p + m_numJusts * 8 >= silf_end, E_BADNUMJUSTS))
    {
        releaseBuffers(); return face.error(e);
    }

    if (m_numJusts)
    {
        m_justs = gralloc<Justinfo>(m_numJusts);
        if (e.test(!m_justs, E_OUTOFMEM)) return face.error(e);

        for (uint8_t i = 0; i < m_numJusts; i++)
        {
            ::new(m_justs + i) Justinfo(p[0], p[1], p[2], p[3]);
            be::skip<byte>(p,8);
        }
    }

    if (e.test(p + sizeof(uint16_t) + sizeof(uint8_t)*8 >= silf_end, E_BADENDJUSTS)) { releaseBuffers(); return face.error(e); }
    m_aLig       = be::read<uint16_t>(p);
    m_aUser      = be::read<uint8_t>(p);
    m_iMaxComp   = be::read<uint8_t>(p);
    m_dir        = be::read<uint8_t>(p) - 1;
    m_aCollision = be::read<uint8_t>(p);
    be::skip<byte>(p,3);
    be::skip<uint16_t>(p, be::read<uint8_t>(p));    // don't need critical features yet
    be::skip<byte>(p);                          // reserved
    if (e.test(p >= silf_end, E_BADCRITFEATURES))   { releaseBuffers(); return face.error(e); }
    be::skip<uint32_t>(p, be::read<uint8_t>(p));    // don't use scriptTag array.
    if (e.test(p + sizeof(uint16_t) + sizeof(uint32_t) >= silf_end, E_BADSCRIPTTAGS)) { releaseBuffers(); return face.error(e); }
    m_gEndLine  = be::read<uint16_t>(p);          // lbGID
    const byte * o_passes = p;
    uint32_t passes_start = be::read<uint32_t>(p);

    const size_t num_attrs = face.glyphs().numAttrs();
    if (e.test(m_aPseudo   >= num_attrs, E_BADAPSEUDO)
        || e.test(m_aBreak >= num_attrs, E_BADABREAK)
        || e.test(m_aBidi  >= num_attrs, E_BADABIDI)
        || e.test(m_aMirror>= num_attrs, E_BADAMIRROR)
        || e.test(m_aCollision && m_aCollision >= num_attrs - 5, E_BADACOLLISION)
        || e.test(m_numPasses > 128, E_BADNUMPASSES) || e.test(passes_start >= lSilf, E_BADPASSESSTART)
        || e.test(m_pPass < m_sPass, E_BADPASSBOUND) || e.test(m_pPass > m_numPasses, E_BADPPASS) || e.test(m_sPass > m_numPasses, E_BADSPASS)
        || e.test(m_jPass < m_pPass, E_BADJPASSBOUND) || e.test(m_jPass > m_numPasses, E_BADJPASS)
        || e.test((m_bPass != 0xFF && (m_bPass < m_jPass || m_bPass > m_numPasses)), E_BADBPASS)
        || e.test(m_aLig > 127, E_BADALIG))
    {
        releaseBuffers();
        return face.error(e);
    }
    be::skip<uint32_t>(p, m_numPasses);
    if (e.test(unsigned(p - silf_start) + sizeof(uint16_t) >= passes_start, E_BADPASSESSTART)) { releaseBuffers(); return face.error(e); }
    m_numPseudo = be::read<uint16_t>(p);
    be::skip<uint16_t>(p, 3); // searchPseudo, pseudoSelector, pseudoShift
    m_pseudos = new Pseudo[m_numPseudo];
    if (e.test(unsigned(p - silf_start) + m_numPseudo*(sizeof(uint32_t) + sizeof(uint16_t)) >= passes_start, E_BADNUMPSEUDO)
        || e.test(!m_pseudos, E_OUTOFMEM))
    {
        releaseBuffers(); return face.error(e);
    }
    for (int i = 0; i < m_numPseudo; i++)
    {
        m_pseudos[i].uid = be::read<uint32_t>(p);
        m_pseudos[i].gid = be::read<uint16_t>(p);
    }

    const size_t clen = readClassMap(p, passes_start + silf_start - p, version, e);
    m_passes = new Pass[m_numPasses];
    if (e || e.test(clen > unsigned(passes_start + silf_start - p), E_BADPASSESSTART)
          || e.test(!m_passes, E_OUTOFMEM))
    { releaseBuffers(); return face.error(e); }

    for (size_t i = 0; i < m_numPasses; ++i)
    {
        uint32_t pass_start = be::read<uint32_t>(o_passes);
        uint32_t pass_end = be::peek<uint32_t>(o_passes);
        face.error_context((face.error_context() & 0xFF00) + EC_ASILF + unsigned(i << 16));
        if (e.test(pass_start > pass_end, E_BADPASSSTART)
                || e.test(pass_start < passes_start, E_BADPASSSTART)
                || e.test(pass_end > lSilf, E_BADPASSEND)) {
            releaseBuffers(); return face.error(e);
        }

        enum passtype pt = PASS_TYPE_UNKNOWN;
        if (i >= m_jPass) pt = PASS_TYPE_JUSTIFICATION;
        else if (i >= m_pPass) pt = PASS_TYPE_POSITIONING;
        else if (i >= m_sPass) pt = PASS_TYPE_SUBSTITUTE;
        else pt = PASS_TYPE_LINEBREAK;

        m_passes[i].init(this);
        if (!m_passes[i].readPass(silf_start + pass_start, pass_end - pass_start, pass_start, face, pt,
            version, e))
        {
            releaseBuffers();
            return false;
        }
    }

    // fill in gr_faceinfo
    m_silfinfo.upem = face.glyphs().unitsPerEm();
    m_silfinfo.has_bidi_pass = (m_bPass != 0xFF);
    m_silfinfo.justifies = (m_numJusts != 0) || (m_jPass < m_pPass);
    m_silfinfo.line_ends = (m_flags & 1);
    m_silfinfo.space_contextuals = gr_faceinfo::gr_space_contextuals((m_flags >> 2) & 0x7);
    return true;
}

template<typename T> inline uint32_t Silf::readClassOffsets(const byte *&p, size_t data_len, Error &e)
{
    const T cls_off = 2*sizeof(uint16_t) + sizeof(T)*(m_nClass+1);
    const uint32_t max_off = (be::peek<T>(p + sizeof(T)*m_nClass) - cls_off)/sizeof(uint16_t);
    // Check that the last+1 offset is less than or equal to the class map length.
    if (e.test(be::peek<T>(p) != cls_off, E_MISALIGNEDCLASSES)
            || e.test(max_off > (data_len - cls_off)/sizeof(uint16_t), E_HIGHCLASSOFFSET))
        return ERROROFFSET;

    // Read in all the offsets.
    m_classOffsets = gralloc<uint32_t>(m_nClass+1);
    if (e.test(!m_classOffsets, E_OUTOFMEM)) return ERROROFFSET;
    for (uint32_t * o = m_classOffsets, * const o_end = o + m_nClass + 1; o != o_end; ++o)
    {
        *o = (be::read<T>(p) - cls_off)/sizeof(uint16_t);
        if (e.test(*o > max_off, E_HIGHCLASSOFFSET))
            return ERROROFFSET;
    }
    return max_off;
}

size_t Silf::readClassMap(const byte *p, size_t data_len, uint32_t version, Error &e)
{
    if (e.test(data_len < sizeof(uint16_t)*2, E_BADCLASSSIZE)) return ERROROFFSET;

    m_nClass  = be::read<uint16_t>(p);
    m_nLinear = be::read<uint16_t>(p);

    // Check that numLinear < numClass,
    // that there is at least enough data for numClasses offsets.
    if (e.test(m_nLinear > m_nClass, E_TOOMANYLINEAR)
     || e.test((m_nClass + 1) * (version >= 0x00040000 ? sizeof(uint32_t) : sizeof(uint16_t)) > (data_len - 4), E_CLASSESTOOBIG))
        return ERROROFFSET;

    uint32_t max_off;
    if (version >= 0x00040000)
        max_off = readClassOffsets<uint32_t>(p, data_len, e);
    else
        max_off = readClassOffsets<uint16_t>(p, data_len, e);

    if (max_off == ERROROFFSET) return ERROROFFSET;

    if (e.test((int)max_off < m_nLinear + (m_nClass - m_nLinear) * 6, E_CLASSESTOOBIG))
        return ERROROFFSET;

    // Check the linear offsets are sane, these must be monotonically increasing.
    assert(m_nClass >= m_nLinear);
    for (const uint32_t *o = m_classOffsets, * const o_end = o + m_nLinear; o != o_end; ++o)
        if (e.test(o[0] > o[1], E_BADCLASSOFFSET))
            return ERROROFFSET;

    // Fortunately the class data is all uint16s so we can decode these now
    m_classData = gralloc<uint16_t>(max_off);
    if (e.test(!m_classData, E_OUTOFMEM)) return ERROROFFSET;
    for (uint16_t *d = m_classData, * const d_end = d + max_off; d != d_end; ++d)
        *d = be::read<uint16_t>(p);

    // Check the lookup class invariants for each non-linear class
    for (const uint32_t *o = m_classOffsets + m_nLinear, * const o_end = m_classOffsets + m_nClass; o != o_end; ++o)
    {
        const uint16_t * lookup = m_classData + *o;
        if (e.test(*o + 4 > max_off, E_HIGHCLASSOFFSET)                        // LookupClass doesn't stretch over max_off
         || e.test(lookup[0] == 0                                                   // A LookupClass with no looks is a suspicious thing ...
                    || lookup[0] * 2 + *o + 4 > max_off                             // numIDs lookup pairs fits within (start of LookupClass' lookups array, max_off]
                    || lookup[3] + lookup[1] != lookup[0], E_BADCLASSLOOKUPINFO)    // rangeShift:   numIDs  - searchRange
         || e.test(((o[1] - *o) & 1) != 0, ERROROFFSET))                         // glyphs are in pairs so difference must be even.
            return ERROROFFSET;
    }

    return max_off;
}

uint16_t Silf::findPseudo(uint32_t uid) const
{
    for (int i = 0; i < m_numPseudo; i++)
        if (m_pseudos[i].uid == uid) return m_pseudos[i].gid;
    return 0;
}

uint16_t Silf::findClassIndex(uint16_t cid, uint16_t gid) const
{
    if (cid > m_nClass) return -1;

    const uint16_t * cls = m_classData + m_classOffsets[cid];
    if (cid < m_nLinear)        // output class being used for input, shouldn't happen
    {
        for (unsigned int i = 0, n = m_classOffsets[cid + 1] - m_classOffsets[cid]; i < n; ++i, ++cls)
            if (*cls == gid) return i;
        return -1;
    }
    else
    {
        const uint16_t *  min = cls + 4,      // lookups array
                     *  max = min + cls[0]*2; // lookups aray is numIDs (cls[0]) uint16_t pairs long
        do
        {
            const uint16_t * p = min + (-2 & ((max-min)/2));
            if  (p[0] > gid)    max = p;
            else                min = p;
        }
        while (max - min > 2);
        return min[0] == gid ? min[1] : -1;
    }
}

uint16_t Silf::getClassGlyph(uint16_t cid, unsigned int index) const
{
    if (cid > m_nClass) return 0;

    uint32_t loc = m_classOffsets[cid];
    if (cid < m_nLinear)
    {
        if (index < m_classOffsets[cid + 1] - loc)
            return m_classData[index + loc];
    }
    else        // input class being used for output. Shouldn't happen
    {
        for (unsigned int i = loc + 4; i < m_classOffsets[cid + 1]; i += 2)
            if (m_classData[i + 1] == index) return m_classData[i];
    }
    return 0;
}


bool Silf::runGraphite(Segment &seg, uint8_t firstPass, uint8_t lastPass, int dobidi) const
{
    size_t             maxSize = seg.slotCount() * MAX_SEG_GROWTH_FACTOR;
    ShapingContext     ctxt(seg, m_dir, maxSize);
    vm::Machine        m(ctxt);
    uint8_t              lbidi = m_bPass;
#if !defined GRAPHITE2_NTRACING
    json * const dbgout = seg.getFace()->logger();
#endif

    if (lastPass == 0)
    {
        if (firstPass == lastPass && lbidi == 0xFF)
            return true;
        lastPass = m_numPasses;
    }
    if ((firstPass < lbidi || (dobidi && firstPass == lbidi)) && (lastPass >= lbidi || (dobidi && lastPass + 1 == lbidi)))
        lastPass++;
    else
        lbidi = 0xFF;

    for (size_t i = firstPass; i < lastPass; ++i)
    {
        // bidi and mirroring
        if (i == lbidi)
        {
#if !defined GRAPHITE2_NTRACING
            if (dbgout)
            {
                *dbgout << json::item << json::object
//							<< "pindex" << i   // for debugging
                            << "id"     << -1
                            << "slotsdir" << (seg.currdir() ? "rtl" : "ltr")
                            << "passdir" << (m_dir & 1 ? "rtl" : "ltr")
                            << "slots"  << json::array;
                seg.positionSlots(nullptr, seg.slots().begin(), seg.slots().end(), seg.currdir());
                for(auto & s: seg.slots())
                    *dbgout     << dslot(&seg, &s);
                *dbgout         << json::close
                            << "rules"  << json::array << json::close
                            << json::close;
            }
#endif
            if (seg.currdir() != (m_dir & 1))
                seg.reverseSlots();
            if (m_aMirror && (seg.dir() & 3) == 3)
                seg.doMirror(m_aMirror);
        --i;
        lbidi = lastPass;
        --lastPass;
        continue;
        }

#if !defined GRAPHITE2_NTRACING
        if (dbgout)
        {
            *dbgout << json::item << json::object
//						<< "pindex" << i   // for debugging
                        << "id"     << i+1
                        << "slotsdir" << (seg.currdir() ? "rtl" : "ltr")
                        << "passdir" << ((m_dir & 1) ^ m_passes[i].reverseDir() ? "rtl" : "ltr")
                        << "slots"  << json::array;
            seg.positionSlots(nullptr, seg.slots().begin(), seg.slots().end(), seg.currdir());
            for(auto & s: seg.slots())
                *dbgout     << dslot(&seg, &s);
            *dbgout         << json::close;
        }
#endif

        // test whether to reorder, prepare for positioning
        bool reverse = (lbidi == 0xFF) && (seg.currdir() != ((m_dir & 1) ^ m_passes[i].reverseDir()));
        if ((i >= 32 || (seg.passBits() & (1 << i)) == 0 || m_passes[i].collisionLoops())
                && !m_passes[i].runGraphite(m, ctxt, reverse))
            return false;
        // only subsitution passes can change segment length, cached subsegments are short for their text
        if (m.status() != vm::Machine::finished
            || (seg.slotCount() && seg.slotCount() > maxSize))
            return false;
    }
    return true;
}
