/*******************************************************************
 *
 *  Copyright 1996-2000 by
 *  David Turner, Robert Wilhelm, and Werner Lemberg.
 *
 *  Copyright 2006  Behdad Esfahbod
 *
 *  This is part of HarfBuzz, an OpenType Layout engine library.
 *
 *  See the file name COPYING for licensing information.
 *
 ******************************************************************/
#ifndef HARFBUZZ_GDEF_PRIVATE_H
#define HARFBUZZ_GDEF_PRIVATE_H

#include "harfbuzz-impl.h"
#include "harfbuzz-stream-private.h"
#include "harfbuzz-buffer-private.h"
#include "harfbuzz-gdef.h"

HB_BEGIN_HEADER


/* Attachment related structures */

struct  HB_AttachPoint_
{
  HB_UShort   PointCount;             /* size of the PointIndex array */
  HB_UShort*  PointIndex;             /* array of contour points      */
};

/* Ligature Caret related structures */

struct  HB_CaretValueFormat1_
{
  HB_Short  Coordinate;               /* x or y value (in design units) */
};

typedef struct HB_CaretValueFormat1_  HB_CaretValueFormat1;


struct  HB_CaretValueFormat2_
{
  HB_UShort  CaretValuePoint;         /* contour point index on glyph */
};

typedef struct HB_CaretValueFormat2_  HB_CaretValueFormat2;


struct  HB_CaretValueFormat3_
{
  HB_Short    Coordinate;             /* x or y value (in design units) */
  HB_Device  Device;                 /* Device table for x or y value  */
};

typedef struct HB_CaretValueFormat3_  HB_CaretValueFormat3;


struct  HB_CaretValueFormat4_
{
  HB_UShort  IdCaretValue;            /* metric ID */
};

typedef struct HB_CaretValueFormat4_  HB_CaretValueFormat4;


struct  HB_CaretValue_
{
  HB_UShort  CaretValueFormat;        /* 1, 2, 3, or 4 */

  union
  {
    HB_CaretValueFormat1  cvf1;
    HB_CaretValueFormat2  cvf2;
    HB_CaretValueFormat3  cvf3;
    HB_CaretValueFormat4  cvf4;
  } cvf;
};

typedef struct HB_CaretValue_  HB_CaretValue;


struct  HB_LigGlyph_
{
  HB_Bool          loaded;

  HB_UShort        CaretCount;        /* number of caret values */
  HB_CaretValue*  CaretValue;        /* array of caret values  */
};


HB_INTERNAL HB_Error
_HB_GDEF_Add_Glyph_Property( HB_GDEFHeader* gdef,
		             HB_UShort      glyphID,
		             HB_UShort      property );

HB_INTERNAL HB_Error
_HB_GDEF_Check_Property( HB_GDEFHeader* gdef,
		         HB_GlyphItem   item,
		         HB_UShort      flags,
		         HB_UShort*     property );

HB_INTERNAL HB_Error
_HB_GDEF_LoadMarkAttachClassDef_From_LookupFlags( HB_GDEFHeader* gdef,
						  HB_Stream      input,
						  HB_Lookup*     lo,
						  HB_UShort      num_lookups );

HB_END_HEADER

#endif /* HARFBUZZ_GDEF_PRIVATE_H */
