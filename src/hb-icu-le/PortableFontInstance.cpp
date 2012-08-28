/*
 *******************************************************************************
 *
 *   Copyright (C) 1999-2008, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *   file name:  PortableFontInstance.cpp
 *
 *   created on: 11/22/1999
 *   created by: Eric R. Mader
 */

#include <stdio.h>

#include "layout/LETypes.h"
#include "layout/LEFontInstance.h"
#include "layout/LESwaps.h"

#include "PortableFontInstance.h"

#include "letest.h"
#include "sfnt.h"

#include <string.h>


PortableFontInstance::PortableFontInstance(hb_face_t *face, float xScale, float yScale, LEErrorCode &status)
    : fFace(face), fXScale(xScale), fYScale(yScale), fUnitsPerEM(0), fAscent(0), fDescent(0), fLeading(0),
      fNAMETable(NULL), fNameCount(0), fNameStringOffset(0), fCMAPMapper(NULL), fHMTXTable(NULL), fNumGlyphs(0), fNumLongHorMetrics(0)
{
    if (LE_FAILURE(status)) {
        return;
    }

    const LETag hheaTag = LE_HHEA_TABLE_TAG;
    const HHEATable *hheaTable = NULL;

    fUnitsPerEM = hb_face_get_upem (face);

    hheaTable = (HHEATable *) getFontTable(hheaTag);

    if (hheaTable == NULL) {
        status = LE_MISSING_FONT_TABLE_ERROR;
	return;
    }

    fAscent  = (le_int32) yUnitsToPoints((float) SWAPW(hheaTable->ascent));
    fDescent = (le_int32) yUnitsToPoints((float) SWAPW(hheaTable->descent));
    fLeading = (le_int32) yUnitsToPoints((float) SWAPW(hheaTable->lineGap));

    fNumLongHorMetrics = SWAPW(hheaTable->numOfLongHorMetrics);

    fCMAPMapper = findUnicodeMapper();

    if (fCMAPMapper == NULL) {
        status = LE_MISSING_FONT_TABLE_ERROR;
	return;
    }
}

PortableFontInstance::~PortableFontInstance()
{
    if (fCMAPMapper)
        delete fCMAPMapper;
}

const void *PortableFontInstance::getFontTable(LETag tableTag) const
{
    return FontTableCache::find(tableTag);
}

hb_blob_t *PortableFontInstance::readFontTable(LETag tableTag) const
{
  return hb_face_reference_table(fFace, tableTag);
}

CMAPMapper *PortableFontInstance::findUnicodeMapper()
{
    LETag cmapTag = LE_CMAP_TABLE_TAG;
    const CMAPTable *cmap = (CMAPTable *) getFontTable(cmapTag);

    if (cmap == NULL) {
        return NULL;
    }

    return CMAPMapper::createUnicodeMapper(cmap);
}

const char *PortableFontInstance::getNameString(le_uint16 nameID, le_uint16 platformID, le_uint16 encodingID, le_uint16 languageID) const
{
    if (fNAMETable == NULL) {
        LETag nameTag = LE_NAME_TABLE_TAG;
        PortableFontInstance *realThis = (PortableFontInstance *) this;

        realThis->fNAMETable = (const NAMETable *) getFontTable(nameTag);

        if (realThis->fNAMETable != NULL) {
            realThis->fNameCount        = SWAPW(realThis->fNAMETable->count);
            realThis->fNameStringOffset = SWAPW(realThis->fNAMETable->stringOffset);
        }
    }

    for(le_int32 i = 0; i < fNameCount; i += 1) {
        const NameRecord *nameRecord = &fNAMETable->nameRecords[i];
        
        if (SWAPW(nameRecord->platformID) == platformID && SWAPW(nameRecord->encodingID) == encodingID &&
            SWAPW(nameRecord->languageID) == languageID && SWAPW(nameRecord->nameID) == nameID) {
            char *name = ((char *) fNAMETable) + fNameStringOffset + SWAPW(nameRecord->offset);
            le_uint16 length = SWAPW(nameRecord->length);
            char *result = NEW_ARRAY(char, length + 2);

            ARRAY_COPY(result, name, length);
            result[length] = result[length + 1] = 0;

            return result;
        }
    }

    return NULL;
}

const LEUnicode16 *PortableFontInstance::getUnicodeNameString(le_uint16 nameID, le_uint16 platformID, le_uint16 encodingID, le_uint16 languageID) const
{
    if (fNAMETable == NULL) {
        LETag nameTag = LE_NAME_TABLE_TAG;
        PortableFontInstance *realThis = (PortableFontInstance *) this;

        realThis->fNAMETable = (const NAMETable *) getFontTable(nameTag);

        if (realThis->fNAMETable != NULL) {
            realThis->fNameCount        = SWAPW(realThis->fNAMETable->count);
            realThis->fNameStringOffset = SWAPW(realThis->fNAMETable->stringOffset);
        }
    }

    for(le_int32 i = 0; i < fNameCount; i += 1) {
        const NameRecord *nameRecord = &fNAMETable->nameRecords[i];
        
        if (SWAPW(nameRecord->platformID) == platformID && SWAPW(nameRecord->encodingID) == encodingID &&
            SWAPW(nameRecord->languageID) == languageID && SWAPW(nameRecord->nameID) == nameID) {
            LEUnicode16 *name = (LEUnicode16 *) (((char *) fNAMETable) + fNameStringOffset + SWAPW(nameRecord->offset));
            le_uint16 length = SWAPW(nameRecord->length) / 2;
            LEUnicode16 *result = NEW_ARRAY(LEUnicode16, length + 2);

            for (le_int32 c = 0; c < length; c += 1) {
                result[c] = SWAPW(name[c]);
            }

            result[length] = 0;

            return result;
        }
    }

    return NULL;
}

void PortableFontInstance::deleteNameString(const char *name) const
{
    DELETE_ARRAY(name);
}

void PortableFontInstance::deleteNameString(const LEUnicode16 *name) const
{
    DELETE_ARRAY(name);
}

void PortableFontInstance::getGlyphAdvance(LEGlyphID glyph, LEPoint &advance) const
{
    TTGlyphID ttGlyph = (TTGlyphID) LE_GET_GLYPH(glyph);

    if (fHMTXTable == NULL) {
        LETag maxpTag = LE_MAXP_TABLE_TAG;
        LETag hmtxTag = LE_HMTX_TABLE_TAG;
        const MAXPTable *maxpTable = (MAXPTable *) getFontTable(maxpTag);
        PortableFontInstance *realThis = (PortableFontInstance *) this;

        if (maxpTable != NULL) {
            realThis->fNumGlyphs = SWAPW(maxpTable->numGlyphs);
        }

        realThis->fHMTXTable = (const HMTXTable *) getFontTable(hmtxTag);
    }

    le_uint16 index = ttGlyph;

    if (ttGlyph >= fNumGlyphs || fHMTXTable == NULL) {
        advance.fX = advance.fY = 0;
        return;
    }

    if (ttGlyph >= fNumLongHorMetrics) {
        index = fNumLongHorMetrics - 1;
    }

    advance.fX = xUnitsToPoints(SWAPW(fHMTXTable->hMetrics[index].advanceWidth));
    advance.fY = 0;
}

le_bool PortableFontInstance::getGlyphPoint(LEGlyphID /*glyph*/, le_int32 /*pointNumber*/, LEPoint &/*point*/) const
{
    return FALSE;
}

le_int32 PortableFontInstance::getUnitsPerEM() const
{
    return fUnitsPerEM;
}

le_uint32 PortableFontInstance::getFontChecksum() const
{
    return 0;
}

le_int32 PortableFontInstance::getAscent() const
{
    return fAscent;
}

le_int32 PortableFontInstance::getDescent() const
{
    return fDescent;
}

le_int32 PortableFontInstance::getLeading() const
{
    return fLeading;
}

// We really want to inherit this method from the superclass, but some compilers
// issue a warning if we don't implement it...
LEGlyphID PortableFontInstance::mapCharToGlyph(LEUnicode32 ch, const LECharMapper *mapper, le_bool filterZeroWidth) const
{
    return LEFontInstance::mapCharToGlyph(ch, mapper, filterZeroWidth);
}

// We really want to inherit this method from the superclass, but some compilers
// issue a warning if we don't implement it...
LEGlyphID PortableFontInstance::mapCharToGlyph(LEUnicode32 ch, const LECharMapper *mapper) const
{
    return LEFontInstance::mapCharToGlyph(ch, mapper);
}

LEGlyphID PortableFontInstance::mapCharToGlyph(LEUnicode32 ch) const
{
    return fCMAPMapper->unicodeToGlyph(ch);
}

float PortableFontInstance::getXPixelsPerEm() const
{
    return fXScale;
}

float PortableFontInstance::getYPixelsPerEm() const
{
    return fYScale;
}

float PortableFontInstance::getScaleFactorX() const
{
    return 1.0;
}

float PortableFontInstance::getScaleFactorY() const
{
    return 1.0;
}
