// SPDX-License-Identifier: MIT OR MPL-2.0 OR GPL-2.0-or-later-2.1-or-later
// Copyright 2010, SIL International, All rights reserved.
#pragma once

#include "RenderedLine.h"


class Renderer
{
public:
    virtual ~Renderer() {}
    virtual void renderText(const char * utf8, size_t length, RenderedLine * result, FILE *log = NULL) = 0;
    virtual const char * name() const = 0;
};
