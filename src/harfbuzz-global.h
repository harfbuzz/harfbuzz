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

#ifdef __cplusplus
#define HB_BEGIN_HEADER  extern "C" {
#define HB_END_HEADER  }
#else
#define HB_BEGIN_HEADER  /* nothing */
#define HB_END_HEADER  /* nothing */
#endif

HB_BEGIN_HEADER

typedef unsigned short HB_UShort;
typedef signed short HB_Short;
typedef unsigned int HB_UInt;
typedef signed int HB_Int;
typedef int HB_Bool;


/* compatible with FT_Error */
typedef enum {
  HB_Err_Invalid_Argument           = FT_Err_Invalid_Argument,
  HB_Err_Invalid_Face_Handle        = FT_Err_Invalid_Face_Handle,
  HB_Err_Invalid_Stream_Operation   = FT_Err_Invalid_Stream_Operation,
  HB_Err_Empty_Script               = 0x1005,

  HB_Err_Ok                         = FT_Err_Ok,
  HB_Err_Not_Covered                = 0x1002,
  HB_Err_Out_Of_Memory              = FT_Err_Out_Of_Memory,
  HB_Err_Table_Missing              = FT_Err_Table_Missing,
  HB_Err_Invalid_SubTable_Format    = 0x1000,
  HB_Err_Invalid_SubTable           = 0x1001,
  HB_Err_Too_Many_Nested_Contexts   = 0x1003,
  HB_Err_No_MM_Interpreter          = 0x1004
} HB_Error;

HB_END_HEADER

#endif
