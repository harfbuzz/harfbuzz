// SPDX-License-Identifier: MIT OR MPL-2.0 OR GPL-2.0-or-later-2.1-or-later
// Copyright 2010, SIL International, All rights reserved.
#pragma once

#include <cstring>
#include <cstdlib>

union FeatID
{
    unsigned char uChar[4];
    unsigned int uId;
};

typedef struct
{
    FeatID m_id;
    union {
        signed short m_sValue;
        unsigned short m_uValue;
    };
} FeatureSetting;

class FeatureParser
{
public:
    FeatureParser(const char * features)
        : m_settings(NULL), m_featureCount(0)
    {
        if (!features)  return;
        const char * pLang = NULL;
        m_lang.uId = 0;
        size_t featuresLength = strlen(features);
        if (featuresLength == 0)
            return;
        m_featureCount = 1;
        if (features && (pLang = strstr(features, "lang=")))
        {
            pLang += 5;
            size_t i = 0;
            while ((i < 4) && (*pLang != '0') && (*pLang != '&'))
            {
                m_lang.uChar[i] = *pLang;
                ++pLang;
                ++i;
            }
            m_lang.uId = swap32(m_lang.uId);
            m_featureCount = 0;
        }
        // count features
        for (size_t i = 0; i < featuresLength; i++)
        {
            if (features[i] == ',') m_featureCount++;
        }
        m_settings = new FeatureSetting[m_featureCount];
        if (!m_settings)
        {
            m_featureCount = 0;
            return;
        }
        //featureList = gr_face_featureval_for_lang(face, lang.uId);
        const char * name = features;
        const char * valueText = NULL;
        size_t nameLength = 0;
        int value = 0;
        FeatID featId;
        //const gr_feature_ref* ref = NULL;
        size_t featIndex = 0;
        featId.uId = 0;
        for (size_t i = 0; i < featuresLength; i++)
        {
            switch (features[i])
            {
                case ',':
                case '&':
                    value = atoi(valueText);
                    if (m_settings)
                    {
                        //gr_fref_set_feature_value(ref, value, featureList);
                        m_settings[featIndex].m_sValue = value;
                        //ref = NULL;
                    }
                    valueText = NULL;
                    name = features + i + 1;
                    nameLength = 0;
                    featId.uId = 0;
                    ++featIndex;
                    break;
                case '=':
                    if (nameLength <= 4 && (*name < '0' || *name > '9'))
                    {
                        featId.uId = swap32(featId.uId);
                        //ref = gr_face_find_fref(face, featId.uId);
                    }
                    else
                    {
                        featId.uId = atoi(name);
                        //ref = gr_face_find_fref(face, featId.uId);
                    }
                    m_settings[featIndex].m_id.uId = featId.uId;
                    valueText = features + i + 1;
                    name = NULL;
                    break;
                default:
                    if (valueText == NULL)
                    {
                        if (nameLength < 4 && features[i] >= 0x20 && features[i] <= 0x7F)
                        {
                            featId.uChar[nameLength++] = features[i];
                        }
                    }
                    break;
            }
            if (featIndex < m_featureCount && valueText)
            {
                value = atoi(valueText);
                m_settings[featIndex].m_sValue = value;
            }
        }
    }
    ~FeatureParser()
    {
        delete[] m_settings;
        m_settings = NULL;
    }
    int swap32(int x)
    {
        return (((x & 0xff) << 24) | ((x & 0xff00) << 8) | ((x & 0xff0000) >> 8) | ((x & 0xff000000) >> 24));
    }
    unsigned int featureId(size_t i) const { return m_settings[i].m_id.uId; }
    unsigned int featureIdBE(size_t i) const { return m_settings[i].m_id.uId; }
    signed short featureSValue(size_t i) const { return m_settings[i].m_sValue; }
    signed short featureUValue(size_t i) const { return m_settings[i].m_uValue; }
    size_t featureCount() const { return m_featureCount; }
    unsigned int langId() const { return m_lang.uId; }
    unsigned int otLang() const {
        FeatID otId;
        static const int lowerCaseOffset = 'a' - 'A';
        otId.uId = m_lang.uId;
        if (m_lang.uId > 0)
        {
            for (size_t i = 0; i < 4; i++)
            {
                // convert to upper case
                if (m_lang.uChar[i] >= 'a' && m_lang.uChar[i] <= 'z')
                    otId.uChar[i] -= lowerCaseOffset;
                // space, not zero pad
                if (m_lang.uChar[i] == 0)
                    otId.uChar[i] = ' ';
            }
        }
        return otId.uId;
    }
private:
    FeatID m_lang;
    FeatureSetting * m_settings;
    size_t m_featureCount;
};
