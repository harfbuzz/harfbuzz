#include "options.hh"
#include "helper-cairo.hh"

#ifndef HB_SHAPE_COMPARE_HH
#define HB_SHAPE_COMPARE_HH


template <typename output_t>
struct shape_compare_t
{
  shape_compare_t (option_parser_t *parser)
		  : failed (false),
		    shaper (parser),
		    output (parser),
		    font (NULL),
		    view_options (parser) {}

  void init (const font_options_t *font_opts)
  {
    font = hb_font_reference (font_opts->get_font ());
    ref_buffer = hb_buffer_create ();
    output.init (font_opts);
    failed = false;
    ref_failed = false;
    if (!hb_font_get_glyph (font, 0x25cc, 0, &dotted_circle))
      dotted_circle = (hb_codepoint_t) -1;
    scale = double (view_options.font_size) / hb_face_get_upem (hb_font_get_face (font));
  }

  void consume_line (hb_buffer_t  *buffer,
		     const char   *text,
		     unsigned int  text_len,
		     const char   *text_before,
		     const char   *text_after,
		     const font_options_t *font_opts)
  {
    output.new_line ();

    shaper.populate_buffer (buffer, text, text_len, text_before, text_after);
    shaper.populate_buffer (ref_buffer, text, text_len, text_before, text_after);

    output.consume_text (buffer, text, text_len, shaper.utf8_clusters);

    if (!shaper.shape (font, buffer)) {
      failed = true;
      hb_buffer_set_length (buffer, 0);
    }

    if (!shaper.shape_reference (font, ref_buffer)) {
      ref_failed = true;
      hb_buffer_set_length (ref_buffer, 0);
    }

    hb_buffer_differences_t diffs = hb_buffers_compare (buffer, ref_buffer, dotted_circle);

    if (diffs & (HB_BUFFER_COMPARE_CODEPOINT_MISMATCH |
                 HB_BUFFER_COMPARE_CLUSTER_MISMATCH |
                 HB_BUFFER_COMPARE_POSITION_MISMATCH |
                 HB_BUFFER_COMPARE_LENGTH_MISMATCH)) {
      output.consume_glyphs (buffer, text, text_len, shaper.utf8_clusters);
      output.consume_glyphs (ref_buffer, text, text_len, shaper.utf8_clusters);

      GString *test_image = NULL;
      GString *reference_image = NULL;

      if (failed)
        output.shape_failed (buffer, text, text_len, shaper.utf8_clusters);
      else {
        helper_cairo_line_t l;
        helper_cairo_line_from_buffer (&l, buffer, text, text_len, scale, shaper.utf8_clusters);
        test_image = output.render_to_png (&l, hb_buffer_get_direction (buffer), font_opts, &view_options);
        l.finish ();
      }
      if (ref_failed)
        output.ref_shape_failed (ref_buffer, text, text_len, shaper.utf8_clusters);
      else {
        helper_cairo_line_t l;
        helper_cairo_line_from_buffer (&l, ref_buffer, text, text_len, scale, shaper.utf8_clusters);
        reference_image = output.render_to_png (&l, hb_buffer_get_direction (ref_buffer), font_opts, &view_options);
        l.finish ();
      }

      if (test_image && reference_image) {
        if (test_image->len != reference_image->len ||
            memcmp(test_image->str, reference_image->str, test_image->len)) {
          output.dump_image (test_image, 1);
          output.dump_image (reference_image, 2);
        }
        else {
          diffs = (hb_buffer_differences_t) (diffs | HB_BUFFER_COMPARE_VISUAL_MATCH);
          output.dump_image (test_image, 1);
        }
      }

      if (test_image)
	g_string_free (test_image, true);
      if (reference_image)
	g_string_free (reference_image, true);

      output.report_differences (diffs);

      output.finish_line ();
    }

    output.collect_stats (diffs);
  }

  void finish (const font_options_t *font_opts)
  {
    output.finish (font_opts);
    hb_font_destroy (font);
    hb_buffer_destroy (ref_buffer);
    font = NULL;
  }

  public:
  bool failed;
  bool ref_failed;

  protected:
  shape_options_t shaper;
  view_options_t view_options;
  output_t output;

  hb_font_t *font;
  hb_buffer_t *ref_buffer;
  hb_codepoint_t dotted_circle;

  // for rendering
  double scale;
};


#endif
