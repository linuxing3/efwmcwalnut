#pragma once
#include <string>
#include <string_view>

#include "imgui.h"
#include "vulkan/vulkan_core.h"

#include "webgpu.h"
#include <webgpu.hpp>

namespace Walnut {

enum class ImageFormat { None = 0, RGBA, RGBA32F };

class Image {
public:
  Image(std::string_view path);
  Image(uint32_t width, uint32_t height, wgpu::TextureFormat format,
        const void *data = nullptr);
  Image(uint32_t width, uint32_t height, Walnut::ImageFormat,
        const void *data = nullptr);
  ~Image();

  static void InitModel(uint32_t width, uint32_t height);
  void SetData(const void *data);

  void SetImageViewId(WGPUTextureView *id);
  VkDescriptorSet GetDescriptorSet() const { return m_DescriptorSet; }
  wgpu::TextureView GetImageViewId() const { return m_ImageView; }
  ImTextureID GetTextureId() { return (ImTextureID)m_ImageView; }

  void Resize(uint32_t width, uint32_t height);

  uint32_t GetWidth() const { return m_Width; }
  uint32_t GetHeight() const { return m_Height; }

  static void *Decode(const void *data, uint64_t length, uint32_t &outWidth,
                      uint32_t &outHeight);

private:
  void AllocateMemory();
  void Release();

private:
  uint32_t m_Width = 640, m_Height = 480;

  wgpu::TextureFormat m_Format = wgpu::TextureFormat::RGBA8Unorm;
  wgpu::TextureView m_ImageView = nullptr;
  wgpu::Texture m_Image = nullptr;
  wgpu::Sampler m_Sampler = nullptr;
  wgpu::Buffer m_StagingBuffer = nullptr;

  bool m_MipLevelEnabled = false;

  size_t m_AlignedSize = 0;

  VkDescriptorSet m_DescriptorSet = nullptr;
  std::string m_Filepath;
};

} // namespace Walnut
