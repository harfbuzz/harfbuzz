/*
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This is part of HarfBuzz, an OpenType Layout engine library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

#include "harfbuzz-shaper.h"
#include "harfbuzz-shaper-private.h"
#include "harfbuzz-external.h"

#include <assert.h>
#include <stdio.h>

typedef int (*th_brk_def)(const char*, int[], int);
static th_brk_def th_brk = 0;
static int libthai_resolved = 0;

static void resolve_libthai()
{
    if (!th_brk)
        th_brk = (th_brk_def)HB_Library_Resolve("thai", 0, "th_brk");
    libthai_resolved = 1;
}

static void to_tis620(const HB_UChar16 *string, hb_uint32 len, const char *cstr)
{
    hb_uint32 i;
    unsigned char *result = (unsigned char *)cstr;

    for (i = 0; i < len; ++i) {
        if (string[i] <= 0xa0)
            result[i] = (unsigned char)string[i];
        if (string[i] >= 0xe01 && string[i] <= 0xe5b)
            result[i] = (unsigned char)(string[i] - 0xe00 + 0xa0);
        else
            result[i] = '?';
    }

    result[len] = 0;
}

static void thaiWordBreaks(const HB_UChar16 *string, hb_uint32 len, HB_CharAttributes *attributes)
{
    char s[128];
    char *cstr = s;
    int brp[128];
    int *break_positions = brp;
    hb_uint32 numbreaks;
    hb_uint32 i;

    if (!libthai_resolved)
        resolve_libthai();

    if (!th_brk)
        return;

    if (len >= 128)
        cstr = (char *)malloc(len*sizeof(char) + 1);

    to_tis620(string, len, cstr);

    numbreaks = th_brk(cstr, break_positions, 128);
    if (numbreaks > 128) {
        break_positions = (int *)malloc(numbreaks * sizeof(int));
        numbreaks = th_brk(cstr, break_positions, numbreaks);
    }

    for (i = 0; i < len; ++i) {
        attributes[i].lineBreakType = HB_NoBreak;
        attributes[i].wordBoundary = FALSE;
    }

    for (i = 0; i < numbreaks; ++i) {
        if (break_positions[i] > 0) {
            attributes[break_positions[i]-1].lineBreakType = HB_Break;
            attributes[break_positions[i]-1].wordBoundary = TRUE;
        }
    }

    if (break_positions != brp)
        free(break_positions);

    if (len >= 128)
        free(cstr);
}

void HB_ThaiAttributes(HB_Script script, const HB_UChar16 *text, hb_uint32 from, hb_uint32 len, HB_CharAttributes *attributes)
{
    assert(script == HB_Script_Thai);
    attributes += from;
    thaiWordBreaks(text + from, len, attributes);
}

