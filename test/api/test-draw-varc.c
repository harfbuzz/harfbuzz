/*
 * Copyright © 2024  Behdad Esfahbod
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
#include <math.h>

#include <hb.h>
#include <hb-ot.h>

typedef struct draw_data_t
{
  unsigned move_to_count;
  unsigned line_to_count;
  unsigned quad_to_count;
  unsigned cubic_to_count;
  unsigned close_path_count;
  float first_move_x;
} draw_data_t;

typedef struct budget_draw_data_t
{
  draw_data_t draw;
  int64_t policy;
  int64_t remaining;
} budget_draw_data_t;

/* Our modified itoa, why not using libc's? it is going to be used
   in harfbuzzjs where libc isn't available */
static void _hb_reverse (char *buf, unsigned int len)
{
  unsigned start = 0, end = len - 1;
  while (start < end)
  {
    char c = buf[end];
    buf[end] = buf[start];
    buf[start] = c;
    start++; end--;
  }
}
static unsigned _hb_itoa (float fnum, char *buf)
{
  int32_t num = (int32_t) floorf (fnum + .5f);
  unsigned int i = 0;
  hb_bool_t is_negative = num < 0;
  if (is_negative) num = -num;
  do
  {
    buf[i++] = '0' + num % 10;
    num /= 10;
  } while (num);
  if (is_negative) buf[i++] = '-';
  _hb_reverse (buf, i);
  buf[i] = '\0';
  return i;
}

#define ITOA_BUF_SIZE 12 // 10 digits in int32, 1 for negative sign, 1 for \0

static void
test_itoa (void)
{
  char s[] = "12345";
  _hb_reverse (s, 5);
  g_assert_cmpmem (s, 5, "54321", 5);

  {
    unsigned num = 12345;
    char buf[ITOA_BUF_SIZE];
    unsigned len = _hb_itoa (num, buf);
    g_assert_cmpmem (buf, len, "12345", 5);
  }

  {
    unsigned num = 3152;
    char buf[ITOA_BUF_SIZE];
    unsigned len = _hb_itoa (num, buf);
    g_assert_cmpmem (buf, len, "3152", 4);
  }

  {
    int num = -6457;
    char buf[ITOA_BUF_SIZE];
    unsigned len = _hb_itoa (num, buf);
    g_assert_cmpmem (buf, len, "-6457", 5);
  }
}

static void
move_to (HB_UNUSED hb_draw_funcs_t *dfuncs, void *draw_data_,
	 HB_UNUSED hb_draw_state_t *st,
	 float to_x, HB_UNUSED float to_y,
	 HB_UNUSED void *user_data)
{
  draw_data_t *draw_data = (draw_data_t *) draw_data_;
  if (!draw_data->move_to_count)
    draw_data->first_move_x = to_x;
  draw_data->move_to_count++;
}

static void
line_to (HB_UNUSED hb_draw_funcs_t *dfuncs, void *draw_data_,
	 HB_UNUSED hb_draw_state_t *st,
	 HB_UNUSED float to_x, HB_UNUSED float to_y,
	 HB_UNUSED void *user_data)
{
  draw_data_t *draw_data = (draw_data_t *) draw_data_;
  draw_data->line_to_count++;
}

static void
quadratic_to (HB_UNUSED hb_draw_funcs_t *dfuncs, void *draw_data_,
	      HB_UNUSED hb_draw_state_t *st,
	      HB_UNUSED float control_x, HB_UNUSED float control_y,
	      HB_UNUSED float to_x, HB_UNUSED float to_y,
	      HB_UNUSED void *user_data)
{
  draw_data_t *draw_data = (draw_data_t *) draw_data_;
  draw_data->quad_to_count++;
}

static void
cubic_to (HB_UNUSED hb_draw_funcs_t *dfuncs, void *draw_data_,
	  HB_UNUSED hb_draw_state_t *st,
	  HB_UNUSED float control1_x, HB_UNUSED float control1_y,
	  HB_UNUSED float control2_x, HB_UNUSED float control2_y,
	  HB_UNUSED float to_x, HB_UNUSED float to_y,
	  HB_UNUSED void *user_data)
{
  draw_data_t *draw_data = (draw_data_t *) draw_data_;
  draw_data->cubic_to_count++;
}

static void
close_path (HB_UNUSED hb_draw_funcs_t *dfuncs, void *draw_data_,
	    HB_UNUSED hb_draw_state_t *st,
	    HB_UNUSED void *user_data)
{
  draw_data_t *draw_data = (draw_data_t *) draw_data_;
  draw_data->close_path_count++;
}

static hb_bool_t
set_budget (HB_UNUSED hb_draw_funcs_t *dfuncs, void *draw_data_,
	    int64_t budget, HB_UNUSED void *user_data)
{
  budget_draw_data_t *draw_data = (budget_draw_data_t *) draw_data_;
  draw_data->policy = budget;
  draw_data->remaining = budget == HB_BUDGET_DEFAULT ? 1 << 20 : budget;
  return TRUE;
}

static int64_t
get_budget (HB_UNUSED hb_draw_funcs_t *dfuncs, void *draw_data_,
	    HB_UNUSED void *user_data)
{
  return ((budget_draw_data_t *) draw_data_)->policy;
}

static int64_t *
get_budget_remaining (HB_UNUSED hb_draw_funcs_t *dfuncs, void *draw_data_,
		      HB_UNUSED void *user_data)
{
  return &((budget_draw_data_t *) draw_data_)->remaining;
}

static hb_draw_funcs_t *funcs;

#ifdef HB_EXPERIMENTAL_API
static void
test_hb_draw_varc_simple_hangul (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/varc-ac00-ac01.ttf");
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);

  draw_data_t draw_data0 = {0};
  draw_data_t draw_data;;
  unsigned gid = 0;

  hb_font_get_nominal_glyph (font, 0xAC00u, &gid);
  draw_data = draw_data0;
  hb_font_draw_glyph (font, gid, funcs, &draw_data);
  g_assert_cmpuint (draw_data.move_to_count, ==, 3);

  hb_font_get_nominal_glyph (font, 0xAC01u, &gid);
  draw_data = draw_data0;
  hb_font_draw_glyph (font, gid, funcs, &draw_data);
  g_assert_cmpuint (draw_data.move_to_count, ==, 4);

  hb_variation_t var;
  var.tag = HB_TAG ('w','g','h','t');
  var.value = 800;
  hb_font_set_variations (font, &var, 1);

  hb_font_get_nominal_glyph (font, 0xAC00u, &gid);
  draw_data = draw_data0;
  hb_font_draw_glyph (font, gid, funcs, &draw_data);
  g_assert_cmpuint (draw_data.move_to_count, ==, 3);

  hb_font_get_nominal_glyph (font, 0xAC01u, &gid);
  draw_data = draw_data0;
  hb_font_draw_glyph (font, gid, funcs, &draw_data);
  g_assert_cmpuint (draw_data.move_to_count, ==, 4);

  hb_font_destroy (font);
}

static void
test_hb_draw_varc_simple_hanzi (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/varc-6868.ttf");
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);

  draw_data_t draw_data0 = {0};
  draw_data_t draw_data;;
  unsigned gid = 0;

  hb_font_get_nominal_glyph (font, 0x6868u, &gid);
  draw_data = draw_data0;
  hb_font_draw_glyph (font, gid, funcs, &draw_data);
  g_assert_cmpuint (draw_data.move_to_count, ==, 11);

  hb_variation_t var;
  var.tag = HB_TAG ('w','g','h','t');
  var.value = 800;
  hb_font_set_variations (font, &var, 1);

  hb_font_get_nominal_glyph (font, 0x6868u, &gid);
  draw_data = draw_data0;
  hb_font_draw_glyph (font, gid, funcs, &draw_data);
  g_assert_cmpuint (draw_data.move_to_count, ==, 11);

  hb_font_destroy (font);
}

static void
test_hb_draw_varc_conditional (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/varc-ac01-conditional.ttf");
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);

  draw_data_t draw_data0 = {0};
  draw_data_t draw_data;;
  unsigned gid = 0;

  hb_font_get_nominal_glyph (font, 0xAC01u, &gid);
  draw_data = draw_data0;
  hb_font_draw_glyph (font, gid, funcs, &draw_data);
  g_assert_cmpuint (draw_data.move_to_count, ==, 2);

  hb_variation_t var;
  var.tag = HB_TAG ('w','g','h','t');
  var.value = 800;
  hb_font_set_variations (font, &var, 1);

  hb_font_get_nominal_glyph (font, 0xAC01u, &gid);
  draw_data = draw_data0;
  hb_font_draw_glyph (font, gid, funcs, &draw_data);
  g_assert_cmpuint (draw_data.move_to_count, ==, 4);

  hb_font_destroy (font);
}

static void
test_hb_draw_varc_static_gvar (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/varc-static-gvar.ttf");
  g_assert_cmpuint (hb_ot_var_get_axis_count (face), ==, 0);
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);

  hb_codepoint_t gid = 0;
  g_assert_true (hb_font_get_nominal_glyph (font, 'a', &gid));

  draw_data_t draw_data = {0};
  hb_font_draw_glyph (font, gid, funcs, &draw_data);
  g_assert_cmpuint (draw_data.move_to_count, ==, 1);
  g_assert_cmpuint (draw_data.line_to_count, ==, 3);
  g_assert_cmpfloat (draw_data.first_move_x, ==, 50.f);

  hb_font_destroy (font);
}

static void
test_hb_draw_varc_short_cff2 (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/varc-short-cff2.otf");
  g_assert_cmpuint (hb_face_get_glyph_count (face), ==, 4);
  hb_font_t *font = hb_font_create (face);

  draw_data_t draw_data = {0};
  hb_font_draw_glyph (font, 1, funcs, &draw_data);
  g_assert_cmpuint (draw_data.move_to_count, ==, 1);
  g_assert_cmpfloat (draw_data.first_move_x, ==, 100.f);

  draw_data = (draw_data_t) {0};
  hb_font_draw_glyph (font, 3, funcs, &draw_data);
  g_assert_cmpuint (draw_data.move_to_count, ==, 1);
  g_assert_cmpuint (draw_data.line_to_count, ==, 3);
  g_assert_cmpfloat (draw_data.first_move_x, ==, 600.f);
  hb_glyph_extents_t extents;
  g_assert_true (hb_font_get_glyph_extents (font, 3, &extents));
  g_assert_cmpint (extents.x_bearing, ==, 600);
  g_assert_cmpint (extents.y_bearing, ==, 200);
  g_assert_cmpint (extents.width, ==, 200);
  g_assert_cmpint (extents.height, ==, -200);
  hb_font_destroy (font);

  /* A count mismatch without VARC must still disable the CFF2 accelerator. */
  hb_face_t *without_varc = hb_face_builder_create ();
  const hb_tag_t tags[] = {
    HB_TAG ('C','F','F','2'), HB_TAG ('m','a','x','p'), HB_TAG ('h','e','a','d'),
    HB_TAG ('h','m','t','x'), HB_TAG ('h','h','e','a')
  };
  for (unsigned i = 0; i < G_N_ELEMENTS (tags); i++)
  {
    hb_blob_t *blob = hb_face_reference_table (face, tags[i]);
    g_assert_true (hb_face_builder_add_table (without_varc, tags[i], blob));
    hb_blob_destroy (blob);
  }
  font = hb_font_create (without_varc);
  draw_data = (draw_data_t) {0};
  hb_font_draw_glyph (font, 1, funcs, &draw_data);
  g_assert_cmpuint (draw_data.move_to_count, ==, 0);
  hb_font_destroy (font);
  hb_face_destroy (without_varc);

  /* VARC does not permit more CharStrings than the font glyph count. */
  hb_face_destroy (face);
  face = hb_test_open_font_file ("fonts/varc-short-cff2.otf");
  hb_face_set_glyph_count (face, 2);
  font = hb_font_create (face);
  draw_data = (draw_data_t) {0};
  hb_font_draw_glyph (font, 1, funcs, &draw_data);
  g_assert_cmpuint (draw_data.move_to_count, ==, 0);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_hb_draw_varc_delta_precision (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/varc-delta-precision.ttf");
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);
  hb_variation_t variation = { HB_TAG ('T','E','S','T'), 1.f };
  hb_font_set_variations (font, &variation, 1);

  const float first_moves[] = {600.f, 664.f, 101.f};
  for (unsigned i = 0; i < G_N_ELEMENTS (first_moves); i++)
  {
    draw_data_t draw_data = {0};
    hb_font_draw_glyph (font, i + 2, funcs, &draw_data);
    g_assert_cmpuint (draw_data.move_to_count, ==, 1);
    g_assert_cmpfloat (draw_data.first_move_x, ==, first_moves[i]);
  }
  hb_font_destroy (font);
}

static void
test_hb_draw_varc_static_deltas (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/varc-static-deltas.ttf");
  g_assert_cmpuint (hb_ot_var_get_axis_count (face), ==, 0);
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);
  unsigned coord_count = 0;
  hb_font_get_var_coords_normalized (font, &coord_count);
  g_assert_cmpuint (coord_count, ==, 0);

  const float first_moves[] = {500.f, 564.f, 600.f};
  for (unsigned i = 0; i < G_N_ELEMENTS (first_moves); i++)
  {
    draw_data_t draw_data = {0};
    hb_font_draw_glyph (font, i + 2, funcs, &draw_data);
    g_assert_cmpuint (draw_data.move_to_count, ==, 1);
    g_assert_cmpfloat (draw_data.first_move_x, ==, first_moves[i]);

    hb_glyph_extents_t extents;
    g_assert_true (hb_font_get_glyph_extents (font, i + 2, &extents));
    g_assert_cmpint (extents.x_bearing, ==, (int) first_moves[i]);
    g_assert_cmpint (extents.y_bearing, ==, 100);
    g_assert_cmpint (extents.width, ==, 100);
    g_assert_cmpint (extents.height, ==, -100);
  }
  hb_font_destroy (font);
}

static void
test_hb_draw_varc_budget (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/varc-6868.ttf");
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);

  hb_codepoint_t gid = 0;
  g_assert_true (hb_font_get_nominal_glyph (font, 0x6868u, &gid));

  hb_draw_funcs_t *budget_funcs = hb_draw_funcs_create ();
  hb_draw_funcs_set_move_to_func (budget_funcs, move_to, NULL, NULL);
  hb_draw_funcs_set_line_to_func (budget_funcs, line_to, NULL, NULL);
  hb_draw_funcs_set_quadratic_to_func (budget_funcs, quadratic_to, NULL, NULL);
  hb_draw_funcs_set_cubic_to_func (budget_funcs, cubic_to, NULL, NULL);
  hb_draw_funcs_set_close_path_func (budget_funcs, close_path, NULL, NULL);
  hb_draw_funcs_set_set_budget_func (budget_funcs, set_budget, NULL, NULL);
  hb_draw_funcs_set_get_budget_func (budget_funcs, get_budget, NULL, NULL);
  hb_draw_funcs_set_get_budget_remaining_func (budget_funcs, get_budget_remaining, NULL, NULL);
  hb_draw_funcs_make_immutable (budget_funcs);

  budget_draw_data_t draw_data = {{0}, HB_BUDGET_DEFAULT, 0};
  g_assert_true (hb_draw_set_budget (budget_funcs, &draw_data, 1 << 20));
  hb_font_draw_glyph (font, gid, budget_funcs, &draw_data);
  g_assert_cmpuint (draw_data.draw.move_to_count, ==, 11);
  g_assert_cmpint (draw_data.remaining, <, 1 << 20);
  g_assert_cmpint (draw_data.remaining, >=, 0);

  draw_data.draw = (draw_data_t) {0};
  g_assert_true (hb_draw_set_budget (budget_funcs, &draw_data, 1));
  hb_font_draw_glyph (font, gid, budget_funcs, &draw_data);
  g_assert_cmpint (draw_data.remaining, <, 0);
  g_assert_cmpuint (draw_data.draw.move_to_count, ==, 0);

  hb_draw_funcs_destroy (budget_funcs);
  hb_font_destroy (font);
}
#endif

int
main (int argc, char **argv)
{
  funcs = hb_draw_funcs_create ();
  hb_draw_funcs_set_move_to_func (funcs, move_to, NULL, NULL);
  hb_draw_funcs_set_line_to_func (funcs, line_to, NULL, NULL);
  hb_draw_funcs_set_quadratic_to_func (funcs, quadratic_to, NULL, NULL);
  hb_draw_funcs_set_cubic_to_func (funcs, cubic_to, NULL, NULL);
  hb_draw_funcs_set_close_path_func (funcs, close_path, NULL, NULL);
  hb_draw_funcs_make_immutable (funcs);

  hb_test_init (&argc, &argv);
  hb_test_add (test_itoa);
#ifdef HB_EXPERIMENTAL_API
  hb_test_add (test_hb_draw_varc_simple_hangul);
  hb_test_add (test_hb_draw_varc_simple_hanzi);
  hb_test_add (test_hb_draw_varc_conditional);
  hb_test_add (test_hb_draw_varc_static_gvar);
  hb_test_add (test_hb_draw_varc_short_cff2);
  hb_test_add (test_hb_draw_varc_delta_precision);
  hb_test_add (test_hb_draw_varc_static_deltas);
  hb_test_add (test_hb_draw_varc_budget);
#endif
  unsigned result = hb_test_run ();

  hb_draw_funcs_destroy (funcs);
  return result;
}
