// SPDX-License-Identifier: MIT OR MPL-2.0 OR GPL-2.0-or-later-2.1-or-later
// Copyright 2010, SIL International, All rights reserved.
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef __GNUC__
#include <unistd.h>
#ifndef __linux__
#include <sys/time.h>
#endif
#endif

#ifdef WIN32
#include <windows.h>
#endif

#include "RendererOptions.h"
#include "Renderer.h"
#include "RenderedLine.h"
#include "FeatureParser.h"

#include "Gr2Renderer.h"
#include "graphite2/Log.h"

const size_t NUM_RENDERERS = 6;

class CompareRenderer
{
public:
    CompareRenderer(const char * testFile, Renderer** renderers, bool verbose)
        : m_fileBuffer(NULL), m_fileLength(0), m_numLines(0), m_lineOffsets(NULL),
        m_renderers(renderers), m_verbose(verbose), m_cfMask(ALL_DIFFERENCE_TYPES)
    {
        // read the file into memory for fast access
        struct stat fileStat;
        if (stat(testFile, &fileStat) == 0)
        {
            FILE * file = fopen(testFile, "rb");
            if (file)
            {
                m_fileBuffer = new char[fileStat.st_size];
                if (m_fileBuffer)
                {
                    m_fileLength = fread(m_fileBuffer, 1, fileStat.st_size, file);
                    assert(m_fileLength == fileStat.st_size);
                    countLines();
                    findLines();
                    for (size_t r = 0; r < NUM_RENDERERS; r++)
                    {
                        if (m_renderers[r])
                        {
                            m_lineResults[r] = new RenderedLine[m_numLines];
                        }
                        else
                        {
                            m_lineResults[r] = NULL;
                        }
                        m_elapsedTime[r] = 0.0f;
                        m_glyphCount[r] = 0;
                    }
                }
                fclose(file);
            }
            else
            {
                fprintf(stderr, "Error opening file %s\n", testFile);
            }
        }
        else
        {
            fprintf(stderr, "Error stating file %s\n", testFile);
            for (size_t r = 0; r < NUM_RENDERERS; r++)
            {
                m_lineResults[r] = NULL;
                m_elapsedTime[r] = 0.0f;
            }
        }
    }

    ~CompareRenderer()
    {
        delete [] m_fileBuffer;
        m_fileBuffer = NULL;
        for (size_t i = 0; i < NUM_RENDERERS; i++)
        {
            if (m_lineResults[i]) delete [] m_lineResults[i];
            m_lineResults[i] = NULL;
        }
        if (m_lineOffsets) delete [] m_lineOffsets;
        m_lineOffsets = NULL;
    }

    void runTests(FILE * log, int repeat = 1)
    {
        for (size_t r = 0; r < NUM_RENDERERS; r++)
        {
            if (m_renderers[r])
            {
                for (int i = 0; i < repeat; i++)
                    m_elapsedTime[r] += runRenderer(*m_renderers[r], m_lineResults[r], m_glyphCount[r], log);
                fprintf(stdout, "Ran %s in %fs (%lu glyphs)\n", m_renderers[r]->name(), m_elapsedTime[r], m_glyphCount[r]);
            }
        }
    }
    int compare(float tolerance, float fractionalTolerance, FILE * log)
    {
        int status = IDENTICAL;
        for (size_t i = 0; i < NUM_RENDERERS; i++)
        {
            for (size_t j = i + 1; j < NUM_RENDERERS; j++)
            {
                if (m_renderers[i] == NULL || m_renderers[j] == NULL) continue;
                if (m_lineResults[i] == NULL || m_lineResults[j] == NULL) continue;
                fprintf(log, "Comparing %s with %s\n", m_renderers[i]->name(), m_renderers[j]->name());
                for (size_t line = 0; line < m_numLines; line++)
                {
                    LineDifference ld = m_lineResults[i][line].compare(m_lineResults[j][line], tolerance, fractionalTolerance);
                    ld = (LineDifference)(m_cfMask & ld);
                    if (ld)
                    {
                        fprintf(log, "Line %u %s\n", (unsigned int)line, DIFFERENCE_DESC[ld]);
                        for (size_t c = m_lineOffsets[line]; c < m_lineOffsets[line+1]; c++)
                        {
                            fprintf(log, "%c", m_fileBuffer[c]);
                        }
                        fprintf(log, "\n");
                        m_lineResults[i][line].dump(log);
                        fprintf(log, "%s\n", m_renderers[i]->name());
                        m_lineResults[j][line].dump(log);
                        fprintf(log, "%s\n", m_renderers[j]->name());
                        status |= ld;
                    }
                }
            }
        }
        return status;
    }
    void setDifferenceMask(LineDifference m) { m_cfMask = m; }
protected:
    float runRenderer(Renderer & renderer, RenderedLine * pLineResult, unsigned long & glyphCount, FILE *log)
    {
        glyphCount = 0;
        unsigned int i = 0;
        const char * pLine = m_fileBuffer;
#ifdef __linux__
        struct timespec startTime;
        struct timespec endTime;
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &startTime);
#else
#ifdef WIN32
        LARGE_INTEGER counterFreq;
        LARGE_INTEGER startCounter;
        LARGE_INTEGER endCounter;
        if (!QueryPerformanceFrequency(&counterFreq))
            fprintf(stderr, "Warning no high performance counter available\n");
        QueryPerformanceCounter(&startCounter);
#else
        struct timeval startTime;
        struct timeval endTime;
        gettimeofday(&startTime,0);
#endif
#endif
        // check for CRLF
        int lfLength = 1;
        if ((m_numLines > 1) && (m_lineOffsets[1] > 2) && (m_fileBuffer[m_lineOffsets[1]-2] == '\r'))
            lfLength = 2;
        if (m_verbose)
        {
            fprintf(log, "[\n");
            while (i < m_numLines)
            {
                size_t lineLength = m_lineOffsets[i+1] - m_lineOffsets[i] - lfLength;
                pLine = m_fileBuffer + m_lineOffsets[i];
                renderer.renderText(pLine, lineLength, pLineResult + i, log);
                pLineResult[i].dump(log);
                glyphCount += pLineResult[i].numGlyphs();
                ++i;
            }
            fprintf(log, "]\n");
        }
        else
        {
            while (i < m_numLines)
            {
                size_t lineLength = m_lineOffsets[i+1] - m_lineOffsets[i] - lfLength;
                pLine = m_fileBuffer + m_lineOffsets[i];
                renderer.renderText(pLine, lineLength, pLineResult + i, log);
                glyphCount += pLineResult[i].numGlyphs();
                ++i;
            }
        }
        float elapsed = 0.;
#ifdef __linux__
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &endTime);
        long deltaSeconds = endTime.tv_sec - startTime.tv_sec;
        long deltaNs = endTime.tv_nsec - startTime.tv_nsec;
        if (deltaNs < 0)
        {
            deltaSeconds -= 1;
            deltaNs += 1000000000;
        }
        elapsed = deltaSeconds + deltaNs / 1000000000.0f;
#else
#ifdef WIN32
        QueryPerformanceCounter(&endCounter);
        elapsed = (endCounter.QuadPart - startCounter.QuadPart) / static_cast<float>(counterFreq.QuadPart);
#else
        gettimeofday(&endTime,0);
        long deltaSeconds = endTime.tv_sec - startTime.tv_sec;
        long deltaUs = endTime.tv_usec - startTime.tv_usec;
        if (deltaUs < 0)
        {
            deltaSeconds -= 1;
            deltaUs += 1000000;
        }
        elapsed = deltaSeconds + deltaUs / 1000000.0f;
#endif
#endif
        return elapsed;
    }

    size_t countLines()
    {
        for (size_t i = 0; i < m_fileLength; i++)
        {
            if (m_fileBuffer[i] == '\n')
            {
                ++m_numLines;
            }
        }
        return m_numLines;
    }
    void findLines()
    {
        m_lineOffsets = new size_t[m_numLines+1];
        m_lineOffsets[0] = 0;
        int line = 0;
        for (size_t i = 0; i < m_fileLength; i++)
        {
            if (m_fileBuffer[i] == '\n')
            {
                m_lineOffsets[++line] = i + 1;
            }
            if (m_fileBuffer[i] > 0 && m_fileBuffer[i] < 32)
                m_fileBuffer[i] = 32;
        }
        m_lineOffsets[m_numLines] = m_fileLength;
    }
private:
    char * m_fileBuffer;
    size_t m_fileLength;
    size_t m_numLines;
    size_t * m_lineOffsets;
    Renderer** m_renderers;
    RenderedLine * m_lineResults[NUM_RENDERERS];
    float m_elapsedTime[NUM_RENDERERS];
    unsigned long m_glyphCount[NUM_RENDERERS];
    bool m_verbose;
    LineDifference m_cfMask;
};


int main(int argc, char ** argv)
{
    if (!parseOptions(argc, argv) ||
        !rendererOptions[OptFontFile].exists() ||
        !rendererOptions[OptTextFile].exists() ||
        !rendererOptions[OptSize].exists())
    {
        fprintf(stderr, "Usage:\n%s [options] -t utf8.txt -f font.ttf -s 12\n", argv[0]);
        fprintf(stderr, "Options:\n");
        showOptions();
        return -1;
    }

    const char * textFile = rendererOptions[OptTextFile].get(argv);
    const char * fontFile = rendererOptions[OptFontFile].get(argv);
    int fontSize = rendererOptions[OptSize].getInt(argv);
    FILE * log = stdout;
    if (rendererOptions[OptLogFile].exists())
    {
        log = fopen(rendererOptions[OptLogFile].get(argv), "wb");
        if (!log)
        {
            fprintf(stderr, "Failed to open log file %s\n",
                    rendererOptions[OptLogFile].get(argv));
            return -2;
        }
    }


    Renderer* renderers[NUM_RENDERERS] = {NULL, NULL, NULL, NULL, NULL};
    FeatureParser * featureSettings = NULL;
    FeatureParser * altFeatureSettings = NULL;
    bool direction = rendererOptions[OptRtl].exists();
    const std::string traceLogPath = rendererOptions[OptTrace].exists() ? rendererOptions[OptTrace].get(argv) : std::string();
	  Gr2Face face(fontFile, traceLogPath, rendererOptions[OptDemand].get(argv));


    if (rendererOptions[OptFeatures].exists())
    {
        featureSettings = new FeatureParser(rendererOptions[OptFeatures].get(argv));
    }

    if (rendererOptions[OptQuiet].exists())
    {
    	fclose(stderr);
    }

    if (rendererOptions[OptAlternativeFont].exists())
    {
        if (rendererOptions[OptAltFeatures].exists())
        {
            altFeatureSettings = new FeatureParser(rendererOptions[OptAltFeatures].get(argv));
        }
        else
        {
            altFeatureSettings = featureSettings;
        }
        const char * altFontFile = rendererOptions[OptAlternativeFont].get(argv);
        if (rendererOptions[OptGraphite2].exists())
        {
            std::string altTraceLogPath = traceLogPath;
            altTraceLogPath.insert(traceLogPath.find_last_of('.'), ".alt");
        	Gr2Face altFace(altFontFile, altTraceLogPath, rendererOptions[OptDemand].get(argv));

            renderers[0] = new Gr2Renderer(face, fontSize, direction, featureSettings);
            renderers[1] = new Gr2Renderer(altFace, fontSize, direction, altFeatureSettings);
        }
    }
    else
    {
        if (rendererOptions[OptGraphite2].exists())
            renderers[0] = new Gr2Renderer(face, fontSize, direction, featureSettings);

        if (rendererOptions[OptGraphite2s].exists())
        {
        	Gr2Face uncached(fontFile,
        			std::string(traceLogPath).insert(traceLogPath.find_last_of('.'), ".uncached"), rendererOptions[OptDemand].get(argv));
            renderers[1] = new Gr2Renderer(uncached, fontSize, direction, featureSettings);
        }
    }

    if (renderers[0] == NULL && renderers[1] == NULL)
    {
        fprintf(stderr, "Please specify at least 1 renderer\n");
        showOptions();
        if (rendererOptions[OptLogFile].exists()) fclose(log);
        return -3;
    }

    CompareRenderer compareRenderers(textFile, renderers, rendererOptions[OptVerbose].exists());
    if (rendererOptions[OptRepeat].exists())
        compareRenderers.runTests(log, rendererOptions[OptRepeat].getInt(argv));
    else
        compareRenderers.runTests(log);
    // set compare options
    if (rendererOptions[OptIgnoreGlyphIdDifferences].exists())
    {
        compareRenderers.setDifferenceMask((LineDifference)(ALL_DIFFERENCE_TYPES ^ DIFFERENT_GLYPHS));
    }
    int status = 0;
    if (rendererOptions[OptCompare].exists())
        status = compareRenderers.compare(rendererOptions[OptTolerance].getFloat(argv),
            rendererOptions[OptFractionalTolerance].getFloat(argv), log);

    for (size_t i = 0; i < NUM_RENDERERS; i++)
    {
        if (renderers[i])
        {
            delete renderers[i];
            renderers[i] = NULL;
        }
    }
    if (altFeatureSettings != featureSettings)
        delete altFeatureSettings;
    delete featureSettings;
    if (rendererOptions[OptLogFile].exists()) fclose(log);

    return status;
}
