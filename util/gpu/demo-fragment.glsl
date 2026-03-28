uniform float u_gamma;
uniform vec4 u_foreground;

in vec2 v_texcoord;
flat in uint v_glyphLoc;

out vec4 fragColor;

void main ()
{
  float coverage = hb_gpu_render (v_texcoord, v_glyphLoc);

  if (u_gamma != 1.0)
    coverage = pow (coverage, u_gamma);

  fragColor = vec4 (u_foreground.rgb, u_foreground.a * coverage);
}
