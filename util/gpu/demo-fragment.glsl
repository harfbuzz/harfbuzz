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
  float cov;
#ifdef HB_GPU_DEMO_DRAW
  cov = hb_gpu_draw (v_texcoord, v_glyphLoc);
  vec4 c = vec4 (u_foreground.rgb * u_foreground.a, u_foreground.a) * cov;
#else
  vec4 c = hb_gpu_paint (v_texcoord, v_glyphLoc, u_foreground, cov);
#endif

  /* Apply stem darkening and gamma correction to the edge
   * coverage only, so interior color is unaffected. */
  if (cov > 0.0 && cov < 1.0)
  {
    float adj = cov;
    if (u_stem_darkening > 0.0)
    {
      float brightness = c.a > 0.0
	? dot (c.rgb, vec3 (1.0 / 3.0)) / c.a : 0.0;
      adj = hb_gpu_stem_darken (adj, brightness,
	1.0 / max (fwidth (v_texcoord).x, fwidth (v_texcoord).y));
    }
    if (u_gamma != 1.0)
      adj = pow (adj, u_gamma);
    c *= adj / cov;
  }

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
