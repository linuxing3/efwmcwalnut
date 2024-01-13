#pragma once
#include "efwmc_wgpu_backend.h"
#include "ResourceManager.h"
#include "imgui.h" // IMGUI_IMPL_API
#include "webgpu.h"
#include "webgpu.hpp"
#include <climits>
#include <cstddef>

#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/polar_coordinates.hpp>

// These differences of implementation should vanish as soon as WebGPU gets in
// version 1.0 stable
#ifdef WEBGPU_BACKEND_WGPU
#include <wgpu.h>
#define wgpuBindGroupLayoutRelease wgpuBindGroupLayoutDrop
#define wgpuBindGroupRelease wgpuBindGroupDrop
#define wgpuRenderPipelineRelease wgpuRenderPipelineDrop
#define wgpuSamplerRelease wgpuSamplerDrop
#define wgpuShaderModuleRelease wgpuShaderModuleDrop
#define wgpuTextureViewRelease wgpuTextureViewDrop
#define wgpuTextureRelease wgpuTextureDrop
#define wgpuBufferRelease wgpuBufferDrop
#define wgpuQueueRelease(...)
#endif // WEBGPU_BACKEND_WGPU

using VertexAttributes = ResourceManager::VertexAttributes;
// Dear ImGui prototypes from imgui_internal.h
extern ImGuiID ImHashData(const void *data_p, size_t data_size, ImU32 seed = 0);

static void
ImGui_ImplGPU_RecreateCommonBindGroup(WGPUBindGroupEntry common_bg_entries[]);

// WebGPU data
static WGPUDevice g_wgpuDevice = nullptr;
static WGPUQueue g_defaultQueue = nullptr;
static WGPUTextureFormat g_renderTargetFormat = WGPUTextureFormat_Undefined;
static WGPUTextureFormat g_depthStencilFormat = WGPUTextureFormat_Undefined;
static WGPURenderPipeline g_pipelineState = nullptr;
static WGPURenderPipelineDescriptor graphics_pipeline_desc = {};

struct RenderResources {
  WGPUTexture FontTexture;         // Font texture
  WGPUTextureView FontTextureView; // Texture view for font texture
  WGPUSampler Sampler;             // Sampler for the font texture
  // NOTE: default is mvp+gamma, changed to m, v, p, time
  WGPUBuffer MyUniforms; // Shader uniforms my uniforms
  // NOTE: lighting uniforms
  WGPUBuffer LightingUniforms; // Shader uniforms lighting
  // NOTE: bg_layouts[0] = MyUniform + sampler + texture + LightingUniforms,
  // without font texture bind group
  WGPUBindGroup CommonBindGroup; // Resources bind-group to bind the common
                                 // resources to pipeline
  ImGuiStorage
      ImageBindGroups; // Resources bind-group to bind the font/image resources
                       // to pipeline (this is a key->value map)
  // NOTE: bg_layouts[1] = fonts texture
  WGPUBindGroup FontImageBindGroup; // Default font-resource of Dear ImGui
  WGPUBindGroupLayout
      FontImageBindGroupLayout; // Cache layout used for the image bind group.
                                // Avoids allocating unnecessary JS objects when
                                // working with WebASM
};
static RenderResources g_resources;

struct FrameResources {
  WGPUBuffer IndexBuffer;
  WGPUBuffer VertexBuffer;
  ImDrawIdx *IndexBufferHost;
  VertexAttributes *VertexBufferHost;
  int IndexBufferSize;
  int VertexBufferSize;
};
static FrameResources *g_pFrameResources = nullptr;
static unsigned int g_numFramesInFlight = 0;
static unsigned int g_frameIndex = UINT_MAX;

constexpr float PI = 3.14159265358979323846f;

using namespace wgpu;
using glm::mat4x4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

// @group(0) @binding(0) var<uniform> uMyUniforms : MyUniforms;
struct MyUniforms {
  mat4x4 projectionMatrix;
  mat4x4 viewMatrix;
  mat4x4 modelMatrix;
  vec4 color;
  vec3 cameraWorldPosition;
  float time;
};
static_assert(sizeof(MyUniforms) % 16 == 0);

// @group(0) @binding(3) var<uniform> uLighting : LightingUniforms;
struct LightingUniforms {
  std::array<vec4, 2> directions;
  std::array<vec4, 2> colors;
  float hardness;
  float kd;
  float ks;
  float _pad;
};
static_assert(sizeof(LightingUniforms) % 16 == 0);
//-----------------------------------------------------------------------------
// SHADERS
//-----------------------------------------------------------------------------

static const char __shader_vert_wgsl[] = R"(
struct VertexInput {
	@location(0) position: vec3<f32>,
	@location(1) normal: vec3<f32>,
	@location(2) color: vec3<f32>,
	@location(3) uv: vec2<f32>,
}

struct VertexOutput {
	@builtin(position) position: vec4<f32>,
	@location(0) color: vec3<f32>,
	@location(1) normal: vec3<f32>,
	@location(2) uv: vec2<f32>,
	@location(3) viewDirection: vec3<f32>,
}

struct MyUniforms {
	projectionMatrix: mat4x4<f32>,
	viewMatrix: mat4x4<f32>,
	modelMatrix: mat4x4<f32>,
	color: vec4<f32>,
	cameraWorldPosition: vec3<f32>,
	time: f32,
}

struct LightingUniforms {
	directions: array<vec4<f32>, 2>,
	colors: array<vec4<f32>, 2>,
	hardness: f32,
	kd: f32,
	ks: f32,
}

// Instead of the simple uTime variable, our uniform variable is a struct
@group(0) @binding(0) var<uniform> uMyUniforms: MyUniforms;
@group(0) @binding(3) var<uniform> uLighting: LightingUniforms;

@vertex
fn main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    let worldPosition = uMyUniforms.modelMatrix * vec4<f32>(in.position, 1.0);
    out.position = uMyUniforms.projectionMatrix * uMyUniforms.viewMatrix * worldPosition;
    out.color = in.color;
    out.normal = (uMyUniforms.modelMatrix * vec4<f32>(in.normal, 0.0)).xyz;
    out.uv = in.uv;
    out.viewDirection = uMyUniforms.cameraWorldPosition - worldPosition.xyz;
    return out;
}
)";

static const char __shader_frag_wgsl[] = R"(
struct VertexOutput {
	@builtin(position) position: vec4<f32>,
	@location(0) color: vec3<f32>,
	@location(1) normal: vec3<f32>,
	@location(2) uv: vec2<f32>,
	@location(3) viewDirection: vec3<f32>,
}

@group(0) @binding(1) var textureSampler: sampler;
@group(0) @binding(2) var baseColorTexture: texture_2d<f32>;

@fragment
fn main(in: VertexOutput) -> @location(0) vec4<f32> {
	// Compute shading
    let N = normalize(in.normal);
    let V = normalize(in.viewDirection);
    var diffuse = vec3<f32>(0.0);
    var specular = vec3<f32>(0.0);
    for (var i: i32 = 0; i < 2; i++) {
        let L = normalize(uLighting.directions[i].xyz);
        let R = -reflect(L, N); // equivalent to 2.0 * dot(N, L) * N - L
        let color = uLighting.colors[i].rgb;
        let RoV = max(0.0, dot(R, V));
        let LoN = max(0.0, dot(L, N));

        diffuse += LoN * color;

		// You may or may not multiply by the light color here, the Phong model
		// even allows to have a different color for diffuse and specular
		// contributions.
        specular += pow(RoV, uLighting.hardness);
    }
	
	// Sample texture
    let baseColor = textureSample(baseColorTexture, textureSampler, in.uv).rgb;

	// Combine texture and lighting
    let color = baseColor * uLighting.kd * diffuse + uLighting.ks * specular;

	// Gamma-correction
    let corrected_color = pow(color, vec3<f32>(2.2));
    return vec4<f32>(corrected_color, uMyUniforms.color.a);
}
)";

static void SafeRelease(ImDrawIdx *&res) {
  if (res)
    delete[] res;
  res = nullptr;
}
static void SafeRelease(ImDrawVert *&res) {
  if (res)
    delete[] res;
  res = nullptr;
}
static void SafeRelease(VertexAttributes *&res) {
  if (res)
    delete[] res;
  res = nullptr;
}
static void SafeRelease(WGPUBindGroupLayout &res) {
  if (res)
    wgpuBindGroupLayoutRelease(res);
  res = nullptr;
}
static void SafeRelease(WGPUBindGroup &res) {
  if (res)
    wgpuBindGroupRelease(res);
  res = nullptr;
}
static void SafeRelease(WGPUBuffer &res) {
  if (res)
    wgpuBufferRelease(res);
  res = nullptr;
}
static void SafeRelease(WGPURenderPipeline &res) {
  if (res)
    wgpuRenderPipelineRelease(res);
  res = nullptr;
}
static void SafeRelease(WGPUSampler &res) {
  if (res)
    wgpuSamplerRelease(res);
  res = nullptr;
}
static void SafeRelease(WGPUShaderModule &res) {
  if (res)
    wgpuShaderModuleRelease(res);
  res = nullptr;
}
static void SafeRelease(WGPUTextureView &res) {
  if (res)
    wgpuTextureViewRelease(res);
  res = nullptr;
}
static void SafeRelease(WGPUTexture &res) {
  if (res)
    wgpuTextureRelease(res);
  res = nullptr;
}

static void SafeRelease(RenderResources &res) {
  SafeRelease(res.FontTexture);
  SafeRelease(res.FontTextureView);
  SafeRelease(res.Sampler);
  SafeRelease(res.MyUniforms);
  SafeRelease(res.CommonBindGroup);
  SafeRelease(res.FontImageBindGroup);
  SafeRelease(res.FontImageBindGroupLayout);
};

static void SafeRelease(FrameResources &res) {
  SafeRelease(res.IndexBuffer);
  SafeRelease(res.VertexBuffer);
  SafeRelease(res.IndexBufferHost);
  SafeRelease(res.VertexBufferHost);
}

static WGPUProgrammableStageDescriptor
ImGui_ImplWGPU_CreateShaderModule(const char *wgsl_source) {
  WGPUShaderModuleWGSLDescriptor wgsl_desc = {};
  wgsl_desc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
#if defined(WEBGPU_BACKEND_WGPU)
  wgsl_desc.code = wgsl_source;
#else
  wgsl_desc.source = wgsl_source;
#endif

  WGPUShaderModuleDescriptor desc = {};
  desc.nextInChain = reinterpret_cast<WGPUChainedStruct *>(&wgsl_desc);

  WGPUProgrammableStageDescriptor stage_desc = {};
  stage_desc.module = wgpuDeviceCreateShaderModule(g_wgpuDevice, &desc);
  stage_desc.entryPoint = "main";
  return stage_desc;
}

// FIXME: only for font image_bind_group
static WGPUBindGroup
ImGui_ImplWGPU_CreateImageBindGroup(WGPUBindGroupLayout layout,
                                    WGPUTextureView texture) {
  WGPUBindGroupEntry image_bg_entries[] = {{nullptr, 0, 0, 0, 0, 0, texture}};

  WGPUBindGroupDescriptor image_bg_descriptor = {};
  image_bg_descriptor.layout = layout;
  image_bg_descriptor.entryCount =
      sizeof(image_bg_entries) / sizeof(WGPUBindGroupEntry);
  image_bg_descriptor.entries = image_bg_entries;
  return wgpuDeviceCreateBindGroup(g_wgpuDevice, &image_bg_descriptor);
}

static void ImGui_ImplWGPU_SetupRenderState(ImDrawData *draw_data,
                                            WGPURenderPassEncoder ctx,
                                            FrameResources *fr) {
  // TODO: Setup mvpct into our constant buffer
  {
    MyUniforms m_uniforms;
    // Upload the initial value of the uniforms
    m_uniforms.time = 1.0f;
    m_uniforms.color = {0.0f, 1.0f, 0.4f, 1.0f};

    // Matrices
    m_uniforms.modelMatrix = {
        {1.0, 0.0, 0.0, 0.0},
        {1.0, 1.0, 0.0, 0.0},
        {1.0, 0.0, 1.0, 0.0},
        {1.0, 0.0, 0.0, 1.0},
    };
    m_uniforms.viewMatrix =
        glm::lookAt(vec3(-2.0f, -3.0f, 2.0f), vec3(0.0f), vec3(0, 0, 1));
    m_uniforms.projectionMatrix =
        glm::perspective(45 * PI / 180, 640.0f / 480.0f, 0.01f, 100.0f);

    wgpuQueueWriteBuffer(g_defaultQueue, g_resources.MyUniforms, 0, &m_uniforms,
                         sizeof(MyUniforms));
  }

  // TODO: Setup lighting matrix into our constant buffer
  {

    LightingUniforms m_lightingUniforms;
    // Upload the initial value of the uniforms
    m_lightingUniforms.directions = {vec4{0.5, -0.9, 0.1, 0.0},
                                     vec4{0.2, 0.4, 0.3, 0.0}};
    m_lightingUniforms.colors = {vec4{1.0, 0.9, 0.6, 1.0},
                                 vec4{0.6, 0.9, 1.0, 1.0}};
    m_lightingUniforms.hardness = 16.0f;
    m_lightingUniforms.kd = 1.0f;
    m_lightingUniforms.ks = 0.5f;
    wgpuQueueWriteBuffer(g_defaultQueue, g_resources.LightingUniforms, 0,
                         &m_lightingUniforms, sizeof(LightingUniforms));
  }

  // Setup viewport
  wgpuRenderPassEncoderSetViewport(
      ctx, 0, 0, draw_data->FramebufferScale.x * draw_data->DisplaySize.x,
      draw_data->FramebufferScale.y * draw_data->DisplaySize.y, 0, 1);

  // NOTE: Bind shader and vertex buffers
  wgpuRenderPassEncoderSetVertexBuffer(ctx, 0, fr->VertexBuffer, 0,
                                       fr->VertexBufferSize *
                                           sizeof(VertexAttributes));
  wgpuRenderPassEncoderSetIndexBuffer(
      ctx, fr->IndexBuffer,
      sizeof(ImDrawIdx) == 2 ? WGPUIndexFormat_Uint16 : WGPUIndexFormat_Uint32,
      0, fr->IndexBufferSize * sizeof(ImDrawIdx));

  wgpuRenderPassEncoderSetPipeline(ctx, g_pipelineState);

  // NOTE: CommonBindGroup = MyUniforms + sampler + texture + lightingMyUniforms
  wgpuRenderPassEncoderSetBindGroup(ctx, 0, g_resources.CommonBindGroup, 0,
                                    nullptr);

  // Setup blend factor
  WGPUColor blend_color = {0.f, 0.f, 0.f, 0.f};
  wgpuRenderPassEncoderSetBlendConstant(ctx, &blend_color);
}

// TODO: render data functions goes here
void ImGui_ImplWGPU_RenderDrawData(ImDrawData *draw_data,
                                   WGPURenderPassEncoder pass_encoder) {
  // Avoid rendering when minimized
  if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
    return;

  // FIXME: Assuming that this only gets called once per frame!
  // If not, we can't just re-allocate the IB or VB, we'll have to do a proper
  // allocator.
  g_frameIndex = g_frameIndex + 1;
  FrameResources *fr = &g_pFrameResources[g_frameIndex % g_numFramesInFlight];

  // Create and grow vertex buffers if needed
  if (fr->VertexBuffer == nullptr ||
      fr->VertexBufferSize < draw_data->TotalVtxCount) {
    if (fr->VertexBuffer) {
      wgpuBufferDestroy(fr->VertexBuffer);
      wgpuBufferRelease(fr->VertexBuffer);
    }
    SafeRelease(fr->VertexBufferHost);
    fr->VertexBufferSize = draw_data->TotalVtxCount + 5000;

    WGPUBufferDescriptor vb_desc = {
        nullptr, "Dear ImGui Vertex buffer",
        WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
        (fr->VertexBufferSize * sizeof(VertexAttributes) + 3) & ~3, false};
    fr->VertexBuffer = wgpuDeviceCreateBuffer(g_wgpuDevice, &vb_desc);
    if (!fr->VertexBuffer)
      return;

    fr->VertexBufferHost = new VertexAttributes[fr->VertexBufferSize];
  }

  // Create and grow index buffers if needed
  if (fr->IndexBuffer == nullptr ||
      fr->IndexBufferSize < draw_data->TotalIdxCount) {
    if (fr->IndexBuffer) {
      wgpuBufferDestroy(fr->IndexBuffer);
      wgpuBufferRelease(fr->IndexBuffer);
    }
    SafeRelease(fr->IndexBufferHost);
    fr->IndexBufferSize = draw_data->TotalIdxCount + 10000;

    WGPUBufferDescriptor ib_desc = {
        nullptr, "Dear ImGui Index buffer",
        WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index,
        (fr->IndexBufferSize * sizeof(VertexAttributes) + 3) & ~3, false};
    fr->IndexBuffer = wgpuDeviceCreateBuffer(g_wgpuDevice, &ib_desc);
    if (!fr->IndexBuffer)
      return;

    fr->IndexBufferHost = new ImDrawIdx[fr->IndexBufferSize];
  }

  // NOTE: Upload vertex/index data into a single contiguous GPU buffer
  VertexAttributes *vtx_dst = (VertexAttributes *)fr->VertexBufferHost;
  ImDrawIdx *idx_dst = (ImDrawIdx *)fr->IndexBufferHost;
  for (int n = 0; n < draw_data->CmdListsCount; n++) {
    const ImDrawList *cmd_list = draw_data->CmdLists[n];
    memcpy(vtx_dst, cmd_list->VtxBuffer.Data,
           cmd_list->VtxBuffer.Size * sizeof(VertexAttributes));
    memcpy(idx_dst, cmd_list->IdxBuffer.Data,
           cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
    vtx_dst += cmd_list->VtxBuffer.Size;
    idx_dst += cmd_list->IdxBuffer.Size;
  }
  int64_t vb_write_size =
      ((char *)vtx_dst - (char *)fr->VertexBufferHost + 3) & ~3;
  int64_t ib_write_size =
      ((char *)idx_dst - (char *)fr->IndexBufferHost + 3) & ~3;
  wgpuQueueWriteBuffer(g_defaultQueue, fr->VertexBuffer, 0,
                       fr->VertexBufferHost, vb_write_size);
  wgpuQueueWriteBuffer(g_defaultQueue, fr->IndexBuffer, 0, fr->IndexBufferHost,
                       ib_write_size);

  // Setup desired render state
  ImGui_ImplWGPU_SetupRenderState(draw_data, pass_encoder, fr);

  // Render command lists
  // (Because we merged all buffers into a single one, we maintain our own
  // offset into them)
  int global_vtx_offset = 0;
  int global_idx_offset = 0;
  ImVec2 clip_scale = draw_data->FramebufferScale;
  ImVec2 clip_off = draw_data->DisplayPos;
  for (int n = 0; n < draw_data->CmdListsCount; n++) {
    const ImDrawList *cmd_list = draw_data->CmdLists[n];
    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
      const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[cmd_i];
      if (pcmd->UserCallback != nullptr) {
        // User callback, registered via ImDrawList::AddCallback()
        // (ImDrawCallback_ResetRenderState is a special callback value used by
        // the user to request the renderer to reset render state.)
        if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
          ImGui_ImplWGPU_SetupRenderState(draw_data, pass_encoder, fr);
        else
          pcmd->UserCallback(cmd_list, pcmd);
      } else {
        ImTextureID tex_id = pcmd->GetTexID();
        WGPUBindGroupEntry common_bg_entries[] = {
            // NOTE: mvp and misc uMyUniforms
            {nullptr, 0, g_resources.MyUniforms, 0, sizeof(MyUniforms), 0, 0},
            // NOTE: sampler bind_group entry here
            {nullptr, 1, 0, 0, 0, g_resources.Sampler, 0},
            // NOTE: texture bind_group entry here
            {nullptr, 2, 0, 0, 0, 0, (WGPUTextureView)tex_id},
            // NOTE: lightingMyUniforms
            {nullptr, 3, g_resources.LightingUniforms, 0,
             sizeof(LightingUniforms), 0, 0},
        };

        ImGui_ImplGPU_RecreateCommonBindGroup(common_bg_entries);
        wgpuRenderPassEncoderSetBindGroup(pass_encoder, 0,
                                          g_resources.CommonBindGroup, 0, 0);

        // Project scissor/clipping rectangles into framebuffer space
        ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x,
                        (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
        ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x,
                        (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
        if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
          continue;

        // Apply scissor/clipping rectangle, Draw
        wgpuRenderPassEncoderSetScissorRect(
            pass_encoder, (uint32_t)clip_min.x, (uint32_t)clip_min.y,
            (uint32_t)(clip_max.x - clip_min.x),
            (uint32_t)(clip_max.y - clip_min.y));
        wgpuRenderPassEncoderDrawIndexed(pass_encoder, pcmd->ElemCount, 1,
                                         pcmd->IdxOffset + global_idx_offset,
                                         pcmd->VtxOffset + global_vtx_offset,
                                         0);
      }
    }
    global_idx_offset += cmd_list->IdxBuffer.Size;
    global_vtx_offset += cmd_list->VtxBuffer.Size;
  }
}

static void ImGui_ImplWGPU_CreateFontsTexture() {
  // Build texture atlas
  ImGuiIO &io = ImGui::GetIO();
  unsigned char *pixels;
  int width, height, size_pp;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &size_pp);

  // Upload texture to graphics system
  {
    WGPUTextureDescriptor tex_desc = {};
    tex_desc.label = "Dear ImGui Font Texture";
    tex_desc.dimension = WGPUTextureDimension_2D;
    tex_desc.size.width = width;
    tex_desc.size.height = height;
    tex_desc.size.depthOrArrayLayers = 1;
    tex_desc.sampleCount = 1;
    tex_desc.format = WGPUTextureFormat_RGBA8Unorm;
    tex_desc.mipLevelCount = 1;
    tex_desc.usage = WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding;
    g_resources.FontTexture = wgpuDeviceCreateTexture(g_wgpuDevice, &tex_desc);

    WGPUTextureViewDescriptor tex_view_desc = {};
    tex_view_desc.format = WGPUTextureFormat_RGBA8Unorm;
    tex_view_desc.dimension = WGPUTextureViewDimension_2D;
    tex_view_desc.baseMipLevel = 0;
    tex_view_desc.mipLevelCount = 1;
    tex_view_desc.baseArrayLayer = 0;
    tex_view_desc.arrayLayerCount = 1;
    tex_view_desc.aspect = WGPUTextureAspect_All;
    g_resources.FontTextureView =
        wgpuTextureCreateView(g_resources.FontTexture, &tex_view_desc);
  }

  // Upload texture data
  {
    WGPUImageCopyTexture dst_view = {};
    dst_view.texture = g_resources.FontTexture;
    dst_view.mipLevel = 0;
    dst_view.origin = {0, 0, 0};
    dst_view.aspect = WGPUTextureAspect_All;
    WGPUTextureDataLayout layout = {};
    layout.offset = 0;
    layout.bytesPerRow = width * size_pp;
    layout.rowsPerImage = height;
    WGPUExtent3D size = {(uint32_t)width, (uint32_t)height, 1};
    wgpuQueueWriteTexture(g_defaultQueue, &dst_view, pixels,
                          (uint32_t)(width * size_pp * height), &layout, &size);
  }

  // Create the associated sampler
  // (Bilinear sampling is required by default. Set 'io.Fonts->Flags |=
  // ImFontAtlasFlags_NoBakedLines' or 'style.AntiAliasedLinesUseTex = false' to
  // allow point/nearest sampling)
  {
    WGPUSamplerDescriptor sampler_desc = {};
    sampler_desc.minFilter = WGPUFilterMode_Linear;
    sampler_desc.magFilter = WGPUFilterMode_Linear;
#if defined(WEBGPU_BACKEND_WGPU)
    sampler_desc.mipmapFilter = WGPUMipmapFilterMode_Linear;
#else
    sampler_desc.mipmapFilter = WGPUFilterMode_Linear;
#endif
    sampler_desc.addressModeU = WGPUAddressMode_Repeat;
    sampler_desc.addressModeV = WGPUAddressMode_Repeat;
    sampler_desc.addressModeW = WGPUAddressMode_Repeat;
    sampler_desc.maxAnisotropy = 1;
    g_resources.Sampler = wgpuDeviceCreateSampler(g_wgpuDevice, &sampler_desc);
  }

  // Store our identifier
  static_assert(
      sizeof(ImTextureID) >= sizeof(g_resources.FontTexture),
      "Can't pack descriptor handle into TexID, 32-bit not supported yet.");
  io.Fonts->SetTexID((ImTextureID)g_resources.FontTextureView);
}

static void ImGui_ImplWGPU_CreateUniformBuffer() {
  WGPUBufferDescriptor ub_desc = {nullptr, "Dear ImGui MyUniform buffer",
                                  WGPUBufferUsage_CopyDst |
                                      WGPUBufferUsage_Uniform,
                                  (sizeof(MyUniforms) + 3) & ~3, false};
  g_resources.MyUniforms = wgpuDeviceCreateBuffer(g_wgpuDevice, &ub_desc);
}

// FIXME: light uniform buffer
static void ImGui_ImplWGPU_CreateLightUniformBuffer() {
  WGPUBufferDescriptor ub_desc = {nullptr, "Dear ImGui LightUniform buffer",
                                  WGPUBufferUsage_CopyDst |
                                      WGPUBufferUsage_Uniform,
                                  (sizeof(LightingUniforms) + 3) & ~3, false};
  g_resources.MyUniforms = wgpuDeviceCreateBuffer(g_wgpuDevice, &ub_desc);
}

static void
ImGui_ImplGPU_RecreateCommonBindGroup(WGPUBindGroupEntry common_bg_entries[]) {

  // Bind group layouts
  WGPUBindGroupLayoutEntry common_bg_layout_entries[4] = {};
  // vertex
  common_bg_layout_entries[0].binding = 0;
  common_bg_layout_entries[0].visibility = WGPUShaderStage_Vertex;
  common_bg_layout_entries[0].buffer.type = WGPUBufferBindingType_Uniform;

  // fragment
  common_bg_layout_entries[1].binding = 1;
  common_bg_layout_entries[1].visibility = WGPUShaderStage_Fragment;
  common_bg_layout_entries[1].sampler.type = WGPUSamplerBindingType_Filtering;

  common_bg_layout_entries[2].binding = 2;
  common_bg_layout_entries[2].visibility = WGPUShaderStage_Fragment;
  common_bg_layout_entries[2].texture.sampleType = WGPUTextureSampleType_Float;
  common_bg_layout_entries[2].texture.viewDimension =
      WGPUTextureViewDimension_2D;

  common_bg_layout_entries[3].binding = 3;
  common_bg_layout_entries[3].visibility = WGPUShaderStage_Fragment;
  common_bg_layout_entries[3].buffer.type = WGPUBufferBindingType_Uniform;

  WGPUBindGroupLayoutDescriptor common_bg_layout_desc = {};
  common_bg_layout_desc.entryCount = 4;
  common_bg_layout_desc.entries = common_bg_layout_entries;

  WGPUBindGroupLayout bg_layouts[1];
  // NOTE: bg_layouts[0] = MyUniforms + sampler + textureView + LightingUniforms
  bg_layouts[0] =
      wgpuDeviceCreateBindGroupLayout(g_wgpuDevice, &common_bg_layout_desc);

  WGPUPipelineLayoutDescriptor layout_desc = {};
  layout_desc.bindGroupLayoutCount = 2;
  layout_desc.bindGroupLayouts = bg_layouts;

  // NOTE: Set pipeline descriptor
  graphics_pipeline_desc.layout =
      wgpuDeviceCreatePipelineLayout(g_wgpuDevice, &layout_desc);

  // Create resource bind group
  // Common bind group is in bg_layouts[0]
  WGPUBindGroupDescriptor common_bg_descriptor = {};
  common_bg_descriptor.layout = bg_layouts[0];
  common_bg_descriptor.entryCount =
      sizeof(common_bg_entries) / sizeof(WGPUBindGroupEntry);
  common_bg_descriptor.entries = common_bg_entries;

  // NOTE: Update common bind group
  g_resources.CommonBindGroup =
      wgpuDeviceCreateBindGroup(g_wgpuDevice, &common_bg_descriptor);
  wgpuBindGroupLayoutRelease(bg_layouts[0]);
}

// FIXME: light textureview buffer
static void ImGui_ImplWGPU_LoadObjToVertexBuffer() {
  std::vector<ResourceManager::VertexAttributes> m_vertexData;
  Buffer m_vertexBuffer = nullptr;
  WGPUBufferDescriptor ub_desc = {
      nullptr, "Dear ImGui Texture buffer",
      WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
      (sizeof(ResourceManager::VertexAttributes) + 3) & ~3, false};
  // [CPU] load geometry
  bool success = ResourceManager::loadGeometryFromObj(
      RESOURCE_DIR "/fourareen.obj", m_vertexData);
  if (!success) {
    std::cerr << "Could not load geometry!" << std::endl;
  }

  // [GPU] Create vertex buffer
  BufferDescriptor bufferDesc;
  bufferDesc.size = m_vertexData.size() * sizeof(VertexAttributes);
  bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
  bufferDesc.mappedAtCreation = false;
  m_vertexBuffer = wgpuDeviceCreateBuffer(g_wgpuDevice, &ub_desc);

  int m_indexCount;
  m_indexCount = static_cast<int>(m_vertexData.size());
}

bool ImGui_ImplWGPU_CreateDeviceObjects() {
  if (!g_wgpuDevice)
    return false;
  if (g_pipelineState)
    ImGui_ImplWGPU_InvalidateDeviceObjects();

  // Create render pipeline
  graphics_pipeline_desc.primitive.topology =
      WGPUPrimitiveTopology_TriangleList;
  graphics_pipeline_desc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
  graphics_pipeline_desc.primitive.frontFace = WGPUFrontFace_CW;
  graphics_pipeline_desc.primitive.cullMode = WGPUCullMode_None;

  graphics_pipeline_desc.multisample.count = 1;
  graphics_pipeline_desc.multisample.mask = UINT_MAX;
  graphics_pipeline_desc.multisample.alphaToCoverageEnabled = false;

  // Create the vertex shader
  WGPUProgrammableStageDescriptor vertex_shader_desc =
      ImGui_ImplWGPU_CreateShaderModule(__shader_vert_wgsl);
  graphics_pipeline_desc.vertex.module = vertex_shader_desc.module;
  graphics_pipeline_desc.vertex.entryPoint = vertex_shader_desc.entryPoint;

  // Vertex input configuration, in array instead of vector
  WGPUVertexAttribute attribute_desc[] = {
      {WGPUVertexFormat_Float32x3,
       (uint64_t)offsetof(VertexAttributes, position), 0},
      {WGPUVertexFormat_Float32x3, (uint64_t)offsetof(VertexAttributes, normal),
       1},
      {WGPUVertexFormat_Float32x3, (uint64_t)offsetof(VertexAttributes, color),
       2},
      {WGPUVertexFormat_Float32x2, (uint64_t)offsetof(VertexAttributes, uv), 3},
  };

  WGPUVertexBufferLayout buffer_layouts[1];
  buffer_layouts[0].arrayStride = sizeof(VertexAttributes);
  buffer_layouts[0].stepMode = WGPUVertexStepMode_Vertex;
  buffer_layouts[0].attributeCount = 4;
  buffer_layouts[0].attributes = attribute_desc;

  graphics_pipeline_desc.vertex.bufferCount = 1;
  graphics_pipeline_desc.vertex.buffers = buffer_layouts;

  // Create the pixel shader
  WGPUProgrammableStageDescriptor pixel_shader_desc =
      ImGui_ImplWGPU_CreateShaderModule(__shader_frag_wgsl);

  // Create the blending setup
  WGPUBlendState blend_state = {};
  blend_state.color.srcFactor = WGPUBlendFactor_SrcAlpha;
  blend_state.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
  blend_state.color.operation = WGPUBlendOperation_Add;
  blend_state.alpha.srcFactor = WGPUBlendFactor_Zero;
  blend_state.alpha.dstFactor = WGPUBlendFactor_One;
  blend_state.alpha.operation = WGPUBlendOperation_Add;

  WGPUColorTargetState color_state = {};
  color_state.format = g_renderTargetFormat;
  color_state.blend = &blend_state;
  color_state.writeMask = WGPUColorWriteMask_All;

  WGPUFragmentState fragment_state = {};
  fragment_state.module = pixel_shader_desc.module;
  fragment_state.entryPoint = pixel_shader_desc.entryPoint;
  fragment_state.targetCount = 1;
  fragment_state.targets = &color_state;

  graphics_pipeline_desc.fragment = &fragment_state;

  // Create depth-stencil State
  WGPUDepthStencilState depth_stencil_state = {};
  depth_stencil_state.format = g_depthStencilFormat;
  depth_stencil_state.depthWriteEnabled = true;
  depth_stencil_state.depthCompare = WGPUCompareFunction_Less;
  depth_stencil_state.stencilWriteMask = 0;
  depth_stencil_state.stencilReadMask = 0;
  depth_stencil_state.stencilFront.compare = WGPUCompareFunction_Always;
  depth_stencil_state.stencilBack.compare = WGPUCompareFunction_Always;

  // Configure disabled depth-stencil state
  graphics_pipeline_desc.depthStencil =
      g_depthStencilFormat == WGPUTextureFormat_Undefined
          ? nullptr
          : &depth_stencil_state;

  g_pipelineState =
      wgpuDeviceCreateRenderPipeline(g_wgpuDevice, &graphics_pipeline_desc);

  // NOTE: Create buffers
  ImGui_ImplWGPU_CreateUniformBuffer();
  ImGui_ImplWGPU_CreateLightUniformBuffer();
  ImGui_ImplWGPU_LoadObjToVertexBuffer();

  ImGui_ImplWGPU_CreateFontsTexture();
  WGPUBindGroupEntry common_bg_entries[] = {
      // NOTE: mvp and misc uMyUniforms
      {nullptr, 0, g_resources.MyUniforms, 0, sizeof(MyUniforms), 0, 0},
      // NOTE: sampler bind_group entry here
      {nullptr, 1, 0, 0, 0, g_resources.Sampler, 0},
      // NOTE: texture bind_group entry here
      {nullptr, 2, 0, 0, 0, 0, g_resources.FontTextureView},
      // NOTE: lightingMyUniforms
      {nullptr, 3, g_resources.LightingUniforms, 0, sizeof(LightingUniforms), 0,
       0},
  };

  ImGui_ImplGPU_RecreateCommonBindGroup(common_bg_entries);

  SafeRelease(vertex_shader_desc.module);
  SafeRelease(pixel_shader_desc.module);

  return true;
}

void ImGui_ImplWGPU_InvalidateDeviceObjects() {
  if (!g_wgpuDevice)
    return;

  SafeRelease(g_pipelineState);
  SafeRelease(g_resources);

  ImGuiIO &io = ImGui::GetIO();
  io.Fonts->SetTexID(0); // We copied g_pFontTextureView to io.Fonts->TexID so
                         // let's clear that as well.

  for (unsigned int i = 0; i < g_numFramesInFlight; i++)
    SafeRelease(g_pFrameResources[i]);
}

bool ImGui_ImplWGPU_Init(WGPUDevice device, int num_frames_in_flight,
                         WGPUTextureFormat rt_format,
                         WGPUTextureFormat depth_format) {
  // Setup backend capabilities flags
  ImGuiIO &io = ImGui::GetIO();
  io.BackendRendererName = "efwmc_impl_webgpu";
  io.BackendFlags |=
      ImGuiBackendFlags_RendererHasVtxOffset; // We can honor the
                                              // ImDrawCmd::VtxOffset field,
                                              // allowing for large meshes.

  g_wgpuDevice = device;
  g_defaultQueue = wgpuDeviceGetQueue(g_wgpuDevice);
  g_renderTargetFormat = rt_format;
  g_depthStencilFormat = depth_format;
  g_pFrameResources = new FrameResources[num_frames_in_flight];
  g_numFramesInFlight = num_frames_in_flight;
  g_frameIndex = UINT_MAX;

  g_resources.FontTexture = nullptr;
  g_resources.FontTextureView = nullptr;
  g_resources.Sampler = nullptr;
  g_resources.MyUniforms = nullptr;
  g_resources.CommonBindGroup = nullptr;
  g_resources.ImageBindGroups.Data.reserve(100);
  g_resources.FontImageBindGroup = nullptr;
  g_resources.FontImageBindGroupLayout = nullptr;

  // Create buffers with a default size (they will later be grown as needed)
  for (int i = 0; i < num_frames_in_flight; i++) {
    FrameResources *fr = &g_pFrameResources[i];
    fr->IndexBuffer = nullptr;
    fr->VertexBuffer = nullptr;
    fr->IndexBufferHost = nullptr;
    fr->VertexBufferHost = nullptr;
    fr->IndexBufferSize = 10000;
    fr->VertexBufferSize = 5000;
  }

  return true;
}

void ImGui_ImplWGPU_Shutdown() {
  ImGui_ImplWGPU_InvalidateDeviceObjects();
  delete[] g_pFrameResources;
  g_pFrameResources = nullptr;
  wgpuQueueRelease(g_defaultQueue);
  g_wgpuDevice = nullptr;
  g_numFramesInFlight = 0;
  g_frameIndex = UINT_MAX;
}

void ImGui_ImplWGPU_NewFrame() {
  if (!g_pipelineState)
    ImGui_ImplWGPU_CreateDeviceObjects();
}
