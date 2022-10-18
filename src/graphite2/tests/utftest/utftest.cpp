// SPDX-License-Identifier: MIT
// Copyright 2011, SIL International, All rights reserved.
#include <graphite2/Segment.h>
#include <stdio.h>

struct test8
{
    size_t        len;
    int           error;
    unsigned char str[12];
};
struct test8 tests8[] = {
    { 0,  0, {0xF4, 0x90, 0x80, 0x80, 0,    0,    0,    0,    0,    0,    0,    0} },   // bad(4) [U+110000]
    { 0,  0, {0xC0, 0x80, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0} },   // bad(4) [U+110000]
    { 0,  0, {0xA0, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0} },   // bad(4) [U+110000]
    { 4, -1, {0x7F, 0xDF, 0xBF, 0xEF, 0xBF, 0xBF, 0xF4, 0x8F, 0xBF, 0xBF, 0,    0} },   // U+7F, U+7FF, U+FFFF, U+10FFF
    { 2,  3, {0x7F, 0xDF, 0xBF, 0xF0, 0x8F, 0xBF, 0xBF, 0xF4, 0x8F, 0xBF, 0xBF, 0} },   // U+7F, U+7FF, long(U+FFFF), U+10FFF
    { 1,  1, {0x7F, 0xE0, 0x9F, 0xBF, 0xEF, 0xBF, 0xBF, 0xF4, 0x8F, 0xBF, 0xBF, 0} },   // U+7F, long(U+7FF), U+FFFF, U+10FFF
    { 0,  0, {0xC1, 0xBF, 0xDF, 0xBF, 0xEF, 0xBF, 0xBF, 0xF4, 0xBF, 0xBF, 0xBF, 0} },   // long(U+7F), U+7FF, U+FFFF, U+10FFF
    { 4, -1, {0x01, 0xC2, 0x80, 0xE0, 0xA0, 0x80, 0xF0, 0x90, 0x80, 0x80, 0,    0} },   // U+01, U+80, U+800, U+10000
    { 1,  1, {0x65, 0x9F, 0x65, 0x65, 0,    0,    0,    0,    0,    0,    0,    0} },   // U+65 bad(1) U+65 U+65
    { 2,  2, {0x65, 0x65, 0xC2, 0xC2, 0x65, 0x65, 0,    0,    0,    0,    0,    0} },   // U+65 U+65 bad(1) bad(1) U+65 U+65
    { 2,  2, {0x65, 0x75, 0xE3, 0x84, 0x75, 0x75, 0,    0,    0,    0,    0,    0} },   // U+65 U+75 bad(2) U+75 U+75
    { 2,  2, {0x65, 0x75, 0xF3, 0x84, 0xA5, 0x75, 0x75, 0,    0,    0,    0,    0} },   // U+65 U+75 bad(3) U+75 U+75
    { 2,  2, {0x65, 0x75, 0xF3, 0x84, 0xA5, 0xF5, 0x75, 0,    0,    0,    0,    0} },   // U+65 U+75 bad(3) bad(1) U+75
};

const int numtests8 = sizeof(tests8)/sizeof(test8);

struct test16
{
    size_t         len;
    int            error;
    unsigned short str[6];
};

struct test16 tests16[] = {
    {4, -1, {0x007F, 0x07FF, 0xFFFF, 0xDBFF, 0xDFFF, 0x0000} },
    {4, -1, {0x0001, 0x0080, 0x0800, 0xD800, 0xDC00, 0x0000} },
    {3,  6, {0x007F, 0x07FF, 0xFFFF, 0xDCFF, 0xDFFF, 0x0000} },
    {3,  6, {0x0001, 0x0080, 0x0800, 0xD800, 0xD800, 0x0000} },
    {2,  6, {0x0045, 0xD900, 0xDD00, 0xD900, 0xFFFF, 0x0000} },
};

const int numtests16 = sizeof(tests16)/sizeof(test16);

int main(int argc, char * argv[])
{
    int i;
    const void * error;

    for (i = 0; i < numtests8; ++i)
    {
        size_t res = gr_count_unicode_characters(gr_utf8, tests8[i].str, tests8[i].str + sizeof(tests8[i].str), &error);
        if (tests8[i].error >= 0)
        {
            if (!error)
            {
                fprintf(stderr, "%s: test 8:%d failed: expected error condition did not occur\n", argv[0], i + 1);
                return (i+1);
            }
            else if (ptrdiff_t(error) - ptrdiff_t(tests8[i].str) != tests8[i].error)
            {
                fprintf(stderr, "%s: test 8:%d failed: error at codepoint %d expected at codepoint %d\n", argv[0], i + 1, int(ptrdiff_t(error) - ptrdiff_t(tests8[i].str)), tests8[i].error);
                return (i+1);
            }
        }
        else if (error)
        {
            fprintf(stderr, "%s: test 8:%d failed: unexpected error occured at codepoint %d\n", argv[0], i + 1, int(ptrdiff_t(error) - ptrdiff_t(tests8[i].str)));
            return (i+1);
        }
        if (res != tests8[i].len)
        {
            fprintf(stderr, "%s: test 8:%d failed: character count failure %zd != %zd\n", argv[0], i + 1, res, tests8[i].len);
            return (i+1);
        }
    }

    for (i = 0; i < numtests16; ++i)
    {
        size_t res = gr_count_unicode_characters(gr_utf16, tests16[i].str, tests16[i].str + sizeof tests16[i].str/sizeof tests16[i].str[0], &error);
        if (tests16[i].error >= 0)
        {
            if (!error)
            {
                fprintf(stderr, "%s: test 16:%d failed: expected error condition did not occur\n", argv[0], i + 1);
                return (i+1);
            }
            else if (ptrdiff_t(error) - ptrdiff_t(tests16[i].str) != tests16[i].error)
            {
                fprintf(stderr, "%s: test 16:%d failed: error at codepoint %d expected at codepoint %d\n", argv[0], i + 1, int(ptrdiff_t(error) - ptrdiff_t(tests16[i].str)), tests16[i].error);
                return (i+1);
            }
        }
        else if (error)
        {
            fprintf(stderr, "%s: test 16:%d failed: unexpected error occured at codepoint %d\n", argv[0], i + 1, int(ptrdiff_t(error) - ptrdiff_t(tests16[i].str)));
            return (i+1);
        }
        if (res != tests16[i].len)
        {
            fprintf(stderr, "%s: test 16:%d failed: character count failure %zd != %zd\n", argv[0], i + 1, res, tests16[i].len);
            return (i+1);
        }
    }
    return 0;
}
