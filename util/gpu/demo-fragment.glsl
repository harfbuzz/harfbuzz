uniform float u_gamma;
uniform float u_debug;
uniform vec4 u_foreground;

in vec2 v_texcoord;
flat in uint v_glyphLoc;

out vec4 fragColor;

void main ()
{
  float coverage = hb_gpu_render (v_texcoord, v_glyphLoc);

  if (u_gamma != 1.0)
    coverage = pow (coverage, u_gamma);

  if (u_debug > 0.0)
  {
    ivec2 counts = _hb_gpu_curve_counts (v_texcoord, v_glyphLoc);
    float r = clamp (float (counts.x) / 8.0, 0.0, 1.0);
    float g = clamp (float (counts.y) / 8.0, 0.0, 1.0);
    /* Blue text on black, then add R/G heatmap */
    fragColor = vec4 (r, g, coverage, max (max (r, g), coverage));
    return;
  }

  fragColor = vec4 (u_foreground.rgb, u_foreground.a * coverage);
}
