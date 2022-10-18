// SPDX-License-Identifier: MIT
// Copyright 2010, SIL International, All rights reserved.

#pragma once
#include <cstring>
#include <cassert>
#include "inc/Main.h"
#include "inc/vector.hpp"

namespace graphite2 {

class FeatureRef;
class FeatureMap;

class FeatureVal : public vector<uint32_t>
{
public:
    FeatureVal() : m_pMap(0) { }
    FeatureVal(size_t num, const FeatureMap & pMap) : vector<uint32_t>(num), m_pMap(&pMap) {}
    FeatureVal(const FeatureVal & rhs) : vector<uint32_t>(rhs), m_pMap(rhs.m_pMap) {}

    FeatureVal & operator = (const FeatureVal & rhs) { vector<uint32_t>::operator = (rhs); m_pMap = rhs.m_pMap; return *this; }

    bool operator ==(const FeatureVal & b) const
    {
        size_t n = size();
        if (n != b.size())      return false;

        for(const_iterator l = begin(), r = b.begin(); n && *l == *r; --n, ++l, ++r);

        return n == 0;
    }

    CLASS_NEW_DELETE
private:
    friend class FeatureRef;        //so that FeatureRefs can manipulate m_vec directly
    const FeatureMap* m_pMap;
};

typedef FeatureVal Features;

} // namespace graphite2


struct gr_feature_val : public graphite2::FeatureVal {};
