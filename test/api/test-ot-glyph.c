/*
 * Copyright © 2020  Ebrahim Byagowi
 *
 *  This is part of HarfBuzz, a text shaping library.
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
 */

#include "hb-test.h"

#include <hb-ot.h>

static void
move_to (hb_position_t to_x, hb_position_t to_y, void *user_data)
{
  char *str = (char *) user_data;
  sprintf (str+strlen (str), "M%d,%d", to_x, to_y);
}

static void
line_to (hb_position_t to_x, hb_position_t to_y, void *user_data)
{
  char *str = (char *) user_data;
  sprintf (str+strlen (str), "L%d,%d", to_x, to_y);
}

static void
conic_to (hb_position_t control_x, hb_position_t control_y,
	  hb_position_t to_x, hb_position_t to_y,
	  void *user_data)
{
  char *str = (char *) user_data;
  sprintf (str+strlen (str), "Q%d,%d %d,%d",
	   control_x, control_y,
	   to_x, to_y);
}

static void
cubic_to (hb_position_t control1_x, hb_position_t control1_y,
	  hb_position_t control2_x, hb_position_t control2_y,
	  hb_position_t to_x, hb_position_t to_y,
	  void *user_data)
{
  char *str = (char *) user_data;
  sprintf (str+strlen (str), "C%d,%d %d,%d %d,%d",
	   control1_x, control1_y,
	   control2_x, control2_y,
	   to_x, to_y);
}

static void
test_hb_ot_glyph_empty (void)
{
  hb_ot_glyph_decompose_funcs_t funcs;
  funcs.move_to = (hb_ot_glyph_decompose_move_to_func_t) move_to;
  funcs.line_to = (hb_ot_glyph_decompose_line_to_func_t) line_to;
  funcs.conic_to = (hb_ot_glyph_decompose_conic_to_func_t) conic_to;
  funcs.cubic_to = (hb_ot_glyph_decompose_cubic_to_func_t) cubic_to;

  g_assert (!hb_ot_glyph_decompose (hb_font_get_empty (), 3, &funcs, NULL));
}

static void
test_hb_ot_glyph_glyf (void)
{
  hb_ot_glyph_decompose_funcs_t funcs;
  funcs.move_to = (hb_ot_glyph_decompose_move_to_func_t) move_to;
  funcs.line_to = (hb_ot_glyph_decompose_line_to_func_t) line_to;
  funcs.conic_to = (hb_ot_glyph_decompose_conic_to_func_t) conic_to;
  funcs.cubic_to = (hb_ot_glyph_decompose_cubic_to_func_t) cubic_to;

  hb_face_t *face = hb_test_open_font_file ("fonts/SourceSerifVariable-Roman-VVAR.abc.ttf");
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);

  char str[1024] = {0};
  g_assert (!hb_ot_glyph_decompose (font, 4, &funcs, str));
  g_assert (hb_ot_glyph_decompose (font, 3, &funcs, str));
  char expected[] = "M275,442L275,442Q232,442 198,420Q164,397 145,353Q126,309 126,245L126,245"
		    "Q126,182 147,139Q167,95 204,73Q240,50 287,50L287,50Q330,50 367,70Q404,90 427,128L427,128L451,116"
		    "Q431,54 384,21Q336,-13 266,-13L266,-13Q198,-13 148,18Q97,48 70,104Q43,160 43,236L43,236Q43,314 76,371"
		    "Q108,427 160,457Q212,487 272,487L272,487Q316,487 354,470Q392,453 417,424Q442,395 448,358"
		    "L448,358Q441,321 403,321L403,321Q378,321 367,334Q355,347 350,366L350,366L325,454L371,417"
		    "Q346,430 321,436Q296,442 275,442";
  g_assert_cmpmem (str, strlen (str), expected, sizeof (expected) - 1);

  hb_variation_t var;
  var.tag = HB_TAG ('w','g','h','t');
  var.value = 800;
  hb_font_set_variations (font, &var, 1);

  char str2[1024] = {0};
  g_assert (hb_ot_glyph_decompose (font, 3, &funcs, str2));
  char expected2[] = "M323,448L323,448Q297,448 271,430Q244,412 227,371Q209,330 209,261L209,261Q209,204 226,166"
		     "Q242,127 273,107Q303,86 344,86L344,86Q378,86 404,101Q430,115 451,137L451,137L488,103"
		     "Q458,42 404,13Q350,-16 279,-16L279,-16Q211,-16 153,13Q95,41 60,99Q25,156 25,241L25,241"
		     "Q25,323 62,382Q99,440 163,471Q226,501 303,501L303,501Q357,501 399,481"
		     "Q440,460 464,426Q488,392 492,352L492,352Q475,297 420,297L420,297Q390,297 366,320"
		     "Q342,342 339,401L339,401L333,469L411,427Q387,438 368,443Q348,448 323,448";
  g_assert_cmpmem (str2, strlen (str2), expected2, sizeof (expected2) - 1);

  hb_font_destroy (font);
}

static void
test_hb_ot_glyph_cff1 (void)
{
  hb_ot_glyph_decompose_funcs_t funcs;
  funcs.move_to = (hb_ot_glyph_decompose_move_to_func_t) move_to;
  funcs.line_to = (hb_ot_glyph_decompose_line_to_func_t) line_to;
  funcs.conic_to = (hb_ot_glyph_decompose_conic_to_func_t) conic_to;
  funcs.cubic_to = (hb_ot_glyph_decompose_cubic_to_func_t) cubic_to;

  hb_face_t *face = hb_test_open_font_file ("fonts/cff1_seac.otf");
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);

  char str[1024] = {0};
  g_assert (hb_ot_glyph_decompose (font, 3, &funcs, str));
  char expected[] = "M203,367C227,440 248,512 268,588L272,588C293,512 314,440 338,367"
		    "L369,267L172,267M3,0L88,0L151,200L390,200L452,0L541,0L319,656L225,656"
		    "M300,653L342,694L201,861L143,806";
  g_assert_cmpmem (str, strlen (str), expected, sizeof (expected) - 1);

  hb_font_destroy (font);
}

static void
test_hb_ot_glyph_cff2 (void)
{
  hb_ot_glyph_decompose_funcs_t funcs;
  funcs.move_to = (hb_ot_glyph_decompose_move_to_func_t) move_to;
  funcs.line_to = (hb_ot_glyph_decompose_line_to_func_t) line_to;
  funcs.conic_to = (hb_ot_glyph_decompose_conic_to_func_t) conic_to;
  funcs.cubic_to = (hb_ot_glyph_decompose_cubic_to_func_t) cubic_to;

  hb_face_t *face = hb_test_open_font_file ("fonts/AdobeVFPrototype.abc.otf");
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);

  char str[1024] = {0};
  g_assert (hb_ot_glyph_decompose (font, 3, &funcs, str));
  char expected[] = "M275,442C303,442 337,435 371,417L325,454L350,366C357,341 370,321 403,321"
		    "C428,321 443,333 448,358C435,432 361,487 272,487C153,487 43,393 43,236"
		    "C43,83 129,-13 266,-13C360,-13 424,33 451,116L427,128C396,78 345,50 287,50"
		    "C193,50 126,119 126,245C126,373 188,442 275,442";
  g_assert_cmpmem (str, strlen (str), expected, sizeof (expected) - 1);

  hb_variation_t var;
  var.tag = HB_TAG ('w','g','h','t');
  var.value = 800;
  hb_font_set_variations (font, &var, 1);

  char str2[1024] = {0};
  g_assert (hb_ot_glyph_decompose (font, 3, &funcs, str2));
  char expected2[] = "M323,448C356,448 380,441 411,427L333,469L339,401C343,322 379,297 420,297"
		     "C458,297 480,314 492,352C486,433 412,501 303,501C148,501 25,406 25,241"
		     "C25,70 143,-16 279,-16C374,-16 447,22 488,103L451,137C423,107 390,86 344,86"
		     "C262,86 209,148 209,261C209,398 271,448 323,448";
  g_assert_cmpmem (str2, strlen (str2), expected2, sizeof (expected2) - 1);

  hb_font_destroy (font);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);
  hb_test_add (test_hb_ot_glyph_empty);
  hb_test_add (test_hb_ot_glyph_glyf);
  hb_test_add (test_hb_ot_glyph_cff1);
  hb_test_add (test_hb_ot_glyph_cff2);
  return hb_test_run ();
}
