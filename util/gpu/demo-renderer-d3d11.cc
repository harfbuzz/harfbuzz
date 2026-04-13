/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 */

#ifdef _WIN32

#include "demo-renderer.h"
#include "demo-atlas.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <string>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>


struct demo_renderer_d3d11_t : demo_renderer_t
{
  GLFWwindow *window;
  HWND hwnd;

  ID3D11Device *device;
  ID3D11DeviceContext *ctx;
  IDXGISwapChain *swapchain;
  ID3D11RenderTargetView *rtv;
  ID3D11VertexShader *vs;
  ID3D11PixelShader *ps;
  ID3D11InputLayout *layout;
  ID3D11Buffer *cbuf;
  ID3D11Buffer *vbuf;
  unsigned vbuf_capacity;
  ID3D11BlendState *blend_state;
  ID3D11RasterizerState *raster_state;

  /* Atlas */
  struct {
    demo_atlas_t *obj;
    ID3D11Buffer *srv_buf;
    ID3D11ShaderResourceView *srv;
    int32_t *shadow;
    unsigned capacity;
    unsigned cursor;
    bool dirty;
  } atlas;

  /* State */
  float bg_r, bg_g, bg_b, bg_a;
  float fg_r, fg_g, fg_b, fg_a;
  float gamma_val;
  float stem_val;
  float debug_val;
  bool vsync_on;
  int cur_width, cur_height;

  /* Uniform layout (must match HLSL cbuffer) */
  struct Uniforms {
    float mvp[16];
    float viewport[2];
    float gamma;
    float stem_darkening;
    float foreground[4];
    float debug;
    float _pad[3];
  };


  demo_renderer_d3d11_t (GLFWwindow *window_) : window (window_)
  {
    hwnd = glfwGetWin32Window (window);

    /* Create device + swap chain */
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;

    D3D11CreateDeviceAndSwapChain (
      nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
      nullptr, 0, D3D11_SDK_VERSION,
      &scd, &swapchain, &device, nullptr, &ctx);

    /* Compile shaders */
    static const char *hlsl_demo = R"hlsl(
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
  float coverage = hb_gpu_draw (input.texcoord, input.glyphLoc);
  if (stem_darkening > 0.0) {
    float2 fw = fwidth (input.texcoord);
    float ppem = 1.0 / max (fw.x, fw.y);
    float sf = smoothstep (8.0, 48.0, ppem);
    bool light = dot (foreground.rgb, float3 (1,1,1)) > 1.5;
    float se = light ? lerp (1.4, 1.0, sf) : lerp (0.7, 1.0, sf);
    coverage = pow (coverage, se);
  }
  if (gamma != 1.0)
    coverage = pow (coverage, gamma);
  return float4 (foreground.rgb, foreground.a * coverage);
}
)hlsl";

    std::string full;
    full += "StructuredBuffer<int4> hb_gpu_atlas : register(t0);\n";
    full += hb_gpu_draw_shader_source (HB_GPU_SHADER_STAGE_VERTEX, HB_GPU_SHADER_LANG_HLSL);
    full += "\n";
    full += hb_gpu_draw_shader_source (HB_GPU_SHADER_STAGE_FRAGMENT, HB_GPU_SHADER_LANG_HLSL);
    full += "\n";
    full += hlsl_demo;

    auto compile = [&](const char *entry, const char *target) -> ID3DBlob* {
      ID3DBlob *blob = nullptr, *errors = nullptr;
      D3DCompile (full.c_str (), full.size (), nullptr, nullptr, nullptr,
		  entry, target, D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &blob, &errors);
      if (errors) { fprintf (stderr, "%s\n", (char *) errors->GetBufferPointer ()); errors->Release (); }
      return blob;
    };

    ID3DBlob *vs_blob = compile ("vs_main", "vs_5_0");
    ID3DBlob *ps_blob = compile ("ps_main", "ps_5_0");

    device->CreateVertexShader (vs_blob->GetBufferPointer (), vs_blob->GetBufferSize (), nullptr, &vs);
    device->CreatePixelShader (ps_blob->GetBufferPointer (), ps_blob->GetBufferSize (), nullptr, &ps);

    /* Input layout */
    D3D11_INPUT_ELEMENT_DESC elems[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof (glyph_vertex_t, x),  D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof (glyph_vertex_t, tx), D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"NORMAL",   0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof (glyph_vertex_t, nx), D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 1, DXGI_FORMAT_R32_FLOAT,    0, offsetof (glyph_vertex_t, emPerPos), D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 2, DXGI_FORMAT_R32_UINT,     0, offsetof (glyph_vertex_t, atlas_offset), D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    device->CreateInputLayout (elems, 5, vs_blob->GetBufferPointer (), vs_blob->GetBufferSize (), &layout);
    vs_blob->Release ();
    ps_blob->Release ();

    /* Constant buffer */
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = (sizeof (Uniforms) + 15) & ~15;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    device->CreateBuffer (&bd, nullptr, &cbuf);

    /* Atlas */
    atlas.capacity = 256 * 1024;
    atlas.cursor = 0;
    atlas.dirty = true;
    atlas.shadow = (int32_t *) calloc (atlas.capacity * 4, sizeof (int32_t));
    {
      D3D11_BUFFER_DESC abd = {};
      abd.ByteWidth = atlas.capacity * 4 * sizeof (int32_t);
      abd.Usage = D3D11_USAGE_DYNAMIC;
      abd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
      abd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      abd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
      abd.StructureByteStride = 16;
      device->CreateBuffer (&abd, nullptr, &atlas.srv_buf);

      D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
      srvd.Format = DXGI_FORMAT_UNKNOWN;
      srvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
      srvd.Buffer.NumElements = atlas.capacity;
      device->CreateShaderResourceView (atlas.srv_buf, &srvd, &atlas.srv);
    }

    /* Atlas backend callbacks */
    demo_atlas_backend_t backend = {};
    backend.ctx = this;
    backend.alloc = [](void *ctx_, const char *data, unsigned len_bytes) -> unsigned {
      auto *self = (demo_renderer_d3d11_t *) ctx_;
      unsigned len_texels = len_bytes / 8;
      unsigned offset = self->atlas.cursor;
      self->atlas.cursor += len_texels;
      const int16_t *src = (const int16_t *) data;
      int32_t *dst = self->atlas.shadow + offset * 4;
      for (unsigned i = 0; i < len_texels * 4; i++)
	dst[i] = (int32_t) src[i];
      self->atlas.dirty = true;
      return offset;
    };
    backend.get_used = [](void *ctx_) -> unsigned {
      return ((demo_renderer_d3d11_t *) ctx_)->atlas.cursor;
    };
    backend.clear = [](void *ctx_) {
      auto *self = (demo_renderer_d3d11_t *) ctx_;
      self->atlas.cursor = 0;
      self->atlas.dirty = true;
    };
    atlas.obj = demo_atlas_create_external (&backend);

    /* Blend state */
    D3D11_BLEND_DESC bld = {};
    bld.RenderTarget[0].BlendEnable = TRUE;
    bld.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    bld.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    bld.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bld.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bld.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    bld.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bld.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    device->CreateBlendState (&bld, &blend_state);

    /* Rasterizer state (no culling) */
    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    device->CreateRasterizerState (&rd, &raster_state);
    ctx->RSSetState (raster_state);

    /* Initial state */
    bg_r = bg_g = bg_b = 1; bg_a = 1;
    fg_r = fg_g = fg_b = 0; fg_a = 1;
    gamma_val = 1;
    stem_val = 1;
    debug_val = 0;
    vsync_on = true;
    vbuf = nullptr;
    vbuf_capacity = 0;
    rtv = nullptr;
    cur_width = cur_height = 0;
  }

  ~demo_renderer_d3d11_t () override
  {
    if (atlas.obj) demo_atlas_destroy (atlas.obj);
    free (atlas.shadow);
  }

  demo_atlas_t *get_atlas () override { return atlas.obj; }
  void setup () override { gamma_val = 1.f / 2.2f; stem_val = 1; }
  void set_gamma (float g) override { gamma_val = g; }
  void set_foreground (float r, float g, float b, float a) override { fg_r = r; fg_g = g; fg_b = b; fg_a = a; }
  void set_background (float r, float g, float b, float a) override { bg_r = r; bg_g = g; bg_b = b; bg_a = a; }
  void set_debug (bool e) override { debug_val = e ? 1.f : 0.f; }
  void set_stem_darkening (bool e) override { stem_val = e ? 1.f : 0.f; }
  bool set_srgb (bool) override { return false; }
  void toggle_vsync (bool &v) override { v = !v; vsync_on = v; }

  void display (glyph_vertex_t *vertices, unsigned count,
		unsigned generation,
		int width, int height, float mat[16]) override
  {
    if (width != cur_width || height != cur_height)
    {
      cur_width = width; cur_height = height;
      if (rtv) { rtv->Release (); rtv = nullptr; }
      swapchain->ResizeBuffers (0, width, height, DXGI_FORMAT_UNKNOWN, 0);
      ID3D11Texture2D *backbuf;
      swapchain->GetBuffer (0, __uuidof (ID3D11Texture2D), (void **) &backbuf);
      device->CreateRenderTargetView (backbuf, nullptr, &rtv);
      backbuf->Release ();
    }

    /* Upload atlas */
    if (atlas.dirty && atlas.cursor > 0)
    {
      D3D11_MAPPED_SUBRESOURCE mapped;
      ctx->Map (atlas.srv_buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
      memcpy (mapped.pData, atlas.shadow, atlas.cursor * 4 * sizeof (int32_t));
      ctx->Unmap (atlas.srv_buf, 0);
      atlas.dirty = false;
    }

    /* Upload vertices */
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

    /* Uniforms */
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
      ctx->OMSetBlendState (blend_state, nullptr, 0xffffffff);
      UINT stride = sizeof (glyph_vertex_t), offset = 0;
      ctx->IASetVertexBuffers (0, 1, &vbuf, &stride, &offset);
      ctx->IASetPrimitiveTopology (D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      ctx->IASetInputLayout (layout);
      ctx->Draw (count, 0);
    }

    swapchain->Present (vsync_on ? 1 : 0, 0);
  }
};


demo_renderer_t *
demo_renderer_create_d3d11 (GLFWwindow *window)
{
  return new demo_renderer_d3d11_t (window);
}

#endif /* _WIN32 */
