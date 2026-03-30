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
 * D3D11 demo.  Cross-compile with MinGW, run under Wine.
 */

#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>

#include <hb.h>
#include <hb-gpu.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <string>

#include "gl-stub.h"

#include "../demo-common.h"
#include "../demo-font.h"
#include "../demo-buffer.h"
#include "../demo-renderer.h"
#include "../demo-view.h"

#include "../default-text-combined.hh"
#include "../default-font.hh"


/* ---- HLSL demo shader ---- */

/* Atlas declaration must precede the library shader (which references it). */
static const char *hlsl_preamble =
  "StructuredBuffer<int4> hb_gpu_atlas : register(t0);\n";

static const char *hlsl_demo_shader = R"hlsl(
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
  float coverage = hb_gpu_render (input.texcoord, input.glyphLoc);

  if (stem_darkening > 0.0) {
    float2 fw = fwidth (input.texcoord);
    float ppem = 1.0 / max (fw.x, fw.y);
    float size_factor = smoothstep (8.0, 48.0, ppem);
    bool light_on_dark = dot (foreground.rgb, float3 (1, 1, 1)) > 1.5;
    float stem_exp = light_on_dark ? lerp (1.4, 1.0, size_factor)
                                   : lerp (0.7, 1.0, size_factor);
    coverage = pow (coverage, stem_exp);
  }

  if (gamma != 1.0)
    coverage = pow (coverage, gamma);

  return float4 (foreground.rgb, foreground.a * coverage);
}
)hlsl";


/* ---- Uniform buffer layout ---- */

struct Uniforms {
  float mvp[16];
  float viewport[2];
  float gamma;
  float stem_darkening;
  float foreground[4];
  float debug;
  float _pad[3];
};


/* ---- Atlas ---- */

struct atlas_t {
  int32_t *shadow;
  unsigned capacity;
  unsigned cursor;
  bool dirty;
  ID3D11Buffer *srv_buf;
  ID3D11ShaderResourceView *srv;
};

static atlas_t atlas;

static unsigned
atlas_alloc_cb (void *ctx, const char *data, unsigned len_bytes)
{
  atlas_t *at = (atlas_t *) ctx;
  unsigned len_texels = len_bytes / 8;
  if (at->cursor + len_texels > at->capacity) abort ();
  unsigned offset = at->cursor;
  at->cursor += len_texels;
  const int16_t *src = (const int16_t *) data;
  int32_t *dst = at->shadow + offset * 4;
  for (unsigned i = 0; i < len_texels * 4; i++)
    dst[i] = (int32_t) src[i];
  at->dirty = true;
  return offset;
}

static unsigned atlas_get_used_cb (void *ctx) { return ((atlas_t *) ctx)->cursor; }
static void atlas_clear_cb (void *ctx) { ((atlas_t *) ctx)->cursor = 0; ((atlas_t *) ctx)->dirty = true; }


/* ---- D3D11 state ---- */

static ID3D11Device *device;
static ID3D11DeviceContext *ctx;
static IDXGISwapChain *swapchain;
static ID3D11RenderTargetView *rtv;
static ID3D11VertexShader *vs;
static ID3D11PixelShader *ps;
static ID3D11InputLayout *layout;
static ID3D11Buffer *cbuf;
static ID3D11Buffer *vbuf;
static unsigned vbuf_capacity;
static ID3D11BlendState *blend;

static HWND hwnd;
static int win_w = 1024, win_h = 768;

/* ---- Demo state ---- */

static demo_view_t *vu;
static demo_buffer_t *buffer;
static demo_font_t *current_demo_font;
static hb_blob_t *current_blob;
static hb_face_t *current_face;
static hb_font_t *current_font;


/* ---- D3D11 renderer ---- */

struct demo_renderer_d3d11_t : demo_renderer_t
{
  float bg_r = 1, bg_g = 1, bg_b = 1, bg_a = 1;
  float fg_r = 0, fg_g = 0, fg_b = 0, fg_a = 1;
  float gamma_val = 1;
  float stem_val = 1;
  float debug_val = 0;
  demo_atlas_t *demo_atlas_obj = nullptr;

  ~demo_renderer_d3d11_t () override { if (demo_atlas_obj) demo_atlas_destroy (demo_atlas_obj); }
  demo_atlas_t *get_atlas () override { return demo_atlas_obj; }
  void setup () override { gamma_val = 1.f / 2.2f; stem_val = 1; }
  void set_gamma (float g) override { gamma_val = g; }
  void set_foreground (float r, float g, float b, float a) override { fg_r = r; fg_g = g; fg_b = b; fg_a = a; }
  void set_background (float r, float g, float b, float a) override { bg_r = r; bg_g = g; bg_b = b; bg_a = a; }
  void set_debug (bool e) override { debug_val = e ? 1.f : 0.f; }
  void set_stem_darkening (bool e) override { stem_val = e ? 1.f : 0.f; }
  bool set_srgb (bool) override { return false; }
  bool vsync_on = true;
  void toggle_vsync (bool &v) override { v = !v; vsync_on = v; }

  void display (glyph_vertex_t *vertices, unsigned count,
		int width, int height, float mat[16]) override
  {
    /* Resize if needed */
    if (width != win_w || height != win_h)
    {
      win_w = width; win_h = height;
      if (rtv) { rtv->Release (); rtv = nullptr; }
      swapchain->ResizeBuffers (0, width, height, DXGI_FORMAT_UNKNOWN, 0);
      ID3D11Texture2D *backbuf;
      swapchain->GetBuffer (0, __uuidof (ID3D11Texture2D), (void **) &backbuf);
      device->CreateRenderTargetView (backbuf, nullptr, &rtv);
      backbuf->Release ();
    }

    /* Update atlas */
    if (atlas.dirty && atlas.cursor > 0)
    {
      D3D11_MAPPED_SUBRESOURCE mapped;
      ctx->Map (atlas.srv_buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
      memcpy (mapped.pData, atlas.shadow, atlas.cursor * 4 * sizeof (int32_t));
      ctx->Unmap (atlas.srv_buf, 0);
      atlas.dirty = false;
    }

    /* Update vertices */
    if (count > 0)
    {
      unsigned needed = count * sizeof (glyph_vertex_t);
      if (needed > vbuf_capacity)
      {
	if (vbuf) vbuf->Release ();
	D3D11_BUFFER_DESC bd = {};
	bd.ByteWidth = needed;
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	device->CreateBuffer (&bd, nullptr, &vbuf);
	vbuf_capacity = needed;
      }
      D3D11_MAPPED_SUBRESOURCE mapped;
      ctx->Map (vbuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
      memcpy (mapped.pData, vertices, needed);
      ctx->Unmap (vbuf, 0);
    }

    /* Update uniforms */
    Uniforms u;
    memcpy (u.mvp, mat, sizeof (u.mvp));
    u.viewport[0] = (float) width;
    u.viewport[1] = (float) height;
    u.gamma = gamma_val;
    u.stem_darkening = stem_val;
    u.foreground[0] = fg_r; u.foreground[1] = fg_g;
    u.foreground[2] = fg_b; u.foreground[3] = fg_a;
    u.debug = debug_val;
    ctx->UpdateSubresource (cbuf, 0, nullptr, &u, 0, 0);

    /* Render */
    D3D11_VIEWPORT vp = {0, 0, (float) width, (float) height, 0, 1};
    ctx->RSSetViewports (1, &vp);
    ctx->OMSetRenderTargets (1, &rtv, nullptr);
    float clear[] = {bg_r, bg_g, bg_b, bg_a};
    ctx->ClearRenderTargetView (rtv, clear);

    if (count > 0)
    {
      ctx->VSSetShader (vs, nullptr, 0);
      ctx->PSSetShader (ps, nullptr, 0);
      ctx->VSSetConstantBuffers (0, 1, &cbuf);
      ctx->PSSetConstantBuffers (0, 1, &cbuf);
      ctx->PSSetShaderResources (0, 1, &atlas.srv);
      ctx->OMSetBlendState (blend, nullptr, 0xffffffff);
      UINT stride = sizeof (glyph_vertex_t), offset = 0;
      ctx->IASetVertexBuffers (0, 1, &vbuf, &stride, &offset);
      ctx->IASetPrimitiveTopology (D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      ctx->IASetInputLayout (layout);
      ctx->Draw (count, 0);
    }

    swapchain->Present (vsync_on ? 1 : 0, 0);
  }
};

static demo_renderer_d3d11_t *renderer;


/* ---- Shader compilation ---- */

static ID3DBlob *
compile_shader (const char *source, const char *entry, const char *target)
{
  ID3DBlob *blob = nullptr, *errors = nullptr;
  HRESULT hr = D3DCompile (source, strlen (source), nullptr, nullptr, nullptr,
			    entry, target, D3DCOMPILE_OPTIMIZATION_LEVEL3, 0,
			    &blob, &errors);
  if (FAILED (hr))
  {
    if (errors)
      fprintf (stderr, "Shader compile error: %s\n", (char *) errors->GetBufferPointer ());
    abort ();
  }
  if (errors) errors->Release ();
  return blob;
}


/* ---- Win32 input → demo_view ---- */

/* GLFW-compatible constants (from gl-stub.h) */
#define VK_TO_GLFW_KEY(vk) ( \
  (vk) == VK_SPACE   ? 32  : \
  (vk) == VK_ESCAPE  ? 256 : \
  (vk) == VK_BACK    ? 259 : \
  (vk) == VK_RIGHT   ? 262 : \
  (vk) == VK_LEFT    ? 263 : \
  (vk) == VK_DOWN    ? 264 : \
  (vk) == VK_UP      ? 265 : \
  ((vk) >= 'A' && (vk) <= 'Z') ? (vk) : 0)

static LRESULT CALLBACK
wnd_proc (HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
  switch (msg)
  {
  case WM_DESTROY:
    PostQuitMessage (0);
    return 0;

  case WM_SIZE:
    demo_view_reshape_func (vu, LOWORD (lp), HIWORD (lp));
    return 0;

  case WM_KEYDOWN:
    {
      if (wp == VK_ESCAPE || wp == 'Q')
      {
	DestroyWindow (h);
	return 0;
      }
      int key = VK_TO_GLFW_KEY ((int) wp);
      if (key)
	demo_view_key_func (vu, key, 0, 1 /* GLFW_PRESS */, 0);
    }
    return 0;

  case WM_CHAR:
    demo_view_char_func (vu, (unsigned) wp);
    return 0;

  case WM_LBUTTONDOWN:
    SetCapture (h);
    demo_view_motion_func (vu, (short) LOWORD (lp), (short) HIWORD (lp));
    demo_view_mouse_func (vu, 0 /* LEFT */, 1 /* PRESS */, 0);
    return 0;

  case WM_LBUTTONUP:
    ReleaseCapture ();
    demo_view_mouse_func (vu, 0, 0 /* RELEASE */, 0);
    return 0;

  case WM_RBUTTONDOWN:
    SetCapture (h);
    demo_view_motion_func (vu, (short) LOWORD (lp), (short) HIWORD (lp));
    demo_view_mouse_func (vu, 1 /* RIGHT */, 1, 0);
    return 0;

  case WM_RBUTTONUP:
    ReleaseCapture ();
    demo_view_mouse_func (vu, 1, 0, 0);
    return 0;

  case WM_MBUTTONDOWN:
    SetCapture (h);
    demo_view_motion_func (vu, (short) LOWORD (lp), (short) HIWORD (lp));
    demo_view_mouse_func (vu, 2 /* MIDDLE */, 1, 0);
    return 0;

  case WM_MBUTTONUP:
    ReleaseCapture ();
    demo_view_mouse_func (vu, 2, 0, 0);
    return 0;

  case WM_MOUSEMOVE:
    demo_view_motion_func (vu, (short) LOWORD (lp), (short) HIWORD (lp));
    return 0;

  case WM_MOUSEWHEEL:
    demo_view_scroll_func (vu, 0, GET_WHEEL_DELTA_WPARAM (wp) / 120.0);
    return 0;
  }
  return DefWindowProc (h, msg, wp, lp);
}


/* ---- current_time for demo_view ---- */

static LARGE_INTEGER perf_freq;

double _hb_gpu_d3d11_time ()
{
  LARGE_INTEGER t;
  QueryPerformanceCounter (&t);
  return (double) t.QuadPart / (double) perf_freq.QuadPart;
}


/* ---- Main ---- */

int WINAPI
WinMain (HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
  QueryPerformanceFrequency (&perf_freq);

  /* Create window */
  WNDCLASS wc = {};
  wc.lpfnWndProc = wnd_proc;
  wc.hInstance = hInst;
  wc.lpszClassName = L"HBGPUDemo";
  wc.hCursor = LoadCursor (nullptr, IDC_ARROW);
  RegisterClass (&wc);

  RECT rc = {0, 0, win_w, win_h};
  AdjustWindowRect (&rc, WS_OVERLAPPEDWINDOW, FALSE);
  hwnd = CreateWindow (L"HBGPUDemo", L"HarfBuzz D3D11 Demo",
		       WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		       CW_USEDEFAULT, CW_USEDEFAULT,
		       rc.right - rc.left, rc.bottom - rc.top,
		       nullptr, nullptr, hInst, nullptr);

  /* Create D3D11 device + swap chain */
  DXGI_SWAP_CHAIN_DESC scd = {};
  scd.BufferCount = 1;
  scd.BufferDesc.Width = win_w;
  scd.BufferDesc.Height = win_h;
  scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  scd.OutputWindow = hwnd;
  scd.SampleDesc.Count = 1;
  scd.Windowed = TRUE;

  D3D_FEATURE_LEVEL fl;
  HRESULT hr = D3D11CreateDeviceAndSwapChain (
    nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
    nullptr, 0, D3D11_SDK_VERSION,
    &scd, &swapchain, &device, &fl, &ctx);
  if (FAILED (hr)) {
    MessageBoxA (nullptr, "Failed to create D3D11 device", "Error", MB_OK);
    return 1;
  }

  /* Create render target view */
  ID3D11Texture2D *backbuf;
  swapchain->GetBuffer (0, __uuidof (ID3D11Texture2D), (void **) &backbuf);
  device->CreateRenderTargetView (backbuf, nullptr, &rtv);
  backbuf->Release ();

  /* Compile shaders */
  std::string full_shader;
  full_shader += hlsl_preamble;
  full_shader += hb_gpu_shader_vertex_source (HB_GPU_SHADER_LANG_HLSL);
  full_shader += "\n";
  full_shader += hb_gpu_shader_fragment_source (HB_GPU_SHADER_LANG_HLSL);
  full_shader += "\n";
  full_shader += hlsl_demo_shader;

  ID3DBlob *vs_blob = compile_shader (full_shader.c_str (), "vs_main", "vs_5_0");
  ID3DBlob *ps_blob = compile_shader (full_shader.c_str (), "ps_main", "ps_5_0");

  device->CreateVertexShader (vs_blob->GetBufferPointer (), vs_blob->GetBufferSize (), nullptr, &vs);
  device->CreatePixelShader (ps_blob->GetBufferPointer (), ps_blob->GetBufferSize (), nullptr, &ps);

  /* Input layout */
  D3D11_INPUT_ELEMENT_DESC elems[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof (glyph_vertex_t, x),  D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof (glyph_vertex_t, tx), D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"NORMAL",   0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof (glyph_vertex_t, nx), D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 1, DXGI_FORMAT_R32_FLOAT,       0, offsetof (glyph_vertex_t, emPerPos), D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 2, DXGI_FORMAT_R32_UINT,        0, offsetof (glyph_vertex_t, atlas_offset), D3D11_INPUT_PER_VERTEX_DATA, 0},
  };
  device->CreateInputLayout (elems, 5, vs_blob->GetBufferPointer (), vs_blob->GetBufferSize (), &layout);
  vs_blob->Release ();
  ps_blob->Release ();

  /* Constant buffer */
  {
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = (sizeof (Uniforms) + 15) & ~15;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    device->CreateBuffer (&bd, nullptr, &cbuf);
  }

  /* Atlas structured buffer */
  unsigned atlas_cap = 256 * 1024;
  atlas.capacity = atlas_cap;
  atlas.cursor = 0;
  atlas.dirty = true;
  atlas.shadow = (int32_t *) calloc (atlas_cap * 4, sizeof (int32_t));
  {
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = atlas_cap * 4 * sizeof (int32_t);
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bd.StructureByteStride = 16; /* int4 = 16 bytes */
    device->CreateBuffer (&bd, nullptr, &atlas.srv_buf);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
    srvd.Format = DXGI_FORMAT_UNKNOWN;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvd.Buffer.NumElements = atlas_cap;
    device->CreateShaderResourceView (atlas.srv_buf, &srvd, &atlas.srv);
  }

  /* Blend state (alpha blending) */
  {
    D3D11_BLEND_DESC bd = {};
    bd.RenderTarget[0].BlendEnable = TRUE;
    bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    device->CreateBlendState (&bd, &blend);
  }

  /* Rasterizer state (no backface culling) */
  {
    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    ID3D11RasterizerState *rs;
    device->CreateRasterizerState (&rd, &rs);
    ctx->RSSetState (rs);
    rs->Release ();
  }

  /* Set up renderer */
  renderer = new demo_renderer_d3d11_t ();
  demo_atlas_backend_t backend = {};
  backend.ctx = &atlas;
  backend.alloc = atlas_alloc_cb;
  backend.get_used = atlas_get_used_cb;
  backend.clear = atlas_clear_cb;
  renderer->demo_atlas_obj = demo_atlas_create_external (&backend);

  vu = demo_view_create_headless (renderer, win_w, win_h, win_w, win_h);

  /* Create font and buffer */
  current_blob = hb_blob_create ((const char *) default_font,
				 sizeof (default_font),
				 HB_MEMORY_MODE_READONLY, nullptr, nullptr);
  current_face = hb_face_create (current_blob, 0);
  current_font = hb_font_create (current_face);
  current_demo_font = demo_font_create (current_font, renderer->get_atlas ());

  buffer = demo_buffer_create ();
  demo_point_t top_left = {0, 0};
  demo_buffer_move_to (buffer, &top_left);
  demo_buffer_add_text (buffer, default_text_combined, current_demo_font, 1);
  demo_font_print_stats (current_demo_font);

  demo_view_setup (vu);

  /* Message loop */
  MSG msg;
  while (true)
  {
    while (PeekMessage (&msg, nullptr, 0, 0, PM_REMOVE))
    {
      if (msg.message == WM_QUIT) goto done;
      TranslateMessage (&msg);
      DispatchMessage (&msg);
    }

    if (demo_view_should_redraw (vu))
      demo_view_display (vu, buffer);
    else
      Sleep (1);
  }

done:
  return 0;
}
