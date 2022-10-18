// SPDX-License-Identifier: MIT OR MPL-2.0 OR GPL-2.0-or-later-2.1-or-later
// Copyright 2010, SIL International, All rights reserved.
#pragma once

#include <cassert>
#include <string>

class RenderedLine;

typedef enum {
    IDENTICAL = 0,
    MORE_GLYPHS = 1,
    LESS_GLYPHS = 2,
    DIFFERENT_ADVANCE = 4,
    DIFFERENT_GLYPHS = 8,
    DIFFERENT_POSITIONS = 16,
    ALL_DIFFERENCE_TYPES = MORE_GLYPHS | LESS_GLYPHS | DIFFERENT_ADVANCE | DIFFERENT_GLYPHS | DIFFERENT_POSITIONS
} LineDifference;

const char * DIFFERENCE_DESC[] = {
    "same",//0
    "more glyphs",//1
    "less glyphs",//2
    "",
    "different advance",//4
    "","","",
    "different glyphs",//8
    "","","","","","","",
    "different positions"//16
};

class GlyphInfo
{
    public:
        GlyphInfo()
        : m_gid(0), m_x(0), m_y(0), m_firstChar(0), m_lastChar(0)
        {}
        GlyphInfo(size_t theGid, float xPos, float yPos, size_t first, size_t last)
            : m_gid(theGid), m_x(xPos), m_y(yPos), m_firstChar(first), m_lastChar(last)
        {}
        void set(size_t theGid, float xPos, float yPos, size_t first, size_t last)
        {
            m_gid = theGid;
            m_x = xPos;
            m_y = yPos;
            m_firstChar = first;
            m_lastChar = last;
        }
        float x() const { return m_x; }
        float y() const { return m_y; }
        size_t glyph() const { return m_gid; }
        LineDifference compare(GlyphInfo & cf, float tolerance, float fractional)
        {
            if (m_gid != cf.m_gid) return DIFFERENT_GLYPHS;
            if (m_x > cf.m_x + tolerance + (fractional * cf.m_x) ||
                m_x < cf.m_x - tolerance - (fractional * cf.m_x) ||
                m_y > cf.m_y + tolerance + (fractional * cf.m_y)||
                m_y < cf.m_y - tolerance - (fractional * cf.m_y))
            {
                return DIFFERENT_POSITIONS;
            }
            return IDENTICAL;
        }
        void dump(FILE * f)
        {
            //fprintf(f, "[%3u,%6.2f,%5.2f,%2u,%2u]", (unsigned int)m_gid,
            //        m_x, m_y, (unsigned int)m_firstChar, (unsigned int)m_lastChar);
            fprintf(f, "[%3u,%6.2f,%5.2f], ", (unsigned int)m_gid, m_x, m_y);
        }
    private:
        size_t m_gid;
        float m_x;
        float m_y;
        size_t m_firstChar;
        size_t m_lastChar;
};


class RenderedLine
{
    public:
        RenderedLine()
        : m_numGlyphs(0), m_advance(0), m_glyphs(NULL)
        {}
        RenderedLine(const std::string & text, size_t numGlyphs, float adv = 0.0f)
        : m_text(text), m_numGlyphs(numGlyphs), m_advance(adv), m_glyphs(new GlyphInfo[numGlyphs])
        {
        }
        ~RenderedLine()
        {
            delete [] m_glyphs;
            m_glyphs = NULL;
        }
        void setAdvance(float newAdv) { m_advance = newAdv; }
        void dump(FILE * f)
        {
            fprintf(f, "{\"%s\" : [", m_text.c_str());
            for (size_t i = 0; i < m_numGlyphs; i++)
            {
                //fprintf(f, "%2u", (unsigned int)i);
                (*this)[i].dump(f);
                // only 3 glyphs fit on 80 char line
                //if ((i + 1) % 3 == 0) fprintf(f, "\n");
            }
            fprintf(f, "(%2u,%4.3f)]}\n", (unsigned int)m_numGlyphs, m_advance);
        }
        LineDifference compare(RenderedLine & cf, float tolerance, float fractional)
        {
            if (m_numGlyphs > cf.m_numGlyphs) return MORE_GLYPHS;
            if (m_numGlyphs < cf.m_numGlyphs) return LESS_GLYPHS;
            if (m_advance > cf.m_advance + tolerance ||
                m_advance < cf.m_advance - tolerance) return DIFFERENT_ADVANCE;
            for (size_t i = 0; i < m_numGlyphs; i++)
            {
                LineDifference ld = (*this)[i].compare(cf[i], tolerance, fractional);
                if (ld) return ld;
            }
            return IDENTICAL;
        }
        GlyphInfo & operator [] (size_t i) { assert(i < m_numGlyphs); return m_glyphs[i]; }
        const GlyphInfo & operator [] (size_t i) const { assert(i < m_numGlyphs); return m_glyphs[i]; }
        unsigned long numGlyphs() const { return m_numGlyphs; }
        float advance() const { return m_advance; }
        // define placement new for windows
        bool resize(size_t newGlyphCount)
        {
            if (newGlyphCount <= m_numGlyphs)
                m_numGlyphs = newGlyphCount;
            else
            {
                GlyphInfo * newGlyphs = new GlyphInfo[newGlyphCount];
                memcpy(newGlyphs, m_glyphs, m_numGlyphs * sizeof(GlyphInfo));
                m_numGlyphs = newGlyphCount;
                delete []m_glyphs;
                m_glyphs = newGlyphs;
            }
            return true;
        }
    private:
        size_t      m_numGlyphs;
        float       m_advance;
        GlyphInfo * m_glyphs;
        std::string m_text;
};
