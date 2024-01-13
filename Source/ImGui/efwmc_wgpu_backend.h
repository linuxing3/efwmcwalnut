#pragma once
#include "imgui.h" // IMGUI_IMPL_API
#include "webgpu.h"

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
