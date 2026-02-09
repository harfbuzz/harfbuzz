/*
 * Copyright © 2011  Google, Inc.
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
 * Google Author(s): Behdad Esfahbod
 */

#ifndef VIEW_OPTIONS_HH
#define VIEW_OPTIONS_HH

#include "options.hh"

#define DEFAULT_MARGIN 16
#define DEFAULT_FORE "#000000"
#define DEFAULT_BACK "#FFFFFF"

struct view_options_t
{
  ~view_options_t ()
  {
    g_free (fore);
    g_free (back);
    g_free (custom_palette);
    if (foreground_palette)
      g_array_unref (foreground_palette);
  }

  void add_options (option_parser_t *parser);
  void setup_foreground ();

  struct rgba_color_t {
    unsigned r, g, b, a;
  };

  char *fore = nullptr;
  char *back = nullptr;
  unsigned int palette = 0;
  char *custom_palette = nullptr;
  GArray *foreground_palette = nullptr;
  hb_bool_t foreground_use_palette = false;
  hb_bool_t foreground_use_rainbow = false;
  double line_space = 0;
  bool have_font_extents = false;
  struct font_extents_t {
    double ascent, descent, line_gap;
  } font_extents = {0., 0., 0.};
  struct margin_t {
    double t, r, b, l;
  } margin = {DEFAULT_MARGIN, DEFAULT_MARGIN, DEFAULT_MARGIN, DEFAULT_MARGIN};
  hb_bool_t logical = false;
  hb_bool_t ink = false;
  hb_bool_t show_extents = false;
};


static gboolean
parse_font_extents (const char *name G_GNUC_UNUSED,
		    const char *arg,
		    gpointer    data,
		    GError    **error G_GNUC_UNUSED)
{
  view_options_t *view_opts = (view_options_t *) data;
  view_options_t::font_extents_t &e = view_opts->font_extents;
  switch (sscanf (arg, "%lf%*[ ,]%lf%*[ ,]%lf", &e.ascent, &e.descent, &e.line_gap)) {
    case 1: HB_FALLTHROUGH;
    case 2: HB_FALLTHROUGH;
    case 3:
      view_opts->have_font_extents = true;
      return true;
    default:
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
		   "%s argument should be one to three space-separated numbers",
		   name);
      return false;
  }
}

static gboolean
parse_margin (const char *name G_GNUC_UNUSED,
	      const char *arg,
	      gpointer    data,
	      GError    **error G_GNUC_UNUSED)
{
  view_options_t *view_opts = (view_options_t *) data;
  view_options_t::margin_t &m = view_opts->margin;
  switch (sscanf (arg, "%lf%*[ ,]%lf%*[ ,]%lf%*[ ,]%lf", &m.t, &m.r, &m.b, &m.l)) {
    case 1: m.r = m.t; HB_FALLTHROUGH;
    case 2: m.b = m.t; HB_FALLTHROUGH;
    case 3: m.l = m.r; HB_FALLTHROUGH;
    case 4: return true;
    default:
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
		   "%s argument should be one to four space-separated numbers",
		   name);
      return false;
  }
}

void
view_options_t::add_options (option_parser_t *parser)
{
  GOptionEntry entries[] =
  {
    {"annotate",	0, G_OPTION_FLAG_HIDDEN,
			      G_OPTION_ARG_NONE,	&this->show_extents,		"Annotate output rendering",				nullptr},
    {"background",	0, 0, G_OPTION_ARG_STRING,	&this->back,			"Set background color (default: " DEFAULT_BACK ")",	"rrggbb/rrggbbaa"},
    {"foreground",	0, 0, G_OPTION_ARG_STRING,	&this->fore,			"Set foreground color (default: " DEFAULT_FORE ")",	"rrggbb/rrggbbaa[,...]"},
    {"rainbow",		0, 0, G_OPTION_ARG_NONE,	&this->foreground_use_rainbow,	"Rotate glyph foreground colors through a default palette",	nullptr},
    {"lgbtq",		0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE,	&this->foreground_use_rainbow,	"Hidden alias for --rainbow",	nullptr},
    {"font-palette",    0, 0, G_OPTION_ARG_INT,         &this->palette,                 "Set font palette (default: 0)",                "index"},
    {"custom-palette",  0, 0, G_OPTION_ARG_STRING,      &this->custom_palette,          "Custom palette",                               "comma-separated colors"},
    {"line-space",	0, 0, G_OPTION_ARG_DOUBLE,	&this->line_space,		"Set space between lines (default: 0)",			"units"},
    {"font-extents",	0, 0, G_OPTION_ARG_CALLBACK,	(gpointer) &parse_font_extents,	"Set font ascent/descent/line-gap (default: auto)","one to three numbers"},
    {"margin",		0, 0, G_OPTION_ARG_CALLBACK,	(gpointer) &parse_margin,	"Margin around output (default: " G_STRINGIFY(DEFAULT_MARGIN) ")","one to four numbers"},
    {"logical",		0, 0, G_OPTION_ARG_NONE,	&this->logical,		"Render to logical box instead of union of logical and ink boxes",	nullptr},
    {"ink",		0, 0, G_OPTION_ARG_NONE,	&this->ink,			"Render to ink box instead of union of logical and ink boxes",	nullptr},
    {"show-extents",	0, 0, G_OPTION_ARG_NONE,	&this->show_extents,		"Draw glyph extents",							nullptr},
    {nullptr}
  };
  parser->add_group (entries,
		     "view",
		     "View options:",
		     "Options for output rendering",
			     this);
}

inline void
view_options_t::setup_foreground ()
{
  foreground_use_palette = false;
  if (foreground_palette)
  {
    g_array_unref (foreground_palette);
    foreground_palette = nullptr;
  }

  const char *foreground = fore ? fore : DEFAULT_FORE;
  if (!foreground)
    return;

  if (foreground_use_rainbow)
  {
    static const char *rainbow[] =
    {
      "#E40303",
      "#FF8C00",
      "#E0CC00",
      "#008026",
      "#004DFF",
      "#750787",
      nullptr
    };

    foreground_palette = g_array_new (false, false, sizeof (rgba_color_t));
    for (const char **entry = rainbow; *entry; entry++)
    {
      rgba_color_t color = {0, 0, 0, 255};
      if (!parse_color (*entry, color.r, color.g, color.b, color.a))
        fail (false, "Failed parsing foreground color `%s`", *entry);
      g_array_append_val (foreground_palette, color);
    }
    foreground_use_palette = true;
    return;
  }

  if (!strchr (foreground, ','))
  {
    rgba_color_t color = {0, 0, 0, 255};
    if (!parse_color (foreground, color.r, color.g, color.b, color.a))
      fail (false, "Failed parsing foreground color `%s`", foreground);
    return;
  }

  foreground_palette = g_array_new (false, false, sizeof (rgba_color_t));
  char **entries = g_strsplit (foreground, ",", -1);
  for (unsigned i = 0; entries[i]; i++)
  {
    char *entry = g_strstrip (entries[i]);
    if (!*entry)
      continue;

    rgba_color_t color = {0, 0, 0, 255};
    if (!parse_color (entry, color.r, color.g, color.b, color.a))
      fail (false, "Failed parsing foreground color `%s`", entry);
    g_array_append_val (foreground_palette, color);
  }
  g_strfreev (entries);

  if (!foreground_palette->len)
  {
    g_array_unref (foreground_palette);
    foreground_palette = nullptr;
    return;
  }

  foreground_use_palette = true;
}

#endif
