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

#include "options.hh"

#ifdef HAVE_FREETYPE
#include <hb-ft.h>
#endif


void
fail (hb_bool_t suggest_help, const char *format, ...)
{
  const char *msg;

  va_list vap;
  va_start (vap, format);
  msg = g_strdup_vprintf (format, vap);
  const char *prgname = g_get_prgname ();
  g_printerr ("%s: %s\n", prgname, msg);
  if (suggest_help)
    g_printerr ("Try `%s --help' for more information.\n", prgname);

  exit (1);
}


hb_bool_t debug = FALSE;

static gchar *
shapers_to_string (void)
{
  GString *shapers = g_string_new (NULL);
  const char **shaper_list = hb_shape_list_shapers ();

  for (; *shaper_list; shaper_list++) {
    g_string_append (shapers, *shaper_list);
    g_string_append_c (shapers, ',');
  }
  g_string_truncate (shapers, MAX (0, (gint)shapers->len - 1));

  return g_string_free (shapers, FALSE);
}

static G_GNUC_NORETURN gboolean
show_version (const char *name G_GNUC_UNUSED,
	      const char *arg G_GNUC_UNUSED,
	      gpointer    data G_GNUC_UNUSED,
	      GError    **error G_GNUC_UNUSED)
{
  g_printf ("%s (%s) %s\n", g_get_prgname (), PACKAGE_NAME, PACKAGE_VERSION);

  char *shapers = shapers_to_string ();
  g_printf ("Available shapers: %s\n", shapers);
  g_free (shapers);
  if (strcmp (HB_VERSION_STRING, hb_version_string ()))
    g_printf ("Linked HarfBuzz library has a different version: %s\n", hb_version_string ());

  exit(0);
}


void
option_parser_t::add_main_options (void)
{
  GOptionEntry entries[] =
  {
    {"version",		0, G_OPTION_FLAG_NO_ARG,
			      G_OPTION_ARG_CALLBACK,	(gpointer) &show_version,	"Show version numbers",			NULL},
    {"debug",		0, 0, G_OPTION_ARG_NONE,	&debug,				"Free all resources before exit",	NULL},
    {NULL}
  };
  g_option_context_add_main_entries (context, entries, NULL);
}

static gboolean
pre_parse (GOptionContext *context G_GNUC_UNUSED,
	   GOptionGroup *group G_GNUC_UNUSED,
	   gpointer data,
	   GError **error)
{
  option_group_t *option_group = (option_group_t *) data;
  option_group->pre_parse (error);
  return *error == NULL;
}

static gboolean
post_parse (GOptionContext *context G_GNUC_UNUSED,
	    GOptionGroup *group G_GNUC_UNUSED,
	    gpointer data,
	    GError **error)
{
  option_group_t *option_group = static_cast<option_group_t *>(data);
  option_group->post_parse (error);
  return *error == NULL;
}

void
option_parser_t::add_group (GOptionEntry   *entries,
			    const gchar    *name,
			    const gchar    *description,
			    const gchar    *help_description,
			    option_group_t *option_group)
{
  GOptionGroup *group = g_option_group_new (name, description, help_description,
					    static_cast<gpointer>(option_group), NULL);
  g_option_group_add_entries (group, entries);
  g_option_group_set_parse_hooks (group, pre_parse, post_parse);
  g_option_context_add_group (context, group);
}

void
option_parser_t::parse (int *argc, char ***argv)
{
  setlocale (LC_ALL, "");

  GError *parse_error = NULL;
  if (!g_option_context_parse (context, argc, argv, &parse_error))
  {
    if (parse_error != NULL) {
      fail (TRUE, "%s", parse_error->message);
      //g_error_free (parse_error);
    } else
      fail (TRUE, "Option parse error");
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
  switch (sscanf (arg, "%lf %lf %lf %lf", &m.t, &m.r, &m.b, &m.l)) {
    case 1: m.r = m.t;
    case 2: m.b = m.t;
    case 3: m.l = m.r;
    case 4: return TRUE;
    default:
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
		   "%s argument should be one to four space-separated numbers",
		   name);
      return FALSE;
  }
}


static gboolean
parse_shapers (const char *name G_GNUC_UNUSED,
	       const char *arg,
	       gpointer    data,
	       GError    **error G_GNUC_UNUSED)
{
  shape_options_t *shape_opts = (shape_options_t *) data;
  shape_opts->shapers = g_strsplit (arg, ",", 0);
  return TRUE;
}

static G_GNUC_NORETURN gboolean
list_shapers (const char *name G_GNUC_UNUSED,
	      const char *arg G_GNUC_UNUSED,
	      gpointer    data G_GNUC_UNUSED,
	      GError    **error G_GNUC_UNUSED)
{
  for (const char **shaper = hb_shape_list_shapers (); *shaper; shaper++)
    g_printf ("%s\n", *shaper);

  exit(0);
}



static void
parse_space (char **pp)
{
  char c;
#define ISSPACE(c) ((c)==' '||(c)=='\f'||(c)=='\n'||(c)=='\r'||(c)=='\t'||(c)=='\v')
  while (c = **pp, ISSPACE (c))
    (*pp)++;
#undef ISSPACE
}

static hb_bool_t
parse_char (char **pp, char c)
{
  parse_space (pp);

  if (**pp != c)
    return FALSE;

  (*pp)++;
  return TRUE;
}

static hb_bool_t
parse_uint (char **pp, unsigned int *pv)
{
  char *p = *pp;
  unsigned int v;

  v = strtol (p, pp, 0);

  if (p == *pp)
    return FALSE;

  *pv = v;
  return TRUE;
}


static hb_bool_t
parse_feature_value_prefix (char **pp, hb_feature_t *feature)
{
  if (parse_char (pp, '-'))
    feature->value = 0;
  else {
    parse_char (pp, '+');
    feature->value = 1;
  }

  return TRUE;
}

static hb_bool_t
parse_feature_tag (char **pp, hb_feature_t *feature)
{
  char *p = *pp, c;

  parse_space (pp);

#define ISALNUM(c) (('a' <= (c) && (c) <= 'z') || ('A' <= (c) && (c) <= 'Z') || ('0' <= (c) && (c) <= '9'))
  while (c = **pp, ISALNUM(c))
    (*pp)++;
#undef ISALNUM

  if (p == *pp)
    return FALSE;

  feature->tag = hb_tag_from_string (p, *pp - p);
  return TRUE;
}

static hb_bool_t
parse_feature_indices (char **pp, hb_feature_t *feature)
{
  parse_space (pp);

  hb_bool_t has_start;

  feature->start = 0;
  feature->end = (unsigned int) -1;

  if (!parse_char (pp, '['))
    return TRUE;

  has_start = parse_uint (pp, &feature->start);

  if (parse_char (pp, ':')) {
    parse_uint (pp, &feature->end);
  } else {
    if (has_start)
      feature->end = feature->start + 1;
  }

  return parse_char (pp, ']');
}

static hb_bool_t
parse_feature_value_postfix (char **pp, hb_feature_t *feature)
{
  return !parse_char (pp, '=') || parse_uint (pp, &feature->value);
}


static hb_bool_t
parse_one_feature (char **pp, hb_feature_t *feature)
{
  return parse_feature_value_prefix (pp, feature) &&
	 parse_feature_tag (pp, feature) &&
	 parse_feature_indices (pp, feature) &&
	 parse_feature_value_postfix (pp, feature) &&
	 (parse_char (pp, ',') || **pp == '\0');
}

static void
skip_one_feature (char **pp)
{
  char *e;
  e = strchr (*pp, ',');
  if (e)
    *pp = e + 1;
  else
    *pp = *pp + strlen (*pp);
}

static gboolean
parse_features (const char *name G_GNUC_UNUSED,
	        const char *arg,
	        gpointer    data,
	        GError    **error G_GNUC_UNUSED)
{
  shape_options_t *shape_opts = (shape_options_t *) data;
  char *s = (char *) arg;
  char *p;

  shape_opts->num_features = 0;
  shape_opts->features = NULL;

  if (!*s)
    return TRUE;

  /* count the features first, so we can allocate memory */
  p = s;
  do {
    shape_opts->num_features++;
    p = strchr (p, ',');
    if (p)
      p++;
  } while (p);

  shape_opts->features = (hb_feature_t *) calloc (shape_opts->num_features, sizeof (*shape_opts->features));

  /* now do the actual parsing */
  p = s;
  shape_opts->num_features = 0;
  while (*p) {
    if (parse_one_feature (&p, &shape_opts->features[shape_opts->num_features]))
      shape_opts->num_features++;
    else
      skip_one_feature (&p);
  }

  return TRUE;
}


void
view_options_t::add_options (option_parser_t *parser)
{
  GOptionEntry entries[] =
  {
    {"annotate",	0, 0, G_OPTION_ARG_NONE,	&this->annotate,		"Annotate output rendering",				NULL},
    {"background",	0, 0, G_OPTION_ARG_STRING,	&this->back,			"Set background color (default: "DEFAULT_BACK")",	"red/#rrggbb/#rrggbbaa"},
    {"foreground",	0, 0, G_OPTION_ARG_STRING,	&this->fore,			"Set foreground color (default: "DEFAULT_FORE")",	"red/#rrggbb/#rrggbbaa"},
    {"line-space",	0, 0, G_OPTION_ARG_DOUBLE,	&this->line_space,		"Set space between lines (default: 0)",			"units"},
    {"margin",		0, 0, G_OPTION_ARG_CALLBACK,	(gpointer) &parse_margin,	"Margin around output (default: "G_STRINGIFY(DEFAULT_MARGIN)")","one to four numbers"},
    {"font-size",	0, 0, G_OPTION_ARG_DOUBLE,	&this->font_size,		"Font size (default: "G_STRINGIFY(DEFAULT_FONT_SIZE)")","size"},
    {NULL}
  };
  parser->add_group (entries,
		     "view",
		     "View options:",
		     "Options controlling the output rendering",
		     this);
}

void
shape_options_t::add_options (option_parser_t *parser)
{
  GOptionEntry entries[] =
  {
    {"list-shapers",	0, G_OPTION_FLAG_NO_ARG,
			      G_OPTION_ARG_CALLBACK,	(gpointer) &list_shapers,	"List available shapers and quit",	NULL},
    {"shapers",		0, 0, G_OPTION_ARG_CALLBACK,	(gpointer) &parse_shapers,	"Comma-separated list of shapers to try","list"},
    {"direction",	0, 0, G_OPTION_ARG_STRING,	&this->direction,		"Set text direction (default: auto)",	"ltr/rtl/ttb/btt"},
    {"language",	0, 0, G_OPTION_ARG_STRING,	&this->language,		"Set text language (default: $LANG)",	"langstr"},
    {"script",		0, 0, G_OPTION_ARG_STRING,	&this->script,			"Set text script (default: auto)",	"ISO-15924 tag"},
    {NULL}
  };
  parser->add_group (entries,
		     "shape",
		     "Shape options:",
		     "Options controlling the shaping process",
		     this);

  const gchar *features_help = "\n"
    "\n"
    "    Comma-separated list of font features to apply to text\n"
    "\n"
    "    Features can be enabled or disabled, either globally or limited to\n"
    "    specific character ranges.  The range indices refer to the positions\n"
    "    between Unicode characters.  The position before the first character\n"
    "    is 0, and the position after the first character is 1, and so on.\n"
    "\n"
    "    The format is Python-esque.  Here is how it all works:\n"
    "\n"
    "      Syntax:       Value:    Start:    End:\n"
    "\n"
    "    Setting value:\n"
    "      \"kern\"        1         0         ∞         # Turn feature on\n"
    "      \"+kern\"       1         0         ∞         # Turn feature on\n"
    "      \"-kern\"       0         0         ∞         # Turn feature off\n"
    "      \"kern=0\"      0         0         ∞         # Turn feature off\n"
    "      \"kern=1\"      1         0         ∞         # Turn feature on\n"
    "      \"aalt=2\"      2         0         ∞         # Choose 2nd alternate\n"
    "\n"
    "    Setting index:\n"
    "      \"kern[]\"      1         0         ∞         # Turn feature on\n"
    "      \"kern[:]\"     1         0         ∞         # Turn feature on\n"
    "      \"kern[5:]\"    1         5         ∞         # Turn feature on, partial\n"
    "      \"kern[:5]\"    1         0         5         # Turn feature on, partial\n"
    "      \"kern[3:5]\"   1         3         5         # Turn feature on, range\n"
    "      \"kern[3]\"     1         3         3+1       # Turn feature on, single char\n"
    "\n"
    "    Mixing it all:\n"
    "\n"
    "      \"kern[3:5]=0\" 1         3         5         # Turn feature off for range";

  GOptionEntry entries2[] =
  {
    {"features",	0, 0, G_OPTION_ARG_CALLBACK,	(gpointer) &parse_features,	features_help,	"list"},
    {NULL}
  };
  parser->add_group (entries2,
		     "features",
		     "Features options:",
		     "Options controlling the OpenType font features applied",
		     this);
}

void
font_options_t::add_options (option_parser_t *parser)
{
  GOptionEntry entries[] =
  {
    {"font-file",	0, 0, G_OPTION_ARG_STRING,	&this->font_file,		"Font file-name",					"filename"},
    {"face-index",	0, 0, G_OPTION_ARG_INT,		&this->face_index,		"Face index (default: 0)",                              "index"},
    {NULL}
  };
  parser->add_group (entries,
		     "font",
		     "Font options:",
		     "Options controlling the font",
		     this);
}

void
text_options_t::add_options (option_parser_t *parser)
{
  GOptionEntry entries[] =
  {
    {"text",		0, 0, G_OPTION_ARG_STRING,	&this->text,			"Set input text",			"string"},
    {"text-file",	0, 0, G_OPTION_ARG_STRING,	&this->text_file,		"Set input text file-name",		"filename"},
    {NULL}
  };
  parser->add_group (entries,
		     "text",
		     "Text options:",
		     "Options controlling the input text",
		     this);
}

void
output_options_t::add_options (option_parser_t *parser)
{
  GOptionEntry entries[] =
  {
    {"output-file",	0, 0, G_OPTION_ARG_STRING,	&this->output_file,		"Set output file-name (default: stdout)","filename"},
    {"output-format",	0, 0, G_OPTION_ARG_STRING,	&this->output_format,		"Set output format",			"format"},
    {NULL}
  };
  parser->add_group (entries,
		     "output",
		     "Output options:",
		     "Options controlling the output",
		     this);
}



hb_font_t *
font_options_t::get_font (void) const
{
  if (font)
    return font;

  hb_blob_t *blob = NULL;

  /* Create the blob */
  {
    char *font_data;
    unsigned int len = 0;
    hb_destroy_func_t destroy;
    void *user_data;
    hb_memory_mode_t mm;

    /* This is a hell of a lot of code for just reading a file! */
    if (!font_file)
      fail (TRUE, "No font file set");

    if (0 == strcmp (font_file, "-")) {
      /* read it */
      GString *gs = g_string_new (NULL);
      char buf[BUFSIZ];
#ifdef HAVE__SETMODE
      _setmode (fileno (stdin), _O_BINARY);
#endif
      while (!feof (stdin)) {
	size_t ret = fread (buf, 1, sizeof (buf), stdin);
	if (ferror (stdin))
	  fail (FALSE, "Failed reading font from standard input: %s",
		strerror (errno));
	g_string_append_len (gs, buf, ret);
      }
      len = gs->len;
      font_data = g_string_free (gs, FALSE);
      user_data = font_data;
      destroy = (hb_destroy_func_t) g_free;
      mm = HB_MEMORY_MODE_WRITABLE;
    } else {
      GMappedFile *mf = g_mapped_file_new (font_file, FALSE, NULL);
      if (mf) {
	font_data = g_mapped_file_get_contents (mf);
	len = g_mapped_file_get_length (mf);
	if (len) {
	  destroy = (hb_destroy_func_t) g_mapped_file_unref;
	  user_data = (void *) mf;
	  mm = HB_MEMORY_MODE_READONLY_MAY_MAKE_WRITABLE;
	} else
	  g_mapped_file_unref (mf);
      }
      if (!len) {
	/* GMappedFile is buggy, it doesn't fail if file isn't regular.
	 * Try reading.
	 * https://bugzilla.gnome.org/show_bug.cgi?id=659212 */
        GError *error = NULL;
	gsize l;
	if (g_file_get_contents (font_file, &font_data, &l, &error)) {
	  len = l;
	  destroy = (hb_destroy_func_t) g_free;
	  user_data = (void *) font_data;
	  mm = HB_MEMORY_MODE_WRITABLE;
	} else {
	  fail (FALSE, "%s", error->message);
	  //g_error_free (error);
	}
      }
    }

    blob = hb_blob_create (font_data, len, mm, user_data, destroy);
  }

  /* Create the face */
  hb_face_t *face = hb_face_create (blob, face_index);
  hb_blob_destroy (blob);


  font = hb_font_create (face);

  unsigned int upem = hb_face_get_upem (face);
  hb_font_set_scale (font, upem, upem);
  hb_face_destroy (face);

#ifdef HAVE_FREETYPE
  hb_ft_font_set_funcs (font);
#endif

  return font;
}


const char *
text_options_t::get_line (unsigned int *len)
{
  if (text) {
    if (text_len == (unsigned int) -1)
      text_len = strlen (text);

    if (!text_len) {
      *len = 0;
      return NULL;
    }

    const char *ret = text;
    const char *p = (const char *) memchr (text, '\n', text_len);
    unsigned int ret_len;
    if (!p) {
      ret_len = text_len;
      text += ret_len;
      text_len = 0;
    } else {
      ret_len = p - ret;
      text += ret_len + 1;
      text_len -= ret_len + 1;
    }

    *len = ret_len;
    return ret;
  }

  if (!fp) {
    if (!text_file)
      fail (TRUE, "At least one of text or text-file must be set");

    if (0 != strcmp (text_file, "-"))
      fp = fopen (text_file, "r");
    else
      fp = stdin;

    if (!fp)
      fail (FALSE, "Failed opening text file `%s': %s",
	    text_file, strerror (errno));

    gs = g_string_new (NULL);
  }

  g_string_set_size (gs, 0);
  char buf[BUFSIZ];
  while (fgets (buf, sizeof (buf), fp)) {
    unsigned int bytes = strlen (buf);
    if (bytes && buf[bytes - 1] == '\n') {
      bytes--;
      g_string_append_len (gs, buf, bytes);
      break;
    }
      g_string_append_len (gs, buf, bytes);
  }
  if (ferror (fp))
    fail (FALSE, "Failed reading text: %s",
	  strerror (errno));
  *len = gs->len;
  return !*len && feof (fp) ? NULL : gs->str;
}


FILE *
output_options_t::get_file_handle (void)
{
  if (fp)
    return fp;

  if (output_file)
    fp = fopen (output_file, "wb");
  else {
#ifdef HAVE__SETMODE
    _setmode (fileno (stdout), _O_BINARY);
#endif
    fp = stdout;
  }
  if (!fp)
    fail (FALSE, "Cannot open output file `%s': %s",
	  g_filename_display_name (output_file), strerror (errno));

  return fp;
}


void
format_options_t::add_options (option_parser_t *parser)
{
  GOptionEntry entries[] =
  {
    {"no-glyph-names",	0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE,	&this->show_glyph_names,	"Use glyph indices instead of names",	NULL},
    {"no-positions",	0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE,	&this->show_positions,		"Do not show glyph positions",		NULL},
    {"no-clusters",	0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE,	&this->show_clusters,		"Do not show cluster mapping",		NULL},
    {"show-text",	0, 0,			  G_OPTION_ARG_NONE,	&this->show_text,		"Show input text",			NULL},
    {"show-unicode",	0, 0,			  G_OPTION_ARG_NONE,	&this->show_unicode,		"Show input Unicode codepoints",	NULL},
    {"show-line-num",	0, 0,			  G_OPTION_ARG_NONE,	&this->show_line_num,		"Show line numbers",			NULL},
    {NULL}
  };
  parser->add_group (entries,
		     "format",
		     "Format options:",
		     "Options controlling the formatting of buffer contents",
		     this);
}

void
format_options_t::serialize_unicode (hb_buffer_t *buffer,
				     GString     *gs)
{
  unsigned int num_glyphs = hb_buffer_get_length (buffer);
  hb_glyph_info_t *info = hb_buffer_get_glyph_infos (buffer, NULL);

  g_string_append_c (gs, '<');
  for (unsigned int i = 0; i < num_glyphs; i++)
  {
    if (i)
      g_string_append_c (gs, ',');
    g_string_append_printf (gs, "U+%04X", info->codepoint);
    info++;
  }
  g_string_append_c (gs, '>');
}

void
format_options_t::serialize_glyphs (hb_buffer_t *buffer,
				    hb_font_t   *font,
				    GString     *gs)
{
  FT_Face ft_face = show_glyph_names ? hb_ft_font_get_face (font) : NULL;

  unsigned int num_glyphs = hb_buffer_get_length (buffer);
  hb_glyph_info_t *info = hb_buffer_get_glyph_infos (buffer, NULL);
  hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (buffer, NULL);

  g_string_append_c (gs, '[');
  for (unsigned int i = 0; i < num_glyphs; i++)
  {
    if (i)
      g_string_append_c (gs, '|');

    char glyph_name[30];
    if (show_glyph_names) {
      if (!FT_Get_Glyph_Name (ft_face, info->codepoint, glyph_name, sizeof (glyph_name)))
	g_string_append_printf (gs, "%s", glyph_name);
      else
	g_string_append_printf (gs, "gid%u", info->codepoint);
    } else
      g_string_append_printf (gs, "%u", info->codepoint);

    if (show_clusters)
      g_string_append_printf (gs, "=%u", info->cluster);

    if (show_positions && (pos->x_offset || pos->y_offset)) {
      g_string_append_c (gs, '@');
      if (pos->x_offset) g_string_append_printf (gs, "%d", pos->x_offset);
      if (pos->y_offset) g_string_append_printf (gs, ",%d", pos->y_offset);
    }
    if (show_positions && (pos->x_advance || pos->y_advance)) {
      g_string_append_c (gs, '+');
      if (pos->x_advance) g_string_append_printf (gs, "%d", pos->x_advance);
      if (pos->y_advance) g_string_append_printf (gs, ",%d", pos->y_advance);
    }

    info++;
    pos++;
  }
  g_string_append_c (gs, ']');
}
void
format_options_t::serialize_line_no (unsigned int  line_no,
				     GString      *gs)
{
  if (show_line_num)
    g_string_append_printf (gs, "%d: ", line_no);
}
void
format_options_t::serialize_line (hb_buffer_t  *buffer,
				  unsigned int  line_no,
				  const char   *text,
				  unsigned int  text_len,
				  hb_font_t    *font,
				  GString      *gs)
{
  if (show_text) {
    serialize_line_no (line_no, gs);
    g_string_append_c (gs, '(');
    g_string_append_len (gs, text, text_len);
    g_string_append_c (gs, ')');
    g_string_append_c (gs, '\n');
  }

  if (show_unicode) {
    serialize_line_no (line_no, gs);
    hb_buffer_reset (scratch);
    hb_buffer_add_utf8 (scratch, text, text_len, 0, -1);
    serialize_unicode (scratch, gs);
    g_string_append_c (gs, '\n');
  }

  serialize_line_no (line_no, gs);
  serialize_glyphs (buffer, font, gs);
  g_string_append_c (gs, '\n');
}
