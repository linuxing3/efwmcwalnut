#pragma once
#include "Common.h"
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

// NOTE: Expose some global variables
static std::vector<VertexAttributes> g_vertexData;
void SetVertexData(std::vector<VertexAttributes> data) { g_vertexData = data; };

static LightingUniforms g_lightingUniforms;
void SetLightingUniforms(LightingUniforms &&data) {
  g_lightingUniforms = data;
};

static MyUniforms g_uniforms;
void SetMyUniforms(MyUniforms &&data) { g_uniforms = data; };

WGPURenderPipeline GetGlobalPipelineState();
RenderResources *GetGResources();
FrameResources *GetPerFrameResources();
} // namespace efwmc
