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
 *
 * Author(s): Behdad Esfahbod
 */

#ifndef HELPER_IMAGE_OUTPUT_HH
#define HELPER_IMAGE_OUTPUT_HH

#include "hb.hh"

#include <cairo.h>

enum class helper_image_protocol_t {
  NONE = 0,
  ITERM2,
  KITTY,
};

static inline cairo_status_t
helper_image_stdio_write_func (void                *closure,
			       const unsigned char *data,
			       unsigned int         size)
{
  FILE *fp = (FILE *) closure;

  while (size)
  {
    size_t ret = fwrite (data, 1, size, fp);
    size -= ret;
    data += ret;
    if (size && ferror (fp))
      fail (false, "Failed to write output: %s", strerror (errno));
  }

  return CAIRO_STATUS_SUCCESS;
}

static inline const char *
helper_image_get_implicit_output_format (FILE                    *fp,
					 const char              *default_format,
					 helper_image_protocol_t *protocol)
{
  *protocol = helper_image_protocol_t::NONE;

#if HAVE_ISATTY
  if (isatty (fileno (fp)))
  {
#ifdef HAVE_PNG
    const char *name;
    /* https://gitlab.com/gnachman/iterm2/-/issues/7154 */
    if ((name = getenv ("LC_TERMINAL")) != nullptr &&
	0 == g_ascii_strcasecmp (name, "iTerm2"))
    {
      *protocol = helper_image_protocol_t::ITERM2;
      return "png";
    }
    if ((name = getenv ("TERM_PROGRAM")) != nullptr &&
	0 == g_ascii_strcasecmp (name, "WezTerm"))
    {
      *protocol = helper_image_protocol_t::ITERM2;
      return "png";
    }
    if ((name = getenv ("TERM")) != nullptr &&
	0 == g_ascii_strcasecmp (name, "xterm-kitty"))
    {
      *protocol = helper_image_protocol_t::KITTY;
      return "png";
    }
#endif
    return "ansi";
  }
#endif

  return default_format;
}

static inline void
helper_image_write_png_data (const unsigned char   *data,
			     unsigned int           length,
			     cairo_write_func_t     write_func,
			     void                  *closure,
			     helper_image_protocol_t protocol)
{
  if (protocol == helper_image_protocol_t::NONE)
  {
    write_func (closure, data, length);
    return;
  }

  gchar *base64 = g_base64_encode (data, length);
  size_t base64_len = strlen (base64);
  GString *string = g_string_new (nullptr);

  if (protocol == helper_image_protocol_t::ITERM2)
  {
    /* https://iterm2.com/documentation-images.html */
    g_string_printf (string, "\033]1337;File=inline=1;size=%zu:%s\a\n",
		     base64_len, base64);
  }
  else if (protocol == helper_image_protocol_t::KITTY)
  {
#define CHUNK_SIZE 4096
    /* https://sw.kovidgoyal.net/kitty/graphics-protocol.html */
    for (size_t pos = 0; pos < base64_len; pos += CHUNK_SIZE)
    {
      size_t len = base64_len - pos;

      if (pos == 0)
	g_string_append (string, "\033_Ga=T,f=100,m=");
      else
	g_string_append (string, "\033_Gm=");

      if (len > CHUNK_SIZE)
      {
	g_string_append (string, "1;");
	g_string_append_len (string, base64 + pos, CHUNK_SIZE);
      }
      else
      {
	g_string_append (string, "0;");
	g_string_append_len (string, base64 + pos, len);
      }

      g_string_append (string, "\033\\");
    }
    g_string_append (string, "\n");
#undef CHUNK_SIZE
  }

  write_func (closure, (const unsigned char *) string->str, string->len);

  g_free (base64);
  g_string_free (string, TRUE);
}

#endif
