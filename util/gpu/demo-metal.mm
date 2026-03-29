/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 */

#ifdef __APPLE__

#include "demo-metal.h"
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


struct demo_metal_t {
  GLFWwindow *window;

  /* Metal objects */
  id<MTLDevice> device;
  id<MTLCommandQueue> commandQueue;
  id<MTLRenderPipelineState> pipelineState;
  CAMetalLayer *metalLayer;

  /* Atlas (CPU + GPU buffer) */
  char *atlas_data;
  unsigned int atlas_capacity;  /* in texels */
  unsigned int atlas_cursor;    /* in texels */
  id<MTLBuffer> atlasBuffer;
  bool atlas_dirty;

  /* Uniforms */
  struct {
    float matViewProjection[16];
    float viewport[2];
    float gamma;
    float _pad1;
    float foreground[4];
  } uniforms;
};


demo_metal_t *
demo_metal_create (GLFWwindow *window, unsigned int atlas_capacity)
{
  @autoreleasepool {

  demo_metal_t *mt = (demo_metal_t *) calloc (1, sizeof (demo_metal_t));
  mt->window = window;

  /* Get Metal device */
  mt->device = MTLCreateSystemDefaultDevice ();
  if (!mt->device)
  {
    free (mt);
    return NULL;
  }

  mt->commandQueue = [mt->device newCommandQueue];

  /* Set up CAMetalLayer on the GLFW window */
  NSWindow *nswindow = glfwGetCocoaWindow (window);
  mt->metalLayer = [CAMetalLayer layer];
  mt->metalLayer.device = mt->device;
  mt->metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
  nswindow.contentView.layer = mt->metalLayer;
  nswindow.contentView.wantsLayer = YES;

  /* Compile shaders */
  const char *vert_src = hb_gpu_shader_vertex_source (HB_GPU_SHADER_LANG_MSL);
  const char *frag_src = hb_gpu_shader_fragment_source (HB_GPU_SHADER_LANG_MSL);

  NSString *preamble = @"#include <metal_stdlib>\nusing namespace metal;\n";
  NSString *source = [NSString stringWithFormat:@"%@%s%s%s",
		      preamble, vert_src, frag_src, demo_metal_shader_source];

  NSError *error = nil;
  id<MTLLibrary> library = [mt->device newLibraryWithSource:source
						    options:nil
						      error:&error];
  if (!library)
  {
    fprintf (stderr, "Metal shader compilation failed: %s\n",
	     [[error localizedDescription] UTF8String]);
    free (mt);
    return NULL;
  }

  id<MTLFunction> vertexFunc = [library newFunctionWithName:@"vertex_main"];
  id<MTLFunction> fragmentFunc = [library newFunctionWithName:@"fragment_main"];

  /* Create vertex descriptor matching glyph_vertex_t layout */
  MTLVertexDescriptor *vertexDesc = [MTLVertexDescriptor vertexDescriptor];
  /* position: float2 at offset 0 */
  vertexDesc.attributes[0].format = MTLVertexFormatFloat2;
  vertexDesc.attributes[0].offset = 0;
  vertexDesc.attributes[0].bufferIndex = 0;
  /* texcoord: float2 at offset 8 */
  vertexDesc.attributes[1].format = MTLVertexFormatFloat2;
  vertexDesc.attributes[1].offset = 8;
  vertexDesc.attributes[1].bufferIndex = 0;
  /* normal: float2 at offset 16 */
  vertexDesc.attributes[2].format = MTLVertexFormatFloat2;
  vertexDesc.attributes[2].offset = 16;
  vertexDesc.attributes[2].bufferIndex = 0;
  /* emPerPos: float at offset 24 */
  vertexDesc.attributes[3].format = MTLVertexFormatFloat;
  vertexDesc.attributes[3].offset = 24;
  vertexDesc.attributes[3].bufferIndex = 0;
  /* glyphLoc: uint at offset 28 */
  vertexDesc.attributes[4].format = MTLVertexFormatUInt;
  vertexDesc.attributes[4].offset = 28;
  vertexDesc.attributes[4].bufferIndex = 0;
  /* stride = sizeof(glyph_vertex_t) = 32 */
  vertexDesc.layouts[0].stride = 32;

  /* Create pipeline */
  MTLRenderPipelineDescriptor *pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
  pipelineDesc.vertexFunction = vertexFunc;
  pipelineDesc.fragmentFunction = fragmentFunc;
  pipelineDesc.vertexDescriptor = vertexDesc;
  pipelineDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
  /* Alpha blending: SRC_ALPHA, ONE_MINUS_SRC_ALPHA */
  pipelineDesc.colorAttachments[0].blendingEnabled = YES;
  pipelineDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
  pipelineDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
  pipelineDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
  pipelineDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

  error = nil;
  mt->pipelineState = [mt->device newRenderPipelineStateWithDescriptor:pipelineDesc
							         error:&error];
  if (!mt->pipelineState)
  {
    fprintf (stderr, "Metal pipeline creation failed: %s\n",
	     [[error localizedDescription] UTF8String]);
    free (mt);
    return NULL;
  }

  /* Atlas */
  mt->atlas_capacity = atlas_capacity;
  mt->atlas_cursor = 0;
  mt->atlas_data = (char *) calloc (atlas_capacity, TEXEL_SIZE);
  mt->atlasBuffer = [mt->device newBufferWithLength:atlas_capacity * TEXEL_SIZE
					    options:MTLResourceStorageModeShared];
  mt->atlas_dirty = false;

  /* Default uniforms */
  mt->uniforms.gamma = 1.0f;
  mt->uniforms.foreground[0] = 0.0f;
  mt->uniforms.foreground[1] = 0.0f;
  mt->uniforms.foreground[2] = 0.0f;
  mt->uniforms.foreground[3] = 1.0f;

  return mt;

  } /* @autoreleasepool */
}

void
demo_metal_destroy (demo_metal_t *mt)
{
  if (!mt)
    return;

  free (mt->atlas_data);
  free (mt);
}


unsigned int
demo_metal_atlas_alloc (demo_metal_t *mt,
			const char   *data,
			unsigned int  len_bytes)
{
  unsigned int len_texels = len_bytes / TEXEL_SIZE;

  if (mt->atlas_cursor + len_texels > mt->atlas_capacity)
    die ("Ran out of atlas memory");

  unsigned int offset = mt->atlas_cursor;
  mt->atlas_cursor += len_texels;

  memcpy (mt->atlas_data + offset * TEXEL_SIZE, data, len_bytes);
  mt->atlas_dirty = true;

  return offset;
}

unsigned int
demo_metal_atlas_get_used (demo_metal_t *mt)
{
  return mt->atlas_cursor;
}

void
demo_metal_atlas_clear (demo_metal_t *mt)
{
  mt->atlas_cursor = 0;
  mt->atlas_dirty = true;
}


void
demo_metal_set_gamma (demo_metal_t *mt, float gamma)
{
  mt->uniforms.gamma = gamma;
}

void
demo_metal_set_foreground (demo_metal_t *mt,
			   float r, float g, float b, float a)
{
  mt->uniforms.foreground[0] = r;
  mt->uniforms.foreground[1] = g;
  mt->uniforms.foreground[2] = b;
  mt->uniforms.foreground[3] = a;
}


void
demo_metal_display (demo_metal_t    *mt,
		    glyph_vertex_t  *vertices,
		    unsigned int     vertex_count,
		    int              width,
		    int              height,
		    float             mat[16])
{
  @autoreleasepool {

  /* Update layer size */
  CGSize size = { (CGFloat) width, (CGFloat) height };
  mt->metalLayer.drawableSize = size;

  /* Update uniforms */
  memcpy (mt->uniforms.matViewProjection, mat, 16 * sizeof (float));
  mt->uniforms.viewport[0] = (float) width;
  mt->uniforms.viewport[1] = (float) height;

  /* Sync atlas if dirty */
  if (mt->atlas_dirty)
  {
    memcpy (mt->atlasBuffer.contents, mt->atlas_data,
	    mt->atlas_cursor * TEXEL_SIZE);
    mt->atlas_dirty = false;
  }

  /* Get drawable */
  id<CAMetalDrawable> drawable = [mt->metalLayer nextDrawable];
  if (!drawable)
    return;

  /* Render pass */
  MTLRenderPassDescriptor *passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
  passDesc.colorAttachments[0].texture = drawable.texture;
  passDesc.colorAttachments[0].loadAction = MTLLoadActionClear;
  passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;
  passDesc.colorAttachments[0].clearColor = MTLClearColorMake (1.0, 1.0, 1.0, 1.0);

  id<MTLCommandBuffer> commandBuffer = [mt->commandQueue commandBuffer];
  id<MTLRenderCommandEncoder> encoder =
    [commandBuffer renderCommandEncoderWithDescriptor:passDesc];

  [encoder setViewport:(MTLViewport){0, 0, (double) width, (double) height, 0, 1}];
  [encoder setRenderPipelineState:mt->pipelineState];

  if (vertex_count > 0)
  {
    id<MTLBuffer> vertexBuffer =
      [mt->device newBufferWithBytes:vertices
			      length:vertex_count * sizeof (glyph_vertex_t)
			     options:MTLResourceStorageModeShared];

    [encoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];
    [encoder setVertexBytes:&mt->uniforms length:sizeof (mt->uniforms) atIndex:1];
    [encoder setFragmentBuffer:mt->atlasBuffer offset:0 atIndex:0];
    [encoder setFragmentBytes:&mt->uniforms length:sizeof (mt->uniforms) atIndex:1];

    [encoder drawPrimitives:MTLPrimitiveTypeTriangle
		vertexStart:0
		vertexCount:vertex_count];
  }

  [encoder endEncoding];
  [commandBuffer presentDrawable:drawable];
  [commandBuffer commit];

  } /* @autoreleasepool */
}

#endif /* __APPLE__ */
