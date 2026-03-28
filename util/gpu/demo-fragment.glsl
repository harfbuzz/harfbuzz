uniform float u_gamma;

in vec2 v_texcoord;
flat in uint v_glyphLoc;

out vec4 fragColor;

void main ()
{
  float coverage = hb_gpu_render (v_texcoord, v_glyphLoc);

  if (u_gamma != 1.0)
    coverage = pow (coverage, u_gamma);

  fragColor = vec4 (0.0, 0.0, 0.0, coverage);
}
