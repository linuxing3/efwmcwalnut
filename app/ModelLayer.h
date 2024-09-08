#include <float.h>

#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <memory>
#include <vector>

#include "Application.h"
#include "Camera.h"
#include "Walnut/Image.h"
#include "Walnut/Timer.h"
#include "imgui.h"
#include "imgui_internal.h"

using namespace Walnut;

class ModelLayer : public Walnut::Layer {
public:
  ModelLayer()  {
    Application::Get()->QueueEvent([this]() {
      Image::InitModel(Application::Get()->GetWidth(),
                       Application::Get()->GetHeight(), &m_ImageView, &m_Sampler);
    });

  }

  virtual void OnUpdate(float ts) override {  }

  virtual void OnUIRender() override {
    ImGuiID tex_id_hash = ImHashData(&m_ImageView, sizeof(m_ImageView));
    auto found = Application::Get()->GetTextureStorage().GetVoidPtr(tex_id_hash);
    if (found) {
      ImGui::Begin("Model from Layer");
      auto region = ImGui::GetContentRegionAvail();
      ImGui::Image((ImTextureID)m_ImageView,
                   {(float)region.x, (float)region.y});
      ImGui::End();
    }
  }

  void Render() {

  }

private:
 wgpu::TextureView m_ImageView = nullptr;
 wgpu::Sampler m_Sampler = nullptr;
};
