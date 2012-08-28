/*
 *******************************************************************************
 *
 *   Copyright (C) 1999-2011, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *   file name:  letest.h
 *
 *   created on: 11/06/2000
 *   created by: Eric R. Mader
 */

#ifndef __LETEST_H
#define __LETEST_H


#if defined(__GNUC__) && (__GNUC__ >= 4) && !defined(__MINGW32__)
# define HB_BEGIN_VISIBILITY _Pragma ("GCC visibility push(hidden)")
# define HB_END_VISIBILITY _Pragma ("GCC visibility pop")
#else
# define HB_BEGIN_VISIBILITY
# define HB_END_VISIBILITY
#endif


#include "layout/LETypes.h"
/*#include "unicode/ctest.h"*/

#include <stdlib.h>
#include <string.h>

HB_BEGIN_VISIBILITY

U_NAMESPACE_USE

#define ARRAY_SIZE(array) (sizeof array / sizeof array[0])

#define ARRAY_COPY(dst, src, count) memcpy((void *) (dst), (void *) (src), (count) * sizeof (src)[0])

#define NEW_ARRAY(type,count) (type *) malloc((count) * sizeof(type))

#define DELETE_ARRAY(array) free((void *) (array))

#define GROW_ARRAY(array,newSize) realloc((void *) (array), (newSize) * sizeof (array)[0])

struct TestResult
{
    le_int32   glyphCount;
    LEGlyphID *glyphs;
    le_int32  *indices;
    float     *positions;
};

#ifndef __cplusplus
typedef struct TestResult TestResult;
#endif

//U_CFUNC void addCTests(TestNode **root);

HB_END_VISIBILITY

#endif
