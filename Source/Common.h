#pragma once
#include "ResourceManager.h"
#include "imgui.h"
#include <cstdint>
#include <vector>
#include <webgpu.hpp>

#include <glm/glm.hpp>

#include <array>
#include <filesystem>

using namespace wgpu;
using namespace std;
using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using mat4x4 = glm::mat4x4;

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
  array<vec4, 2> directions;
  array<vec4, 2> colors;
  float hardness;
  float kd;
  float ks;
  float _pad;
};
static_assert(sizeof(LightingUniforms) % 16 == 0);
// --------------------------------------------------------
// Camera & Animation control
// --------------------------------------------------------
struct CameraState {
  // angles.x is the rotation of the camera around the global vertical axis,
  // affected by mouse.x angles.y is the rotation of the camera around its
  // local horizontal axis, affected by mouse.y
  vec2 angles = {0.8f, 0.5f};
  // zoom is the position of the camera along its local forward axis, affected
  // by the scroll wheel
  float zoom = -1.2f;
};

struct DragState {
  // Whether a drag action is ongoing (i.e., we are between mouse press and
  // mouse release)
  bool active = false;
  // The position of the mouse at the beginning of the drag action
  vec2 startMouse;
  // The camera state at the beginning of the drag action
  CameraState startCameraState;

  // Constant settings
  float sensitivity = 0.01f;
  float scrollSensitivity = 0.1f;

  // Inertia
  vec2 velocity = {0.0, 0.0};
  vec2 previousDelta;
  float intertia = 0.9f;
};

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
