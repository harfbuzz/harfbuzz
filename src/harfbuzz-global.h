/*******************************************************************
 *
 *  Copyright 1996-2000 by
 *  David Turner, Robert Wilhelm, and Werner Lemberg.
 *  Copyright 2007  Trolltech ASA
 *  Copyright 2007  Behdad Esfahbod
 *
 *  This is part of HarfBuzz, an OpenType Layout engine library.
 *
 *  See the file name COPYING for licensing information.
 *
 ******************************************************************/
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

typedef unsigned char HB_Byte;
typedef signed char HB_Char;
typedef unsigned short HB_UShort;
typedef signed short HB_Short;
typedef unsigned int HB_UInt;
typedef signed int HB_Int;
typedef int HB_Bool;
typedef void *HB_Pointer;


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
