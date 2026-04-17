/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 */

struct Uniforms {
  mvp: mat4x4f,
  viewport: vec2f,
  gamma: f32,
  stem_darkening: f32,
  foreground: vec4f,
  debug: f32,
};

@group(0) @binding(0) var<uniform> u: Uniforms;
@group(0) @binding(1) var<storage, read> hb_gpu_atlas: array<vec4<i32>>;

struct VertexInput {
  @location(0) position: vec2f,
  @location(1) texcoord: vec2f,
  @location(2) normal: vec2f,
  @location(3) emPerPos: f32,
  @location(4) glyphLoc: u32,
};

struct VertexOutput {
  @builtin(position) clip_position: vec4f,
  @location(0) texcoord: vec2f,
  @location(1) @interpolate(flat) glyphLoc: u32,
};

@vertex fn vs_main (in: VertexInput) -> VertexOutput {
  var pos = in.position;
  var tc = in.texcoord;
  let jac = vec4f (in.emPerPos, 0.0, 0.0, -in.emPerPos);
  let result = hb_gpu_dilate (pos, tc, in.normal, jac, u.mvp, u.viewport);
  pos = result[0];
  tc = result[1];

  var out: VertexOutput;
  out.clip_position = u.mvp * vec4f (pos, 0.0, 1.0);
  out.texcoord = tc;
  out.glyphLoc = in.glyphLoc;
  return out;
}

@fragment fn fs_main (in: VertexOutput) -> @location(0) vec4f {
  /* Compute ppem up front so fwidth is called at uniform control
   * flow, before the non-uniform branches below. */
  let fw = fwidth (in.texcoord);
  let ppem = 1.0 / max (fw.x, fw.y);

  var cov: f32;
  var c = hb_gpu_paint (in.texcoord, in.glyphLoc, u.foreground,
                        &hb_gpu_atlas, &cov);

  if (cov > 0.0 && cov < 1.0) {
    var adj = cov;
    if (u.stem_darkening > 0.0) {
      let brightness = select (0.0,
        dot (c.rgb, vec3f (1.0 / 3.0)) / c.a, c.a > 0.0);
      adj = hb_gpu_stem_darken (adj, brightness, ppem);
    }
    if (u.gamma != 1.0) {
      adj = pow (adj, u.gamma);
    }
    c = c * (adj / cov);
  }

  if (u.debug > 0.0) {
    let counts = _hb_gpu_curve_counts (in.texcoord, in.glyphLoc, &hb_gpu_atlas);
    let r = clamp (f32 (counts.x) / 8.0, 0.0, 1.0);
    let g = clamp (f32 (counts.y) / 8.0, 0.0, 1.0);
    return vec4f (r, g, c.a, max (max (r, g), c.a));
  }

  return c;
}
