/* Pango
 * otttest.c: Test program for OpenType
 *
 * Copyright (C) 2000 Red Hat Software
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>

#include "ftxopen.h"
#include <freetype/internal/ftmemory.h>

#include "disasm.h"

#define N_ELEMENTS(arr) (sizeof(arr)/ sizeof((arr)[0]))

int
croak (const char *situation, FT_Error error)
{
  fprintf (stderr, "%s: Error %d\n", situation, error);

  exit (1);
}

enum {
  I = 1 << 0,
  M = 1 << 1,
  F = 1 << 2,
  L = 1 << 3
};

void
print_tag (FT_ULong tag)
{
  fprintf (stderr, "%c%c%c%c", 
	  (unsigned char)(tag >> 24),
	  (unsigned char)((tag & 0xff0000) >> 16),
	  (unsigned char)((tag & 0xff00) >> 8),
	  (unsigned char)(tag & 0xff));
}

void
maybe_add_feature (TTO_GSUB  gsub,
		   FT_UShort script_index,
		   FT_ULong  tag,
		   FT_UShort property)
{
  FT_Error error;
  FT_UShort feature_index;
  
  /* 0xffff == default language system */
  error = TT_GSUB_Select_Feature (gsub, tag, script_index, 0xffff, &feature_index);
  
  if (error)
    {
      if (error == TTO_Err_Not_Covered)
	{
	  print_tag (tag);
	  fprintf (stderr, " not covered, ignored\n");
	  return;
	}

      croak ("TT_GSUB_Select_Feature", error);
    }

  if ((error = TT_GSUB_Add_Feature (gsub, feature_index, property)))
    croak ("TT_GSUB_Add_Feature", error);
}

void
select_cmap (FT_Face face)
{
  FT_UShort  i;
  FT_CharMap cmap = NULL;
  
  for (i = 0; i < face->num_charmaps; i++)
    {
      if (face->charmaps[i]->platform_id == 3 && face->charmaps[i]->encoding_id == 1)
	{
	  cmap = face->charmaps[i];
	  break;
	}
    }
  
  /* we try only pid/eid (0,0) if no (3,1) map is found -- many Windows
     fonts have only rudimentary (0,0) support.                         */

  if (!cmap)
    for (i = 0; i < face->num_charmaps; i++)
      {
	if (face->charmaps[i]->platform_id == 3 && face->charmaps[i]->encoding_id == 1)
	  {
	    cmap = face->charmaps[i];
	    break;
	  }
      }

  if (cmap)
    FT_Set_Charmap (face, cmap);
  else
    {
      fprintf (stderr, "Sorry, but this font doesn't contain"
	       " any Unicode mapping table.\n");
      exit (1);
    }
}

void
add_features (TTO_GSUB gsub)
{
  FT_Error error;
  FT_ULong tag = FT_MAKE_TAG ('a', 'r', 'a', 'b');
  FT_UShort script_index;

  error = TT_GSUB_Select_Script (gsub, tag, &script_index);

  if (error)
    {
      if (error == TTO_Err_Not_Covered)
	{
	  fprintf (stderr, "Arabic not covered, no features used\n");
	  return;
	}

      croak ("TT_GSUB_Select_Script", error);
    }

  maybe_add_feature (gsub, script_index, FT_MAKE_TAG ('i', 'n', 'i', 't'), I);
  maybe_add_feature (gsub, script_index, FT_MAKE_TAG ('m', 'e', 'd', 'i'), M);
  maybe_add_feature (gsub, script_index, FT_MAKE_TAG ('f', 'i', 'n', 'a'), F);
  maybe_add_feature (gsub, script_index, FT_MAKE_TAG ('l', 'i', 'g', 'a'), L);
}

void 
dump_string (TTO_GSUB_String *str)
{
  int i;

  fprintf (stderr, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
  for (i = 0; i < str->length; i++)
    {
      fprintf (stderr, "%2d: %#06x %#06x %4d %4d\n",
	       i,
	       str->string[i],
	       str->properties[i],
	       str->components[i],
	       str->ligIDs[i]);
    }
  fprintf (stderr, "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

FT_UShort arabic_str[]   = { 0x645, 0x643, 0x64a, 0x644, 0x639, 0x20, 0x645, 0x627, 0x644, 0x633, 0x644, 0x627 };
FT_UShort arabic_props[] = { I|L,   M|L,   M|L,   M|L,   M|L,   F|L,   I|L,  M|L,   M|L,   M|L,   M|L,   F|L };

void
try_string (FT_Library library,
	    FT_Face    face,
	    TTO_GSUB   gsub)
{
  FT_Error error;
  TTO_GSUB_String *in_str;
  TTO_GSUB_String *out_str;
  int i;

  if ((error = TT_GSUB_String_New (face->memory, &in_str)))
    croak ("TT_GSUB_String_New", error);
  if ((error = TT_GSUB_String_New (face->memory, &out_str)))
    croak ("TT_GSUB_String_New", error);

  if ((error = TT_GSUB_String_Set_Length (in_str, N_ELEMENTS (arabic_str))))
    croak ("TT_GSUB_String_Set_Length", error);

  for (i=0; i < N_ELEMENTS (arabic_str); i++)
    {
      in_str->string[i] = FT_Get_Char_Index (face, arabic_str[i]);
      in_str->properties[i] = arabic_props[i];
      in_str->components[i] = i;
      in_str->ligIDs[i] = i;
    }

  if ((error = TT_GSUB_Apply_String (gsub, in_str, out_str)))
    croak ("TT_GSUB_Apply_String", error);

  dump_string (in_str);
  dump_string (out_str);

  if ((error = TT_GSUB_String_Done (in_str)))
    croak ("TT_GSUB_String_New", error);
  if ((error = TT_GSUB_String_Done (out_str)))
    croak ("TT_GSUB_String_New", error);
}

int 
main (int argc, char **argv)
{
  FT_Error error;
  FT_Library library;
  FT_Face face;
  TTO_GSUB gsub;
  TTO_GPOS gpos;

  if (argc != 2)
    {
      fprintf (stderr, "Usage: ottest MYFONT.TTF\n");
      exit(1);
    }

  if ((error = FT_Init_FreeType (&library)))
    croak ("FT_Init_FreeType", error);

  if ((error = FT_New_Face (library, argv[1], 0, &face)))
    croak ("FT_New_Face", error);

  if (!(error = TT_Load_GSUB_Table (face, &gsub, NULL)))
    {
      TT_Dump_GSUB_Table (gsub, stdout);
      
      if ((error = TT_Done_GSUB_Table (gsub)))
	croak ("FT_Done_GSUB_Table", error);
    }
  else
    fprintf (stderr, "TT_Load_GSUB_Table %d\n", error);

  if (!(error = TT_Load_GPOS_Table (face, &gpos, NULL)))
    {
      TT_Dump_GPOS_Table (gpos, stdout);
      
      if ((error = TT_Done_GPOS_Table (gpos)))
	croak ("FT_Done_GPOS_Table", error);
    }
  else
    fprintf (stderr, "TT_Load_GPOS_Table %d\n", error);

#if 0  
  select_cmap (face);

  add_features (gsub);
  try_string (library, face, gsub);
#endif


  if ((error = FT_Done_Face (face)))
    croak ("FT_Done_Face", error);

  if ((error = FT_Done_FreeType (library)))
    croak ("FT_Done_FreeType", error);
  
  return 0;
}

