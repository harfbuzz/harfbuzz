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
#ifdef HB_GPU_DEMO_DRAW
  /* Fast path for fonts with no color paint: call the Slug
   * coverage sampler directly and tint by the foreground uniform.
   * Smaller shader program, better occupancy. */
  float cov = hb_gpu_draw (v_texcoord, v_glyphLoc);
  vec4 c = vec4 (u_foreground.rgb * u_foreground.a, u_foreground.a) * cov;
#else
  /* Paint interpreter -- handles monochrome via a synthesized
   * LAYER_SOLID and color paint trees otherwise. */
  vec4 c = hb_gpu_paint (v_texcoord, v_glyphLoc, u_foreground);
#endif

  /* Stem darkening: scale the premultiplied value by the
   * alpha-channel darkening ratio so RGB and A stay in lockstep. */
  if (u_stem_darkening > 0.0 && c.a > 0.0)
  {
    float darkened = hb_gpu_stem_darken (c.a,
      dot (u_foreground.rgb, vec3 (1.0 / 3.0)),
      1.0 / max (fwidth (v_texcoord).x, fwidth (v_texcoord).y));
    c *= darkened / c.a;
  }

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
