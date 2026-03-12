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

#ifdef HAVE_CAIRO
#include <cairo.h>
#endif

#ifdef HAVE_CAIRO
using helper_image_status_t = cairo_status_t;
using helper_image_write_func_t = cairo_write_func_t;
#define HELPER_IMAGE_STATUS_SUCCESS CAIRO_STATUS_SUCCESS
#else
using helper_image_status_t = int;
using helper_image_write_func_t = helper_image_status_t (*) (void                *closure,
							     const unsigned char *data,
							     unsigned int         size);
#define HELPER_IMAGE_STATUS_SUCCESS 0
#endif

#include "ansi-print.hh"

enum class helper_image_protocol_t {
  NONE = 0,
  ITERM2,
  KITTY,
};

static inline helper_image_status_t
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

  return HELPER_IMAGE_STATUS_SUCCESS;
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
			     helper_image_write_func_t write_func,
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

#ifdef HAVE_CHAFA
# include <chafa.h>

/* Similar to ansi-print.cc */
# define CELL_W 8
# define CELL_H (2 * CELL_W)

static void
chafa_print_image_rgb24 (const void *data, int width, int height, int stride, int level,
			 helper_image_write_func_t write_func,
			 void		     *closure)
{
  ChafaTermInfo *term_info;
  ChafaSymbolMap *symbol_map;
  ChafaCanvasConfig *config;
  ChafaCanvas *canvas;
  GString *gs;
  unsigned int cols = (width +  CELL_W - 1) / CELL_W;
  unsigned int rows = (height + CELL_H - 1) / CELL_H;
  gchar **environ;
  ChafaCanvasMode mode;
  ChafaPixelMode pixel_mode;

  /* Adapt to terminal; use sixels if available, and fall back to symbols
   * with as many colors as are supported */

  chafa_set_n_threads (1); // https://github.com/hpjansson/chafa/issues/125#issuecomment-1397475217

  environ = g_get_environ ();
  term_info = chafa_term_db_detect (chafa_term_db_get_default (),
                                    environ);

  pixel_mode = CHAFA_PIXEL_MODE_SYMBOLS;

  if (chafa_term_info_have_seq (term_info, CHAFA_TERM_SEQ_BEGIN_SIXELS))
  {
    pixel_mode = CHAFA_PIXEL_MODE_SIXELS;
    mode = CHAFA_CANVAS_MODE_TRUECOLOR;
  }
  else if (chafa_term_info_have_seq (term_info, CHAFA_TERM_SEQ_SET_COLOR_FGBG_DIRECT))
    mode = CHAFA_CANVAS_MODE_TRUECOLOR;
  else if (chafa_term_info_have_seq (term_info, CHAFA_TERM_SEQ_SET_COLOR_FGBG_256))
    mode = CHAFA_CANVAS_MODE_INDEXED_240;
  else if (chafa_term_info_have_seq (term_info, CHAFA_TERM_SEQ_SET_COLOR_FGBG_16))
    mode = CHAFA_CANVAS_MODE_INDEXED_16;
  else if (chafa_term_info_have_seq (term_info, CHAFA_TERM_SEQ_INVERT_COLORS))
    mode = CHAFA_CANVAS_MODE_FGBG_BGFG;
  else
    mode = CHAFA_CANVAS_MODE_FGBG;

  symbol_map = chafa_symbol_map_new ();
  chafa_symbol_map_add_by_tags (symbol_map,
                                (ChafaSymbolTags) (CHAFA_SYMBOL_TAG_BLOCK
                                                   | CHAFA_SYMBOL_TAG_SPACE
						   | (level >= 2 ? CHAFA_SYMBOL_TAG_WEDGE : 0)
						   | (level >= 3 ? CHAFA_SYMBOL_TAG_ALL : 0)
				));

  config = chafa_canvas_config_new ();
  chafa_canvas_config_set_canvas_mode (config, mode);
  chafa_canvas_config_set_pixel_mode (config, pixel_mode);
  chafa_canvas_config_set_cell_geometry (config, CELL_W, CELL_H);
  chafa_canvas_config_set_geometry (config, cols, rows);
  chafa_canvas_config_set_symbol_map (config, symbol_map);
  chafa_canvas_config_set_color_extractor (config, CHAFA_COLOR_EXTRACTOR_MEDIAN);
  chafa_canvas_config_set_work_factor (config, 1.0f);

  canvas = chafa_canvas_new (config);
  chafa_canvas_draw_all_pixels (canvas,
                                G_BYTE_ORDER == G_LITTLE_ENDIAN
                                  ? CHAFA_PIXEL_BGRA8_PREMULTIPLIED
                                  : CHAFA_PIXEL_ARGB8_PREMULTIPLIED,
                                (const guint8 *) data,
                                width,
                                height,
                                stride);
  gs = chafa_canvas_print (canvas, term_info);

  write_func (closure, (const unsigned char *) gs->str, gs->len);

  if (pixel_mode != CHAFA_PIXEL_MODE_SIXELS)
    write_func (closure, (const unsigned char *) "\n", 1);

  g_string_free (gs, TRUE);
  chafa_canvas_unref (canvas);
  chafa_canvas_config_unref (config);
  chafa_symbol_map_unref (symbol_map);
  chafa_term_info_unref (term_info);
  g_strfreev (environ);
}

#endif /* HAVE_CHAFA */

static inline helper_image_status_t
helper_image_write_to_ansi_stream_rgb24 (const uint32_t        *data,
					 unsigned int           width,
					 unsigned int           height,
					 unsigned int           stride,
					 helper_image_write_func_t write_func,
					 void                  *closure)
{
  /* We don't have rows to spare on the terminal window...
   * Find the tight image top/bottom and only print in between. */

  uint32_t bg_color = data ? * (uint32_t *) data : 0;

  auto orig_data = data;
  while (height)
  {
    unsigned int i;
    for (i = 0; i < width; i++)
      if (data[i] != bg_color)
	break;
    if (i < width)
      break;
    data += stride / 4;
    height--;
  }
  if (orig_data < data)
  {
    data -= stride / 4;
    height++;
  }

  auto orig_height = height;
  while (height)
  {
    const uint32_t *row = data + (height - 1) * stride / 4;
    unsigned int i;
    for (i = 0; i < width; i++)
      if (row[i] != bg_color)
	break;
    if (i < width)
      break;
    height--;
  }
  if (height < orig_height)
    height++;

  if (width && height)
  {
#ifdef HAVE_CHAFA
    const char *env = getenv ("HB_CHAFA");
    int chafa_level = 1;
    if (env)
      chafa_level = atoi (env);
    if (chafa_level)
      chafa_print_image_rgb24 (data, width, height, stride, chafa_level,
			       write_func, closure);
    else
#endif
      ansi_print_image_rgb24 (data, width, height, stride / 4,
			      write_func, closure);
  }

  return HELPER_IMAGE_STATUS_SUCCESS;
}

#endif
