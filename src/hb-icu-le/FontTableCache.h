/*
 **********************************************************************
 *   Copyright (C) 2003-2008, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
 */

#ifndef __FONTTABLECACHE_H

#define __FONTTABLECACHE_H

#define HB_H_IN
#include <hb-blob.h>

#include "layout/LETypes.h"
#include "letest.h"

HB_BEGIN_VISIBILITY

U_NAMESPACE_USE

struct FontTableCacheEntry;

class FontTableCache
{
public:
    FontTableCache();

    virtual ~FontTableCache();

    const void *find(LETag tableTag) const;

protected:
    virtual hb_blob_t *readFontTable(LETag tableTag) const = 0;
    virtual void freeFontTable(hb_blob_t *blob) const;

private:

    void add(LETag tableTag, hb_blob_t *blob);

    FontTableCacheEntry *fTableCache;
    le_int32 fTableCacheCurr;
    le_int32 fTableCacheSize;
};

HB_END_VISIBILITY

#endif
