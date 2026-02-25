#include <hb.h>
#include <hb-vector.h>

#include <stdlib.h>
#include <stdio.h>

int
main (int argc, char **argv)
{
  if (argc < 2)
  {
    fprintf (stderr, "Usage: %s font-file [font-funcs] [wght]\n", argv[0]);
    return 1;
  }

  hb_face_t *face = hb_face_create_from_file_or_fail (argv[1], 0);
  if (!face)
  {
    fprintf (stderr, "Failed to create face\n");
    return 1;
  }

  hb_font_t *font = hb_font_create (face);
  if (!font)
  {
    hb_face_destroy (face);
    return 1;
  }

  if (argc > 2)
    hb_font_set_funcs_using (font, argv[2]);

  if (argc > 3)
  {
    hb_variation_t variations[] = {
      { HB_TAG ('w', 'g', 'h', 't'), atoi (argv[3]) },
    };
    hb_font_set_variations (font, variations, 1);
  }

  hb_vector_draw_t *draw = hb_vector_draw_create_or_fail (HB_VECTOR_FORMAT_SVG);
  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (HB_VECTOR_FORMAT_SVG);
  if (!draw || !paint)
  {
    hb_vector_draw_destroy (draw);
    hb_vector_paint_destroy (paint);
    hb_font_destroy (font);
    hb_face_destroy (face);
    return 1;
  }

  hb_vector_paint_set_palette (paint, 0);
  hb_vector_paint_set_foreground (paint, HB_COLOR (0, 0, 0, 255));

  unsigned glyph_count = hb_face_get_glyph_count (face);
  for (unsigned gid = 0; gid < glyph_count; gid++)
  {
    hb_vector_paint_set_transform (paint, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f);
    if (hb_vector_paint_glyph (paint, font, gid, 0.f, 0.f,
                               HB_VECTOR_EXTENTS_MODE_EXPAND))
    {
      hb_blob_t *blob = hb_vector_paint_render (paint);
      hb_vector_paint_recycle_blob (paint, blob);
      continue;
    }

    hb_vector_draw_set_transform (draw, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f);
    if (hb_vector_draw_glyph (draw, font, gid, 0.f, 0.f,
                              HB_VECTOR_EXTENTS_MODE_EXPAND))
    {
      hb_blob_t *blob = hb_vector_draw_render (draw);
      hb_vector_draw_recycle_blob (draw, blob);
    }
  }

  hb_vector_draw_destroy (draw);
  hb_vector_paint_destroy (paint);
  hb_font_destroy (font);
  hb_face_destroy (face);
  return 0;
}
