/*
 * Copyright © 2018  Ebrahim Byagowi
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
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <pthread.h>

#include <hb.h>
#include <hb-ft.h>
#include <hb-ot.h>

const char *text = "طرح‌نَما";
const char *path =
#if defined(__linux__)
		"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
#elif defined(_WIN32) || defined(_WIN64)
		"C:\\Windows\\Fonts\\tahoma.ttf";
#elif __APPLE__
		"/Library/Fonts/Tahoma.ttf";
#endif

int num_threads = 30;
int num_iters = 200;

hb_font_t *font;
hb_buffer_t *ref_buffer;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void
fill_the_buffer (hb_buffer_t *buffer)
{
  hb_buffer_add_utf8 (buffer, text, sizeof (text), 0, sizeof (text));
  hb_buffer_guess_segment_properties (buffer);
  hb_shape (font, buffer, NULL, 0);
}

static void
validity_check (hb_buffer_t *buffer) {
  if (hb_buffer_diff (ref_buffer, buffer, (hb_codepoint_t) -1, 0))
  {
    fprintf (stderr, "One of the buffers was different from the reference.\n");
    char out[255];

    hb_buffer_serialize_glyphs (buffer, 0, hb_buffer_get_length (ref_buffer),
				out, sizeof (out), NULL,
				font, HB_BUFFER_SERIALIZE_FORMAT_TEXT,
				HB_BUFFER_SERIALIZE_FLAG_NO_GLYPH_NAMES);
    fprintf (stderr, "Actual:   %s\n", out);

    hb_buffer_serialize_glyphs (ref_buffer, 0, hb_buffer_get_length (ref_buffer),
				out, sizeof (out), NULL,
				font, HB_BUFFER_SERIALIZE_FORMAT_TEXT,
				HB_BUFFER_SERIALIZE_FLAG_NO_GLYPH_NAMES);
    fprintf (stderr, "Expected: %s\n", out);

    exit (1);
  }
}

static void *
thread_func (void *data)
{
  hb_buffer_t *buffer = (hb_buffer_t *) data;

  pthread_mutex_lock (&mutex);
  pthread_mutex_unlock (&mutex);

  int i;
  for (i = 0; i < num_iters; i++)
  {
    hb_buffer_clear_contents (buffer);
    fill_the_buffer (buffer);
    validity_check (buffer);
  }

  return 0;
}

void
test_body ()
{
  int i;
  int num_threads = 30;
  pthread_t *threads = calloc (num_threads, sizeof (pthread_t));
  hb_buffer_t **buffers = calloc (num_threads, sizeof (hb_buffer_t *));

  pthread_mutex_lock (&mutex);

  for (i = 0; i < num_threads; i++)
  {
    hb_buffer_t *buffer = hb_buffer_create ();
    buffers[i] = buffer;
    pthread_create (&threads[i], NULL, thread_func, buffer);
  }

  /* Let them loose! */
  pthread_mutex_unlock (&mutex);

  for (i = 0; i < num_threads; i++)
  {
    pthread_join (threads[i], NULL);
    hb_buffer_destroy (buffers[i]);
  }

  free (buffers);
  free (threads);
}

int
main (int argc, char **argv)
{
  if (argc > 1)
    num_threads = atoi (argv[1]);
  if (argc > 2)
    num_iters = atoi (argv[2]);

  // Dummy call to alleviate _guess_segment_properties thread safety-ness
  // https://github.com/harfbuzz/harfbuzz/issues/1191
  hb_language_get_default ();

  hb_blob_t *blob = hb_blob_create_from_file (path);
  hb_face_t *face = hb_face_create (blob, 0);
  font = hb_font_create (face);

  hb_ot_font_set_funcs (font);

  ref_buffer = hb_buffer_create ();
  fill_the_buffer (ref_buffer);

  test_body ();

  /* hb-font backed by FreeType functions can only be used from
   * one thread at a time, because that's FT_Face's MT guarantee.
   * So, disable this, even though it works "most of the time". */
  //hb_ft_font_set_funcs (font);
  //test_body ();

  hb_buffer_destroy (ref_buffer);

  hb_font_destroy (font);
  hb_face_destroy (face);
  hb_blob_destroy (blob);

  return 0;
}
