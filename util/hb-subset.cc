/*
 * Copyright © 2010  Behdad Esfahbod
 * Copyright © 2011,2012  Google, Inc.
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
 * Google Author(s): Garret Rieger, Rod Sheeter
 */

#include <stdio.h>

#include "subset-options.hh"
#include "output-options.hh"
#include "face-options.hh"
#include "text-options.hh"
#include "batch.hh"
#include "main-font-text.hh"

/*
 * Command line interface to the harfbuzz font subsetter.
 */

struct subset_main_t : option_parser_t, face_options_t, text_options_t, subset_options_t, output_options_t<false>
{
  int operator () (int argc, char **argv)
  {
    add_options ();
    parse (&argc, &argv);

    hb_set_t *codepoints = hb_subset_input_unicode_set (input);
    unsigned int text_len;
    const char *text;
    while ((text = get_line (&text_len)))
    {
      if (0 == strcmp (text, "*"))
      {
	hb_face_collect_unicodes (face, codepoints);
	continue;
      }

      if (*text)
      {
	gchar *c = (gchar *)text;
	do
	{
	  gunichar cp = g_utf8_get_char(c);
	  hb_codepoint_t hb_cp = cp;
	  hb_set_add (codepoints, hb_cp);
	}
	while ((c = g_utf8_find_next_char(c, text + text_len)));
      }
    }

    hb_face_t *new_face = nullptr;
    for (unsigned i = 0; i < num_iterations; i++)
    {
      hb_face_destroy (new_face);
      new_face = hb_subset_or_fail (face, input);
    }

    bool success = new_face;
    if (success)
    {
      hb_blob_t *result = hb_face_reference_blob (new_face);
      write_file (output_file, result);
      hb_blob_destroy (result);
    }

    hb_face_destroy (new_face);

    return success ? 0 : 1;
  }

  bool
  write_file (const char *output_file, hb_blob_t *blob)
  {
    assert (out_fp);

    unsigned int size;
    const char* data = hb_blob_get_data (blob, &size);

    while (size)
    {
      size_t ret = fwrite (data, 1, size, out_fp);
      size -= ret;
      data += ret;
      if (size && ferror (out_fp))
        fail (false, "Failed to write output: %s", strerror (errno));
    }

    return true;
  }

  protected:

  void add_options ()
  {
    face_options_t::add_options (this);
    text_options_t::add_options (this);
    subset_options_t::add_options (this);
    output_options_t::add_options (this);

    GOptionEntry entries[] =
    {
      {G_OPTION_REMAINING,	0, G_OPTION_FLAG_IN_MAIN,
				G_OPTION_ARG_CALLBACK,	(gpointer) &collect_rest,	nullptr,	"[FONT-FILE] [TEXT]"},
      {nullptr}
    };
    add_main_group (entries, this);
    option_parser_t::add_options ();
  }

  private:

  static gboolean
  collect_rest (const char *name G_GNUC_UNUSED,
		const char *arg,
		gpointer    data,
		GError    **error)
  {
    subset_main_t *thiz = (subset_main_t *) data;

    if (!thiz->font_file)
    {
      thiz->font_file = g_strdup (arg);
      return true;
    }

    if (!thiz->text && !thiz->text_file)
    {
      thiz->text = g_strdup (arg);
      return true;
    }

    g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_FAILED,
		 "Too many arguments on the command line");
    return false;
  }
};

int
main (int argc, char **argv)
{
  return batch_main<subset_main_t, true> (argc, argv);
}
