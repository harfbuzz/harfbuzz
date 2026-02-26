#include <hb.h>
#include <hb-vector.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int
hexval (char c)
{
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

static hb_bool_t
parse_hex_color (const char *s, hb_color_t *color)
{
  if (!s || !*s || !color)
    return 0;

  if (*s == '#')
    s++;

  size_t len = strlen (s);
  if (len != 6 && len != 8)
    return 0;

  int vals[8];
  for (unsigned i = 0; i < len; i++)
  {
    vals[i] = hexval (s[i]);
    if (vals[i] < 0)
      return 0;
  }

  unsigned r = (unsigned) (vals[0] * 16 + vals[1]);
  unsigned g = (unsigned) (vals[2] * 16 + vals[3]);
  unsigned b = (unsigned) (vals[4] * 16 + vals[5]);
  unsigned a = len == 8 ? (unsigned) (vals[6] * 16 + vals[7]) : 255u;
  *color = HB_COLOR (b, g, r, a);
  return 1;
}

static void
apply_custom_palette (hb_vector_paint_t *paint, const char *spec)
{
  hb_vector_paint_clear_custom_palette_colors (paint);
  if (!spec || !*spec)
    return;

  char *copy = strdup (spec);
  if (!copy)
    return;

  unsigned idx = 0;
  char *saveptr = NULL;
  for (char *tok = strtok_r (copy, ",", &saveptr);
       tok;
       tok = strtok_r (NULL, ",", &saveptr), idx++)
  {
    while (*tok == ' ' || *tok == '\t')
      tok++;
    if (!*tok)
      continue;

    hb_color_t color;
    if (!parse_hex_color (tok, &color))
    {
      fprintf (stderr, "Ignoring invalid custom palette color: %s\n", tok);
      continue;
    }
    hb_vector_paint_set_custom_palette_color (paint, idx, color);
  }

  free (copy);
}

int
main (int argc, char **argv)
{
  if (argc < 2)
  {
    fprintf (stderr, "Usage: %s font-file [font-funcs] [wght] [custom-palette]\n", argv[0]);
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
  if (argc > 4)
    apply_custom_palette (paint, argv[4]);

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
