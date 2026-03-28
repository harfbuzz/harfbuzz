/*
 * Copyright (C) 2026  Behdad Esfahbod
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
 * Author(s): Behdad Esfahbod
 */

#include "font-options.hh"
#include "main-font-text.hh"
#include "shape-consumer.hh"
#include "text-options.hh"
#include "gpu-output.hh"

#include <hb-ot.h>

const unsigned DEFAULT_FONT_SIZE = FONT_SIZE_UPEM;
const unsigned SUBPIXEL_BITS = 0;

#include "gpu/default-text-en.hh"
#include "gpu/default-text-fa.hh"

struct demo_script_text_t {
  hb_script_t script;
  const char *text;
};

static const demo_script_text_t demo_texts[] = {
  { HB_SCRIPT_LATIN,  default_text_en },
  { HB_SCRIPT_ARABIC, default_text_fa },
};

static void
append_lines (GString *s, const char *text, unsigned max_lines)
{
  const char *p = text;
  unsigned lines = 0;
  while (*p && lines < max_lines)
  {
    const char *end = strchr (p, '\n');
    if (!end)
      end = p + strlen (p);
    if (s->len)
      g_string_append_c (s, '\n');
    g_string_append_len (s, p, end - p);
    lines++;
    p = *end ? end + 1 : end;
  }
}

#define MAX_TOTAL_LINES 70

static bool
font_has_script (hb_face_t *face, hb_script_t script)
{
  hb_tag_t script_tags[2];
  unsigned num_tags = 2;
  hb_ot_tags_from_script_and_language (script, HB_LANGUAGE_INVALID,
				       &num_tags, script_tags, nullptr, nullptr);
  for (unsigned t = 0; t < num_tags; t++)
  {
    unsigned script_index;
    if (hb_ot_layout_table_find_script (face, HB_OT_TAG_GSUB,
					script_tags[t],
					&script_index))
      return true;
  }
  return false;
}

static char *
build_default_text (hb_face_t *face)
{
  /* Count how many scripts the font supports. */
  unsigned num_supported = 0;
  for (unsigned i = 0; i < G_N_ELEMENTS (demo_texts); i++)
    if (font_has_script (face, demo_texts[i].script))
      num_supported++;

  if (!num_supported)
    num_supported = 1; /* fallback */

  unsigned lines_per = MAX_TOTAL_LINES / num_supported;

  GString *s = g_string_new (nullptr);

  for (unsigned i = 0; i < G_N_ELEMENTS (demo_texts); i++)
  {
    if (!font_has_script (face, demo_texts[i].script))
      continue;
    append_lines (s, demo_texts[i].text, lines_per);
  }

  /* Fallback to English if nothing matched. */
  if (!s->len)
    append_lines (s, default_text_en, MAX_TOTAL_LINES);

  return g_string_free (s, FALSE);
}

static const char *gpu_default_text_sentinel = "";

struct gpu_text_options_t : shape_text_options_t
{
  const char *default_text () override
  {
    return gpu_default_text_sentinel;
  }
};

struct gpu_font_options_t : font_options_t
{
  hb_face_t *default_face () override
  {
#ifdef _WIN32
    return nullptr;
#else
    #include "gpu/default-font.hh"
    hb_blob_t *blob = hb_blob_create ((const char *) default_font,
				      sizeof (default_font),
				      HB_MEMORY_MODE_READONLY,
				      nullptr, nullptr);
    hb_face_t *face_ = hb_face_create (blob, 0);
    hb_blob_destroy (blob);
    return face_;
#endif
  }
};

using base_t = main_font_text_t<shape_consumer_t<gpu_output_t>,
				gpu_font_options_t,
				gpu_text_options_t>;

struct gpu_main_t : base_t
{
  int operator () (int argc, char **argv)
  {
    add_options ();
    add_exit_code (RETURN_VALUE_OPERATION_FAILED, "Operation failed.");

    parse (&argc, &argv);

    /* If no text was provided, build from font's supported scripts. */
    if (!text || !*text)
    {
      g_free (text);
      text = build_default_text (face);
    }

    this->init (this);

    while (this->consume_line (*this))
      ;

    this->finish (this);

    if (this->failed && return_value == RETURN_VALUE_SUCCESS)
      return_value = RETURN_VALUE_OPERATION_FAILED;

    return return_value;
  }
};

int
main (int argc, char **argv)
{
  gpu_main_t driver;
  return driver (argc, argv);
}
