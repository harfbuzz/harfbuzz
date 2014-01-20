#include "main-font-text.hh"
#include "shape-compare.hh"
#include "hb-ft.h"

struct output_buffer_t
{
  output_buffer_t (option_parser_t *parser)
		  : options (parser,
			     g_strjoinv ("/", (gchar**) hb_buffer_serialize_list_formats ())),
		    format (parser),
		    gs (NULL),
		    line_no (0),
		    font (NULL) {}

  void init (const font_options_t *font_opts)
  {
    options.get_file_handle ();
    gs = g_string_new (NULL);
    line_no = 0;
    font = hb_font_reference (font_opts->get_font ());
    memset (&stats, 0, sizeof (stats));

    if (!options.output_format)
      output_format = HB_BUFFER_SERIALIZE_FORMAT_TEXT;
    else
      output_format = hb_buffer_serialize_format_from_string (options.output_format, -1);
    if (!hb_buffer_serialize_format_to_string (output_format))
    {
      if (options.explicit_output_format)
	fail (false, "Unknown output format `%s'; supported formats are: %s",
	      options.output_format, options.supported_formats);
      else
	/* Just default to TEXT if not explicitly requested and the
	 * file extension is not recognized. */
	output_format = HB_BUFFER_SERIALIZE_FORMAT_TEXT;
    }

    unsigned int flags = HB_BUFFER_SERIALIZE_FLAG_DEFAULT;
    if (!format.show_glyph_names)
      flags |= HB_BUFFER_SERIALIZE_FLAG_NO_GLYPH_NAMES;
    if (!format.show_clusters)
      flags |= HB_BUFFER_SERIALIZE_FLAG_NO_CLUSTERS;
    if (!format.show_positions)
      flags |= HB_BUFFER_SERIALIZE_FLAG_NO_POSITIONS;
    format_flags = (hb_buffer_serialize_flags_t) flags;
  }
  void new_line (void)
  {
    g_string_set_size (gs, 0);
    line_no++;
  }
  void consume_text (hb_buffer_t  *buffer,
		     const char   *text,
		     unsigned int  text_len,
		     hb_bool_t     utf8_clusters)
  {
    format.serialize_buffer_of_text (buffer, line_no, text, text_len, font, gs);
  }
  void shape_failed (hb_buffer_t  *buffer,
		     const char   *text,
		     unsigned int  text_len,
		     hb_bool_t     utf8_clusters)
  {
    format.serialize_message (line_no, "msg: test shaper failed", gs);
  }
  void ref_shape_failed (hb_buffer_t  *buffer,
		     const char   *text,
		     unsigned int  text_len,
		     hb_bool_t     utf8_clusters)
  {
    format.serialize_message (line_no, "msg: reference shaper failed", gs);
  }
  void visual_match ()
  {
    format.serialize_message (line_no, "msg: visual match", gs);
  }
  void consume_glyphs (hb_buffer_t  *buffer,
		       const char   *text,
		       unsigned int  text_len,
		       hb_bool_t     utf8_clusters)
  {
    format.serialize_buffer_of_glyphs (buffer, line_no, text, text_len, font,
				       output_format, format_flags, gs);
  }
  void dump_image (GString *image_data, int image_num)
  {
    if (!options.output_file)
      return;
    GString *name = g_string_new (0);
    g_string_printf (name, "%s.%d.image%d.png",
                     options.output_file,
                     line_no,
                     image_num);
    FILE *f = fopen (name->str, "wb");
    if (f) {
      fwrite (image_data->str, image_data->len, 1, f);
      fclose (f);
    }
    g_string_free (name, true);
  }
  void report_differences (hb_buffer_differences_t diffs)
  {
    if (diffs & HB_BUFFER_COMPARE_LENGTH_MISMATCH)
      format.serialize_message (line_no, "diff: L", gs);
    else if (diffs & HB_BUFFER_COMPARE_CODEPOINT_MISMATCH)
      format.serialize_message (line_no, "diff: G", gs);
    else if (diffs & HB_BUFFER_COMPARE_POSITION_MISMATCH)
      format.serialize_message (line_no, "diff: P", gs);
    else if (diffs & HB_BUFFER_COMPARE_CLUSTER_MISMATCH)
      format.serialize_message (line_no, "diff: C", gs);
    if (diffs & HB_BUFFER_COMPARE_VISUAL_MATCH)
      format.serialize_message (line_no, "note: visual match", gs);
    if (diffs & HB_BUFFER_COMPARE_NOTDEF_PRESENT)
      format.serialize_message (line_no, "note: .notdef", gs);
    if (diffs & HB_BUFFER_COMPARE_DOTTED_CIRCLE_PRESENT)
      format.serialize_message (line_no, "note: dottedcircle", gs);
  }
  void collect_stats (hb_buffer_differences_t diffs)
  {
    unsigned int *counters;
    if (diffs & HB_BUFFER_COMPARE_LENGTH_MISMATCH)
      counters = stats.length;
    else if (diffs & HB_BUFFER_COMPARE_CODEPOINT_MISMATCH)
      counters = stats.glyph;
    else if (diffs & HB_BUFFER_COMPARE_POSITION_MISMATCH)
      counters = stats.position;
    else if (diffs & HB_BUFFER_COMPARE_CLUSTER_MISMATCH)
      counters = stats.cluster;
    else
      counters = stats.match;
    counters[0]++;
    if (diffs & HB_BUFFER_COMPARE_VISUAL_MATCH)
      counters[1]++;
    else if (diffs & HB_BUFFER_COMPARE_DOTTED_CIRCLE_PRESENT)
      counters[2]++;
    else if (diffs & HB_BUFFER_COMPARE_NOTDEF_PRESENT)
      counters[3]++;
  }
  void report_summary ()
  {
    fprintf (options.fp, "summary: Total    %d\n", stats.match[0] + stats.length[0] + stats.glyph[0] + stats.position[0] + stats.cluster[0]);
    fprintf (options.fp, "summary: Match    %d\n", stats.match[0]);
    fprintf (options.fp, "summary: Length   %d (%d/%d/%d)\n", stats.length[0], stats.length[1], stats.length[2], stats.length[3]);
    fprintf (options.fp, "summary: Glyph    %d (%d/%d/%d)\n", stats.glyph[0], stats.glyph[1], stats.glyph[2], stats.glyph[3]);
    fprintf (options.fp, "summary: Position %d (%d/%d/%d)\n", stats.position[0], stats.position[1], stats.position[2], stats.position[3]);
    fprintf (options.fp, "summary: Cluster  %d (%d/%d/%d)\n", stats.cluster[0], stats.cluster[1], stats.cluster[2], stats.cluster[3]);
  }
  void finish_line ()
  {
    fprintf (options.fp, "%s", gs->str);
  }
  void finish (const font_options_t *font_opts)
  {
    report_summary ();
    hb_font_destroy (font);
    g_string_free (gs, true);
    gs = NULL;
    font = NULL;
  }
  void get_surface_size (cairo_scaled_font_t *scaled_font,
                         helper_cairo_line_t *line,
                         hb_direction_t direction,
                         const view_options_t *view_opts,
                         double *w, double *h);
  GString *render_to_png (helper_cairo_line_t *line,
                          hb_direction_t direction,
                          const font_options_t *font_opts,
                          const view_options_t *view_options);

  protected:
  output_options_t options;
  format_options_t format;

  struct {
    unsigned int length[4];
    unsigned int glyph[4];
    unsigned int position[4];
    unsigned int cluster[4];
    unsigned int match[4];
  } stats;

  GString *gs;
  unsigned int line_no;
  hb_font_t *font;
  hb_buffer_serialize_format_t output_format;
  hb_buffer_serialize_flags_t format_flags;
};

void
output_buffer_t::get_surface_size (cairo_scaled_font_t *scaled_font,
                                   helper_cairo_line_t *line,
                                   hb_direction_t direction,
                                   const view_options_t *view_opts,
                                   double *w, double *h)
{
  cairo_font_extents_t font_extents;

  cairo_scaled_font_extents (scaled_font, &font_extents);

  bool vertical = HB_DIRECTION_IS_VERTICAL (direction);
  (vertical ? *w : *h) = font_extents.height;
  (vertical ? *h : *w) = 0;
  double x_advance, y_advance;
  line->get_advance (&x_advance, &y_advance);
  if (vertical)
    *h = y_advance;
  else
    *w = x_advance;
  *w += view_opts->margin.l + view_opts->margin.r;
  *h += view_opts->margin.t + view_opts->margin.b;
}

// returns a string containing the rendering of the line
GString *
output_buffer_t::render_to_png (helper_cairo_line_t *line,
                                hb_direction_t direction,
                                const font_options_t *font_opts,
                                const view_options_t *view_opts)
{
  cairo_scaled_font_t *scaled_font = helper_cairo_create_scaled_font (font_opts, view_opts->font_size);

  double width, height;
  get_surface_size (scaled_font, line, direction, view_opts, &width, &height);

  GString *rval = g_string_new (NULL);

  const char *save_output_format = options.output_format;
  options.output_format = "png";
  cairo_t *cr = helper_cairo_create_context (width, height, view_opts, &options, rval);
  options.output_format = save_output_format;
  cairo_set_scaled_font (cr, scaled_font);
  cairo_scaled_font_destroy (scaled_font);

  bool vertical = HB_DIRECTION_IS_VERTICAL (direction);
  int v = vertical ? 1 : 0;
  int h = vertical ? 0 : 1;
  cairo_font_extents_t font_extents;
  cairo_font_extents (cr, &font_extents);
  cairo_translate (cr, view_opts->margin.l, view_opts->margin.t);
  double descent;
  if (vertical)
    descent = font_extents.height * 1.5;
  else
    descent = font_extents.height - font_extents.ascent;
  cairo_translate (cr, v * descent, h * -descent);

  cairo_translate (cr, v * -font_extents.height, h * font_extents.height);

  if (line->num_clusters)
    cairo_show_text_glyphs (cr,
			    line->utf8, line->utf8_len,
			    line->glyphs, line->num_glyphs,
			    line->clusters, line->num_clusters,
			    line->cluster_flags);
  else
    cairo_show_glyphs (cr, line->glyphs, line->num_glyphs);

  helper_cairo_destroy_context (cr);

  return rval;
}

int
main (int argc, char **argv)
{
  main_font_text_t<shape_compare_t<output_buffer_t> > driver;
  return driver.main (argc, argv);
}
