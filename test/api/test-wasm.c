/*
 * Copyright © 2026  Behdad Esfahbod
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

/* The module in fonts/wasm-api.ttf answers with a few values, one per
 * glyph.  Its source is fonts/wasm-api.cc. */

static hb_font_t *
open_font (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/wasm-api.ttf");
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);
  return font;
}

static hb_buffer_t *
shape_text (hb_font_t *font, const char *text)
{
  const char *shapers[] = {"wasm", NULL};
  hb_buffer_t *buffer = hb_buffer_create ();
  hb_buffer_add_utf8 (buffer, text, -1, 0, -1);
  hb_buffer_guess_segment_properties (buffer);
  g_assert_true (hb_shape_full (font, buffer, NULL, 0, shapers));
  return buffer;
}

static unsigned
value (hb_buffer_t *buffer, unsigned i)
{
  unsigned length;
  hb_glyph_info_t *info = hb_buffer_get_glyph_infos (buffer, &length);
  g_assert_cmpuint (i, <, length);
  return info[i].codepoint;
}


/* Count points and contours the way the outline handed to a module does. */

typedef struct {
  unsigned points;
  unsigned contours;
} outline_size_t;

static void
count_one (hb_draw_funcs_t *dfuncs HB_UNUSED, void *draw_data,
	   hb_draw_state_t *st HB_UNUSED,
	   float to_x HB_UNUSED, float to_y HB_UNUSED,
	   void *user_data HB_UNUSED)
{ ((outline_size_t *) draw_data)->points += 1; }

static void
count_quadratic (hb_draw_funcs_t *dfuncs HB_UNUSED, void *draw_data,
		 hb_draw_state_t *st HB_UNUSED,
		 float control_x HB_UNUSED, float control_y HB_UNUSED,
		 float to_x HB_UNUSED, float to_y HB_UNUSED,
		 void *user_data HB_UNUSED)
{ ((outline_size_t *) draw_data)->points += 2; }

static void
count_cubic (hb_draw_funcs_t *dfuncs HB_UNUSED, void *draw_data,
	     hb_draw_state_t *st HB_UNUSED,
	     float control1_x HB_UNUSED, float control1_y HB_UNUSED,
	     float control2_x HB_UNUSED, float control2_y HB_UNUSED,
	     float to_x HB_UNUSED, float to_y HB_UNUSED,
	     void *user_data HB_UNUSED)
{ ((outline_size_t *) draw_data)->points += 3; }

static void
count_close (hb_draw_funcs_t *dfuncs HB_UNUSED, void *draw_data,
	     hb_draw_state_t *st HB_UNUSED,
	     void *user_data HB_UNUSED)
{ ((outline_size_t *) draw_data)->contours += 1; }

static outline_size_t
outline_size (hb_font_t *font, hb_codepoint_t unicode)
{
  outline_size_t size = {0, 0};
  hb_codepoint_t glyph = 0;
  hb_draw_funcs_t *funcs = hb_draw_funcs_create ();
  hb_draw_funcs_set_move_to_func (funcs, count_one, NULL, NULL);
  hb_draw_funcs_set_line_to_func (funcs, count_one, NULL, NULL);
  hb_draw_funcs_set_quadratic_to_func (funcs, count_quadratic, NULL, NULL);
  hb_draw_funcs_set_cubic_to_func (funcs, count_cubic, NULL, NULL);
  hb_draw_funcs_set_close_path_func (funcs, count_close, NULL, NULL);
  g_assert_true (hb_font_get_nominal_glyph (font, unicode, &glyph));
  hb_font_draw_glyph (font, glyph, funcs, &size);
  hb_draw_funcs_destroy (funcs);
  return size;
}

static void
test_wasm_copy_lengths (void)
{
  hb_font_t *font = open_font ();
  hb_blob_t *maxp = hb_face_reference_table (hb_font_get_face (font),
					     HB_TAG ('m','a','x','p'));
  outline_size_t size = outline_size (font, 'a');
  hb_buffer_t *buffer = shape_text (font, "abc");

  /* The module passed arrays larger than needed, and reads back how
   * much was copied into each. */
  g_assert_cmpuint (hb_buffer_get_length (buffer), ==, 6);
  g_assert_cmpuint (value (buffer, 0), ==, 3);
  g_assert_cmpuint (value (buffer, 1), ==, hb_blob_get_length (maxp));
  g_assert_cmpuint (value (buffer, 2), ==, 0);
  g_assert_cmpuint (size.points, >, 0);
  g_assert_cmpuint (value (buffer, 3), ==, size.points);
  g_assert_cmpuint (value (buffer, 4), ==, size.contours);

  /* It then reused one outline structure for two glyphs in turn, and
   * counted the copies that failed. */
  g_assert_cmpuint (value (buffer, 5), ==, 0);

  hb_buffer_destroy (buffer);
  hb_blob_destroy (maxp);
  hb_font_destroy (font);
}

static void
test_wasm_handles_end_with_the_call (void)
{
  hb_font_t *font = open_font ();
  hb_buffer_t *first, *second;

  first = shape_text (font, "b");
  g_assert_cmpuint (value (first, 0), ==, (unsigned) -1);

  /* The module passes the handle it had for the first buffer.  That
   * buffer is still alive and has a direction; the handle is not to
   * name it any more. */
  second = shape_text (font, "b");
  g_assert_cmpuint (value (second, 0), ==, HB_DIRECTION_INVALID);

  hb_buffer_destroy (first);
  hb_buffer_destroy (second);
  hb_font_destroy (font);
}

static int face_destroyed;

static void
note_face_destroyed (void *data HB_UNUSED)
{ face_destroyed++; }

static void
test_wasm_created_objects (void)
{
  static hb_user_data_key_t key;
  hb_face_t *face = hb_test_open_font_file ("fonts/wasm-api.ttf");
  hb_font_t *font = hb_font_create (face);
  hb_buffer_t *buffer;

  hb_face_set_user_data (face, &key, &key, note_face_destroyed, TRUE);
  hb_face_destroy (face);

  /* The module creates a font on our face, and a face and a font of
   * its own.  It also passes shape_with() a handle that names no
   * buffer. */
  buffer = shape_text (font, "c");
  g_assert_cmpuint (hb_buffer_get_length (buffer), ==, 3);
  g_assert_cmpuint (value (buffer, 0), ==, 0);
  g_assert_cmpuint (value (buffer, 1), ==, hb_face_get_upem (hb_font_get_face (font)));
  g_assert_cmpuint (value (buffer, 2), ==, 1);
  hb_buffer_destroy (buffer);

  /* The handle resolved to the empty buffer, which is to stay as it was. */
  g_assert_cmpint (hb_buffer_get_content_type (hb_buffer_get_empty ()), ==,
		   HB_BUFFER_CONTENT_TYPE_INVALID);

  /* Nothing the module created keeps our face alive. */
  g_assert_cmpint (face_destroyed, ==, 0);
  hb_font_destroy (font);
  g_assert_cmpint (face_destroyed, ==, 1);
}

static void
test_wasm_handles_belong_to_the_instance (void)
{
  hb_font_t *font1 = open_font ();
  hb_font_t *font2 = open_font ();
  hb_buffer_t *buffer1, *buffer2, *buffer3;

  /* Two faces, so two instances of the module.  Each numbers its
   * handles by itself, so both see the same numbers, which name
   * different objects. */
  buffer1 = shape_text (font1, "d");
  buffer2 = shape_text (font2, "d");
  g_assert_cmpuint (hb_buffer_get_length (buffer1), ==, 4);
  for (unsigned i = 0; i < 4; i++)
  {
    g_assert_cmpuint (value (buffer1, i), !=, 0);
    g_assert_cmpuint (value (buffer1, i), ==, value (buffer2, i));
  }

  /* The face keeps its handle from shape_plan_create() on... */
  g_assert_cmpuint (value (buffer1, 0), ==, value (buffer1, 1));
  buffer3 = shape_text (font1, "d");
  g_assert_cmpuint (value (buffer3, 0), ==, value (buffer1, 0));
  /* ...while the font and the buffer get new ones in every call. */
  g_assert_cmpuint (value (buffer3, 2), !=, value (buffer1, 2));
  g_assert_cmpuint (value (buffer3, 3), !=, value (buffer1, 3));
  g_assert_cmpuint (value (buffer3, 2), !=, value (buffer1, 3));

  hb_buffer_destroy (buffer1);
  hb_buffer_destroy (buffer2);
  hb_buffer_destroy (buffer3);
  hb_font_destroy (font1);
  hb_font_destroy (font2);
}

static void
test_wasm_handle_limit (void)
{
  hb_font_t *font = open_font ();
  hb_buffer_t *buffer = shape_text (font, "e");

  /* The module asked for a thousand fonts.  It got some, was refused
   * the rest, and the ones it got work. */
  g_assert_cmpuint (value (buffer, 0), >=, 16);
  g_assert_cmpuint (value (buffer, 0), <, 1000);
  g_assert_cmpuint (value (buffer, 1), ==, hb_face_get_upem (hb_font_get_face (font)));

  hb_buffer_destroy (buffer);
  hb_font_destroy (font);
}

static void
test_wasm_destroy (void)
{
  hb_font_t *font = open_font ();
  hb_codepoint_t a = 0;
  unsigned upem = hb_face_get_upem (hb_font_get_face (font));
  unsigned empty_upem = hb_face_get_upem (hb_face_get_empty ());

  g_assert_true (hb_font_get_nominal_glyph (font, 'a', &a));
  g_assert_cmpuint (upem, !=, empty_upem);

  for (unsigned call = 1; call <= 3; call++)
  {
    hb_buffer_t *buffer = shape_text (font, "f");
    g_assert_cmpuint (hb_buffer_get_length (buffer), ==, 8);

    /* The module destroyed all it created, so its instance is kept:
     * it has counted every call so far. */
    g_assert_cmpuint (value (buffer, 0), ==, call);

    /* Its font worked, and the handle named nothing once destroyed. */
    g_assert_cmpuint (value (buffer, 1), ==, a);
    g_assert_cmpuint (value (buffer, 2), ==, 0);

    /* So for its face.  The font on that face still had it, under a
     * new handle, and kept it alive until that handle went too. */
    g_assert_cmpuint (value (buffer, 3), ==, empty_upem);
    g_assert_cmpuint (value (buffer, 4), ==, 1);
    g_assert_cmpuint (value (buffer, 5), !=, 0);

    /* What it was given is not its to destroy. */
    g_assert_cmpuint (value (buffer, 6), ==, a);
    g_assert_cmpuint (value (buffer, 7), ==, upem);

    hb_buffer_destroy (buffer);
  }

  hb_font_destroy (font);

  /* A module that keeps a font gets a new instance for each call. */
  font = open_font ();
  for (unsigned call = 1; call <= 3; call++)
  {
    hb_buffer_t *buffer = shape_text (font, "g");
    g_assert_cmpuint (value (buffer, 0), ==, 1);
    g_assert_cmpuint (value (buffer, 1), ==, 1);
    hb_buffer_destroy (buffer);
  }
  hb_font_destroy (font);
}

static void
test_wasm_start_function (void)
{
  const char *shapers[] = {"wasm", NULL};
  hb_face_t *face = hb_test_open_font_file ("fonts/wasm-start.ttf");
  hb_font_t *font = hb_font_create (face);
  hb_buffer_t *buffer = hb_buffer_create ();

  /* The module called into us while its instance was being made, and
   * got nothing for it. */
  hb_buffer_add_utf8 (buffer, "abc", -1, 0, -1);
  hb_buffer_guess_segment_properties (buffer);
#ifdef HB_TEST_WASM_BUDGET
  /* With the limits, a module that the runtime runs by itself is not
   * run at all. */
  g_assert_false (hb_shape_full (font, buffer, NULL, 0, shapers));
#else
  g_assert_true (hb_shape_full (font, buffer, NULL, 0, shapers));
#endif

  hb_buffer_destroy (buffer);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static hb_bool_t
shapes (const char *font_file)
{
  const char *shapers[] = {"wasm", NULL};
  hb_face_t *face = hb_test_open_font_file (font_file);
  hb_font_t *font = hb_font_create (face);
  hb_buffer_t *buffer = hb_buffer_create ();
  hb_bool_t ret;

  hb_buffer_add_utf8 (buffer, "abc", -1, 0, -1);
  hb_buffer_guess_segment_properties (buffer);
  ret = hb_shape_full (font, buffer, NULL, 0, shapers);

  hb_buffer_destroy (buffer);
  hb_font_destroy (font);
  hb_face_destroy (face);
  return ret;
}

static void
test_wasm_refused_modules (void)
{
  /* A module may import a function that nothing provides, as long as
   * it does not call it.  Modules made with wasm-bindgen do. */
  g_assert_true (shapes ("fonts/wasm-import.ttf"));

  /* Each of these modules has the runtime run something that we cannot
   * limit: a function of the runtime's own C library, the module's own
   * allocator, a start function.  With the limits they are not run.
   * A runtime built without a C library does not provide the first,
   * and the module's call to it fails. */
#ifdef HB_TEST_WASM_BUDGET
  g_assert_false (shapes ("fonts/wasm-import-libc.ttf"));
  g_assert_false (shapes ("fonts/wasm-allocator.ttf"));
  g_assert_false (shapes ("fonts/wasm-start-section.ttf"));
#else
  g_assert_true (shapes ("fonts/wasm-allocator.ttf"));
  g_assert_true (shapes ("fonts/wasm-start-section.ttf"));
#endif
}

static void
test_wasm_instance_kept (void)
{
  const char *shapers[] = {"wasm", NULL};
  hb_face_t *face = hb_test_open_font_file ("fonts/wasm-fixed-memory.ttf");
  hb_font_t *font = hb_font_create (face);
  hb_buffer_t *buffer = hb_buffer_create ();
  char text[2001];

  hb_buffer_add_utf8 (buffer, "abc", -1, 0, -1);
  hb_buffer_guess_segment_properties (buffer);
  g_assert_true (hb_shape_full (font, buffer, NULL, 0, shapers));

  /* The module's memory cannot grow past what we allow it, so a longer
   * buffer goes to the instance it already has.  The module checks. */
  memset (text, 'a', sizeof (text) - 1);
  text[sizeof (text) - 1] = '\0';
  hb_buffer_reset (buffer);
  hb_buffer_add_utf8 (buffer, text, -1, 0, -1);
  hb_buffer_guess_segment_properties (buffer);
  g_assert_true (hb_shape_full (font, buffer, NULL, 0, shapers));

  hb_buffer_destroy (buffer);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_wasm_module_allocator (void)
{
  /* These modules export malloc() and free(), which the runtime then
   * runs for what we allocate in the module. */
#ifdef HB_TEST_WASM_BUDGET
  g_assert_false (shapes ("fonts/wasm-allocator-grow.ttf"));
  g_assert_false (shapes ("fonts/wasm-allocator-reenter.ttf"));
#else
  /* One enlarges the module's memory at every allocation, which can
   * move it, and checks what we wrote into it after. */
  g_assert_true (shapes ("fonts/wasm-allocator-grow.ttf"));

  /* The other calls back into us from its allocator, in the middle of
   * our copying the buffer, and is stopped. */
  g_assert_false (shapes ("fonts/wasm-allocator-reenter.ttf"));
#endif
}

#ifdef HB_TEST_WASM_BUDGET

static void
test_wasm_memory_limit (void)
{
  hb_font_t *font = open_font ();
  hb_buffer_t *buffer = shape_text (font, "h");

  /* The module asked for 256MiB in steps and was refused before that. */
  g_assert_cmpuint (value (buffer, 1), >, 0);
  g_assert_cmpuint (value (buffer, 1), <, 4096);
  hb_buffer_destroy (buffer);

  /* It made faces of 4MiB of data until it was refused that too. */
  buffer = shape_text (font, "i");
  g_assert_cmpuint (value (buffer, 0), >, 0);
  g_assert_cmpuint (value (buffer, 0), <, 64);
  hb_buffer_destroy (buffer);

  hb_font_destroy (font);
}

static void
test_wasm_instruction_limit (void)
{
  const char *shapers[] = {"wasm", NULL};
  hb_font_t *font = open_font ();
  hb_buffer_t *buffer = hb_buffer_create ();

  /* The module runs more than it may, and is stopped. */
  hb_buffer_add_utf8 (buffer, "j", -1, 0, -1);
  hb_buffer_guess_segment_properties (buffer);
  g_assert_false (hb_shape_full (font, buffer, NULL, 0, shapers));
  hb_buffer_destroy (buffer);

  /* The font works again right after. */
  buffer = shape_text (font, "abc");
  g_assert_cmpuint (hb_buffer_get_length (buffer), ==, 6);
  hb_buffer_destroy (buffer);

  hb_font_destroy (font);
}

static void
test_wasm_host_work_limit (void)
{
  const char *shapers[] = {"wasm", NULL};
  hb_font_t *font = open_font ();
  hb_buffer_t *buffer = hb_buffer_create ();

  /* The module stays within its instructions, and is stopped for what
   * it has us do. */
  hb_buffer_add_utf8 (buffer, "k", -1, 0, -1);
  hb_buffer_guess_segment_properties (buffer);
  g_assert_false (hb_shape_full (font, buffer, NULL, 0, shapers));
  hb_buffer_destroy (buffer);

  buffer = shape_text (font, "abc");
  g_assert_cmpuint (hb_buffer_get_length (buffer), ==, 6);
  hb_buffer_destroy (buffer);

  hb_font_destroy (font);
}

#endif

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_wasm_copy_lengths);
  hb_test_add (test_wasm_handles_end_with_the_call);
  hb_test_add (test_wasm_created_objects);
  hb_test_add (test_wasm_handles_belong_to_the_instance);
  hb_test_add (test_wasm_handle_limit);
  hb_test_add (test_wasm_destroy);
  hb_test_add (test_wasm_start_function);
  hb_test_add (test_wasm_refused_modules);
  hb_test_add (test_wasm_instance_kept);
  hb_test_add (test_wasm_module_allocator);
#ifdef HB_TEST_WASM_BUDGET
  hb_test_add (test_wasm_memory_limit);
  hb_test_add (test_wasm_instruction_limit);
  hb_test_add (test_wasm_host_work_limit);
#endif

  return hb_test_run ();
}
