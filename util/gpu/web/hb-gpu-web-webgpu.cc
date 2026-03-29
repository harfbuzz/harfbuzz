/*
 * Copyright (C) 2026  Behdad Esfahbod
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Author(s): Behdad Esfahbod
 */

/*
 * WebGPU web demo.
 *
 * Uses Emscripten WebGPU bindings (-sUSE_WEBGPU=1) instead of WebGL.
 * Does not use GLFW; input is handled via Emscripten HTML5 APIs.
 */

#include <emscripten.h>
#include <emscripten/html5.h>
#include <webgpu/webgpu.h>

#include <hb.h>
#include <hb-gpu.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <string>

/* Math helpers — no GL dependency */
#include "../trackball.hh"
#include "../matrix4x4.hh"

/* Font/buffer infrastructure needs GL type stubs */
#include "gl-stub.h"

/* Now we can include the demo headers for shaping */
#include "../demo-common.h"
#include "../demo-font.h"
#include "../demo-buffer.h"

#include "../default-text-combined.hh"
#include "../default-text-en.hh"
#include "../default-font.hh"


/* ---- WGSL demo shader ---- */

static const char *wgsl_demo_shader =
"struct Uniforms {\n"
"  mvp: mat4x4f,\n"
"  viewport: vec2f,\n"
"  gamma: f32,\n"
"  _pad: f32,\n"
"  foreground: vec4f,\n"
"};\n"
"\n"
"@group(0) @binding(0) var<uniform> u: Uniforms;\n"
"@group(0) @binding(1) var<storage, read> hb_gpu_atlas: array<vec4<i32>>;\n"
"\n"
"struct VertexInput {\n"
"  @location(0) position: vec2f,\n"
"  @location(1) texcoord: vec2f,\n"
"  @location(2) normal: vec2f,\n"
"  @location(3) emPerPos: f32,\n"
"  @location(4) glyphLoc: u32,\n"
"};\n"
"\n"
"struct VertexOutput {\n"
"  @builtin(position) clip_position: vec4f,\n"
"  @location(0) texcoord: vec2f,\n"
"  @location(1) @interpolate(flat) glyphLoc: u32,\n"
"};\n"
"\n"
"@vertex fn vs_main (in: VertexInput) -> VertexOutput {\n"
"  var pos = in.position;\n"
"  var tc = in.texcoord;\n"
"  let jac = vec4f (in.emPerPos, 0.0, 0.0, -in.emPerPos);\n"
"  let result = hb_gpu_dilate (pos, tc, in.normal, jac, u.mvp, u.viewport);\n"
"  pos = result[0];\n"
"  tc = result[1];\n"
"\n"
"  var out: VertexOutput;\n"
"  out.clip_position = u.mvp * vec4f (pos, 0.0, 1.0);\n"
"  out.texcoord = tc;\n"
"  out.glyphLoc = in.glyphLoc;\n"
"  return out;\n"
"}\n"
"\n"
"@fragment fn fs_main (in: VertexOutput) -> @location(0) vec4f {\n"
"  let coverage = hb_gpu_render (in.texcoord, in.glyphLoc, &hb_gpu_atlas);\n"
"  var c = coverage;\n"
"  if (u.gamma != 1.0) {\n"
"    c = pow (c, u.gamma);\n"
"  }\n"
"  return vec4f (u.foreground.rgb, u.foreground.a * c);\n"
"}\n";


/* ---- Uniform buffer layout (must match WGSL struct) ---- */

struct Uniforms {
  float mvp[16];
  float viewport[2];
  float gamma;
  float _pad;
  float foreground[4];
};


/* ---- Atlas (storage buffer) ---- */

struct atlas_t {
  WGPUBuffer buf;
  int32_t *shadow;      /* CPU shadow (i32 expanded from i16) */
  unsigned capacity;    /* in texels */
  unsigned cursor;      /* in texels */
};

static atlas_t atlas;

static unsigned
atlas_alloc (void *ctx, const char *data, unsigned len_bytes)
{
  atlas_t *at = (atlas_t *) ctx;
  unsigned len_texels = len_bytes / 8; /* 8 bytes per i16x4 texel */

  if (at->cursor + len_texels > at->capacity)
  {
    fprintf (stderr, "Ran out of atlas memory\n");
    abort ();
  }

  unsigned offset = at->cursor;
  at->cursor += len_texels;

  /* Expand i16x4 → i32x4 into shadow buffer */
  const int16_t *src = (const int16_t *) data;
  int32_t *dst = at->shadow + offset * 4;
  for (unsigned i = 0; i < len_texels * 4; i++)
    dst[i] = (int32_t) src[i];

  return offset;
}

static unsigned
atlas_get_used (void *ctx)
{
  return ((atlas_t *) ctx)->cursor;
}

static void
atlas_clear (void *ctx)
{
  ((atlas_t *) ctx)->cursor = 0;
}


/* ---- WebGPU state ---- */

static WGPUAdapter adapter;
static WGPUDevice device;
static WGPUQueue queue;
static WGPUSurface surface;
static WGPURenderPipeline pipeline;
static WGPUBuffer uniform_buf;
static WGPUBuffer vertex_buf;
static unsigned vertex_buf_capacity;
static WGPUBindGroup bind_group;
static WGPUTextureFormat surface_format;

/* ---- Demo state ---- */

static demo_font_t *current_demo_font;
static demo_buffer_t *buffer;
static hb_blob_t *current_blob;
static hb_face_t *current_face;
static hb_font_t *current_font;
static char *current_text;
static bool custom_text;

static int canvas_w, canvas_h;
static int css_w, css_h;
static bool needs_redraw = true;
static bool dark_mode = false;
static int gamma_mode = 0; /* 0=2.2, 1=none */
static bool animate = false;

/* View state */
/* View state matching demo_view_t */
static double translate_x, translate_y;
static double scalex = 1, scaley = 1;
static double perspective = 16;
static float quat[4];
static float rot_axis[3] = {0, 0, 1};
static double rot_speed = 1.0;

static float
compute_gamma ()
{
  if (gamma_mode == 0)
    return dark_mode ? 1.f / 2.2f : 2.2f;
  return 1.f; /* none */
}

static bool dragging = false;
static bool right_dragging = false;
static double last_mouse_x, last_mouse_y;
static double drag_start_x, drag_start_y;
static bool drag_started = false;


/* ---- Forward declarations ---- */

static void rebuild_buffer (const char *text);
static void render_frame ();
static void create_bind_group ();


/* ---- View helpers ---- */

static void
compute_mvp (float mat[16])
{
  int width = canvas_w, height = canvas_h;

  m4LoadIdentity (mat);

  /* Apply view transform — matches demo_view_apply_transform exactly */
  m4Scale (mat, scalex, scaley, 1);
  m4Translate (mat, translate_x, translate_y, 0);

  {
    double d = std::max (width, height);
    double near = d / perspective;
    double far = near + d;
    double factor = near / (2 * near + d);
    m4Frustum (mat, -width * factor, width * factor, -height * factor, height * factor, near, far);
    m4Translate (mat, 0, 0, -(near + d * .5));
  }

  float m[4][4];
  build_rotmatrix (m, quat);
  m4MultMatrix (mat, &m[0][0]);

  m4Scale (mat, 1, -1, 1);

  /* Content centering — matches demo_view_display */
  demo_extents_t extents;
  demo_buffer_extents (buffer, NULL, &extents);
  double content_scale = .9 * std::min (width  / (extents.max_x - extents.min_x),
					height / (extents.max_y - extents.min_y));
  m4Scale (mat, content_scale, content_scale, 1);
  m4Translate (mat,
	       -(extents.max_x + extents.min_x) / 2.,
	       -(extents.max_y + extents.min_y) / 2., 0);
}


/* ---- Atlas upload to GPU ---- */

static void
atlas_upload ()
{
  unsigned used = atlas.cursor;
  if (used == 0) return;
  wgpuQueueWriteBuffer (queue, atlas.buf, 0,
			atlas.shadow, used * 4 * sizeof (int32_t));
}


/* ---- Rendering ---- */

static void
render_frame ()
{
  WGPUSurfaceTexture surfTex;
  wgpuSurfaceGetCurrentTexture (surface, &surfTex);
  if (surfTex.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal &&
      surfTex.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal)
    return;

  WGPUTextureView view = wgpuTextureCreateView (surfTex.texture, NULL);

  /* Update uniforms */
  Uniforms u;
  compute_mvp (u.mvp);
  u.viewport[0] = (float) canvas_w;
  u.viewport[1] = (float) canvas_h;
  u.gamma = compute_gamma ();
  u._pad = 0;
  if (dark_mode)
  {
    u.foreground[0] = u.foreground[1] = u.foreground[2] = 1.0f;
    u.foreground[3] = 1.0f;
  }
  else
  {
    u.foreground[0] = u.foreground[1] = u.foreground[2] = 0.0f;
    u.foreground[3] = 1.0f;
  }
  wgpuQueueWriteBuffer (queue, uniform_buf, 0, &u, sizeof (u));

  /* Upload atlas data */
  atlas_upload ();

  /* Upload vertices */
  unsigned vtx_count = 0;
  glyph_vertex_t *vertices = demo_buffer_get_vertices (buffer, &vtx_count);

  if (vtx_count > 0)
  {
    unsigned needed = vtx_count * sizeof (glyph_vertex_t);
    if (needed > vertex_buf_capacity)
    {
      if (vertex_buf)
	wgpuBufferRelease (vertex_buf);
      WGPUBufferDescriptor desc = {};
      desc.size = needed;
      desc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
      vertex_buf = wgpuDeviceCreateBuffer (device, &desc);
      vertex_buf_capacity = needed;
    }
    wgpuQueueWriteBuffer (queue, vertex_buf, 0, vertices, needed);
  }

  /* Render pass */
  float bg_r, bg_g, bg_b;
  if (dark_mode)
    bg_r = bg_g = bg_b = 0.133f;
  else
    bg_r = bg_g = bg_b = 1.0f;

  WGPURenderPassColorAttachment colorAtt = {};
  colorAtt.view = view;
  colorAtt.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
  colorAtt.loadOp = WGPULoadOp_Clear;
  colorAtt.storeOp = WGPUStoreOp_Store;
  colorAtt.clearValue = {bg_r, bg_g, bg_b, 1.0};

  WGPURenderPassDescriptor rpDesc = {};
  rpDesc.colorAttachmentCount = 1;
  rpDesc.colorAttachments = &colorAtt;

  WGPUCommandEncoderDescriptor ceDesc = {};
  WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder (device, &ceDesc);
  WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass (encoder, &rpDesc);

  if (vtx_count > 0)
  {
    wgpuRenderPassEncoderSetPipeline (pass, pipeline);
    wgpuRenderPassEncoderSetBindGroup (pass, 0, bind_group, 0, NULL);
    wgpuRenderPassEncoderSetVertexBuffer (pass, 0, vertex_buf, 0, vtx_count * sizeof (glyph_vertex_t));
    wgpuRenderPassEncoderDraw (pass, vtx_count, 1, 0, 0);
  }

  wgpuRenderPassEncoderEnd (pass);
  wgpuRenderPassEncoderRelease (pass);

  WGPUCommandBufferDescriptor cbDesc = {};
  WGPUCommandBuffer cmdBuf = wgpuCommandEncoderFinish (encoder, &cbDesc);
  wgpuCommandEncoderRelease (encoder);
  wgpuQueueSubmit (queue, 1, &cmdBuf);
  wgpuCommandBufferRelease (cmdBuf);

  wgpuTextureViewRelease (view);
  wgpuTextureRelease (surfTex.texture);

  /* Browser auto-presents; wgpuSurfacePresent is not supported in Emscripten. */
}


/* ---- Input handling ---- */

static EM_BOOL
on_mousedown (int type, const EmscriptenMouseEvent *e, void *ud)
{
  last_mouse_x = e->targetX;
  last_mouse_y = e->targetY;
  drag_start_x = e->targetX;
  drag_start_y = e->targetY;
  drag_started = false;

  if (e->button == 0)
    dragging = true;
  else if (e->button == 2)
    right_dragging = true;
  return EM_TRUE;
}

static EM_BOOL
on_mouseup (int type, const EmscriptenMouseEvent *e, void *ud)
{
  if (e->button == 0 && dragging && !drag_started)
  {
    /* Click — toggle animation */
    animate = !animate;
    needs_redraw = true;
  }
  dragging = false;
  right_dragging = false;
  return EM_TRUE;
}

static EM_BOOL
on_mousemove (int type, const EmscriptenMouseEvent *e, void *ud)
{
  double prev_x = last_mouse_x;
  double prev_y = last_mouse_y;
  last_mouse_x = e->targetX;
  last_mouse_y = e->targetY;

  if (!dragging && !right_dragging)
    return EM_FALSE;

  double dist = sqrt ((e->targetX - drag_start_x) * (e->targetX - drag_start_x) +
		       (e->targetY - drag_start_y) * (e->targetY - drag_start_y));
  if (dist > 5) drag_started = true;

  if (dragging && drag_started)
  {
    double dx = last_mouse_x - prev_x;
    double dy = last_mouse_y - prev_y;
    translate_x += 2.0 * dx / css_w / scalex;
    translate_y -= 2.0 * dy / css_h / scaley;
    needs_redraw = true;
  }
  else if (right_dragging)
  {
    float dquat[4];
    trackball (dquat,
	       (2.0 * prev_x - css_w) / css_w,
	       (css_h - 2.0 * prev_y) / css_h,
	       (2.0 * last_mouse_x - css_w) / css_w,
	       (css_h - 2.0 * last_mouse_y) / css_h);
    add_quats (dquat, quat, quat);
    needs_redraw = true;
  }
  return EM_TRUE;
}

static EM_BOOL
on_wheel (int type, const EmscriptenWheelEvent *e, void *ud)
{
  float delta = (float) -e->deltaY;
  if (e->deltaMode == DOM_DELTA_LINE)
    delta *= 40.0f;
  double factor = pow (1.001, delta);
  scalex *= factor;
  scaley *= factor;
  needs_redraw = true;
  return EM_TRUE;
}

static EM_BOOL
on_keydown (int type, const EmscriptenKeyboardEvent *e, void *ud)
{
  if (strcmp (e->key, "b") == 0)
  {
    dark_mode = !dark_mode;
    printf ("Dark mode: %s\n", dark_mode ? "on" : "off");
    needs_redraw = true;
  }
  else if (strcmp (e->key, "g") == 0)
  {
    gamma_mode = (gamma_mode + 1) % 2;
    const char *names[] = {"gamma 2.2", "none"};
    printf ("Gamma correction: %s\n", names[gamma_mode]);
    needs_redraw = true;
  }
  else if (strcmp (e->key, "r") == 0)
  {
    perspective = 16;
    scalex = scaley = 1;
    translate_x = translate_y = 0;
    trackball (quat, 0., 0., 0., 0.);
    needs_redraw = true;
  }
  else if (strcmp (e->key, " ") == 0)
  {
    animate = !animate;
    needs_redraw = true;
  }
  else if (strcmp (e->key, "=") == 0 || strcmp (e->key, "+") == 0)
  {
    scalex *= 1.2;
    scaley *= 1.2;
    needs_redraw = true;
  }
  else if (strcmp (e->key, "-") == 0)
  {
    scalex /= 1.2;
    scaley /= 1.2;
    needs_redraw = true;
  }
  return EM_FALSE; /* let text modal handler see it too */
}

/* Touch handling */
static double pinch_dist;

static EM_BOOL
on_touchstart (int type, const EmscriptenTouchEvent *e, void *ud)
{
  if (e->numTouches == 1)
  {
    last_mouse_x = e->touches[0].targetX;
    last_mouse_y = e->touches[0].targetY;
    drag_start_x = last_mouse_x;
    drag_start_y = last_mouse_y;
    drag_started = false;
    dragging = true;
  }
  else if (e->numTouches == 2)
  {
    dragging = false;
    double dx = e->touches[1].targetX - e->touches[0].targetX;
    double dy = e->touches[1].targetY - e->touches[0].targetY;
    pinch_dist = sqrt (dx * dx + dy * dy);
  }
  return EM_TRUE;
}

static EM_BOOL
on_touchmove (int type, const EmscriptenTouchEvent *e, void *ud)
{
  if (e->numTouches == 1 && dragging)
  {
    double dx = e->touches[0].targetX - last_mouse_x;
    double dy = e->touches[0].targetY - last_mouse_y;
    last_mouse_x = e->touches[0].targetX;
    last_mouse_y = e->touches[0].targetY;

    double dist = sqrt ((last_mouse_x - drag_start_x) * (last_mouse_x - drag_start_x) +
			 (last_mouse_y - drag_start_y) * (last_mouse_y - drag_start_y));
    if (dist > 5) drag_started = true;

    if (drag_started)
    {
      translate_x += 2.0 * dx / css_w / scalex;
      translate_y -= 2.0 * dy / css_h / scaley;
      needs_redraw = true;
    }
  }
  else if (e->numTouches == 2)
  {
    double dx = e->touches[1].targetX - e->touches[0].targetX;
    double dy = e->touches[1].targetY - e->touches[0].targetY;
    double dist = sqrt (dx * dx + dy * dy);
    if (pinch_dist > 0)
    {
      double factor = dist / pinch_dist;
      scalex *= factor;
      scaley *= factor;
      needs_redraw = true;
    }
    pinch_dist = dist;
  }
  return EM_TRUE;
}

static EM_BOOL
on_touchend (int type, const EmscriptenTouchEvent *e, void *ud)
{
  if (dragging && !drag_started)
  {
    animate = !animate;
    needs_redraw = true;
  }
  dragging = false;
  return EM_TRUE;
}


/* ---- Buffer rebuild ---- */

static void
rebuild_buffer (const char *text)
{
  free (current_text);
  current_text = strdup (text);

  demo_font_clear_cache (current_demo_font);
  atlas_clear (&atlas);

  demo_buffer_clear (buffer);
  demo_point_t top_left = {0, 0};
  demo_buffer_move_to (buffer, &top_left);
  demo_buffer_add_text (buffer, text, current_demo_font, 1);

  /* Re-upload atlas and recreate bind group (buffer contents changed) */
  atlas_upload ();
  needs_redraw = true;
}


/* ---- Exported functions for JS ---- */

extern "C" {

EMSCRIPTEN_KEEPALIVE void
web_load_font (const char *data, int len)
{
  hb_blob_t *blob = hb_blob_create (data, len,
				     HB_MEMORY_MODE_DUPLICATE,
				     NULL, NULL);
  hb_face_t *face = hb_face_create (blob, 0);
  hb_font_t *font = hb_font_create (face);

  hb_blob_destroy (current_blob);
  hb_face_destroy (current_face);
  hb_font_destroy (current_font);

  current_blob = blob;
  current_face = face;
  current_font = font;

  demo_font_destroy (current_demo_font);
  demo_atlas_backend_t backend = {};
  backend.ctx = &atlas;
  backend.alloc = atlas_alloc;
  backend.get_used = atlas_get_used;
  backend.clear = atlas_clear;
  demo_atlas_t *new_atlas = demo_atlas_create_external (&backend);
  current_demo_font = demo_font_create (font, new_atlas);

  rebuild_buffer (custom_text ? current_text : default_text_en);
  demo_font_print_stats (current_demo_font);
}

EMSCRIPTEN_KEEPALIVE void
web_set_text (const char *utf8)
{
  custom_text = true;
  rebuild_buffer (utf8);
}

EMSCRIPTEN_KEEPALIVE const char *
web_get_text ()
{
  return current_text;
}

} /* extern "C" */


/* ---- Main loop ---- */

static void
main_loop_iter ()
{
  if (animate)
  {
    float dquat[4];
    float axis[3] = {rot_axis[0], rot_axis[1], rot_axis[2]};
    axis_to_quat (axis, (float) (rot_speed * 0.02), dquat);
    add_quats (dquat, quat, quat);
    needs_redraw = true;
  }

  if (needs_redraw && pipeline)
  {
    render_frame ();
    needs_redraw = false;
  }
}


/* ---- WebGPU initialization chain ---- */

static void init_demo ();
static void create_pipeline ();

static void
on_device (WGPURequestDeviceStatus status,
	   WGPUDevice dev, WGPUStringView msg, void *ud1, void *ud2)
{
  if (status != WGPURequestDeviceStatus_Success)
  {
    fprintf (stderr, "Failed to get WebGPU device\n");
    EM_ASM ({
      var el = document.getElementById ('loading');
      if (el) { el.textContent = 'Failed to create WebGPU device.'; el.style.color = '#666'; }
    });
    return;
  }
  device = dev;
  queue = wgpuDeviceGetQueue (device);

  init_demo ();
}

static void
on_adapter (WGPURequestAdapterStatus status,
	    WGPUAdapter adpt, WGPUStringView msg, void *ud1, void *ud2)
{
  if (status != WGPURequestAdapterStatus_Success)
  {
    fprintf (stderr, "Failed to get WebGPU adapter\n");
    EM_ASM ({
      var el = document.getElementById ('loading');
      if (el) {
        var html = 'WebGPU adapter not available.<br><br>See <a href="https://github.com/gpuweb/gpuweb/wiki/Implementation-Status" ' +
                 'style="color:#88f">browser support</a>.';
        if (navigator.userAgent.indexOf ('Linux') !== -1 &&
            navigator.userAgent.indexOf ('Chrome') !== -1)
          html += '<br><br>On Linux Chrome, try restarting with:<br>' +
                 '<code style="font-size:13px">google-chrome --enable-unsafe-webgpu --ozone-platform=x11 ' +
                 '--use-angle=vulkan --enable-features=Vulkan,VulkanFromANGLE</code>';
        el.innerHTML = html;
        el.style.fontSize = '16px';
        el.style.padding = '40px';
        el.style.color = '#666';
      }
    });
    return;
  }

  adapter = adpt;

  WGPUDeviceDescriptor devDesc = {};
  WGPURequestDeviceCallbackInfo cbInfo = {};
  cbInfo.mode = WGPUCallbackMode_AllowSpontaneous;
  cbInfo.callback = on_device;
  wgpuAdapterRequestDevice (adapter, &devDesc, cbInfo);
}


static void
create_pipeline ()
{
  /* Assemble full WGSL: library vertex + library fragment + demo shader */
  std::string wgsl;
  wgsl += hb_gpu_shader_vertex_source (HB_GPU_SHADER_LANG_WGSL);
  wgsl += "\n";
  wgsl += hb_gpu_shader_fragment_source (HB_GPU_SHADER_LANG_WGSL);
  wgsl += "\n";
  wgsl += wgsl_demo_shader;

  WGPUShaderSourceWGSL wgslSrc = {};
  wgslSrc.chain.sType = WGPUSType_ShaderSourceWGSL;
  wgslSrc.code = {wgsl.c_str (), wgsl.size ()};

  WGPUShaderModuleDescriptor smDesc = {};
  smDesc.nextInChain = &wgslSrc.chain;
  WGPUShaderModule module = wgpuDeviceCreateShaderModule (device, &smDesc);

  /* Vertex buffer layout matching glyph_vertex_t */
  WGPUVertexAttribute attrs[5] = {};
  /* position: float2 at offset 0 */
  attrs[0].format = WGPUVertexFormat_Float32x2;
  attrs[0].offset = offsetof (glyph_vertex_t, x);
  attrs[0].shaderLocation = 0;
  /* texcoord: float2 at offset 8 */
  attrs[1].format = WGPUVertexFormat_Float32x2;
  attrs[1].offset = offsetof (glyph_vertex_t, tx);
  attrs[1].shaderLocation = 1;
  /* normal: float2 at offset 16 */
  attrs[2].format = WGPUVertexFormat_Float32x2;
  attrs[2].offset = offsetof (glyph_vertex_t, nx);
  attrs[2].shaderLocation = 2;
  /* emPerPos: float at offset 24 */
  attrs[3].format = WGPUVertexFormat_Float32;
  attrs[3].offset = offsetof (glyph_vertex_t, emPerPos);
  attrs[3].shaderLocation = 3;
  /* atlas_offset: uint32 at offset 28 */
  attrs[4].format = WGPUVertexFormat_Uint32;
  attrs[4].offset = offsetof (glyph_vertex_t, atlas_offset);
  attrs[4].shaderLocation = 4;

  WGPUVertexBufferLayout vbLayout = {};
  vbLayout.arrayStride = sizeof (glyph_vertex_t);
  vbLayout.stepMode = WGPUVertexStepMode_Vertex;
  vbLayout.attributeCount = 5;
  vbLayout.attributes = attrs;

  /* Color target with alpha blending */
  WGPUBlendState blend = {};
  blend.color.srcFactor = WGPUBlendFactor_SrcAlpha;
  blend.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
  blend.color.operation = WGPUBlendOperation_Add;
  blend.alpha.srcFactor = WGPUBlendFactor_One;
  blend.alpha.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
  blend.alpha.operation = WGPUBlendOperation_Add;

  WGPUColorTargetState colorTarget = {};
  colorTarget.format = surface_format;
  colorTarget.blend = &blend;
  colorTarget.writeMask = WGPUColorWriteMask_All;

  WGPUFragmentState fragState = {};
  fragState.module = module;
  fragState.entryPoint = {"fs_main", WGPU_STRLEN};
  fragState.targetCount = 1;
  fragState.targets = &colorTarget;

  WGPURenderPipelineDescriptor pipeDesc = {};
  pipeDesc.vertex.module = module;
  pipeDesc.vertex.entryPoint = {"vs_main", WGPU_STRLEN};
  pipeDesc.vertex.bufferCount = 1;
  pipeDesc.vertex.buffers = &vbLayout;
  pipeDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
  pipeDesc.primitive.cullMode = WGPUCullMode_None;
  pipeDesc.multisample.count = 1;
  pipeDesc.multisample.mask = ~0u;
  pipeDesc.fragment = &fragState;

  pipeline = wgpuDeviceCreateRenderPipeline (device, &pipeDesc);
  wgpuShaderModuleRelease (module);
}


static void
create_bind_group ()
{
  WGPUBindGroupEntry entries[2] = {};
  entries[0].binding = 0;
  entries[0].buffer = uniform_buf;
  entries[0].size = sizeof (Uniforms);

  entries[1].binding = 1;
  entries[1].buffer = atlas.buf;
  entries[1].size = atlas.capacity * 4 * sizeof (int32_t);

  WGPUBindGroupDescriptor bgDesc = {};
  bgDesc.layout = wgpuRenderPipelineGetBindGroupLayout (pipeline, 0);
  bgDesc.entryCount = 2;
  bgDesc.entries = entries;

  if (bind_group)
    wgpuBindGroupRelease (bind_group);
  bind_group = wgpuDeviceCreateBindGroup (device, &bgDesc);
}


static void
init_demo ()
{
  /* Configure surface */
  WGPUSurfaceCapabilities caps = {};
  wgpuSurfaceGetCapabilities (surface, adapter, &caps);
  surface_format = caps.formats[0]; /* preferred format */

  WGPUSurfaceConfiguration config = {};
  config.device = device;
  config.format = surface_format;
  config.usage = WGPUTextureUsage_RenderAttachment;
  config.width = canvas_w;
  config.height = canvas_h;
  config.presentMode = WGPUPresentMode_Fifo;
  config.alphaMode = WGPUCompositeAlphaMode_Opaque;
  wgpuSurfaceConfigure (surface, &config);

  /* Create atlas storage buffer */
  unsigned atlas_capacity = 256 * 1024; /* 256K texels */
  atlas.capacity = atlas_capacity;
  atlas.cursor = 0;
  atlas.shadow = (int32_t *) calloc (atlas_capacity * 4, sizeof (int32_t));

  WGPUBufferDescriptor atlasDesc = {};
  atlasDesc.size = atlas_capacity * 4 * sizeof (int32_t);
  atlasDesc.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst;
  atlas.buf = wgpuDeviceCreateBuffer (device, &atlasDesc);

  /* Create uniform buffer */
  WGPUBufferDescriptor uniDesc = {};
  uniDesc.size = sizeof (Uniforms);
  uniDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
  uniform_buf = wgpuDeviceCreateBuffer (device, &uniDesc);

  /* Create pipeline */
  create_pipeline ();

  /* Set up atlas backend for demo-font */
  demo_atlas_backend_t backend = {};
  backend.ctx = &atlas;
  backend.alloc = atlas_alloc;
  backend.get_used = atlas_get_used;
  backend.clear = atlas_clear;
  demo_atlas_t *demo_atlas = demo_atlas_create_external (&backend);

  /* Create font and buffer */
  current_blob = hb_blob_create ((const char *) default_font,
				 sizeof (default_font),
				 HB_MEMORY_MODE_READONLY,
				 NULL, NULL);
  current_face = hb_face_create (current_blob, 0);
  current_font = hb_font_create (current_face);
  current_demo_font = demo_font_create (current_font, demo_atlas);

  current_text = strdup (default_text_combined);

  buffer = demo_buffer_create ();
  demo_point_t top_left = {0, 0};
  demo_buffer_move_to (buffer, &top_left);
  demo_buffer_add_text (buffer, current_text, current_demo_font, 1);

  demo_font_print_stats (current_demo_font);

  /* Upload atlas to GPU */
  atlas_upload ();

  /* Create bind group */
  create_bind_group ();

  /* Initialize trackball */
  trackball (quat, 0., 0., 0., 0.);

  /* Register input handlers */
  emscripten_set_mousedown_callback ("#canvas", NULL, EM_TRUE, on_mousedown);
  emscripten_set_mouseup_callback ("#canvas", NULL, EM_TRUE, on_mouseup);
  emscripten_set_mousemove_callback ("#canvas", NULL, EM_TRUE, on_mousemove);
  emscripten_set_wheel_callback ("#canvas", NULL, EM_TRUE, on_wheel);
  emscripten_set_keydown_callback (EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL, EM_TRUE, on_keydown);
  emscripten_set_touchstart_callback ("#canvas", NULL, EM_TRUE, on_touchstart);
  emscripten_set_touchmove_callback ("#canvas", NULL, EM_TRUE, on_touchmove);
  emscripten_set_touchend_callback ("#canvas", NULL, EM_TRUE, on_touchend);

  /* Start render loop */
  emscripten_set_main_loop (main_loop_iter, 0, 0);

  /* Hide loading screen */
  EM_ASM ({
    var el = document.getElementById ('loading');
    if (el) el.style.display = 'none';
    document.body.style.background = '#222';
  });

  printf ("WebGPU demo initialized: %u atlas texels\n", atlas.cursor);
}


int
main ()
{
  /* Get canvas size */
  emscripten_get_canvas_element_size ("#canvas", &canvas_w, &canvas_h);
  double css_w_d, css_h_d;
  emscripten_get_element_css_size ("#canvas", &css_w_d, &css_h_d);
  css_w = (int) css_w_d;
  css_h = (int) css_h_d;

  /* Create WebGPU surface from canvas */
  WGPUInstanceDescriptor instDesc = {};
  WGPUInstance instance = wgpuCreateInstance (&instDesc);

  WGPUEmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc = {};
  canvasDesc.chain.sType = WGPUSType_EmscriptenSurfaceSourceCanvasHTMLSelector;
  canvasDesc.selector = {"#canvas", WGPU_STRLEN};

  WGPUSurfaceDescriptor surfDesc = {};
  surfDesc.nextInChain = &canvasDesc.chain;
  surface = wgpuInstanceCreateSurface (instance, &surfDesc);

  /* Request adapter → device → init_demo */
  WGPURequestAdapterOptions opts = {};
  opts.compatibleSurface = surface;
  opts.powerPreference = WGPUPowerPreference_HighPerformance;

  WGPURequestAdapterCallbackInfo cbInfo = {};
  cbInfo.mode = WGPUCallbackMode_AllowSpontaneous;
  cbInfo.callback = on_adapter;
  wgpuInstanceRequestAdapter (instance, &opts, cbInfo);

  return 0;
}
