in vec2 v_texcoord;
flat in uint v_glyphLoc;

out vec4 fragColor;

void main ()
{
  float coverage = hb_gpu_render (v_texcoord, v_glyphLoc);

  fragColor = vec4 (0.0, 0.0, 0.0, coverage);
}
