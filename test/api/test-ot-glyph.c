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

static void
test_hb_ot_glyph_ttf_parser_tests (void)
{
  /* https://github.com/RazrFalcon/ttf-parser/blob/337e7d1c/tests/tests.rs#L50-L133 */
  char str[1024] = {0};
  user_data_t user_data = {
    .str = str,
    .size = sizeof (str)
  };
  {
    hb_face_t *face = hb_test_open_font_file ("fonts/glyphs.ttf");
    hb_font_t *font = hb_font_create (face);
    hb_face_destroy (face);
    {
      /* We aren't identical on paths points for glyf with ttf-parser but visually, investigate */
      user_data.consumed = 0;
      g_assert (hb_ot_glyph_decompose (font, 0, funcs, &user_data));
      char expected[] = "M450,0L50,0L50,750L450,750L450,0Z";
      g_assert_cmpmem (str, user_data.consumed, expected, sizeof (expected) - 1);
    }
    {
      user_data.consumed = 0;
      g_assert (hb_ot_glyph_decompose (font, 1, funcs, &user_data));
      char expected[] = "M514,416L56,416L56,487L514,487L514,416ZM514,217L56,217L56,288L514,288L514,217Z";
      g_assert_cmpmem (str, user_data.consumed, expected, sizeof (expected) - 1);
    }
    {
      user_data.consumed = 0;
      g_assert (hb_ot_glyph_decompose (font, 4, funcs, &user_data));
      char expected[] = "M332,536L332,468L197,468L197,0L109,0L109,468L15,468L15,509L109,539L109,570"
			"Q109,674 155,720Q201,765 283,765L283,765Q315,765 342,760Q368,754 387,747"
			"L387,747L364,678Q348,683 327,688Q306,693 284,693L284,693Q240,693 219,664"
			"Q197,634 197,571L197,571L197,536L332,536ZM474,737L474,737Q494,737 510,724"
			"Q525,710 525,681L525,681Q525,653 510,639Q494,625 474,625L474,625"
			"Q452,625 437,639Q422,653 422,681L422,681Q422,710 437,724Q452,737 474,737Z"
			"M429,536L517,536L517,0L429,0L429,536Z";
      g_assert_cmpmem (str, user_data.consumed, expected, sizeof (expected) - 1);
    }
    {
      /* According to tests on tts-parser we should return an empty on single point but we aren't */
      user_data.consumed = 0;
      g_assert (hb_ot_glyph_decompose (font, 5, funcs, &user_data));
      char expected[] = "M15,0Q15,0 15,0Z";
      g_assert_cmpmem (str, user_data.consumed, expected, sizeof (expected) - 1);
    }
    {
      user_data.consumed = 0;
      g_assert (hb_ot_glyph_decompose (font, 6, funcs, &user_data));
      char expected[] = "M346,536L346,468L211,468L211,0L123,0L123,468L29,468"
			"L29,509L123,539L123,570Q123,674 169,720Q215,765 297,765"
			"L297,765Q329,765 356,760Q382,754 401,747L401,747L378,678"
			"Q362,683 341,688Q320,693 298,693L298,693Q254,693 233,664"
			"Q211,634 211,571L211,571L211,536L346,536ZM15,0Q15,0 15,0Z";
      g_assert_cmpmem (str, user_data.consumed, expected, sizeof (expected) - 1);
    }

    hb_font_destroy (font);
  }
  {
    hb_face_t *face = hb_test_open_font_file ("fonts/cff1_flex.otf");
    hb_font_t *font = hb_font_create (face);
    hb_face_destroy (face);

    user_data.consumed = 0;
    g_assert (hb_ot_glyph_decompose (font, 1, funcs, &user_data));
    char expected[] = "M0,0C100,0 150,-20 250,-20C350,-20 400,0 500,0C500,100 520,150 520,250"
		      "C520,350 500,400 500,500C400,500 350,520 250,520C150,520 100,500 0,500"
		      "C0,400 -20,350 -20,250C-20,150 0,100 0,0ZM50,50C50,130 34,170 34,250"
		      "C34,330 50,370 50,450C130,450 170,466 250,466C330,466 370,450 450,450"
		      "C450,370 466,330 466,250C466,170 450,130 450,50C370,50 330,34 250,34"
		      "C170,34 130,50 50,50Z";
    g_assert_cmpmem (str, user_data.consumed, expected, sizeof (expected) - 1);

    hb_font_destroy (font);
  }
  {
    hb_face_t *face = hb_test_open_font_file ("fonts/cff1_dotsect.nohints.otf");
    hb_font_t *font = hb_font_create (face);
    hb_face_destroy (face);

    user_data.consumed = 0;
    g_assert (hb_ot_glyph_decompose (font, 1, funcs, &user_data));
    char expected[] = "M82,0L164,0L164,486L82,486ZM124,586C156,586 181,608 181,639"
		      "C181,671 156,692 124,692C92,692 67,671 67,639C67,608 92,586 124,586Z";
    g_assert_cmpmem (str, user_data.consumed, expected, sizeof (expected) - 1);

    hb_font_destroy (font);
  }
}

/* TODO: https://github.com/foliojs/fontkit/blob/master/test/glyphs.js */

static void
test_hb_ot_glyph_font_kit_variations_tests (void)
{
  /* https://github.com/foliojs/fontkit/blob/b310db5/test/variations.js */
  char str[4096] = {0};
  user_data_t user_data = {
    .str = str,
    .size = sizeof (str)
  };
  /* Skia */
  {
    /* Skipping Skia tests for now even the fact we can actually do platform specific tests using our CIs */
  }
  /* truetype variations */
  /* should support sharing all points */
  {
    hb_face_t *face = hb_test_open_font_file ("fonts/TestGVAROne.ttf");
    hb_font_t *font = hb_font_create (face);
    hb_face_destroy (face);

    hb_variation_t var;
    var.tag = HB_TAG ('w','g','h','t');
    var.value = 300;
    hb_font_set_variations (font, &var, 1);

    hb_buffer_t *buffer = hb_buffer_create ();
    hb_codepoint_t codepoint = 24396; /* 彌 */
    hb_buffer_add_codepoints (buffer, &codepoint, 1, 0, -1);
    hb_buffer_set_direction (buffer, HB_DIRECTION_LTR);
    hb_shape (font, buffer, NULL, 0);
    codepoint = hb_buffer_get_glyph_infos (buffer, NULL)[0].codepoint;
    hb_buffer_destroy (buffer);

    user_data.consumed = 0;
    g_assert (hb_ot_glyph_decompose (font, codepoint, funcs, &user_data));
    char expected[] = "M414,-102L371,-102L371,539L914,539L914,-27Q914,-102 840,-102L840,-102Q796,-102 755,-98"
		      "L755,-98L742,-59Q790,-66 836,-66L836,-66Q871,-66 871,-31L871,-31L871,504L414,504L414,-102Z"
		      "M203,-94L203,-94Q138,-94 86,-90L86,-90L74,-52Q137,-59 188,-59L188,-59Q211,-59 222,-47"
		      "Q233,-34 236,12Q238,58 240,135Q242,211 242,262L242,262L74,262L94,527L242,527L242,719"
		      "L63,719L63,754L285,754L285,492L133,492L117,297L285,297Q285,241 284,185L284,185"
		      "Q284,104 281,46L281,46Q278,-20 269,-49Q260,-78 242,-86Q223,-94 203,-94ZM461,12L461,12"
		      "L434,43Q473,73 503,115L503,115Q478,150 441,188L441,188L469,211Q501,179 525,147"
		      "L525,147Q538,172 559,230L559,230L594,211Q571,152 551,117L551,117Q577,84 602,43L602,43"
		      "L566,20Q544,64 528,86L528,86Q500,44 461,12ZM465,258L465,258L438,285Q474,316 501,351L501,351"
		      "Q474,388 445,418L445,418L473,441Q500,414 523,381L523,381Q546,413 563,453L563,453"
		      "L598,434Q571,382 549,352L549,352Q576,320 598,285L598,285L563,262Q546,294 525,322"
		      "L525,322Q491,280 465,258ZM707,12L707,12L680,43Q717,68 753,115L753,115Q731,147 691,188"
		      "L691,188L719,211Q739,190 754,172Q769,154 774,147L774,147Q793,185 809,230L809,230"
		      "L844,211Q822,155 801,117L801,117Q828,82 852,43L852,43L820,20Q798,58 778,87"
		      "L778,87Q747,43 707,12ZM664,-94L621,-94L621,730L664,730L664,-94ZM348,570L348,570"
		      "L324,605Q425,629 527,688L527,688L555,656Q491,621 439,601Q386,581 348,570ZM715,258"
		      "L715,258L688,285Q727,318 753,351L753,351Q733,378 695,418L695,418L723,441Q754,410 775,381"
		      "L775,381Q794,407 813,453L813,453L848,434Q826,387 801,352L801,352Q823,321 848,281"
		      "L848,281L813,262Q791,301 775,323L775,323Q749,288 715,258ZM941,719L348,719L348,754"
		      "L941,754L941,719ZM957,605L936,570Q870,602 817,622Q764,641 727,652L727,652L749,688"
		      "Q852,655 957,605L957,605Z";
    g_assert_cmpmem (str, user_data.consumed, expected, sizeof (expected) - 1);

    hb_font_destroy (font);
  }
  /* should support sharing enumerated points */
  {
    hb_face_t *face = hb_test_open_font_file ("fonts/TestGVARTwo.ttf");
    hb_font_t *font = hb_font_create (face);
    hb_face_destroy (face);

    hb_variation_t var;
    var.tag = HB_TAG ('w','g','h','t');
    var.value = 300;
    hb_font_set_variations (font, &var, 1);

    hb_buffer_t *buffer = hb_buffer_create ();
    hb_codepoint_t codepoint = 24396; /* 彌 */
    hb_buffer_add_codepoints (buffer, &codepoint, 1, 0, -1);
    hb_buffer_set_direction (buffer, HB_DIRECTION_LTR);
    hb_shape (font, buffer, NULL, 0);
    codepoint = hb_buffer_get_glyph_infos (buffer, NULL)[0].codepoint;
    hb_buffer_destroy (buffer);

    user_data.consumed = 0;
    g_assert (hb_ot_glyph_decompose (font, codepoint, funcs, &user_data));
    char expected[] = "M414,-102L371,-102L371,539L914,539L914,-27Q914,-102 840,-102L840,-102Q796,-102 755,-98"
		      "L755,-98L742,-59Q790,-66 836,-66L836,-66Q871,-66 871,-31L871,-31L871,504L414,504"
		      "L414,-102ZM203,-94L203,-94Q138,-94 86,-90L86,-90L74,-52Q137,-59 188,-59L188,-59"
		      "Q211,-59 222,-47Q233,-34 236,12Q238,58 240,135Q242,211 242,262L242,262L74,262"
		      "L94,527L242,527L242,719L63,719L63,754L285,754L285,492L133,492L117,297"
		      "L285,297Q285,241 284,185L284,185Q284,104 281,46L281,46Q278,-20 269,-49Q260,-78 242,-86"
		      "Q223,-94 203,-94ZM461,12L461,12L434,43Q473,73 503,115L503,115Q478,150 441,188L441,188"
		      "L469,211Q501,179 525,147L525,147Q538,172 559,230L559,230L594,211Q571,152 551,117"
		      "L551,117Q577,84 602,43L602,43L566,20Q544,64 528,86L528,86Q500,44 461,12ZM465,258"
		      "L465,258L438,285Q474,316 501,351L501,351Q474,388 445,418L445,418L473,441"
		      "Q500,414 523,381L523,381Q546,413 563,453L563,453L598,434Q571,382 549,352L549,352"
		      "Q576,320 598,285L598,285L563,262Q546,294 525,322L525,322Q491,280 465,258ZM707,12"
		      "L707,12L680,43Q717,68 753,115L753,115Q731,147 691,188L691,188L719,211Q739,190 754,172"
		      "Q769,154 774,147L774,147Q793,185 809,230L809,230L844,211Q822,155 801,117L801,117"
		      "Q828,82 852,43L852,43L820,20Q798,58 778,87L778,87Q747,43 707,12ZM664,-94L621,-94L621,730"
		      "L664,730L664,-94ZM348,570L348,570L324,605Q425,629 527,688L527,688L555,656Q491,621 439,601"
		      "Q386,581 348,570ZM715,258L715,258L688,285Q727,318 753,351L753,351Q733,378 695,418"
		      "L695,418L723,441Q754,410 775,381L775,381Q794,407 813,453L813,453L848,434Q826,387 801,352"
		      "L801,352Q823,321 848,281L848,281L813,262Q791,301 775,323L775,323Q749,288 715,258ZM941,719"
		      "L348,719L348,754L941,754L941,719ZM957,605L936,570Q870,602 817,622Q764,641 727,652L727,652"
		      "L749,688Q852,655 957,605L957,605Z";
    g_assert_cmpmem (str, user_data.consumed, expected, sizeof (expected) - 1);

    hb_font_destroy (font);
  }
  /* should support sharing no points */
  {
    hb_face_t *face = hb_test_open_font_file ("fonts/TestGVARThree.ttf");
    hb_font_t *font = hb_font_create (face);
    hb_face_destroy (face);

    hb_variation_t var;
    var.tag = HB_TAG ('w','g','h','t');
    var.value = 300;
    hb_font_set_variations (font, &var, 1);

    hb_buffer_t *buffer = hb_buffer_create ();
    hb_codepoint_t codepoint = 24396; /* 彌 */
    hb_buffer_add_codepoints (buffer, &codepoint, 1, 0, -1);
    hb_buffer_set_direction (buffer, HB_DIRECTION_LTR);
    hb_shape (font, buffer, NULL, 0);
    codepoint = hb_buffer_get_glyph_infos (buffer, NULL)[0].codepoint;
    hb_buffer_destroy (buffer);

    user_data.consumed = 0;
    g_assert (hb_ot_glyph_decompose (font, codepoint, funcs, &user_data));
    char expected[] = "M414,-102L371,-102L371,539L914,539L914,-27Q914,-102 840,-102L840,-102"
		      "Q796,-102 755,-98L755,-98L742,-59Q790,-66 836,-66L836,-66Q871,-66 871,-31"
		      "L871,-31L871,504L414,504L414,-102ZM203,-94L203,-94Q138,-94 86,-90"
		      "L86,-90L74,-52Q137,-59 188,-59L188,-59Q211,-59 222,-47Q233,-34 236,12"
		      "Q238,58 240,135Q242,211 242,262L242,262L74,262L94,527L242,527L242,719"
		      "L63,719L63,754L285,754L285,492L133,492L117,297L285,297Q285,241 284,185"
		      "L284,185Q284,104 281,46L281,46Q278,-20 269,-49Q260,-78 242,-86Q223,-94 203,-94Z"
		      "M461,12L461,12L434,43Q473,73 503,115L503,115Q478,150 441,188L441,188L469,211"
		      "Q501,179 525,147L525,147Q538,172 559,230L559,230L594,211Q571,152 551,117L551,117"
		      "Q577,84 602,43L602,43L566,20Q544,64 528,86L528,86Q500,44 461,12ZM465,258L465,258L438,285"
		      "Q474,316 501,351L501,351Q474,388 445,418L445,418L473,441Q500,414 523,381L523,381"
		      "Q546,413 563,453L563,453L598,434Q571,382 549,352L549,352Q576,320 598,285L598,285"
		      "L563,262Q546,294 525,322L525,322Q491,280 465,258ZM707,12L707,12L680,43Q717,68 753,115"
		      "L753,115Q731,147 691,188L691,188L719,211Q739,190 754,172Q769,154 774,147L774,147"
		      "Q793,185 809,230L809,230L844,211Q822,155 801,117L801,117Q828,82 852,43L852,43L820,20"
		      "Q798,58 778,87L778,87Q747,43 707,12ZM664,-94L621,-94L621,730L664,730L664,-94Z"
		      "M348,570L348,570L324,605Q425,629 527,688L527,688L555,656Q491,621 439,601"
		      "Q386,581 348,570ZM715,258L715,258L688,285Q727,318 753,351L753,351Q733,378 695,418"
		      "L695,418L723,441Q754,410 775,381L775,381Q794,407 813,453L813,453L848,434"
		      "Q826,387 801,352L801,352Q823,321 848,281L848,281L813,262Q791,301 775,323L775,323"
		      "Q749,288 715,258ZM941,719L348,719L348,754L941,754L941,719ZM957,605L936,570"
		      "Q870,602 817,622Q764,641 727,652L727,652L749,688Q852,655 957,605L957,605Z";
    g_assert_cmpmem (str, user_data.consumed, expected, sizeof (expected) - 1);

    hb_font_destroy (font);
  }

  /* CFF2 variations */
  {
    hb_face_t *face = hb_test_open_font_file ("fonts/AdobeVFPrototype-Subset.otf");
    hb_font_t *font = hb_font_create (face);
    hb_face_destroy (face);

    hb_variation_t var;
    var.tag = HB_TAG ('w','g','h','t');
    /* applies variations to CFF2 glyphs */
    {
      var.value = 100;
      hb_font_set_variations (font, &var, 1);

      hb_buffer_t *buffer = hb_buffer_create ();
      hb_codepoint_t codepoint = '$';
      hb_buffer_add_codepoints (buffer, &codepoint, 1, 0, -1);
      hb_buffer_set_direction (buffer, HB_DIRECTION_LTR);
      hb_shape (font, buffer, NULL, 0);
      codepoint = hb_buffer_get_glyph_infos (buffer, NULL)[0].codepoint;
      hb_buffer_destroy (buffer);

      user_data.consumed = 0;
      g_assert (hb_ot_glyph_decompose (font, codepoint, funcs, &user_data));
      char expected[] = "M246,15C188,15 147,27 101,68L142,23L117,117C111,143 96,149 81,149"
		        "C65,149 56,141 52,126C71,40 137,-13 244,-13C348,-13 436,46 436,156"
		        "C436,229 405,295 271,349L247,359C160,393 119,439 119,506"
		        "C119,592 178,637 262,637C311,637 346,626 390,585L348,629L373,535"
		        "C380,510 394,503 408,503C424,503 434,510 437,526C418,614 348,665 259,665"
		        "C161,665 78,606 78,500C78,414 128,361 224,321L261,305C367,259 395,217 395,152"
		        "C395,65 334,15 246,15ZM267,331L267,759L240,759L240,331L267,331ZM240,-115"
		        "L267,-115L267,331L240,331L240,-115Z";
      g_assert_cmpmem (str, user_data.consumed, expected, sizeof (expected) - 1);
    }
    {
      var.value = 500;
      hb_font_set_variations (font, &var, 1);

      hb_buffer_t *buffer = hb_buffer_create ();
      hb_codepoint_t codepoint = '$';
      hb_buffer_add_codepoints (buffer, &codepoint, 1, 0, -1);
      hb_buffer_set_direction (buffer, HB_DIRECTION_LTR);
      hb_shape (font, buffer, NULL, 0);
      codepoint = hb_buffer_get_glyph_infos (buffer, NULL)[0].codepoint;
      hb_buffer_destroy (buffer);

      user_data.consumed = 0;
      g_assert (hb_ot_glyph_decompose (font, codepoint, funcs, &user_data));
      char expected[] = "M251,36C206,36 165,42 118,61L176,21L161,99C151,152 129,167 101,167"
			"C78,167 61,155 51,131C54,43 133,-14 247,-14C388,-14 474,64 474,171"
			"C474,258 430,321 294,370L257,383C188,406 150,438 150,499"
			"C150,571 204,606 276,606C308,606 342,601 386,582L327,621"
			"L343,546C355,490 382,476 408,476C428,476 448,487 455,512"
			"C450,597 370,656 264,656C140,656 57,576 57,474C57,373 119,318 227,279"
			"L263,266C345,236 379,208 379,145C379,76 329,36 251,36ZM289,320"
			"L289,746L242,746L242,320L289,320ZM240,-115L286,-115L286,320L240,320L240,-115Z";
      g_assert_cmpmem (str, user_data.consumed, expected, sizeof (expected) - 1);
    }
    /* substitutes GSUB features depending on variations */
    {
      var.value = 900;
      hb_font_set_variations (font, &var, 1);

      hb_buffer_t *buffer = hb_buffer_create ();
      hb_codepoint_t codepoint = '$';
      hb_buffer_add_codepoints (buffer, &codepoint, 1, 0, -1);
      hb_buffer_set_direction (buffer, HB_DIRECTION_LTR);
      hb_shape (font, buffer, NULL, 0);
      codepoint = hb_buffer_get_glyph_infos (buffer, NULL)[0].codepoint;
      hb_buffer_destroy (buffer);

      user_data.consumed = 0;
      g_assert (hb_ot_glyph_decompose (font, codepoint, funcs, &user_data));
      char expected[] = "M258,38C197,38 167,48 118,71L192,19L183,103C177,155 155,174 115,174"
			"C89,174 64,161 51,125C52,36 124,-16 258,-16C417,-16 513,67 513,175"
			"C513,278 457,328 322,388L289,403C232,429 203,452 203,500C203,562 244,589 301,589"
			"C342,589 370,585 420,562L341,607L352,539C363,468 398,454 434,454C459,454 486,468 492,506"
			"C491,590 408,643 290,643C141,643 57,563 57,460C57,357 122,307 233,256L265,241"
			"C334,209 363,186 363,130C363,77 320,38 258,38ZM318,616L318,734L252,734L252,616"
			"L318,616ZM253,-115L319,-115L319,14L253,14L253,-115Z";
      g_assert_cmpmem (str, user_data.consumed, expected, sizeof (expected) - 1);
    }

    hb_font_destroy (font);
  }
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
  hb_test_add (test_hb_ot_glyph_ttf_parser_tests);
  hb_test_add (test_hb_ot_glyph_font_kit_variations_tests);
  unsigned result = hb_test_run ();

  hb_ot_glyph_decompose_funcs_destroy (funcs);
  return result;
}
