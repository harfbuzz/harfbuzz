// SPDX-License-Identifier: MIT
// Copyright (C) 2005 www.thanlwinsoft.org, SIL International
/*
Responsibility: Keith Stribley, Martin Hosken
Description:
A simple console app that creates a segment using FileFont and dumps a
diagnostic table of the resulting glyph vector to the console.
If graphite has been built with -DTRACING then it will also produce a
diagnostic log of the segment creation in grSegmentLog.txt
*/

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <climits>
#include <iomanip>
#include <cstring>

#include "UtfCodec.h"

#include "graphite2/Types.h"
#include "graphite2/Segment.h"
#include "graphite2/Log.h"

class Gr2TextSrc
{

public:
    Gr2TextSrc(const uint32_t* base, size_t len) : m_buff(base), m_len(len) { }
    gr_encform utfEncodingForm() const { return gr_utf32; }
    size_t getLength() const { return m_len; }
    const void* get_utf_buffer_begin() const { return m_buff; }

private:
    const uint32_t* m_buff;
    size_t m_len;
};

#ifndef HAVE_STRTOF
float strtof(char * text, char ** /*ignore*/)
{
  return static_cast<float>(atof(text));
}
#endif

#ifndef HAVE_STRTOL
long strtol(char * text, char ** /*ignore*/)
{
  return atol(text);
}
#endif

class Parameters
{
public:
    Parameters();
    ~Parameters();
    void clear();
    void closeLog();
    bool loadFromArgs(int argc, char *argv[]);
    int testFileFont() const;
    gr_feature_val* parseFeatures(const gr_face * face) const;
    void printFeatures(const gr_face * face) const;
public:
    const char * fileName;
    const char * features;
    float pointSize;
    int dpi;
    bool lineStart;
    bool lineEnd;
    bool ws;
    bool rtl;
    bool useLineFill;
    bool noprint;
    int useCodes;
    bool autoCodes;
    int justification;
    float width;
    int textArgIndex;
    unsigned int * pText32;
    size_t charLength;
    size_t offset;
    FILE * log;
    char * trace;
    char * alltrace;
    int codesize;
    gr_face_options opts;

private :  //defensive since log should not be copied
    Parameters(const Parameters&);
    Parameters& operator=(const Parameters&);
};

Parameters::Parameters()
{
  log = stdout ;
  clear();
}


Parameters::~Parameters()
{
  free(pText32);
  pText32 = NULL;
  closeLog();
}

void Parameters::clear()
{
    closeLog() ;
    fileName = "";
    features = nullptr;
    pointSize = 12.0f;
    dpi = 72;
    lineStart = false;
    lineEnd = false;
    rtl = false;
    ws = false;
    useLineFill = false;
    useCodes = 0;
    autoCodes = false;
    noprint = false;
    justification = 0;
    width = 100.0f;
    pText32 = NULL;
    textArgIndex = 0;
    charLength = 0;
    codesize = 4;
    offset = 0;
    log = stdout;
    trace = NULL;
    alltrace = NULL;
    opts = gr_face_preloadAll;
}


void Parameters::closeLog()
{
  if (log==stdout)
    return ;

  fclose(log);
  log = stdout;
}

int lookup(size_t *map, size_t val);

namespace gr2 = graphite2;

template <typename utf>
size_t convertUtf(const void * src, unsigned int * & dest)
{
    dest = static_cast<unsigned int *>(malloc(sizeof(unsigned int)*(strlen(reinterpret_cast<const char *>(src))+1)));
    if (!dest)
    	return 0;

    typename utf::const_iterator ui = src;
    unsigned int * out = dest;
	while ((*out = *ui) != 0 && !ui.error())
	{
		++ui;
		++out;
	}

	if (ui.error())
	{
		free(dest);
		dest = 0;
		return size_t(-1);
	}

	return (out-dest);
}


bool Parameters::loadFromArgs(int argc, char *argv[])
{
    int mainArgOffset = 0;
    pText32 = NULL;
    features = NULL;
    log = stdout;
    codesize = 4;
    bool argError = false;
    char* pText = NULL;
    typedef enum
    {
        NONE,
        POINT_SIZE,
        DPI,
        JUSTIFY,
        LINE_START,
        LINE_END,
        LINE_FILL,
        CODES,
        AUTOCODES,
        FEAT,
        LOG,
        TRACE,
        ALLTRACE,
        SIZE
    } TestOptions;
    TestOptions option = NONE;
    char * pIntEnd = NULL;
    char * pFloatEnd = NULL;
    long lTestSize = 0;
    float fTestSize = 0.0f;
    for (int a = 1; a < argc; a++)
    {
        switch (option)
        {
        case DPI:
            pIntEnd = NULL;
            lTestSize = strtol(argv[a],&pIntEnd, 10);
            if (lTestSize > 0 && lTestSize < INT_MAX && lTestSize != LONG_MAX)
            {
                dpi = lTestSize;
            }
            else
            {
                fprintf(stderr,"Invalid dpi %s\n", argv[a]);
            }
            option = NONE;
            break;
        case POINT_SIZE:
            pFloatEnd = NULL;
            fTestSize = strtof(argv[a],&pFloatEnd);
            // what is a reasonable maximum here
            if (fTestSize > 0 && fTestSize < 5000.0f)
            {
                pointSize = fTestSize;
            }
            else
            {
                fprintf(stderr,"Invalid point size %s\n", argv[a]);
                argError = true;
            }
            option = NONE;
            break;
        case JUSTIFY:
            pIntEnd = NULL;
            justification = strtol(argv[a], &pIntEnd, 10);
            if (justification <= 0)
            {
                fprintf(stderr, "Justification value must be > 0 but was %s\n", argv[a]);
                justification = 0;
            }
            option = NONE;
            break;
        case LINE_FILL:
            pFloatEnd = NULL;
            fTestSize = strtof(argv[a],&pFloatEnd);
            // what is a good max width?
            if (fTestSize > 0 && fTestSize < 10000)
            {
                width = fTestSize;
            }
            else
            {
                fprintf(stderr,"Invalid line width %s\n", argv[a]);
                argError = true;
            }
            option = NONE;
            break;
        case FEAT:
                features = argv[a];
                option = NONE;
                break;
        case LOG:
            closeLog();
            log = fopen(argv[a], "w");
            if (log == NULL)
            {
                fprintf(stderr,"Failed to open %s\n", argv[a]);
                log = stdout;
            }
            option = NONE;
            break;
        case TRACE:
            trace = argv[a];
            option = NONE;
            break;
        case ALLTRACE:
            alltrace = argv[a];
            option = NONE;
            break;
        case SIZE :
            pIntEnd = NULL;
            codesize = strtol(argv[a],&pIntEnd, 10);
            option = NONE;
            break;
        default:
            option = NONE;
            if (argv[a][0] == '-')
            {
                if (strcmp(argv[a], "-pt") == 0)
                {
                    option = POINT_SIZE;
                }
                else if (strcmp(argv[a], "-dpi") == 0)
                {
                    option = DPI;
                }
                else if (strcmp(argv[a], "-ls") == 0)
                {
                    option = NONE;
                    lineStart = true;
                }
                else if (strcmp(argv[a], "-le") == 0)
                {
                    option = NONE;
                    lineEnd = true;
                }
                else if (strcmp(argv[a], "-le") == 0)
                {
                    option = NONE;
                    lineEnd = true;
                }
                else if (strcmp(argv[a], "-rtl") == 0)
                {
                    option = NONE;
                    rtl = true;
                }
                else if (strcmp(argv[a], "-ws") == 0)
                {
                    option = NONE;
                    ws = true;
                }
                else if (strcmp(argv[a], "-feat") == 0)
                {
                    option = FEAT;
                }
                else if (strcmp(argv[a], "-bytes") == 0)
                {
                    option = SIZE;
                }
                else if (strcmp(argv[a], "-noprint") == 0)
                {
                    noprint = true;
                }
                else if (strcmp(argv[a], "-codes") == 0)
                {
                    option = NONE;
                    useCodes = 4;
                    // must be less than argc
                    //pText32 = new unsigned int[argc];
                    pText32 = (unsigned int *)malloc(sizeof(unsigned int) * argc);
                    fprintf(log, "Text codes\n");
                }
                else if (strcmp(argv[a], "-auto") == 0)
                {
                    const unsigned kCodeLimit = 0x1000;
                    charLength = kCodeLimit - 1;
                    pText32 = (unsigned int *)malloc(sizeof(unsigned int) * kCodeLimit);
                    unsigned int i;
                    for (i = 1; i < kCodeLimit; ++i)
                        pText32[i - 1] = i;
                    pText32[charLength] = 0;
                    autoCodes = true;
                    option = NONE;
                }
                else if (strcmp(argv[a], "-linefill") == 0)
                {
                    option = LINE_FILL;
                    useLineFill = true;
                }
                else if (strcmp(argv[a], "-j") == 0)
                {
                    option = JUSTIFY;
                }
                else if (strcmp(argv[a], "-log") == 0)
                {
                    option = LOG;
                }
                else if (strcmp(argv[a], "-trace") == 0)
                {
                    option = TRACE;
                }
                else if (strcmp(argv[a], "-alltrace") == 0)
                {
                    option = ALLTRACE;
                }
                else if (strcmp(argv[a], "-demand") == 0)
                {
                    option = NONE;
                    opts = gr_face_default;
                }
                else
                {
                    argError = true;
                    fprintf(stderr,"Unknown option %s\n",argv[a]);
                }
            }
            else if (mainArgOffset == 0)
            {
                fileName = argv[a];
                mainArgOffset++;
            }
            else if (useCodes)
            {
                pIntEnd = NULL;
                mainArgOffset++;
                unsigned int code = strtol(argv[a],&pIntEnd, 16);
                if (code > 0)
                {
// convert text to utfOut using iconv because its easier to debug string placements
                    assert(pText32);
                    pText32[charLength++] = code;
                    if (charLength % 10 == 0)
                        fprintf(log, "%4x\n",code);
                    else
                        fprintf(log, "%4x\t",code);
                }
                else
                {
                    fprintf(stderr,"Invalid dpi %s\n", argv[a]);
                }
            }
            else if (mainArgOffset == 1)
            {
                mainArgOffset++;
                pText = argv[a];
                textArgIndex = a;
            }
            else
            {
                argError = true;
                fprintf(stderr,"too many arguments %s\n",argv[a]);
            }
            break;
        }
    }
    if (mainArgOffset < 1) argError = true;
    else if (mainArgOffset > 1)
    {
        if (!autoCodes && !useCodes && pText != NULL)
        {
            charLength = convertUtf<gr2::utf8>(pText, pText32);
            if (!pText32)
            {
            	if (charLength == ~0)
            		perror("decoding utf-8 data failed");
            	else
            		perror("insufficent memory for text buffer");
            }
            fprintf(log, "String has %d characters\n", (int)charLength);
            size_t ci;
            for (ci = 0; ci < 10 && ci < charLength; ci++)
            {
                    fprintf(log, "%d\t", (int)ci);
            }
            fprintf(log, "\n");
            for (ci = 0; ci < charLength; ci++)
            {
                    fprintf(log, "%04x\t", (int)ci);
                    if (((ci + 1) % 10) == 0)
                        fprintf(log, "\n");
            }
            fprintf(log, "\n");
        }
        else
        {
            assert(pText32);
            pText32[charLength] = 0;
            fprintf(log, "\n");
        }
    }
    return (argError) ? false : true;
}

typedef uint32_t tag_t;

void Parameters::printFeatures(const gr_face * face) const
{
    uint16_t numFeatures = gr_face_n_fref(face);
    fprintf(log, "%d features\n", numFeatures);
    uint16_t langId = 0x0409;
    for (uint16_t i = 0; i < numFeatures; i++)
    {
        const gr_feature_ref * f = gr_face_fref(face, i);
        uint32_t length = 0;
        char * label = reinterpret_cast<char *>(gr_fref_label(f, &langId, gr_utf8, &length));
        const tag_t featId = gr_fref_id(f);
        if (label)
            if ((char(featId >> 24) >= 0x20 && char(featId >> 24) < 0x7F) &&
                (char(featId >> 16) >= 0x20 && char(featId >> 16) < 0x7F) &&
                (char(featId >> 8)  >= 0x20 && char(featId >> 8)  < 0x7F) &&
                (char(featId)       >= 0x20 && char(featId)       < 0x7F))
            {
                fprintf(log, "%d %c%c%c%c %s\n", featId, featId >> 24, featId >> 16, featId >> 8, featId, label);
            }
            else
            {
                fprintf(log, "%d %s\n", featId, label);
            }
        else
            fprintf(log, "%d\n", featId);
        gr_label_destroy(reinterpret_cast<void*>(label));
        uint16_t numSettings = gr_fref_n_values(f);
        for (uint16_t j = 0; j < numSettings; j++)
        {
            int16_t value = gr_fref_value(f, j);
            label = reinterpret_cast<char *>(gr_fref_value_label
                (f, j, &langId, gr_utf8, &length));
            fprintf(log, "\t%d\t%s\n", value, label);
            gr_label_destroy(reinterpret_cast<void*>(label));
        }
    }
    uint16_t numLangs = gr_face_n_languages(face);
    fprintf(log, "Feature Languages:");
    for (uint16_t i = 0; i < numLangs; i++)
    {
    	const tag_t lang_id = gr_face_lang_by_index(face, i);
        fprintf(log, "\t");
        for (size_t j = 4; j; --j)
        {
        	const unsigned char c = lang_id >> (j*8-8);
            if ((c >= 0x20) && (c < 0x80))
                fprintf(log, "%c", c);
        }
    }
    fprintf(log, "\n");
}

gr_feature_val * Parameters::parseFeatures(const gr_face * face) const
{
    gr_feature_val * featureList = NULL;
    const char * pLang = NULL;
    tag_t lang_id = 0;
    if (features && (pLang = strstr(features, "lang=")))
    {
        pLang += 5;
        for (int i = 4; i; --i)
        {
            lang_id <<= 8;
        	if (!*pLang || *pLang == '0' || *pLang == '&') continue;
        	lang_id |= *pLang++;
        }
    }
    featureList = gr_face_featureval_for_lang(face, lang_id);
    if (!features || strlen(features) == 0)
        return featureList;
    size_t featureLength = strlen(features);
    const char * name = features;
    const char * valueText = NULL;
    size_t nameLength = 0;
    int32_t value = 0;
    const gr_feature_ref* ref = NULL;
    tag_t feat_id = 0;
    for (size_t i = 0; i < featureLength; i++)
    {
        switch (features[i])
        {
            case ',':
            case '&':
                assert(valueText);
                value = atoi(valueText);
                if (ref)
                {
                    gr_fref_set_feature_value(ref, value, featureList);
                    ref = NULL;
                }
                valueText = NULL;
                name = features + i + 1;
                nameLength = 0;
                feat_id = 0;
                break;
            case '=':
                if (nameLength <= 4)
                {
                    ref = gr_face_find_fref(face, feat_id);
                }
                if (!ref)
                {
                    assert(name);
                    feat_id = atoi(name);
                    ref = gr_face_find_fref(face, feat_id);
                }
                valueText = features + i + 1;
                name = NULL;
                break;
            default:
                if (valueText == NULL)
                {
                    if (nameLength < 4)
                    	feat_id = feat_id << 8 | features[i];
                }
                break;
        }
        if (ref)
        {
            assert(valueText);
            value = atoi(valueText);
            gr_fref_set_feature_value(ref, value, featureList);
            if (feat_id > 0x20000000)
            {
                fprintf(log, "%c%c%c%c=%d\n", feat_id >> 24, feat_id >> 16, feat_id >> 8, feat_id, value);
            }
            else
                fprintf(log, "%u=%d\n", feat_id, value);
            ref = NULL;
        }
    }
    return featureList;
}

int Parameters::testFileFont() const
{
    int returnCode = 0;
//    try
    {
        gr_face *face = NULL;
        FILE * testfile = fopen(fileName, "rb");
        if (!testfile)
        {
            fprintf(stderr, "Unable to open font file\n");
            return 4;
        }
        else
            fclose(testfile);

        if (alltrace) gr_start_logging(NULL, alltrace);
        face = gr_make_file_face(fileName, opts);

        // use the -trace option to specify a file
    	if (trace)	gr_start_logging(face, trace);

        if (!face)
        {
            fprintf(stderr, "Invalid font, failed to read or parse tables\n");
            return 3;
        }
        if (charLength == 0)
        {
            printFeatures(face);
            gr_stop_logging(face);
            gr_face_destroy(face);
            return 0;
        }

        gr_font *sizedFont = gr_make_font(pointSize * dpi / 72, face);
        gr_feature_val * featureList = NULL;
        Gr2TextSrc textSrc(pText32, charLength);
        gr_segment* pSeg = NULL;
        if (features)
            featureList = parseFeatures(face);
        if (codesize == 2)
        {
            unsigned short *pText16 = (unsigned short *)malloc((textSrc.getLength() * 2 + 1) * sizeof(unsigned short));
            gr2::utf16::iterator ui = pText16;
            unsigned int *p = pText32;
            for (unsigned int i = 0; i < textSrc.getLength(); ++i)
            {
                *ui = *p++;
                ui++;
            }
            *ui = 0;
            pSeg = gr_make_seg(sizedFont, face, 0, features ? featureList : NULL, (gr_encform)codesize, pText16, textSrc.getLength(), rtl ? 1 : 0);
        }
        else if (codesize == 1)
        {
            unsigned char *pText8 = (unsigned char *)malloc((textSrc.getLength() + 1) * 4);
            gr2::utf8::iterator ui = pText8;
            unsigned int *p = pText32;
            for (unsigned int i = 0; i < textSrc.getLength(); ++i)
            {
                *ui = *p++;
                ui++;
            }
            *ui = 0;
            pSeg = gr_make_seg(sizedFont, face, 0, features ? featureList : NULL, (gr_encform)codesize, pText8, textSrc.getLength(), rtl ? 1 : 0);
            free(pText8);
        }
        else
            pSeg = gr_make_seg(sizedFont, face, 0, features ? featureList : NULL, textSrc.utfEncodingForm(),
                textSrc.get_utf_buffer_begin(), textSrc.getLength(), rtl ? 1 : 0);

        if (pSeg && !noprint)
        {
            int i = 0;
            float advanceWidth;
    #ifndef NDEBUG
            int numSlots = gr_seg_n_slots(pSeg);
    #endif
    //        size_t *map = new size_t [seg.length() + 1];
            if (justification > 0)
                advanceWidth = gr_seg_justify(pSeg, gr_seg_first_slot(pSeg), sizedFont, gr_seg_advance_X(pSeg) * justification / 100., gr_justCompleteLine, NULL, NULL);
            else
                advanceWidth = gr_seg_advance_X(pSeg);
            size_t *map = (size_t*)malloc((gr_seg_n_slots(pSeg) + 1) * sizeof(size_t));
            for (const gr_slot* slot = gr_seg_first_slot(pSeg); slot; slot = gr_slot_next_in_segment(slot), ++i)
            { map[i] = (size_t)slot; }
            map[i] = 0;
            fprintf(log, "Segment length: %d\n", gr_seg_n_slots(pSeg));
            fprintf(log, "pos  gid   attach\t     x\t     y\tins bw\t  chars\t\tUnicode\t");
            fprintf(log, "\n");
            i = 0;
            for (const gr_slot* slot = gr_seg_first_slot(pSeg); slot; slot = gr_slot_next_in_segment(slot), ++i)
            {
                // consistency check for last slot
                assert((i + 1 < numSlots) || (slot == gr_seg_last_slot(pSeg)));
                float orgX = gr_slot_origin_X(slot);
                float orgY = gr_slot_origin_Y(slot);
                const gr_char_info *cinfo = gr_seg_cinfo(pSeg, gr_slot_original(slot));
                fprintf(log, "%02d  %4d %3d@%d,%d\t%6.1f\t%6.1f\t%2d%4d\t%3d %3d\t",
                        i, gr_slot_gid(slot), lookup(map, (size_t)gr_slot_attached_to(slot)),
                        gr_slot_attr(slot, pSeg, gr_slatAttX, 0),
                        gr_slot_attr(slot, pSeg, gr_slatAttY, 0), orgX, orgY, gr_slot_can_insert_before(slot) ? 1 : 0,
                        cinfo ? gr_cinfo_break_weight(cinfo) : 0, gr_slot_before(slot), gr_slot_after(slot));

                if (pText32 != NULL && gr_slot_before(slot) + offset < charLength
                                    && gr_slot_after(slot) + offset < charLength)
                {
                    fprintf(log, "%7x\t%7x",
                        pText32[gr_slot_before(slot) + offset],
                        pText32[gr_slot_after(slot) + offset]);
                }
                fprintf(log, "\n");
            }
            assert(i == numSlots);
            // assign last point to specify advance of the whole array
            // position arrays must be one bigger than what countGlyphs() returned
            fprintf(log, "Advance width = %6.1f\n", advanceWidth);
            unsigned int numchar = gr_seg_n_cinfo(pSeg);
            fprintf(log, "\nChar\tUnicode\tBefore\tAfter\tBase\n");
            for (unsigned int j = 0; j < numchar; j++)
            {
                const gr_char_info *c = gr_seg_cinfo(pSeg, j);
                fprintf(log, "%d\t%04X\t%d\t%d\t%d\n", j, gr_cinfo_unicode_char(c), gr_cinfo_before(c), gr_cinfo_after(c), int(gr_cinfo_base(c)));
            }
            free(map);
        }
        if (pSeg)
            gr_seg_destroy(pSeg);
        if (featureList) gr_featureval_destroy(featureList);
        gr_font_destroy(sizedFont);
        if (trace) gr_stop_logging(face);
        gr_face_destroy(face);
        if (alltrace) gr_stop_logging(NULL);
    }
    return returnCode;
}

int lookup(size_t *map, size_t val)
{
    int i = 0;
    for ( ; map[i] != val && map[i]; i++) {}
    return map[i] ? i : -1;
}

int main(int argc, char *argv[])
{

    Parameters parameters;

    if (!parameters.loadFromArgs(argc, argv))
    {
        fprintf(stderr,"Usage: %s [options] fontfile utf8text \n",argv[0]);
        fprintf(stderr,"Options: (default in brackets)\n");
        fprintf(stderr,"-dpi d\tDots per Inch (72)\n");
        fprintf(stderr,"-pt d\tPoint size (12)\n");
        fprintf(stderr,"-codes\tEnter text as hex code points instead of utf8 (false)\n");
        fprintf(stderr,"\te.g. %s font.ttf -codes 1000 102f\n",argv[0]);
        fprintf(stderr,"-auto\tAutomatically generate a test string of all codes 1-0xFFF\n");
        fprintf(stderr,"-noprint\tDon't print results\n");
        //fprintf(stderr,"-ls\tStart of line = true (false)\n");
        //fprintf(stderr,"-le\tEnd of line = true (false)\n");
        fprintf(stderr,"-rtl\tRight to left = true (false)\n");
        fprintf(stderr,"-j percentage\tJustify to percentage of string width\n");
        //fprintf(stderr,"-ws\tAllow trailing whitespace = true (false)\n");
        //fprintf(stderr,"-linefill w\tuse a LineFillSegment of width w (RangeSegment)\n");
        fprintf(stderr,"\nIf a font, but no text is specified, then a list of features will be shown.\n");
        fprintf(stderr,"-feat f=g\tSet feature f to value g. Separate multiple features with ,\n");
        fprintf(stderr,"-log out.log\tSet log file to use rather than stdout\n");
        fprintf(stderr,"-trace trace.json\tDefine a file for the JSON trace log\n");
        fprintf(stderr,"-demand\tDemand load glyphs and cmap cache\n");
        fprintf(stderr,"-bytes\tword size for character transfer [1,2,4] defaults to 4\n");
        return 1;
    }
    return parameters.testFileFont();
}
