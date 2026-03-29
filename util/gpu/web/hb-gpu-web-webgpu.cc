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
 * Uses Emscripten WebGPU bindings instead of WebGL.
 * Uses demo_view for input/view (with HB_GPU_NO_GLFW).
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

/* GL type stubs for headers */
#include "gl-stub.h"

#include "../demo-common.h"
#include "../demo-font.h"
#include "../demo-buffer.h"
#include "../demo-renderer.h"
#include "../demo-view.h"

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
  int32_t *shadow;
  unsigned capacity;
  unsigned cursor;
  bool dirty;
};

static atlas_t atlas;

static unsigned
atlas_alloc_cb (void *ctx, const char *data, unsigned len_bytes)
{
  atlas_t *at = (atlas_t *) ctx;
  unsigned len_texels = len_bytes / 8;

  if (at->cursor + len_texels > at->capacity)
  {
    fprintf (stderr, "Ran out of atlas memory\n");
    abort ();
  }

  unsigned offset = at->cursor;
  at->cursor += len_texels;

  const int16_t *src = (const int16_t *) data;
  int32_t *dst = at->shadow + offset * 4;
  for (unsigned i = 0; i < len_texels * 4; i++)
    dst[i] = (int32_t) src[i];

  at->dirty = true;
  return offset;
}

static unsigned
atlas_get_used_cb (void *ctx) { return ((atlas_t *) ctx)->cursor; }

static void
atlas_clear_cb (void *ctx) { ((atlas_t *) ctx)->cursor = 0; ((atlas_t *) ctx)->dirty = true; }


/* ---- WebGPU state ---- */

static WGPUAdapter g_adapter;
static WGPUDevice g_device;
static WGPUQueue g_queue;
static WGPUSurface g_surface;
static WGPURenderPipeline g_pipeline;
static WGPUBuffer g_uniform_buf;
static WGPUBuffer g_vertex_buf;
static unsigned g_vertex_buf_capacity;
static WGPUBindGroup g_bind_group;
static WGPUTextureFormat g_surface_format;

static int canvas_w, canvas_h;
static int css_w, css_h;


/* ---- WebGPU renderer (implements demo_renderer_t) ---- */

struct demo_renderer_webgpu_t : demo_renderer_t
{
  float bg_r, bg_g, bg_b, bg_a;
  float fg_r, fg_g, fg_b, fg_a;
  float gamma;
  demo_atlas_t *demo_atlas;

  demo_renderer_webgpu_t ()
    : bg_r (1), bg_g (1), bg_b (1), bg_a (1),
      fg_r (0), fg_g (0), fg_b (0), fg_a (1),
      gamma (1), demo_atlas (nullptr) {}

  ~demo_renderer_webgpu_t () override
  {
    if (demo_atlas)
      demo_atlas_destroy (demo_atlas);
  }

  demo_atlas_t *get_atlas () override { return demo_atlas; }

  void setup () override
  {
    /* Gamma defaults to 2.2 correction */
    gamma = 1.f / 2.2f;
  }

  void set_gamma (float g) override { gamma = g; }
  void set_foreground (float r, float g, float b, float a) override
  { fg_r = r; fg_g = g; fg_b = b; fg_a = a; }
  void set_background (float r, float g, float b, float a) override
  { bg_r = r; bg_g = g; bg_b = b; bg_a = a; }

  bool set_srgb (bool enabled) override { return false; /* no sRGB framebuffer */ }

  void toggle_vsync (bool &vsync) override { /* always vsync in browser */ }

  void display (glyph_vertex_t *vertices, unsigned int count,
		int width, int height, float mat[16]) override
  {
    WGPUSurfaceTexture surfTex;
    wgpuSurfaceGetCurrentTexture (g_surface, &surfTex);
    if (surfTex.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal &&
	surfTex.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal)
      return;

    WGPUTextureView view = wgpuTextureCreateView (surfTex.texture, NULL);

    /* Update uniforms */
    Uniforms u;
    memcpy (u.mvp, mat, sizeof (u.mvp));
    u.viewport[0] = (float) width;
    u.viewport[1] = (float) height;
    u.gamma = gamma;
    u._pad = 0;
    u.foreground[0] = fg_r; u.foreground[1] = fg_g;
    u.foreground[2] = fg_b; u.foreground[3] = fg_a;
    wgpuQueueWriteBuffer (g_queue, g_uniform_buf, 0, &u, sizeof (u));

    /* Upload atlas if dirty */
    if (atlas.dirty && atlas.cursor > 0)
    {
      wgpuQueueWriteBuffer (g_queue, atlas.buf, 0,
			    atlas.shadow, atlas.cursor * 4 * sizeof (int32_t));
      atlas.dirty = false;
    }

    /* Upload vertices */
    if (count > 0)
    {
      unsigned needed = count * sizeof (glyph_vertex_t);
      if (needed > g_vertex_buf_capacity)
      {
	if (g_vertex_buf)
	  wgpuBufferRelease (g_vertex_buf);
	WGPUBufferDescriptor desc = {};
	desc.size = needed;
	desc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
	g_vertex_buf = wgpuDeviceCreateBuffer (g_device, &desc);
	g_vertex_buf_capacity = needed;
      }
      wgpuQueueWriteBuffer (g_queue, g_vertex_buf, 0, vertices, needed);
    }

    /* Render pass */
    WGPURenderPassColorAttachment colorAtt = {};
    colorAtt.view = view;
    colorAtt.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    colorAtt.loadOp = WGPULoadOp_Clear;
    colorAtt.storeOp = WGPUStoreOp_Store;
    colorAtt.clearValue = {bg_r, bg_g, bg_b, bg_a};

    WGPURenderPassDescriptor rpDesc = {};
    rpDesc.colorAttachmentCount = 1;
    rpDesc.colorAttachments = &colorAtt;

    WGPUCommandEncoderDescriptor ceDesc = {};
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder (g_device, &ceDesc);
    WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass (encoder, &rpDesc);

    if (count > 0)
    {
      wgpuRenderPassEncoderSetPipeline (pass, g_pipeline);
      wgpuRenderPassEncoderSetBindGroup (pass, 0, g_bind_group, 0, NULL);
      wgpuRenderPassEncoderSetVertexBuffer (pass, 0, g_vertex_buf, 0,
					    count * sizeof (glyph_vertex_t));
      wgpuRenderPassEncoderDraw (pass, count, 1, 0, 0);
    }

    wgpuRenderPassEncoderEnd (pass);
    wgpuRenderPassEncoderRelease (pass);

    WGPUCommandBufferDescriptor cbDesc = {};
    WGPUCommandBuffer cmdBuf = wgpuCommandEncoderFinish (encoder, &cbDesc);
    wgpuCommandEncoderRelease (encoder);
    wgpuQueueSubmit (g_queue, 1, &cmdBuf);
    wgpuCommandBufferRelease (cmdBuf);

    wgpuTextureViewRelease (view);
    wgpuTextureRelease (surfTex.texture);
  }
};

static demo_renderer_webgpu_t *renderer;


/* ---- Demo state ---- */

static demo_view_t *vu;
static demo_buffer_t *buffer;
static demo_font_t *current_demo_font;
static hb_blob_t *current_blob;
static hb_face_t *current_face;
static hb_font_t *current_font;
static char *current_text;
static bool custom_text;


/* ---- Buffer rebuild ---- */

static void
rebuild_buffer (const char *text)
{
  free (current_text);
  current_text = strdup (text);

  demo_font_clear_cache (current_demo_font);
  atlas_clear_cb (&atlas);

  demo_buffer_clear (buffer);
  demo_point_t top_left = {0, 0};
  demo_buffer_move_to (buffer, &top_left);
  demo_buffer_add_text (buffer, text, current_demo_font, 1);

  atlas.dirty = true;
  demo_view_reset (vu);
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
  current_demo_font = demo_font_create (font, renderer->get_atlas ());

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


/* ---- Input handling (feeds into demo_view) ---- */

/* GLFW-compatible button/action constants */
#define BUTTON_LEFT 0
#define BUTTON_RIGHT 1
#define ACTION_RELEASE 0
#define ACTION_PRESS 1

static int active_buttons;

static EM_BOOL
on_mousedown (int type, const EmscriptenMouseEvent *e, void *ud)
{
  int button = (e->button == 2) ? BUTTON_RIGHT : BUTTON_LEFT;
  active_buttons |= (1 << button);
  demo_view_motion_func (vu, e->targetX, e->targetY);
  demo_view_mouse_func (vu, button, ACTION_PRESS, 0);
  return EM_TRUE;
}

static EM_BOOL
on_mouseup (int type, const EmscriptenMouseEvent *e, void *ud)
{
  int button = (e->button == 2) ? BUTTON_RIGHT : BUTTON_LEFT;
  if (!(active_buttons & (1 << button)))
    return EM_FALSE; /* Not our drag — ignore (e.g. UI button click) */
  active_buttons &= ~(1 << button);
  /* Don't call motion_func here — it would zero dx/dy needed for
   * drag-release velocity detection.  Cursor is already up to date
   * from the last mousemove event. */
  demo_view_mouse_func (vu, button, ACTION_RELEASE, 0);
  return EM_TRUE;
}

static EM_BOOL
on_mousemove (int type, const EmscriptenMouseEvent *e, void *ud)
{
  demo_view_motion_func (vu, e->targetX, e->targetY);
  return EM_TRUE;
}

static EM_BOOL
on_wheel (int type, const EmscriptenWheelEvent *e, void *ud)
{
  double delta = -e->deltaY;
  if (e->deltaMode == DOM_DELTA_LINE)
    delta *= 40.0;
  demo_view_scroll_func (vu, 0, delta / 120.0);
  return EM_TRUE;
}

/* Key mapping from DOM key strings to GLFW key codes */
static int
dom_key_to_glfw (const char *key)
{
  if (!strcmp (key, " "))         return 32;  /* GLFW_KEY_SPACE */
  if (!strcmp (key, "Escape"))    return 256; /* GLFW_KEY_ESCAPE */
  if (!strcmp (key, "ArrowUp"))   return 265; /* GLFW_KEY_UP */
  if (!strcmp (key, "ArrowDown")) return 264; /* GLFW_KEY_DOWN */
  if (!strcmp (key, "ArrowLeft")) return 263; /* GLFW_KEY_LEFT */
  if (!strcmp (key, "ArrowRight"))return 262; /* GLFW_KEY_RIGHT */
  if (key[0] >= 'a' && key[0] <= 'z' && key[1] == 0)
    return key[0] - 'a' + 65; /* GLFW_KEY_A..Z */
  return 0;
}

static EM_BOOL
on_keydown (int type, const EmscriptenKeyboardEvent *e, void *ud)
{
  /* Let char_func handle printable characters */
  int key = dom_key_to_glfw (e->key);
  if (key)
    demo_view_key_func (vu, key, 0, ACTION_PRESS, e->shiftKey ? 1 : 0);

  /* Also send as char for =/-/etc */
  if (e->key[0] && !e->key[1])
    demo_view_char_func (vu, (unsigned int) e->key[0]);

  return EM_FALSE;
}

/* Touch handling */
static double pinch_dist;

static EM_BOOL
on_touchstart (int type, const EmscriptenTouchEvent *e, void *ud)
{
  if (e->numTouches == 1)
  {
    demo_view_motion_func (vu, e->touches[0].targetX, e->touches[0].targetY);
    demo_view_mouse_func (vu, BUTTON_LEFT, ACTION_PRESS, 0);
  }
  else if (e->numTouches == 2)
  {
    demo_view_mouse_func (vu, BUTTON_LEFT, ACTION_RELEASE, 0);
    double dx = e->touches[1].targetX - e->touches[0].targetX;
    double dy = e->touches[1].targetY - e->touches[0].targetY;
    pinch_dist = sqrt (dx * dx + dy * dy);
  }
  return EM_TRUE;
}

static EM_BOOL
on_touchmove (int type, const EmscriptenTouchEvent *e, void *ud)
{
  if (e->numTouches == 1)
  {
    demo_view_motion_func (vu, e->touches[0].targetX, e->touches[0].targetY);
  }
  else if (e->numTouches == 2)
  {
    double dx = e->touches[1].targetX - e->touches[0].targetX;
    double dy = e->touches[1].targetY - e->touches[0].targetY;
    double dist = sqrt (dx * dx + dy * dy);
    if (pinch_dist > 0)
      demo_view_scroll_func (vu, 0, (dist - pinch_dist) * 0.05);
    pinch_dist = dist;
  }
  return EM_TRUE;
}

static EM_BOOL
on_touchend (int type, const EmscriptenTouchEvent *e, void *ud)
{
  demo_view_mouse_func (vu, BUTTON_LEFT, ACTION_RELEASE, 0);
  return EM_TRUE;
}


/* ---- Main loop ---- */

static void
main_loop_iter ()
{
  if (demo_view_should_redraw (vu))
    demo_view_display (vu, buffer);
}


/* ---- WebGPU initialization ---- */

static void init_demo ();
static void create_pipeline ();
static void create_bind_group ();

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
  g_device = dev;
  g_queue = wgpuDeviceGetQueue (g_device);
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
        var html = 'WebGPU adapter not available.<br>' +
                 '<a href="https://github.com/gpuweb/gpuweb/wiki/Implementation-Status" ' +
                 'style="color:#88f">Browser support status</a>';
        if (navigator.userAgent.indexOf ('Linux') !== -1 &&
            navigator.userAgent.indexOf ('Chrome') !== -1)
          html += '<br><br>On Linux Chrome, try restarting with:<br>' +
                 '<code style="font-size:13px">google-chrome --enable-unsafe-webgpu --ozone-platform=x11 ' +
                 '--use-angle=vulkan --enable-features=Vulkan,VulkanFromANGLE</code>';
        el.innerHTML = html;
        el.style.font = '18px sans-serif';
        el.style.color = '#666';
        el.style.textAlign = 'center';
        el.style.flexDirection = 'column';
      }
    });
    return;
  }

  g_adapter = adpt;

  WGPUDeviceDescriptor devDesc = {};
  WGPURequestDeviceCallbackInfo cbInfo = {};
  cbInfo.mode = WGPUCallbackMode_AllowSpontaneous;
  cbInfo.callback = on_device;
  wgpuAdapterRequestDevice (g_adapter, &devDesc, cbInfo);
}

static void
create_pipeline ()
{
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
  WGPUShaderModule module = wgpuDeviceCreateShaderModule (g_device, &smDesc);

  WGPUVertexAttribute attrs[5] = {};
  attrs[0].format = WGPUVertexFormat_Float32x2;
  attrs[0].offset = offsetof (glyph_vertex_t, x);
  attrs[0].shaderLocation = 0;
  attrs[1].format = WGPUVertexFormat_Float32x2;
  attrs[1].offset = offsetof (glyph_vertex_t, tx);
  attrs[1].shaderLocation = 1;
  attrs[2].format = WGPUVertexFormat_Float32x2;
  attrs[2].offset = offsetof (glyph_vertex_t, nx);
  attrs[2].shaderLocation = 2;
  attrs[3].format = WGPUVertexFormat_Float32;
  attrs[3].offset = offsetof (glyph_vertex_t, emPerPos);
  attrs[3].shaderLocation = 3;
  attrs[4].format = WGPUVertexFormat_Uint32;
  attrs[4].offset = offsetof (glyph_vertex_t, atlas_offset);
  attrs[4].shaderLocation = 4;

  WGPUVertexBufferLayout vbLayout = {};
  vbLayout.arrayStride = sizeof (glyph_vertex_t);
  vbLayout.stepMode = WGPUVertexStepMode_Vertex;
  vbLayout.attributeCount = 5;
  vbLayout.attributes = attrs;

  WGPUBlendState blend = {};
  blend.color.srcFactor = WGPUBlendFactor_SrcAlpha;
  blend.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
  blend.color.operation = WGPUBlendOperation_Add;
  blend.alpha.srcFactor = WGPUBlendFactor_One;
  blend.alpha.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
  blend.alpha.operation = WGPUBlendOperation_Add;

  WGPUColorTargetState colorTarget = {};
  colorTarget.format = g_surface_format;
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

  g_pipeline = wgpuDeviceCreateRenderPipeline (g_device, &pipeDesc);
  wgpuShaderModuleRelease (module);
}

static void
create_bind_group ()
{
  WGPUBindGroupEntry entries[2] = {};
  entries[0].binding = 0;
  entries[0].buffer = g_uniform_buf;
  entries[0].size = sizeof (Uniforms);
  entries[1].binding = 1;
  entries[1].buffer = atlas.buf;
  entries[1].size = atlas.capacity * 4 * sizeof (int32_t);

  WGPUBindGroupDescriptor bgDesc = {};
  bgDesc.layout = wgpuRenderPipelineGetBindGroupLayout (g_pipeline, 0);
  bgDesc.entryCount = 2;
  bgDesc.entries = entries;

  if (g_bind_group)
    wgpuBindGroupRelease (g_bind_group);
  g_bind_group = wgpuDeviceCreateBindGroup (g_device, &bgDesc);
}

static void
init_demo ()
{
  /* Configure surface */
  WGPUSurfaceCapabilities caps = {};
  wgpuSurfaceGetCapabilities (g_surface, g_adapter, &caps);
  g_surface_format = caps.formats[0];

  WGPUSurfaceConfiguration config = {};
  config.device = g_device;
  config.format = g_surface_format;
  config.usage = WGPUTextureUsage_RenderAttachment;
  config.width = canvas_w;
  config.height = canvas_h;
  config.presentMode = WGPUPresentMode_Fifo;
  config.alphaMode = WGPUCompositeAlphaMode_Opaque;
  wgpuSurfaceConfigure (g_surface, &config);

  /* Create atlas storage buffer */
  unsigned atlas_capacity = 256 * 1024;
  atlas.capacity = atlas_capacity;
  atlas.cursor = 0;
  atlas.dirty = true;
  atlas.shadow = (int32_t *) calloc (atlas_capacity * 4, sizeof (int32_t));

  WGPUBufferDescriptor atlasDesc = {};
  atlasDesc.size = atlas_capacity * 4 * sizeof (int32_t);
  atlasDesc.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst;
  atlas.buf = wgpuDeviceCreateBuffer (g_device, &atlasDesc);

  /* Create uniform buffer */
  WGPUBufferDescriptor uniDesc = {};
  uniDesc.size = sizeof (Uniforms);
  uniDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
  g_uniform_buf = wgpuDeviceCreateBuffer (g_device, &uniDesc);

  /* Create pipeline and bind group */
  create_pipeline ();

  /* Set up renderer with atlas backend */
  renderer = new demo_renderer_webgpu_t ();

  demo_atlas_backend_t backend = {};
  backend.ctx = &atlas;
  backend.alloc = atlas_alloc_cb;
  backend.get_used = atlas_get_used_cb;
  backend.clear = atlas_clear_cb;
  renderer->demo_atlas = demo_atlas_create_external (&backend);

  /* Create view (headless — no GLFW) */
  vu = demo_view_create_headless (renderer, canvas_w, canvas_h, css_w, css_h);

  /* Create font and buffer */
  current_blob = hb_blob_create ((const char *) default_font,
				 sizeof (default_font),
				 HB_MEMORY_MODE_READONLY,
				 NULL, NULL);
  current_face = hb_face_create (current_blob, 0);
  current_font = hb_font_create (current_face);
  current_demo_font = demo_font_create (current_font, renderer->get_atlas ());

  current_text = strdup (default_text_combined);

  buffer = demo_buffer_create ();
  demo_point_t top_left = {0, 0};
  demo_buffer_move_to (buffer, &top_left);
  demo_buffer_add_text (buffer, current_text, current_demo_font, 1);

  demo_font_print_stats (current_demo_font);

  /* Create bind group (after atlas populated) */
  create_bind_group ();

  /* Setup renderer (sets initial gamma/colors) */
  demo_view_setup (vu);

  /* Register input handlers */
  emscripten_set_mousedown_callback ("#canvas", NULL, EM_TRUE, on_mousedown);
  emscripten_set_mouseup_callback ("#canvas", NULL, EM_TRUE, on_mouseup);
  /* Catch mouseup outside canvas for drag-release; filtered by active_buttons */
  emscripten_set_mouseup_callback (EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL, EM_TRUE, on_mouseup);
  emscripten_set_mousemove_callback ("#canvas", NULL, EM_TRUE, on_mousemove);
  emscripten_set_wheel_callback ("#canvas", NULL, EM_TRUE, on_wheel);
  emscripten_set_keydown_callback (EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL, EM_TRUE, on_keydown);
  emscripten_set_touchstart_callback ("#canvas", NULL, EM_TRUE, on_touchstart);
  emscripten_set_touchmove_callback ("#canvas", NULL, EM_TRUE, on_touchmove);
  emscripten_set_touchend_callback ("#canvas", NULL, EM_TRUE, on_touchend);

  /* Hide loading screen */
  EM_ASM ({
    var el = document.getElementById ('loading');
    if (el && !window._webgpuError) {
      el.style.display = 'none';
      document.body.style.background = '#222';
    }
  });

  printf ("WebGPU demo initialized\n");

  /* Start render loop */
  emscripten_set_main_loop (main_loop_iter, 0, 0);
}


int
main ()
{
  emscripten_get_canvas_element_size ("#canvas", &canvas_w, &canvas_h);
  double css_w_d, css_h_d;
  emscripten_get_element_css_size ("#canvas", &css_w_d, &css_h_d);
  css_w = (int) css_w_d;
  css_h = (int) css_h_d;

  WGPUInstanceDescriptor instDesc = {};
  WGPUInstance instance = wgpuCreateInstance (&instDesc);

  WGPUEmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc = {};
  canvasDesc.chain.sType = WGPUSType_EmscriptenSurfaceSourceCanvasHTMLSelector;
  canvasDesc.selector = {"#canvas", WGPU_STRLEN};

  WGPUSurfaceDescriptor surfDesc = {};
  surfDesc.nextInChain = &canvasDesc.chain;
  g_surface = wgpuInstanceCreateSurface (instance, &surfDesc);

  WGPURequestAdapterOptions opts = {};
  opts.compatibleSurface = g_surface;
  opts.powerPreference = WGPUPowerPreference_HighPerformance;

  WGPURequestAdapterCallbackInfo cbInfo = {};
  cbInfo.mode = WGPUCallbackMode_AllowSpontaneous;
  cbInfo.callback = on_adapter;
  wgpuInstanceRequestAdapter (instance, &opts, cbInfo);

  return 0;
}
