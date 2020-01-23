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

typedef struct user_data_t {
  char *str;
  unsigned size;
  unsigned consumed;
} user_data_t;

static void
move_to (hb_position_t to_x, hb_position_t to_y, user_data_t *user_data)
{
  user_data->consumed += snprintf (user_data->str + user_data->consumed,
				   user_data->size - user_data->consumed,
				   "M%d,%d", to_x, to_y);
}

static void
line_to (hb_position_t to_x, hb_position_t to_y, user_data_t *user_data)
{
  user_data->consumed += snprintf (user_data->str + user_data->consumed,
				   user_data->size - user_data->consumed,
				   "L%d,%d", to_x, to_y);
}

static void
conic_to (hb_position_t control_x, hb_position_t control_y,
	  hb_position_t to_x, hb_position_t to_y,
	  user_data_t *user_data)
{
  user_data->consumed += snprintf (user_data->str + user_data->consumed,
				   user_data->size - user_data->consumed,
				   "Q%d,%d %d,%d",
				   control_x, control_y,
				   to_x, to_y);
}

static void
cubic_to (hb_position_t control1_x, hb_position_t control1_y,
	  hb_position_t control2_x, hb_position_t control2_y,
	  hb_position_t to_x, hb_position_t to_y,
	  user_data_t *user_data)
{
  user_data->consumed += snprintf (user_data->str + user_data->consumed,
				   user_data->size - user_data->consumed,
				   "C%d,%d %d,%d %d,%d",
				   control1_x, control1_y,
				   control2_x, control2_y,
				   to_x, to_y);
}

static void
close_path (user_data_t *user_data)
{
  user_data->consumed += snprintf (user_data->str + user_data->consumed,
				   user_data->size - user_data->consumed,
				   "Z");
}

static hb_ot_glyph_decompose_funcs_t *funcs;

static void
test_hb_ot_glyph_empty (void)
{
  g_assert (!hb_ot_glyph_decompose (hb_font_get_empty (), 3, funcs, NULL));
}

static void
test_hb_ot_glyph_glyf (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/SourceSerifVariable-Roman-VVAR.abc.ttf");
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);

  char str[1024] = {0};
  user_data_t user_data = {
    .str = str,
    .size = sizeof (str),
    .consumed = 0
  };
  g_assert (!hb_ot_glyph_decompose (font, 4, funcs, &user_data));
  g_assert (hb_ot_glyph_decompose (font, 3, funcs, &user_data));
  char expected[] = "M275,442L275,442Q232,442 198,420Q164,397 145,353Q126,309 126,245L126,245"
		    "Q126,182 147,139Q167,95 204,73Q240,50 287,50L287,50Q330,50 367,70"
		    "Q404,90 427,128L427,128L451,116Q431,54 384,21Q336,-13 266,-13L266,-13Q198,-13 148,18"
		    "Q97,48 70,104Q43,160 43,236L43,236Q43,314 76,371Q108,427 160,457Q212,487 272,487L272,487"
		    "Q316,487 354,470Q392,453 417,424Q442,395 448,358L448,358Q441,321 403,321L403,321"
		    "Q378,321 367,334Q355,347 350,366L350,366L325,454L371,417Q346,430 321,436Q296,442 275,442Z";
  g_assert_cmpmem (str, user_data.consumed, expected, sizeof (expected) - 1);

  hb_variation_t var;
  var.tag = HB_TAG ('w','g','h','t');
  var.value = 800;
  hb_font_set_variations (font, &var, 1);

  char str2[1024] = {0};
  user_data_t user_data2 = {
    .str = str2,
    .size = sizeof (str2),
    .consumed = 0
  };
  g_assert (hb_ot_glyph_decompose (font, 3, funcs, &user_data2));
  char expected2[] = "M323,448L323,448Q297,448 271,430Q244,412 227,371"
		     "Q209,330 209,261L209,261Q209,204 226,166Q242,127 273,107Q303,86 344,86L344,86Q378,86 404,101"
		     "Q430,115 451,137L451,137L488,103Q458,42 404,13Q350,-16 279,-16L279,-16Q211,-16 153,13Q95,41 60,99"
		     "Q25,156 25,241L25,241Q25,323 62,382Q99,440 163,471Q226,501 303,501L303,501Q357,501 399,481"
		     "Q440,460 464,426Q488,392 492,352L492,352Q475,297 420,297L420,297Q390,297 366,320"
		     "Q342,342 339,401L339,401L333,469L411,427Q387,438 368,443Q348,448 323,448Z";
  g_assert_cmpmem (str2, user_data2.consumed, expected2, sizeof (expected2) - 1);

  hb_font_destroy (font);
}

static void
test_hb_ot_glyph_cff1 (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/cff1_seac.otf");
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);

  char str[1024] = {0};
  user_data_t user_data = {
    .str = str,
    .size = sizeof (str),
    .consumed = 0
  };
  g_assert (hb_ot_glyph_decompose (font, 3, funcs, &user_data));
  char expected[] = "M203,367C227,440 248,512 268,588L272,588C293,512 314,440 338,367L369,267L172,267Z"
		    "M3,0L88,0L151,200L390,200L452,0L541,0L319,656L225,656Z"
		    "M300,653L342,694L201,861L143,806Z";
  g_assert_cmpmem (str, user_data.consumed, expected, sizeof (expected) - 1);

  hb_font_destroy (font);
}

static void
test_hb_ot_glyph_cff1_rline (void)
{
  /* https://github.com/harfbuzz/harfbuzz/pull/2053 */
  hb_face_t *face = hb_test_open_font_file ("fonts/RanaKufi-Regular.subset.otf");
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);

  char str[1024] = {0};
  user_data_t user_data = {
    .str = str,
    .size = sizeof (str),
    .consumed = 0
  };
  g_assert (hb_ot_glyph_decompose (font, 1, funcs, &user_data));
  char expected[] = "M775,400C705,400 650,343 650,274L650,250L391,250L713,572L392,893"
		    "L287,1000C311,942 296,869 250,823C250,823 286,858 321,823L571,572"
		    "L150,150L750,150L750,276C750,289 761,300 775,300C789,300 800,289 800,276"
		    "L800,100L150,100C100,100 100,150 100,150C100,85 58,23 0,0L900,0L900,274"
		    "C900,343 844,400 775,400Z";
  g_assert_cmpmem (str, user_data.consumed, expected, sizeof (expected) - 1);

  hb_font_destroy (font);
}

static void
test_hb_ot_glyph_cff2 (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/AdobeVFPrototype.abc.otf");
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);

  char str[1024] = {0};
  user_data_t user_data = {
    .str = str,
    .size = sizeof (str),
    .consumed = 0
  };
  g_assert (hb_ot_glyph_decompose (font, 3, funcs, &user_data));
  char expected[] = "M275,442C303,442 337,435 371,417L325,454L350,366"
		    "C357,341 370,321 403,321C428,321 443,333 448,358"
		    "C435,432 361,487 272,487C153,487 43,393 43,236"
		    "C43,83 129,-13 266,-13C360,-13 424,33 451,116L427,128"
		    "C396,78 345,50 287,50C193,50 126,119 126,245C126,373 188,442 275,442Z";
  g_assert_cmpmem (str, user_data.consumed, expected, sizeof (expected) - 1);

  hb_variation_t var;
  var.tag = HB_TAG ('w','g','h','t');
  var.value = 800;
  hb_font_set_variations (font, &var, 1);

  char str2[1024] = {0};
  user_data_t user_data2 = {
    .str = str2,
    .size = sizeof (str2),
    .consumed = 0
  };
  g_assert (hb_ot_glyph_decompose (font, 3, funcs, &user_data2));
  char expected2[] = "M323,448C356,448 380,441 411,427L333,469L339,401"
		     "C343,322 379,297 420,297C458,297 480,314 492,352"
		     "C486,433 412,501 303,501C148,501 25,406 25,241"
		     "C25,70 143,-16 279,-16C374,-16 447,22 488,103L451,137"
		     "C423,107 390,86 344,86C262,86 209,148 209,261C209,398 271,448 323,448Z";
  g_assert_cmpmem (str2, user_data2.consumed, expected2, sizeof (expected2) - 1);

  hb_font_destroy (font);
}

int
main (int argc, char **argv)
{
  funcs = hb_ot_glyph_decompose_funcs_create ();
  hb_ot_glyph_decompose_funcs_set_move_to_func (funcs, (hb_ot_glyph_decompose_move_to_func_t) move_to);
  hb_ot_glyph_decompose_funcs_set_line_to_func (funcs, (hb_ot_glyph_decompose_line_to_func_t) line_to);
  hb_ot_glyph_decompose_funcs_set_conic_to_func (funcs, (hb_ot_glyph_decompose_conic_to_func_t) conic_to);
  hb_ot_glyph_decompose_funcs_set_cubic_to_func (funcs, (hb_ot_glyph_decompose_cubic_to_func_t) cubic_to);
  hb_ot_glyph_decompose_funcs_set_close_path_func (funcs, (hb_ot_glyph_decompose_close_path_func_t) close_path);

  hb_test_init (&argc, &argv);
  hb_test_add (test_hb_ot_glyph_empty);
  hb_test_add (test_hb_ot_glyph_glyf);
  hb_test_add (test_hb_ot_glyph_cff1);
  hb_test_add (test_hb_ot_glyph_cff1_rline);
  hb_test_add (test_hb_ot_glyph_cff2);
  unsigned result = hb_test_run ();

  hb_ot_glyph_decompose_funcs_destroy (funcs);
  return result;
}
