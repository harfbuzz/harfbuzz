/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 */

#ifdef __APPLE__

#include "demo-renderer.h"
#include "demo-atlas.h"
#include <hb-gpu.h>

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

#define TEXEL_SIZE 8  /* sizeof short4 texel */

static const char *demo_metal_shader_source = R"msl(

struct Uniforms {
  float4x4 matViewProjection;
  float2 viewport;
  float gamma;
  float _pad1;
  float4 foreground;
};

struct VertexIn {
  float2 position [[attribute(0)]];
  float2 texcoord [[attribute(1)]];
  float2 normal   [[attribute(2)]];
  float  emPerPos [[attribute(3)]];
  uint   glyphLoc [[attribute(4)]];
};

struct VertexOut {
  float4 position [[position]];
  float2 texcoord;
  uint   glyphLoc [[flat]];
};

vertex VertexOut vertex_main(VertexIn in [[stage_in]],
                             constant Uniforms& uniforms [[buffer(1)]]) {
  float2 pos = in.position;
  float2 tex = in.texcoord;
  float4 jac = float4(in.emPerPos, 0.0, 0.0, -in.emPerPos);

  hb_gpu_dilate(pos, tex, in.normal, jac,
                uniforms.matViewProjection, uniforms.viewport);

  VertexOut out;
  out.position = uniforms.matViewProjection * float4(pos, 0.0, 1.0);
  out.texcoord = tex;
  out.glyphLoc = in.glyphLoc;
  return out;
}

fragment float4 fragment_main(VertexOut in [[stage_in]],
                              constant Uniforms& uniforms [[buffer(1)]],
                              device const short4* atlas [[buffer(0)]]) {
  float coverage = hb_gpu_render(in.texcoord, in.glyphLoc, atlas);

  if (uniforms.gamma != 1.0)
    coverage = pow(coverage, uniforms.gamma);

  return float4(uniforms.foreground.xyz, uniforms.foreground.w * coverage);
}

)msl";


struct demo_renderer_metal_t : demo_renderer_t
{
  GLFWwindow *window;

  /* Metal objects */
  id<MTLDevice> device;
  id<MTLCommandQueue> commandQueue;
  id<MTLRenderPipelineState> pipelineState;
  CAMetalLayer *metalLayer;

  /* Atlas (CPU shadow + GPU buffer) */
  demo_atlas_t *atlas;
  char *atlas_data;
  unsigned int atlas_capacity;  /* in texels */
  unsigned int atlas_cursor;    /* in texels */
  id<MTLBuffer> atlasBuffer;
  bool atlas_dirty;

  /* Clear color */
  float bg[4];

  /* Uniforms */
  struct {
    float matViewProjection[16];
    float viewport[2];
    float gamma;
    float _pad1;
    float foreground[4];
  } uniforms;


  demo_renderer_metal_t (GLFWwindow *window_) : window (window_)
  {
    bg[0] = bg[1] = bg[2] = bg[3] = 1.0f;
    memset (&uniforms, 0, sizeof (uniforms));
    uniforms.gamma = 1.0f;
    uniforms.foreground[3] = 1.0f;
  }

  ~demo_renderer_metal_t () override
  {
    demo_atlas_destroy (atlas);
    free (atlas_data);
  }


  /* -- Atlas -- */

  static unsigned int
  atlas_alloc_cb (void *ctx, const char *data, unsigned int len_bytes)
  {
    auto *self = (demo_renderer_metal_t *) ctx;
    unsigned int len_texels = len_bytes / TEXEL_SIZE;
    if (self->atlas_cursor + len_texels > self->atlas_capacity)
      die ("Ran out of atlas memory");
    unsigned int offset = self->atlas_cursor;
    self->atlas_cursor += len_texels;
    memcpy (self->atlas_data + offset * TEXEL_SIZE, data, len_bytes);
    self->atlas_dirty = true;
    return offset;
  }

  static unsigned int
  atlas_get_used_cb (void *ctx)
  {
    return ((demo_renderer_metal_t *) ctx)->atlas_cursor;
  }

  static void
  atlas_clear_cb (void *ctx)
  {
    auto *self = (demo_renderer_metal_t *) ctx;
    self->atlas_cursor = 0;
    self->atlas_dirty = true;
  }

  demo_atlas_t *get_atlas () override
  {
    return atlas;
  }


  /* -- State -- */

  void setup () override {}

  void set_gamma (float gamma) override
  {
    uniforms.gamma = gamma;
  }

  void set_foreground (float r, float g, float b, float a) override
  {
    uniforms.foreground[0] = r;
    uniforms.foreground[1] = g;
    uniforms.foreground[2] = b;
    uniforms.foreground[3] = a;
  }

  void set_background (float r, float g, float b, float a) override
  {
    bg[0] = r; bg[1] = g; bg[2] = b; bg[3] = a;
  }

  bool set_srgb (bool enabled [[maybe_unused]]) override
  {
    /* Metal uses MTLPixelFormatBGRA8Unorm_sRGB; sRGB is always on. */
    return true;
  }

  void toggle_vsync (bool &vsync) override
  {
    vsync = !vsync;
    metalLayer.displaySyncEnabled = vsync;
  }


  /* -- Frame -- */

  void display (glyph_vertex_t *vertices, unsigned int count,
		int width, int height, float mat[16]) override
  {
    @autoreleasepool {

    /* Update layer size */
    CGSize size = { (CGFloat) width, (CGFloat) height };
    metalLayer.drawableSize = size;

    /* Update uniforms */
    memcpy (uniforms.matViewProjection, mat, 16 * sizeof (float));
    uniforms.viewport[0] = (float) width;
    uniforms.viewport[1] = (float) height;

    /* Sync atlas if dirty */
    if (atlas_dirty)
    {
      memcpy (atlasBuffer.contents, atlas_data,
	      atlas_cursor * TEXEL_SIZE);
      atlas_dirty = false;
    }

    /* Get drawable */
    id<CAMetalDrawable> drawable = [metalLayer nextDrawable];
    if (!drawable)
      return;

    /* Render pass */
    MTLRenderPassDescriptor *passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
    passDesc.colorAttachments[0].texture = drawable.texture;
    passDesc.colorAttachments[0].loadAction = MTLLoadActionClear;
    passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;
    passDesc.colorAttachments[0].clearColor = MTLClearColorMake (bg[0], bg[1], bg[2], bg[3]);

    id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
    id<MTLRenderCommandEncoder> encoder =
      [commandBuffer renderCommandEncoderWithDescriptor:passDesc];

    [encoder setViewport:(MTLViewport){0, 0, (double) width, (double) height, 0, 1}];
    [encoder setRenderPipelineState:pipelineState];

    if (count > 0)
    {
      id<MTLBuffer> vertexBuffer =
	[device newBufferWithBytes:vertices
			    length:count * sizeof (glyph_vertex_t)
			   options:MTLResourceStorageModeShared];

      [encoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];
      [encoder setVertexBytes:&uniforms length:sizeof (uniforms) atIndex:1];
      [encoder setFragmentBuffer:atlasBuffer offset:0 atIndex:0];
      [encoder setFragmentBytes:&uniforms length:sizeof (uniforms) atIndex:1];

      [encoder drawPrimitives:MTLPrimitiveTypeTriangle
		  vertexStart:0
		  vertexCount:count];
    }

    [encoder endEncoding];
    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];

    } /* @autoreleasepool */
  }
};


demo_renderer_t *
demo_renderer_create_metal (GLFWwindow *window)
{
  @autoreleasepool {

  auto *r = new demo_renderer_metal_t (window);

  /* Get Metal device */
  r->device = MTLCreateSystemDefaultDevice ();
  if (!r->device)
  {
    delete r;
    return nullptr;
  }

  r->commandQueue = [r->device newCommandQueue];

  /* Set up CAMetalLayer on the GLFW window */
  NSWindow *nswindow = glfwGetCocoaWindow (window);
  r->metalLayer = [CAMetalLayer layer];
  r->metalLayer.device = r->device;
  r->metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
  nswindow.contentView.layer = r->metalLayer;
  nswindow.contentView.wantsLayer = YES;

  /* Compile shaders */
  const char *vert_src = hb_gpu_shader_vertex_source (HB_GPU_SHADER_LANG_MSL);
  const char *frag_src = hb_gpu_shader_fragment_source (HB_GPU_SHADER_LANG_MSL);

  NSString *preamble = @"#include <metal_stdlib>\nusing namespace metal;\n";
  NSString *source = [NSString stringWithFormat:@"%@%s%s%s",
		      preamble, vert_src, frag_src, demo_metal_shader_source];

  NSError *error = nil;
  id<MTLLibrary> library = [r->device newLibraryWithSource:source
						   options:nil
						     error:&error];
  if (!library)
  {
    fprintf (stderr, "Metal shader compilation failed: %s\n",
	     [[error localizedDescription] UTF8String]);
    delete r;
    return nullptr;
  }

  id<MTLFunction> vertexFunc = [library newFunctionWithName:@"vertex_main"];
  id<MTLFunction> fragmentFunc = [library newFunctionWithName:@"fragment_main"];

  /* Create vertex descriptor matching glyph_vertex_t layout */
  MTLVertexDescriptor *vertexDesc = [MTLVertexDescriptor vertexDescriptor];
  vertexDesc.attributes[0].format = MTLVertexFormatFloat2;
  vertexDesc.attributes[0].offset = 0;
  vertexDesc.attributes[0].bufferIndex = 0;
  vertexDesc.attributes[1].format = MTLVertexFormatFloat2;
  vertexDesc.attributes[1].offset = 8;
  vertexDesc.attributes[1].bufferIndex = 0;
  vertexDesc.attributes[2].format = MTLVertexFormatFloat2;
  vertexDesc.attributes[2].offset = 16;
  vertexDesc.attributes[2].bufferIndex = 0;
  vertexDesc.attributes[3].format = MTLVertexFormatFloat;
  vertexDesc.attributes[3].offset = 24;
  vertexDesc.attributes[3].bufferIndex = 0;
  vertexDesc.attributes[4].format = MTLVertexFormatUInt;
  vertexDesc.attributes[4].offset = 28;
  vertexDesc.attributes[4].bufferIndex = 0;
  vertexDesc.layouts[0].stride = 32;

  /* Create pipeline */
  MTLRenderPipelineDescriptor *pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
  pipelineDesc.vertexFunction = vertexFunc;
  pipelineDesc.fragmentFunction = fragmentFunc;
  pipelineDesc.vertexDescriptor = vertexDesc;
  pipelineDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
  pipelineDesc.colorAttachments[0].blendingEnabled = YES;
  pipelineDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
  pipelineDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
  pipelineDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
  pipelineDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

  error = nil;
  r->pipelineState = [r->device newRenderPipelineStateWithDescriptor:pipelineDesc
							       error:&error];
  if (!r->pipelineState)
  {
    fprintf (stderr, "Metal pipeline creation failed: %s\n",
	     [[error localizedDescription] UTF8String]);
    delete r;
    return nullptr;
  }

  /* Atlas */
  unsigned int atlas_capacity = 1024 * 1024;
  r->atlas_capacity = atlas_capacity;
  r->atlas_cursor = 0;
  r->atlas_data = (char *) calloc (atlas_capacity, TEXEL_SIZE);
  r->atlasBuffer = [r->device newBufferWithLength:atlas_capacity * TEXEL_SIZE
					  options:MTLResourceStorageModeShared];
  r->atlas_dirty = false;

  /* Create atlas wrapper for demo_font_t */
  demo_atlas_backend_t backend = {
    r,
    demo_renderer_metal_t::atlas_alloc_cb,
    demo_renderer_metal_t::atlas_get_used_cb,
    demo_renderer_metal_t::atlas_clear_cb,
  };
  r->atlas = demo_atlas_create_external (&backend);

  return r;

  } /* @autoreleasepool */
}

#endif /* __APPLE__ */
