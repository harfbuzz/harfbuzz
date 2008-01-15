/*
 * Copyright (C) 2007  Trolltech ASA
 * Copyright (C) 2007  Red Hat, Inc.
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

#ifndef HARFBUZZ_GLOBAL_H
#define HARFBUZZ_GLOBAL_H

#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef __cplusplus
#define HB_BEGIN_HEADER  extern "C" {
#define HB_END_HEADER  }
#else
#define HB_BEGIN_HEADER  /* nothing */
#define HB_END_HEADER  /* nothing */
#endif

HB_BEGIN_HEADER

#define HB_MAKE_TAG(a,b,c,d) FT_MAKE_TAG(a,b,c,d)
typedef FT_Pos		HB_Fixed;	/* 26.6 */
typedef FT_Fixed	HB_16Dot16;	/* 16.6 */
typedef FT_Face		HB_Font;

typedef unsigned char	HB_Byte;
typedef signed char	HB_Char;
typedef unsigned short	HB_UShort;
typedef signed short	HB_Short;
typedef unsigned int	HB_UInt;
typedef signed int	HB_Int;
typedef int		HB_Bool;
typedef void *		HB_Pointer;


typedef enum {
  /* no error */
  HB_Err_Ok                           = 0x0000,
  HB_Err_Not_Covered                  = 0xFFFF,

  /* _hb_err() is called whenever returning the following errors,
   * and in a couple places for HB_Err_Not_Covered too. */

  /* programmer error */
  HB_Err_Invalid_Argument             = 0x1A66,

  /* font error */
  HB_Err_Invalid_SubTable_Format      = 0x157F,
  HB_Err_Invalid_SubTable             = 0x1570,
  HB_Err_Read_Error                   = 0x6EAD,

  /* system error */
  HB_Err_Out_Of_Memory                = 0xDEAD
} HB_Error;

HB_END_HEADER

#endif
