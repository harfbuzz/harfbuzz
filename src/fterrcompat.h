
#ifndef FTERRCOMPAT_H
#define FTERRCOMPAT_H

#include <config.h>

#ifdef HAVE_FREETYPE_2_0_3
#include <freetype/internal/tterrors.h>
#else
#define TT_Err_Ok FT_Err_Ok
#define TT_Err_Invalid_Argument FT_Err_Invalid_Argument
#define TT_Err_Invalid_Face_Handle FT_Err_Invalid_Face_Handle
#define TT_Err_Table_Missing FT_Err_Table_Missing
#endif

#endif
