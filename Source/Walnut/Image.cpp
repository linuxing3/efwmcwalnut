#pragma once
#include "Image.h"

#include "Application.h"

#include "ResourceManager.h"
#include "imgui_internal.h"
#include "vulkan/vulkan_core.h"
#include <cstdint>

/* #ifndef STB_IMAGE_IMPLEMENTATION */
/* #define STB_IMAGE_IMPLEMENTATION */
/* #endif // !STB_IMAGE_IMPLEMENTATION */
#include "stb/stb_image.h"
#include "webgpu.hpp"
#include "wgpu.h" // wgpuTextureViewDrop

using namespace wgpu;

namespace Walnut {

namespace Utils {
static TextureFormat WalnutFormatToWGPU(Walnut::ImageFormat format) {
  if (format == Walnut::ImageFormat::RGBA)
    return TextureFormat::RGBA8Unorm;
  if (format == Walnut::ImageFormat::RGBA32F)
    return TextureFormat::RGBA32Float;
  return TextureFormat::BGRA8Unorm;
}

} // namespace Utils

Image::Image(std::string_view path) : m_Filepath(path) {
  // use stb_image to load a picture from a path and save data into a uint8_t
  // array
  int width, height, channels;
  uint8_t *data = nullptr;
  data = stbi_load(m_Filepath.c_str(), &width, &height, &channels, 4);

  m_Width = width;
  m_Height = height;

  AllocateMemory();
  SetData(data);
  stbi_image_free(data);
}

Image::Image(uint32_t width, uint32_t height, TextureFormat format,
             const void *data)
    : m_Width(width), m_Height(height), m_Format(format) {
  AllocateMemory();
  if (data)
    SetData(data);
}

Image::Image(uint32_t width, uint32_t height, ImageFormat format,
             const void *data)
    : m_Width(width), m_Height(height) {
  m_Format = Utils::WalnutFormatToWGPU(format);
  AllocateMemory();
  if (data)
    SetData(data);
}

Image::~Image() { Release(); }

void Image::AllocateMemory() {
  auto app = Application::Get();
  Device device = app->GetDevice();
  m_Image = ResourceManager::initTexture(
      m_Width, m_Height,
      TextureUsage::RenderAttachment | TextureUsage::TextureBinding |
          TextureUsage::CopyDst,
      m_Format, device, &m_ImageView, &m_Sampler, m_MipLevelEnabled);

  m_DescriptorSet = (VkDescriptorSet)(ImTextureID)m_ImageView;
}

void Image::Release() {
  m_Sampler = nullptr;
  m_ImageView = nullptr;
  m_Image = nullptr;
  m_StagingBuffer = nullptr;
}

void Image::SetData(const void *data) {
  Device device = Application::Get()->GetDevice();

  if (!ResourceManager::updateTexture(m_Width, m_Height, m_Format, device,
                                      &m_Image, data, m_MipLevelEnabled)) {
    std::cerr << "Could not set data to texture!" << std::endl;
  };
}

void Image::Resize(uint32_t width, uint32_t height) {
  if (m_Image && m_Width == width && m_Height == height)
    return;

  m_Width = width;
  m_Height = height;

  Release();
  AllocateMemory();
}

void *Image::Decode(const void *buffer, uint64_t length, uint32_t &outWidth,
                    uint32_t &outHeight) {
  int width, height, channels;
  uint8_t *data = nullptr;
  uint64_t size = 0;

  data = stbi_load_from_memory((const stbi_uc *)buffer, length, &width, &height,
                               &channels, 4);
  size = width * height * 4;

  outWidth = width;
  outHeight = height;

  return data;
}

void Image::InitModel(uint32_t width, uint32_t height,
                      TextureView *tex_id, Sampler *sampler) {
  auto app = Application::Get();
  Device device = app->GetDevice();
  TextureFormat swapChainFormat = app->GetSwapChainFormat();


  ResourceManager::initTexture(width, height,
                               TextureUsage::RenderAttachment |
                                   TextureUsage::TextureBinding |
                                   TextureUsage::CopyDst,
                               swapChainFormat, device, tex_id, sampler);
  ImGuiID tex_id_hash = ImHashData(tex_id, sizeof(*tex_id));
  auto found = app->m_TextureStorage.GetVoidPtr(tex_id_hash);
  if (!found) {
    app->m_TextureStorage.SetVoidPtr(tex_id_hash, *tex_id);
    app->m_TextureIdSet.insert(*tex_id);
  }
}

} // namespace Walnut
