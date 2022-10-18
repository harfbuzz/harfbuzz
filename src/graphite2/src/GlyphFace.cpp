// SPDX-License-Identifier: MIT
// Copyright 2010, SIL International, All rights reserved.

#include "inc/GlyphFace.h"


using namespace graphite2;

int32_t GlyphFace::getMetric(uint8_t metric) const
{
    switch (metrics(metric))
    {
        case kgmetLsb       : return int32_t(m_bbox.bl.x);
        case kgmetRsb       : return int32_t(m_advance.x - m_bbox.tr.x);
        case kgmetBbTop     : return int32_t(m_bbox.tr.y);
        case kgmetBbBottom  : return int32_t(m_bbox.bl.y);
        case kgmetBbLeft    : return int32_t(m_bbox.bl.x);
        case kgmetBbRight   : return int32_t(m_bbox.tr.x);
        case kgmetBbHeight  : return int32_t(m_bbox.tr.y - m_bbox.bl.y);
        case kgmetBbWidth   : return int32_t(m_bbox.tr.x - m_bbox.bl.x);
        case kgmetAdvWidth  : return int32_t(m_advance.x);
        case kgmetAdvHeight : return int32_t(m_advance.y);
        default : return 0;
    }
}
