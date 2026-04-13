/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 */

uniform float u_gamma;
uniform float u_debug;
uniform float u_stem_darkening;
uniform vec4 u_foreground;

in vec2 v_texcoord;
flat in uint v_glyphLoc;

out vec4 fragColor;

void main ()
{
  /* The paint interpreter returns a premultiplied RGBA.  For
   * non-color glyphs the encoder synthesizes a single
   * LAYER_SOLID with is_foreground=true, so the result is
   * u_foreground * coverage -- same visual as the old draw
   * path.  For COLRv0 glyphs it returns the composited color. */
  vec4 c = hb_gpu_paint (v_texcoord, v_glyphLoc, u_foreground);

  if (u_gamma != 1.0)
    c.a = pow (c.a, u_gamma);

  if (u_debug > 0.0)
  {
    ivec2 counts = _hb_gpu_curve_counts (v_texcoord, v_glyphLoc);
    float r = clamp (float (counts.x) / 8.0, 0.0, 1.0);
    float g = clamp (float (counts.y) / 8.0, 0.0, 1.0);
    fragColor = vec4 (r, g, c.a, max (max (r, g), c.a));
    return;
  }

  fragColor = c;
}
