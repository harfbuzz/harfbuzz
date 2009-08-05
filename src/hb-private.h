/*
 * Copyright (C) 2007,2008,2009  Red Hat, Inc.
 *
 *  This is part of HarfBuzz, an OpenType Layout engine library.
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
 *
 * Red Hat Author(s): Behdad Esfahbod
 */

#ifndef HB_PRIVATE_H
#define HB_PRIVATE_H

#if HAVE_CONFIG_H
#include "config.h"
#endif

/* #define HB_DEBUG 1 */
#ifndef HB_DEBUG
#define HB_DEBUG 0
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#if HB_DEBUG
#include <stdio.h>
#include <errno.h>
#endif

#include "hb-common.h"

/* Basics */

#undef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#ifndef HB_INTERNAL
# define HB_INTERNAL
#endif

#ifndef NULL
# define NULL ((void *)0)
#endif

#ifndef FALSE
# define FALSE 0
#endif

#ifndef TRUE
# define TRUE 1
#endif

#define HB_STMT_START do
#define HB_STMT_END   while (0)

#define _ASSERT_STATIC1(_line, _cond) typedef int _static_assert_on_line_##_line##_failed[(_cond)?1:-1]
#define _ASSERT_STATIC0(_line, _cond) _ASSERT_STATIC1 (_line, (_cond))
#define ASSERT_STATIC(_cond) _ASSERT_STATIC0 (__LINE__, (_cond))

#define ASSERT_SIZE(_type, _size) ASSERT_STATIC (sizeof (_type) == (_size))


#if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
#define _CAIRO_BOOLEAN_EXPR(expr)                   \
 __extension__ ({                               \
   int _cairo_boolean_var_;                         \
   if (expr)                                    \
      _cairo_boolean_var_ = 1;                      \
   else                                         \
      _cairo_boolean_var_ = 0;                      \
   _cairo_boolean_var_;                             \
})
#define HB_LIKELY(expr) (__builtin_expect (_CAIRO_BOOLEAN_EXPR(expr), 1))
#define HB_UNLIKELY(expr) (__builtin_expect (_CAIRO_BOOLEAN_EXPR(expr), 0))
#else
#define HB_LIKELY(expr) (expr)
#define HB_UNLIKELY(expr) (expr)
#endif

#ifndef __GNUC__
#undef __attribute__
#define __attribute__(x)
#endif

#if __GNUC__ >= 3
#define HB_GNUC_UNUSED	__attribute__((unused))
#define HB_GNUC_PURE	__attribute__((pure))
#define HB_GNUC_CONST	__attribute__((const))
#else
#define HB_GNUC_UNUSED
#define HB_GNUC_PURE
#define HB_GNUC_CONST
#endif


#if (defined(__WIN32__) && !defined(__WINE__)) || defined(_MSC_VER)
#define snprintf _snprintf
#endif

#ifdef _MSC_VER
#undef inline
#define inline __inline
#endif

#ifdef __STRICT_ANSI__
#undef inline
#define inline __inline__
#endif


/* Return the number of 1 bits in mask.
 *
 * GCC 3.4 supports a "population count" builtin, which on many targets is
 * implemented with a single instruction. There is a fallback definition
 * in libgcc in case a target does not have one, which should be just as
 * good as the open-coded solution below, (which is "HACKMEM 169").
 */
static HB_GNUC_UNUSED inline unsigned int
_hb_popcount32 (uint32_t mask)
{
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
    return __builtin_popcount (mask);
#else
    register int y;

    y = (mask >> 1) &033333333333;
    y = mask - y - ((y >>1) & 033333333333);
    return (((y + (y >> 3)) & 030707070707) % 077);
#endif
}

static HB_GNUC_UNUSED inline uint16_t
_hb_be_uint16 (uint16_t v)
{
  return (v>>8) + (v<<8);
}
static HB_GNUC_UNUSED inline uint32_t
_hb_be_uint32 (uint32_t v)
{
  return _hb_be_uint16 (v>>16) + (_hb_be_uint16 (v) <<16);
}

/* Macros to convert to/from BigEndian */
#define hb_be_uint8
#define hb_be_int8
#define hb_be_uint16(v)	_hb_be_uint16 (v)
#define hb_be_int16(v)	((int16_t) hb_be_uint16 (v))
#define hb_be_uint32(v)	_hb_be_uint32 (v)
#define hb_be_int32(v)	((int32_t) hb_be_uint32 (v))


#include "hb-object-private.h"

#endif /* HB_PRIVATE_H */
