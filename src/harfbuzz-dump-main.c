/*
 * Copyright (C) 2000  Red Hat, Inc.
 *
 * This is part of HarfBuzz, an OpenType Layout engine library.
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
 * Red Hat Author(s): Owen Taylor
 */

#include <stdio.h>
#include <stdlib.h>

#include "harfbuzz.h"
#include "harfbuzz-dump.h"

#define N_ELEMENTS(arr) (sizeof(arr)/ sizeof((arr)[0]))

static int
croak (const char *situation, HB_Error error)
{
  fprintf (stderr, "%s: Error %d\n", situation, error);

  exit (1);
}

#if 0
enum {
  I = 1 << 0,
  M = 1 << 1,
  F = 1 << 2,
  L = 1 << 3
};

static void
print_tag (HB_UInt tag)
{
  fprintf (stderr, "%c%c%c%c", 
	  (unsigned char)(tag >> 24),
	  (unsigned char)((tag >> 16) & 0xff),
	  (unsigned char)((tag >> 8) & 0xff),
	  (unsigned char)(tag & 0xff));
}

static void
maybe_add_feature (HB_GSUB  gsub,
		   HB_UShort script_index,
		   HB_UInt  tag,
		   HB_UShort property)
{
  HB_Error error;
  HB_UShort feature_index;
  
  /* 0xffff == default language system */
  error = HB_GSUB_Select_Feature (gsub, tag, script_index, 0xffff, &feature_index);
  
  if (error)
    {
      if (error == HB_Err_Not_Covered)
	{
	  print_tag (tag);
	  fprintf (stderr, " not covered, ignored\n");
	  return;
	}

      croak ("HB_GSUB_Select_Feature", error);
    }

  if ((error = HB_GSUB_Add_Feature (gsub, feature_index, property)))
    croak ("HB_GSUB_Add_Feature", error);
}

static void
add_features (HB_GSUB gsub)
{
  HB_Error error;
  HB_UInt tag = HB_MAKE_TAG ('a', 'r', 'a', 'b');
  HB_UShort script_index;

  error = HB_GSUB_Select_Script (gsub, tag, &script_index);

  if (error)
    {
      if (error == HB_Err_Not_Covered)
	{
	  fprintf (stderr, "Arabic not covered, no features used\n");
	  return;
	}

      croak ("HB_GSUB_Select_Script", error);
    }

  maybe_add_feature (gsub, script_index, HB_MAKE_TAG ('i', 'n', 'i', 't'), I);
  maybe_add_feature (gsub, script_index, HB_MAKE_TAG ('m', 'e', 'd', 'i'), M);
  maybe_add_feature (gsub, script_index, HB_MAKE_TAG ('f', 'i', 'n', 'a'), F);
  maybe_add_feature (gsub, script_index, HB_MAKE_TAG ('l', 'i', 'g', 'a'), L);
}
#endif

#if 0
void 
dump_string (HB_GSUB_String *str)
{
  HB_UInt i;

  fprintf (stderr, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
  for (i = 0; i < str->length; i++)
    {
      fprintf (stderr, "%2lu: %#06x %#06x %4d %4d\n",
	       i,
	       str->string[i],
	       str->properties[i],
	       str->components[i],
	       str->ligIDs[i]);
    }
  fprintf (stderr, "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

HB_UShort arabic_str[]   = { 0x645, 0x643, 0x64a, 0x644, 0x639, 0x20, 0x645, 0x627, 0x644, 0x633, 0x644, 0x627 };
HB_UShort arabic_props[] = { I|L,   M|L,   M|L,   M|L,   M|L,   F|L,   I|L,  M|L,   M|L,   M|L,   M|L,   F|L };

void
try_string (FT_Library library,
	    HB_Font    font,
	    HB_GSUB   gsub)
{
  HB_Error error;
  HB_GSUB_String *in_str;
  HB_GSUB_String *out_str;
  HB_UInt i;

  if ((error = HB_GSUB_String_New (font->memory, &in_str)))
    croak ("HB_GSUB_String_New", error);
  if ((error = HB_GSUB_String_New (font->memory, &out_str)))
    croak ("HB_GSUB_String_New", error);

  if ((error = HB_GSUB_String_Set_Length (in_str, N_ELEMENTS (arabic_str))))
    croak ("HB_GSUB_String_Set_Length", error);

  for (i=0; i < N_ELEMENTS (arabic_str); i++)
    {
      in_str->string[i] = FT_Get_Char_Index (font, arabic_str[i]);
      in_str->properties[i] = arabic_props[i];
      in_str->components[i] = i;
      in_str->ligIDs[i] = i;
    }

  if ((error = HB_GSUB_Apply_String (gsub, in_str, out_str)))
    croak ("HB_GSUB_Apply_String", error);

  dump_string (in_str);
  dump_string (out_str);

  if ((error = HB_GSUB_String_Done (in_str)))
    croak ("HB_GSUB_String_New", error);
  if ((error = HB_GSUB_String_Done (out_str)))
    croak ("HB_GSUB_String_New", error);
}
#endif

int 
main (int argc, char **argv)
{
  HB_Error error;
  FT_Library library;
  HB_Font font;
  HB_GSUB gsub;
  HB_GPOS gpos;

  if (argc != 2)
    {
      fprintf (stderr, "Usage: ottest MYFONT.TTF\n");
      exit(1);
    }

  if ((error = FT_Init_FreeType (&library)))
    croak ("FT_Init_FreeType", error);

  if ((error = FT_New_Face (library, argv[1], 0, &font)))
    croak ("FT_New_Face", error);

  printf ("<?xml version=\"1.0\"?>\n");
  printf ("<OpenType>\n");

  if (!(error = HB_Load_GSUB_Table (font, &gsub, NULL)))
    {
      HB_Dump_GSUB_Table (gsub, stdout);
      
      if ((error = HB_Done_GSUB_Table (gsub)))
	croak ("HB_Done_GSUB_Table", error);
    }
  else if (error != HB_Err_Not_Covered)
    fprintf (stderr, "HB_Load_GSUB_Table: error 0x%x\n", error);

  if (!(error = HB_Load_GPOS_Table (font, &gpos, NULL)))
    {
      HB_Dump_GPOS_Table (gpos, stdout);
      
      if ((error = HB_Done_GPOS_Table (gpos)))
	croak ("HB_Done_GPOS_Table", error);
    }
  else if (error != HB_Err_Not_Covered)
    fprintf (stderr, "HB_Load_GPOS_Table: error 0x%x\n", error);

  printf ("</OpenType>\n");

#if 0  
  select_cmap (font);

  add_features (gsub);
  try_string (library, font, gsub);
#endif


  if ((error = FT_Done_Face (font)))
    croak ("FT_Done_Face", error);

  if ((error = FT_Done_FreeType (library)))
    croak ("FT_Done_FreeType", error);
  
  return 0;
}

