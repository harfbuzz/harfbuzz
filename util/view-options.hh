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
#define DEFAULT_STROKE "#888"
#define DEFAULT_STROKE_WIDTH 1

struct view_options_t
{
  ~view_options_t ()
  {
    g_free (fore);
    g_free (stroke);
    g_free (back);
    g_free (custom_palette);
    if (foreground_palette)
      g_array_unref (foreground_palette);
    if (custom_palette_entries)
      g_array_unref (custom_palette_entries);
  }

  void add_options (option_parser_t *parser);
  void post_parse (GError **error G_GNUC_UNUSED)
  {
    stroke_enabled = false;
    stroke_width = DEFAULT_STROKE_WIDTH;
    if (!parse_color (DEFAULT_STROKE, stroke_color.r, stroke_color.g, stroke_color.b, stroke_color.a))
      fail (false, "Failed parsing default stroke color `%s`", DEFAULT_STROKE);

    const bool stroke_specified = stroke != nullptr;
    if (stroke_specified)
    {
      stroke_enabled = true;
      char *spec = g_strdup (stroke);
      char *s = g_strstrip (spec);
      if (*s)
      {
        char *plus = strchr (s, '+');
        if (plus && strchr (plus + 1, '+'))
          fail (false, "Failed parsing stroke spec `%s`", stroke);

        char *color_spec = s;
        char *width_spec = nullptr;
        if (plus)
        {
          *plus = '\0';
          width_spec = g_strstrip (plus + 1);
        }
        color_spec = g_strstrip (color_spec);

        if (*color_spec)
        {
          if (!parse_color (color_spec, stroke_color.r, stroke_color.g, stroke_color.b, stroke_color.a))
            fail (false, "Failed parsing stroke color `%s`", color_spec);
        }

        if (width_spec && *width_spec)
        {
          char *end = nullptr;
          errno = 0;
          double w = g_ascii_strtod (width_spec, &end);
          if (!end || errno || *g_strstrip (end) || w <= 0)
            fail (false, "Failed parsing stroke width `%s`", width_spec);
          stroke_width = w;
        }
      }
      g_free (spec);
    }

    if (!parse_custom_palette_entries (error))
      return;

    if (!parse_color (DEFAULT_BACK,
		      background_color.r, background_color.g,
		      background_color.b, background_color.a))
      fail (false, "Failed parsing default background color `%s`", DEFAULT_BACK);
    has_background = false;
    if (back && *back)
    {
      if (!parse_color (back,
			background_color.r, background_color.g,
			background_color.b, background_color.a))
      {
	g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
		     "Invalid background color `%s`", back);
	return;
      }
      has_background = true;
    }

    foreground_use_palette = false;
    if (foreground_palette)
    {
      g_array_unref (foreground_palette);
      foreground_palette = nullptr;
    }

    const char *foreground = fore ? fore : DEFAULT_FORE;
    if (!foreground)
      return;

    if (!parse_color (DEFAULT_FORE,
		      foreground_color.r, foreground_color.g,
		      foreground_color.b, foreground_color.a))
      fail (false, "Failed parsing default foreground color `%s`", DEFAULT_FORE);

    const bool use_rainbow =
      foreground_use_rainbow ||
      0 == g_ascii_strcasecmp (foreground, "rainbow") ||
      0 == g_ascii_strcasecmp (foreground, "lgbtq");

    if (use_rainbow)
    {
      static const char *rainbow[] =
      {
        "#DA0000BB",
        "#FF6200BB",
        "#D4B900BB",
        "#005100BB",
        "#000CFFBB",
        "#42005BBB",
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
      if (foreground_palette->len)
	foreground_color =
	  g_array_index (foreground_palette, rgba_color_t, 0);
      foreground_use_palette = true;

      if (!stroke_specified)
        stroke_enabled = true;

      return;
    }

    if (!strchr (foreground, ','))
    {
      rgba_color_t color = {0, 0, 0, 255};
      if (!parse_color (foreground, color.r, color.g, color.b, color.a))
        fail (false, "Failed parsing foreground color `%s`", foreground);
      foreground_color = color;
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

    foreground_color = g_array_index (foreground_palette, rgba_color_t, 0);
    foreground_use_palette = true;
  }

  struct rgba_color_t {
    unsigned r, g, b, a;
  };
  struct custom_palette_entry_t {
    unsigned index;
    rgba_color_t color;
  };

  char *fore = nullptr;
  char *stroke = nullptr;
  char *back = nullptr;
  unsigned int palette = 0;
  char *custom_palette = nullptr;
  GArray *custom_palette_entries = nullptr;
  GArray *foreground_palette = nullptr;
  hb_bool_t foreground_use_palette = false;
  hb_bool_t foreground_use_rainbow = false;
  hb_bool_t stroke_enabled = false;
  rgba_color_t stroke_color = {0, 0, 0, 255};
  double stroke_width = DEFAULT_STROKE_WIDTH;
  rgba_color_t foreground_color = {0, 0, 0, 255};
  rgba_color_t background_color = {255, 255, 255, 255};
  hb_bool_t has_background = false;
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

  bool parse_custom_palette_entries (GError **error)
  {
    if (custom_palette_entries)
    {
      g_array_unref (custom_palette_entries);
      custom_palette_entries = nullptr;
    }
    if (!custom_palette || !*custom_palette)
      return true;

    custom_palette_entries = g_array_new (false, false, sizeof (custom_palette_entry_t));
    char **entries = g_strsplit (custom_palette, ",", -1);
    unsigned next_index = 0;
    for (unsigned i = 0; entries && entries[i]; i++)
    {
      char *entry = g_strstrip (entries[i]);
      if (!*entry)
        continue;

      unsigned idx = next_index;
      char *color_spec = entry;
      char *eq = strchr (entry, '=');
      if (eq)
      {
        *eq = '\0';
        char *idx_spec = g_strstrip (entry);
        color_spec = g_strstrip (eq + 1);
        if (!*idx_spec || !*color_spec)
        {
          g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                       "Invalid --custom-palette entry");
          g_strfreev (entries);
          return false;
        }
        char *end = nullptr;
        errno = 0;
        unsigned long parsed = strtoul (idx_spec, &end, 10);
        if (errno || !end || *g_strstrip (end))
        {
          g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                       "Invalid --custom-palette index `%s`", idx_spec);
          g_strfreev (entries);
          return false;
        }
        idx = (unsigned) parsed;
        next_index = idx + 1;
      }

      rgba_color_t color = {0, 0, 0, 255};
      if (!parse_color (color_spec, color.r, color.g, color.b, color.a))
      {
        g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                     "Invalid --custom-palette color `%s`", color_spec);
        g_strfreev (entries);
        return false;
      }

      custom_palette_entry_t parsed = {idx, color};
      g_array_append_val (custom_palette_entries, parsed);
      if (!eq)
        next_index++;
    }
    g_strfreev (entries);

    if (!custom_palette_entries->len)
    {
      g_array_unref (custom_palette_entries);
      custom_palette_entries = nullptr;
    }
    return true;
  }
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
  if (parse_1to4_doubles (arg, &m.t, &m.r, &m.b, &m.l))
    return true;
  g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
	       "%s argument should be one to four space-separated numbers",
	       name);
  return false;
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
    {"stroke",		0, 0, G_OPTION_ARG_STRING,	&this->stroke,			"Stroke glyph outlines; `--stroke=` enables defaults",	"rrggbb/rrggbbaa[+width]"},
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

#endif
