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
 * Google Author(s): Behdad Esfahbod
 */

#include "batch.hh"
#include "font-options.hh"

const unsigned DEFAULT_FONT_SIZE = FONT_SIZE_UPEM;
const unsigned SUBPIXEL_BITS = 0;


struct info_t
{
  void add_options (option_parser_t *parser)
  {
    GOptionEntry entries[] =
    {
      {"list-all",	0, 0, G_OPTION_ARG_NONE,	&this->list_all,		"List everything",		nullptr},
      {"list-tables",	0, 0, G_OPTION_ARG_NONE,	&this->list_tables,		"List tables",			nullptr},
      {"list-unicodes",	0, 0, G_OPTION_ARG_NONE,	&this->list_unicodes,		"List characters",		nullptr},
      {"list-glyphs",	0, 0, G_OPTION_ARG_NONE,	&this->list_glyphs,		"List glyphs",			nullptr},
      {"list-features",	0, 0, G_OPTION_ARG_NONE,	&this->list_features,		"List layout features",		nullptr},
#ifndef HB_NO_VAR
      {"list-variations",0, 0, G_OPTION_ARG_NONE,	&this->list_variations,		"List variations",		nullptr},
#endif
      {nullptr}
    };
    parser->add_group (entries,
		       "query",
		       "Query options:",
		       "Options to query the font instance",
		       this,
		       false /* We add below. */);
  }

  private:
  hb_face_t *face = nullptr;
  hb_font_t *font = nullptr;
  hb_bool_t list_all = false;
  hb_bool_t list_tables = false;
  hb_bool_t list_unicodes = false;
  hb_bool_t list_glyphs = false;
  hb_bool_t list_features = false;
#ifndef HB_NO_VAR
  hb_bool_t list_variations = false;
#endif
  public:

  void operator () (font_options_t *font_opts)
  {
    face = hb_face_reference (font_opts->face);
    font = hb_font_reference (font_opts->font);

    if (list_all)
    {
      list_tables =
      list_unicodes =
      list_glyphs =
      list_features =
#ifndef HB_NO_VAR
      list_variations =
#endif
      true;
    }

    if (list_tables)	  _list_tables ();
    if (list_unicodes)	  _list_unicodes ();
    if (list_glyphs)	  _list_glyphs ();
    if (list_features)	  _list_features ();
#ifndef HB_NO_VAR
    if (list_variations)  _list_variations ();
#endif

    hb_font_destroy (font);
    hb_face_destroy (face);
  }

  private:

  void _list_tables ()
  {
    unsigned count = hb_face_get_table_tags (face, 0, nullptr, nullptr);
    hb_tag_t *tags = (hb_tag_t *) calloc (count, sizeof (hb_tag_t));
    hb_face_get_table_tags (face, 0, &count, tags);

    for (unsigned i = 0; i < count; i++)
    {
      hb_tag_t tag = tags[i];

      hb_blob_t *blob = hb_face_reference_table (face, tag);

      printf ("%c%c%c%c: %8u bytes\n", HB_UNTAG (tag), hb_blob_get_length (blob));

      hb_blob_destroy (blob);
    }

    free (tags);
  }

  void
  _list_unicodes ()
  {
    hb_face_t *face = hb_font_get_face (font);

    hb_set_t *unicodes = hb_set_create ();
    hb_map_t *cmap = hb_map_create ();

    hb_face_collect_nominal_glyph_mapping (face, cmap, unicodes);

    for (hb_codepoint_t u = HB_SET_VALUE_INVALID;
	 hb_set_next (unicodes, &u);)
    {
      hb_codepoint_t gid = hb_map_get (cmap, u);

      char glyphname[64];
      hb_font_glyph_to_string (font, gid,
			       glyphname, sizeof glyphname);

      printf ("U+%04X	%s\n", u, glyphname);
    }

    hb_map_destroy (cmap);


    /* List variation-selector sequences. */
    hb_set_t *vars = hb_set_create ();

    hb_face_collect_variation_selectors (face, vars);

    for (hb_codepoint_t vs = HB_SET_VALUE_INVALID;
	 hb_set_next (vars, &vs);)
    {
      hb_set_clear (unicodes);
      hb_face_collect_variation_unicodes (face, vs, unicodes);

      for (hb_codepoint_t u = HB_SET_VALUE_INVALID;
	   hb_set_next (unicodes, &u);)
      {
	hb_codepoint_t gid = 0;
	bool b = hb_font_get_variation_glyph (font, u, vs, &gid);
	assert (b);

	char glyphname[64];
	hb_font_glyph_to_string (font, gid,
				 glyphname, sizeof glyphname);

	printf ("U+%04X,U+%04X	%s\n", vs, u, glyphname);
      }
    }

    hb_set_destroy (vars);
    hb_set_destroy (unicodes);
  }

  void
  _list_glyphs ()
  {
    hb_face_t *face = hb_font_get_face (font);

    unsigned num_glyphs = hb_face_get_glyph_count (face);

    for (hb_codepoint_t gid = 0; gid < num_glyphs; gid++)
    {
      char glyphname[64];
      hb_font_glyph_to_string (font, gid,
			       glyphname, sizeof glyphname);

      printf ("%u	%s\n", gid, glyphname);
    }
  }

  void
  _list_features ()
  {
    hb_tag_t table_tags[] = {HB_OT_TAG_GSUB, HB_OT_TAG_GPOS, HB_TAG_NONE};
    auto language = hb_language_get_default ();
    hb_set_t *features = hb_set_create ();

    for (unsigned int i = 0; table_tags[i]; i++)
    {
      printf ("Table: %c%c%c%c\n", HB_UNTAG (table_tags[i]));

      hb_tag_t script_array[32];
      unsigned script_count = sizeof script_array / sizeof script_array[0];
      unsigned script_offset = 0;
      do
      {
	hb_ot_layout_table_get_script_tags (face, table_tags[i],
					    script_offset,
					    &script_count,
					    script_array);

	for (unsigned script_index = 0; script_index < script_count; script_index++)
	{
	  printf ("  Script: %c%c%c%c\n", HB_UNTAG (script_array[script_index]));

	  hb_tag_t language_array[32];
	  unsigned language_count = sizeof language_array / sizeof language_array[0];
	  unsigned language_offset = 0;
	  do
	  {
	    hb_ot_layout_script_get_language_tags (face, table_tags[i],
						   script_offset + script_index,
						   language_offset,
						   &language_count,
						   language_array);

	    for (unsigned language_index = 0; language_index < language_count; language_index++)
	    {
	      printf ("    Language: %c%c%c%c\n", HB_UNTAG (language_array[language_index]));
	    }

	    language_offset += language_count;
	  }
	  while (language_count == sizeof language_array / sizeof language_array[0]);
	}

	script_offset += script_count;
      }
      while (script_count == sizeof script_array / sizeof script_array[0]);

      hb_set_clear (features);
      hb_tag_t feature_array[32];
      unsigned feature_count = sizeof feature_array / sizeof feature_array[0];
      unsigned feature_offset = 0;
      do
      {
	hb_ot_layout_table_get_feature_tags (face, table_tags[i],
					     feature_offset,
					     &feature_count,
					     feature_array);

	for (unsigned feature_index = 0; feature_index < feature_count; feature_index++)
	{
	  if (hb_set_has (features, feature_array[feature_index]))
	    continue;
	  hb_set_add (features, feature_array[feature_index]);

	  hb_ot_name_id_t label_id;

	  hb_ot_layout_feature_get_name_ids (face,
					     table_tags[i],
					     feature_offset + feature_index,
					     &label_id,
					     nullptr,
					     nullptr,
					     nullptr,
					     nullptr);

	  char name[64];
	  unsigned name_len = sizeof name;

	  hb_ot_name_get_utf8 (face, label_id,
			       language,
			       &name_len, name);

	  printf ("  Feature: %c%c%c%c", HB_UNTAG (feature_array[feature_index]));

	  if (*name)
	    printf (" \"%s\"", name);

	  printf ("\n");
	}

	feature_offset += feature_count;
      }
      while (feature_count == sizeof feature_array / sizeof feature_array[0]);
    }

    hb_set_destroy (features);
  }

#ifndef HB_NO_VAR
  void
  _list_variations ()
  {
    hb_ot_var_axis_info_t *axes;

    unsigned count = hb_ot_var_get_axis_infos (face, 0, nullptr, nullptr);
    axes = (hb_ot_var_axis_info_t *) calloc (count, sizeof (hb_ot_var_axis_info_t));
    hb_ot_var_get_axis_infos (face, 0, &count, axes);

    auto language = hb_language_get_default ();
    bool has_hidden = false;

    printf ("Varitation axes:\n");
    printf ("Tag:	Minimum	Default	Maximum	Name\n\n");
    for (unsigned i = 0; i < count; i++)
    {
      const auto &axis = axes[i];
      if (axis.flags & HB_OT_VAR_AXIS_FLAG_HIDDEN)
	has_hidden = true;

      char name[64];
      unsigned name_len = sizeof name;

      hb_ot_name_get_utf8 (face, axis.name_id,
			   language,
			   &name_len, name);

      printf ("%c%c%c%c%s:	%g	%g	%g	%s\n",
	      HB_UNTAG (axis.tag),
	      axis.flags & HB_OT_VAR_AXIS_FLAG_HIDDEN ? "*" : "",
	      (double) axis.min_value,
	      (double) axis.default_value,
	      (double) axis.max_value,
	      name);
    }
    if (has_hidden)
      printf ("\n[*] Hidden axis\n");

    free (axes);
    axes = nullptr;

    count = hb_ot_var_get_named_instance_count (face);
    if (count)
    {
      printf ("\n\nNamed instances: \n\n");

      for (unsigned i = 0; i < count; i++)
      {
	char name[64];
	unsigned name_len = sizeof name;

	hb_ot_name_id_t name_id = hb_ot_var_named_instance_get_subfamily_name_id (face, i);
	hb_ot_name_get_utf8 (face, name_id,
			     language,
			     &name_len, name);

	unsigned coords_count = hb_ot_var_named_instance_get_design_coords (face, i, nullptr, nullptr);
	float* coords;
	coords = (float *) calloc (coords_count, sizeof (float));
	hb_ot_var_named_instance_get_design_coords (face, i, &coords_count, coords);

	printf ("%u. %-32s", i, name);
	for (unsigned j = 0; j < coords_count; j++)
	  printf ("%g, ", (double) coords[j]);
	printf ("\n");

	free (coords);
      }
    }
  }
#endif
};


template <typename consumer_t,
	  typename font_options_type>
struct main_font_t :
       option_parser_t,
       font_options_type,
       consumer_t
{
  int operator () (int argc, char **argv)
  {
    add_options ();
    parse (&argc, &argv);

    consumer_t::operator () (this);

    return 0;
  }

  protected:

  void add_options ()
  {
    font_options_type::add_options (this);
    consumer_t::add_options (this);

    GOptionEntry entries[] =
    {
      {G_OPTION_REMAINING,	0, G_OPTION_FLAG_IN_MAIN,
				G_OPTION_ARG_CALLBACK,	(gpointer) &collect_rest,	nullptr,	"[FONT-FILE]"},
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
    main_font_t *thiz = (main_font_t *) data;

    if (!thiz->font_file)
    {
      thiz->font_file = g_strdup (arg);
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
  using main_t = main_font_t<info_t, font_options_t>;
  return batch_main<main_t> (argc, argv);
}