#include <hb.h>

#include <stdlib.h>
#include <stdio.h>

int main (int argc, char **argv)
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

  if (argc > 2)
    hb_font_set_funcs_using (font, argv[2]);

  if (argc > 3)
  {
    hb_variation_t variations[] = {
      { HB_TAG ('w', 'g', 'h', 't'), atoi (argv[3]) },
    };
    hb_font_set_variations (font, variations, 1);
  }

  hb_paint_funcs_t *funcs = hb_paint_funcs_create ();

  unsigned glyph_count = hb_face_get_glyph_count (face);
  for (unsigned gid = 0; gid < glyph_count; gid++)
    hb_font_paint_glyph (font, gid, funcs, NULL, 0, 0);

  hb_paint_funcs_destroy (funcs);
  hb_font_destroy (font);
  hb_face_destroy (face);
  return 0;
}
