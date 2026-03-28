static const char *demo_fragment_glsl =
"in vec2 v_texcoord;\n"
"flat in uint v_glyphLoc;\n"
"\n"
"out vec4 fragColor;\n"
"\n"
"void main ()\n"
"{\n"
"  float coverage = hb_gpu_render (v_texcoord, v_glyphLoc);\n"
"\n"
"  fragColor = vec4 (0.0, 0.0, 0.0, coverage);\n"
"}\n"
;
