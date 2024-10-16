// SPDX-License-Identifier: MIT
// Copyright 2010, SIL International, All rights reserved.

#pragma once

#include <graphite2/Segment.h>
#include "inc/TtfTypes.h"
#include "inc/locale2lcid.h"

namespace graphite2 {

class NameTable
{
    NameTable(const NameTable &);
    NameTable & operator = (const NameTable &);

public:
    NameTable(const void * data, size_t length, uint16_t platfromId=3, uint16_t encodingID = 1);
    ~NameTable() { free(const_cast<TtfUtil::Sfnt::FontNames *>(m_table)); }
    enum eNameFallback {
        eNoFallback = 0,
        eEnUSFallbackOnly = 1,
        eEnOrAnyFallback = 2
    };
    uint16_t setPlatformEncoding(uint16_t platfromId=3, uint16_t encodingID = 1);
    void * getName(uint16_t & languageId, uint16_t nameId, gr_encform enc, uint32_t & length);
    uint16_t getLanguageId(const char * bcp47Locale);

    CLASS_NEW_DELETE
private:
    uint16_t m_platformId;
    uint16_t m_encodingId;
    uint16_t m_languageCount;
    uint16_t m_platformOffset; // offset of first NameRecord with for platform 3, encoding 1
    uint16_t m_platformLastRecord;
    uint16_t m_nameDataLength;
    const TtfUtil::Sfnt::FontNames * m_table;
    const uint8_t * m_nameData;
    Locale2Lang m_locale2Lang;
};

} // namespace graphite2
