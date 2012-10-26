
/*
 *******************************************************************************
 *
 *   Copyright (C) 1999-2008, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *   file name:  PortableFontInstance.h
 *
 *   created on: 11/12/1999
 *   created by: Eric R. Mader
 */

#ifndef __PORTABLEFONTINSTANCE_H
#define __PORTABLEFONTINSTANCE_H

#define HB_H_IN
#include <hb-font.h>
#include <hb-blob.h>

#include "layout/LETypes.h"
#include "layout/LEFontInstance.h"
#include "letest.h"

#include "FontTableCache.h"

#include "cmaps.h"

HB_BEGIN_VISIBILITY

class PortableFontInstance : public LEFontInstance, protected FontTableCache
{
private:
    hb_face_t *fFace;

    float     fXScale;
    float     fYScale;
    le_int32  fUnitsPerEM;
    le_int32  fAscent;
    le_int32  fDescent;
    le_int32  fLeading;

    const NAMETable *fNAMETable;
    le_uint16 fNameCount;
    le_uint16 fNameStringOffset;

    CMAPMapper *fCMAPMapper;

    const HMTXTable *fHMTXTable;
    le_uint16 fNumGlyphs;
    le_uint16 fNumLongHorMetrics;

    void getMetrics();

    CMAPMapper *findUnicodeMapper();

protected:
    hb_blob_t *readFontTable(LETag tableTag) const;

public:
    PortableFontInstance(hb_face_t *face, float xScale, float yScale, LEErrorCode &status);

    virtual ~PortableFontInstance();

    virtual const void *getFontTable(LETag tableTag) const;

    virtual const char *getNameString(le_uint16 nameID, le_uint16 platform, le_uint16 encoding, le_uint16 language) const;

    virtual const LEUnicode16 *getUnicodeNameString(le_uint16 nameID, le_uint16 platform, le_uint16 encoding, le_uint16 language) const;

    virtual void deleteNameString(const char *name) const;

    virtual void deleteNameString(const LEUnicode16 *name) const;

    virtual le_int32 getUnitsPerEM() const;

    virtual le_uint32 getFontChecksum() const;

    virtual le_int32 getAscent() const;

    virtual le_int32 getDescent() const;

    virtual le_int32 getLeading() const;

    // We really want to inherit this method from the superclass, but some compilers
    // issue a warning if we don't implement it...
    virtual LEGlyphID mapCharToGlyph(LEUnicode32 ch, const LECharMapper *mapper, le_bool filterZeroWidth) const;
    
    // We really want to inherit this method from the superclass, but some compilers
    // issue a warning if we don't implement it...
    virtual LEGlyphID mapCharToGlyph(LEUnicode32 ch, const LECharMapper *mapper) const;

    virtual LEGlyphID mapCharToGlyph(LEUnicode32 ch) const;

    virtual void getGlyphAdvance(LEGlyphID glyph, LEPoint &advance) const;

    virtual le_bool getGlyphPoint(LEGlyphID glyph, le_int32 pointNumber, LEPoint &point) const;

    virtual float getXPixelsPerEm() const;

    virtual float getYPixelsPerEm() const;

    virtual float getScaleFactorX() const;

    virtual float getScaleFactorY() const;

};

HB_END_VISIBILITY

#endif
