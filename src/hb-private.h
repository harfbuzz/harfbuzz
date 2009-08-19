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

#define hb_be_uint8
#define hb_be_int8
#define hb_be_uint16(v)			((uint16_t) hb_be_int16 ((uint16_t) v))
#define hb_be_uint32(v)			((uint32_t) hb_be_int32 ((uint32_t) v))

/* We need external help for these */

#if HAVE_GLIB

#include <glib.h>

/* Macros to convert to/from BigEndian */
#define hb_be_int16(v)		GINT16_FROM_BE (v)
#define hb_be_int32(v)		GINT32_FROM_BE (v)

typedef int hb_atomic_int_t;
#define hb_atomic_int_fetch_and_add(AI, V)	g_atomic_int_exchange_and_add (&(AI), V)
#define hb_atomic_int_get(AI)			g_atomic_int_get (&(AI))
#define hb_atomic_int_set(AI, V)		g_atomic_int_set (&(AI), V)

typedef GStaticMutex hb_mutex_t;
#define HB_MUTEX_INIT			G_STATIC_MUTEX_INIT
#define hb_mutex_init(M)		g_static_mutex_init (&M)
#define hb_mutex_lock(M)		g_static_mutex_lock (&M)
#define hb_mutex_trylock(M)		g_static_mutex_trylock (&M)
#define hb_mutex_unlock(M)		g_static_mutex_unlock (&M)

#else
#error "Could not find any system to define platform macros, see hb-private.h"
#endif


#define hb_be_uint8_put_unaligned(v,V)	(v[0] = (V), 0)
#define hb_be_uint8_get_unaligned(v)	(uint8_t) (v[0])
#define hb_be_uint8_cmp_unaligned(a,b)	(a[0] == b[0])
#define hb_be_int8_put_unaligned	hb_be_uint8_put_unaligned
#define hb_be_int8_get_unaligned	(int8_t) hb_be_uint8_get_unaligned
#define hb_be_int8_cmp_unaligned	hb_be_uint8_cmp_unaligned

#define hb_be_uint16_put_unaligned(v,V)	(v[0] = (V>>8), v[1] = (V), 0)
#define hb_be_uint16_get_unaligned(v)	(uint16_t) ((v[0] << 8) + v[1])
#define hb_be_uint16_cmp_unaligned(a,b)	(a[0] == b[0] && a[1] == b[1])
#define hb_be_int16_put_unaligned	hb_be_uint16_put_unaligned
#define hb_be_int16_get_unaligned	(int16_t) hb_be_uint16_get_unaligned
#define hb_be_int16_cmp_unaligned	hb_be_uint16_cmp_unaligned

#define hb_be_uint32_put_unaligned(v,V)	(v[0] = (V>>24), v[1] = (V>>16), v[2] = (V>>8), v[3] = (V), 0)
#define hb_be_uint32_get_unaligned(v)	(uint32_t) ((v[0] << 24) + (v[1] << 16) + (v[2] << 8) + v[3])
#define hb_be_uint32_cmp_unaligned(a,b)	(a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3])
#define hb_be_int32_put_unaligned	hb_be_uint32_put_unaligned
#define hb_be_int32_get_unaligned	(int32_t) hb_be_uint32_get_unaligned
#define hb_be_int32_cmp_unaligned	hb_be_uint32_cmp_unaligned


/* Basics */

#undef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#ifndef HB_INTERNAL
# define HB_INTERNAL extern
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
#define _HB_BOOLEAN_EXPR(expr)                   \
 __extension__ ({                               \
   int _cairo_boolean_var_;                         \
   if (expr)                                    \
      _cairo_boolean_var_ = 1;                      \
   else                                         \
      _cairo_boolean_var_ = 0;                      \
   _cairo_boolean_var_;                             \
})
#define HB_LIKELY(expr) (__builtin_expect (_HB_BOOLEAN_EXPR(expr), 1))
#define HB_UNLIKELY(expr) (__builtin_expect (_HB_BOOLEAN_EXPR(expr), 0))
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
    register uint32_t y;

    y = (mask >> 1) &033333333333;
    y = mask - y - ((y >>1) & 033333333333);
    return (((y + (y >> 3)) & 030707070707) % 077);
#endif
}


/* Multiplies a 16dot16 value by another value, then truncates the result */
#define _hb_16dot16_mul_trunc(A,B) ((int64_t) (A) * (B) / 0x10000)

#include "hb-object-private.h"

#endif /* HB_PRIVATE_H */
