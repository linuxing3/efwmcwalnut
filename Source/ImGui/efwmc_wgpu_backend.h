#pragma once
#include "ResourceManager.h"
#include "imgui.h" // IMGUI_IMPL_API
#include "webgpu.h"
#include <cstddef>

#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/polar_coordinates.hpp>

// Dear ImGui prototypes from imgui_internal.h
extern ImGuiID ImHashData(const void *data_p, size_t data_size, ImU32 seed = 0);

using VertexAttributes = ResourceManager::VertexAttributes;

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
  WGPUBindGroup ImageBindGroup; // Default font-resource of Dear ImGui
  WGPUBindGroupLayout
      ImageBindGroupLayout; // Cache layout used for the image bind group.
                            // Avoids allocating unnecessary JS objects when
                            // working with WebASM
};

struct FrameResources {
  WGPUBuffer IndexBuffer;
  WGPUBuffer VertexBuffer;
  ImDrawIdx *IndexBufferHost;
  VertexAttributes *VertexBufferHost;
  int IndexBufferSize;
  int VertexBufferSize;
};

using namespace std;
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

namespace efwmc {

// Helpers
bool ImGui_ImplWGPU_Init(
    WGPUDevice device, int num_frames_in_flight, WGPUTextureFormat rt_format,
    WGPUTextureFormat depth_format = WGPUTextureFormat_Undefined);
void ImGui_ImplWGPU_Shutdown();
void ImGui_ImplWGPU_NewFrame();
void ImGui_ImplWGPU_RenderDrawData(ImDrawData *draw_data,
                                   WGPURenderPassEncoder pass_encoder);

// Use if you want to reset your rendering device without losing Dear ImGui
// state.
void ImGui_ImplWGPU_InvalidateDeviceObjects();
bool ImGui_ImplWGPU_CreateDeviceObjects();

// Expose some global variables
static std::vector<VertexAttributes> g_vertexData;
static LightingUniforms g_lightingUniforms;
static MyUniforms g_uniforms;

WGPURenderPipeline GetGlobalPipelineState();
RenderResources *GetGResources();
FrameResources *GetPerFrameResources();
} // namespace efwmc
