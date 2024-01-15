/**
 * This file is part of the "Learn WebGPU for C++" book.
 *   https://github.com/eliemichel/LearnWebGPU
 *
 * MIT License
 * Copyright (c) 2022-2023 Elie Michel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "Common.h"
#include "Layer.h"
#include "ResourceManager.h"
#include "SharedData.h"
#include "Walnut/Image.h"

#include <GLFW/glfw3.h>
#include <cstdint>
#include <queue>
#include <vector>
#include <webgpu.hpp>

#include <glm/glm.hpp>

#include <filesystem>

using namespace wgpu;
using namespace std;

class Application {
public:
  Application(const ApplicationSpecification &applicationSpecification =
                  ApplicationSpecification());
  ~Application();

  static Application *Get();
  // A function called only once at the beginning. Returns false is init
  // failed.
  bool onInit();

  void onCompute();
  // A function called at each frame, guarantied never to be called before
  // `onInit`.
  void onFrame();

  // A function called only once at the very end.
  void onFinish();

  // A function called when the window is resized.
  void onResize();

  void onMouseMove(double xpos, double ypos);
  // Mouse events
  void onMouseButton(int button, int action, int mods);
  void onScroll(double xoffset, double yoffset);

  // A function that tells if the application is still running.
  bool isRunning();

  Device GetDevice() { return m_device; };
  Queue GetQueue() { return m_device.getQueue(); };

  bool IsMaximized() const;
  GLFWwindow *GetWindowHandle() { return m_WindowHandle; };

  TextureView GetCurrentTextureView() { return m_currentTextureView; }
  TextureView GetCurrentDepthView() { return m_depthTextureView; }
  vector<BindGroupEntry> GetBindings() { return m_bindingEntries; };

  static CommandBuffer
  RunSingleCommand(function<void(RenderPassEncoder renderPass)> &&renderFunc);
  static CommandBuffer RunSingleCommand(function<void()> &&prepareFunc);

private:
  void buildWindow();
  void buildDeviceObject();
  void buildSwapChain();
  void buildRenderPipeline();
  void buildComputePipeline();
  void buildDepthBuffer();
  void updateViewMatrix();
  void updateDragInertia();

  void initGui();                               // called in onInit
  void updateGui(RenderPassEncoder renderPass); // called in onFrame

  bool initTexture(const filesystem::path &path);

  void initLighting();
  void updateLighting();
  void initComputeBindGroup();
  void terminateBindGroup();

  void initComputeBindGroupLayout();
  void terminateBindGroupLayout();

  void initComputePipeline();
  void terminateComputePipeline();

  void initComputeBuffers();
  void terminateBuffers();
  void UI_DrawMenubar();

private:
  using vec2 = glm::vec2;
  using vec3 = glm::vec3;
  using vec4 = glm::vec4;
  using mat4x4 = glm::mat4x4;
  TextureFormat m_swapChainFormat = TextureFormat::RGBA8UnormSrgb;
  TextureFormat m_depthTextureFormat = TextureFormat::Depth32Float;

  bool m_Running = false;
  // Everything that is initialized in `onInit` and needed in `onFrame`.
  Instance m_instance = nullptr;
  Surface m_surface = nullptr;
  Device m_device = nullptr;

  SwapChainDescriptor m_swapChainDesc{};
  SwapChain m_swapChain = nullptr;

  RenderPipeline m_pipeline = nullptr;
  ComputePipeline m_computePipeline = nullptr;
  PipelineLayout m_pipelineLayout = nullptr;

  // --------------------------------------------------------
  // BindGroup = BindGroupLayoutEntry + BindingGroupEntry
  // --------------------------------------------------------
  BindGroup m_bindGroup = nullptr;
  vector<BindGroupEntry> m_bindingEntries;
  vector<BindGroupLayoutEntry> m_bindingLayoutEntries;

  BindGroupLayout m_bindGroupLayout = nullptr;
  BindGroup m_computeBindGroup = nullptr;
  vector<BindGroupEntry> m_computeBindingEntries;
  vector<BindGroupLayoutEntry> m_computeBindingLayoutEntries;
  uint32_t m_bufferSize = 1000;
  wgpu::Buffer m_inputBuffer = nullptr;
  wgpu::Buffer m_outputBuffer = nullptr;
  wgpu::Buffer m_mapBuffer = nullptr;
  // --------------------------------------------------------
  // Vertex part
  vector<ResourceManager::VertexAttributes> m_vertexData;
  Buffer m_vertexBuffer = nullptr;
  int m_indexCount;

  Buffer m_uniformBuffer = nullptr;
  MyUniforms m_uniforms;

  // --------------------------------------------------------
  // Fragement part
  // --------------------------------------------------------

  // @group(0) @binding(2) var baseColorTexture : texture_2d<f32>;
  vector<Texture> m_textures;
  TextureView m_currentTextureView = nullptr;
  Texture m_depthTexture = nullptr;
  TextureView m_depthTextureView = nullptr;

  // @group(0) @binding(3) var LightingUniforms
  LightingUniforms m_lightingUniforms;
  Buffer m_lightingUniformBuffer = nullptr;
  bool m_lightingUniformsChanged = false;
  // --------------------------------------------------------

  CameraState m_cameraState;
  DragState m_drag;
  // --------------------------------------------------------

  unique_ptr<ErrorCallback> m_uncapturedErrorCallback;
  std::function<void()> m_MenubarCallback;

  GLFWwindow *m_WindowHandle = nullptr;
  bool m_TitleBarHovered = false;
  void UI_DrawTitlebar(float &outTitlebarHeight);

  std::mutex m_EventQueueMutex;
  std::queue<std::function<void()>> m_EventQueue;

  std::shared_ptr<Walnut::Image> GetApplicationIcon() const {
    return g_AppHeaderIcon;
  }

  GLFWwindow *GetWindowHandle() const { return m_WindowHandle; }
  bool IsTitleBarHovered() const { return m_TitleBarHovered; }

  static ImFont *GetFont(const std::string &name);

public:
  ApplicationSpecification m_Specification;
  vector<shared_ptr<Walnut::Layer>> m_LayerStack;

  template <typename T> void PushLayer() {
    static_assert(is_base_of<Walnut::Layer, T>::value,
                  "Pushed type is not subclass of Layer!");
    m_LayerStack.emplace_back(make_shared<T>())->OnAttach();
  }

  template <typename Func> void QueueEvent(Func &&func) {
    m_EventQueue.push(func);
  }

  void SetMenubarCallback(const std::function<void()> &menubarCallback) {
    m_MenubarCallback = menubarCallback;
  }

  void PushLayer(const shared_ptr<Walnut::Layer> &layer) {
    m_LayerStack.emplace_back(layer);
    layer->OnAttach();
  }
  void onRun();
  void onClose();
  float GetTime();
};
