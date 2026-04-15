/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 */

cbuffer Uniforms : register(b0) {
  float4x4 mvp;
  float2 viewport;
  float gamma;
  float stem_darkening;
  float4 foreground;
  float debug;
  float3 _pad;
};

struct VSInput {
  float2 position : POSITION;
  float2 texcoord : TEXCOORD0;
  float2 normal   : NORMAL;
  float emPerPos   : TEXCOORD1;
  uint glyphLoc    : TEXCOORD2;
};

struct PSInput {
  float4 pos      : SV_Position;
  float2 texcoord : TEXCOORD0;
  nointerpolation uint glyphLoc : TEXCOORD1;
};

PSInput vs_main (VSInput input) {
  float2 pos = input.position;
  float2 tex = input.texcoord;
  float4 jac = float4 (input.emPerPos, 0.0, 0.0, -input.emPerPos);
  hb_gpu_dilate (pos, tex, input.normal, jac, mvp, viewport);
  PSInput output;
  output.pos = mul (mvp, float4 (pos, 0.0, 1.0));
  output.texcoord = tex;
  output.glyphLoc = input.glyphLoc;
  return output;
}

float4 ps_main (PSInput input) : SV_Target {
#ifdef HB_GPU_DEMO_DRAW
  float cov = hb_gpu_draw (input.texcoord, input.glyphLoc);
  float4 c = float4 (foreground.rgb * foreground.a, foreground.a) * cov;
#else
  float4 c = hb_gpu_paint (input.texcoord, input.glyphLoc, foreground);
#endif

  if (stem_darkening > 0.0 && c.a > 0.0) {
    float2 fw = fwidth (input.texcoord);
    float ppem = 1.0 / max (fw.x, fw.y);
    float brightness = dot (c.rgb, float3 (1.0/3.0, 1.0/3.0, 1.0/3.0)) / c.a;
    float darkened = hb_gpu_stem_darken (c.a, brightness, ppem);
    c *= darkened / c.a;
  }

  if (gamma != 1.0)
    c.a = pow (c.a, gamma);
  return c;
}
