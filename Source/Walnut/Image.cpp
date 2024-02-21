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
#include "stb_image.h"
#include "webgpu.hpp"

using namespace wgpu;

namespace Walnut {

namespace Utils {

static uint32_t bit_width(uint32_t m) {
  if (m == 0)
    return 0;
  else {
    uint32_t w = 0;
    while (m >>= 1)
      ++w;
    return w;
  }
}
static uint32_t BytesPerPixel(WGPUTextureFormat format) {
  if (format == TextureFormat::RGBA8Unorm)
    return 4;
  if (format == TextureFormat::RGBA32Float)
    return 16;
  return 0;
}

static uint32_t BytesPerPixel(Walnut::ImageFormat format) {
  if (format == Walnut::ImageFormat::RGBA)
    return 4;
  if (format == Walnut::ImageFormat::RGBA32F)
    return 16;
  return 0;
}
static VkFormat WalnutFormatToVulkan(Walnut::ImageFormat format) {
  if (format == Walnut::ImageFormat::RGBA)
    return VK_FORMAT_R8G8B8A8_UNORM;
  if (format == Walnut::ImageFormat::RGBA32F)
    return VK_FORMAT_R32G32B32A32_SFLOAT;
  return (VkFormat)0;
}

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
  m_Format = TextureFormat::RGBA8Unorm;

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
  AllocateMemory();
  if (data)
    SetData(data);
}

Image::~Image() { Release(); }

void Image::SetImageViewId(WGPUTextureView *textView) {
  m_ImageView = *textView;
  m_DescriptorSet = (VkDescriptorSet)(ImTextureID)m_ImageView;
}

void Image::AllocateMemory() {
  auto app = Application::Get();
  Device device = app->GetDevice();
  m_Format = TextureFormat::RGBA8Unorm;

#define IMGUI_WGPU
#ifdef IMGUI_WGPU
  // Build texture atlas
  m_Image = ResourceManager::initTexture(
      m_Width, m_Height,
      TextureUsage::RenderAttachment | TextureUsage::TextureBinding |
          TextureUsage::CopyDst,
      m_Format, device, &m_ImageView, &m_Sampler);

  m_DescriptorSet = (VkDescriptorSet)(ImTextureID)m_ImageView;
#endif // IMGUI_WGPU
}

void Image::Release() {
  m_Sampler = nullptr;
  m_ImageView = nullptr;
  m_Image = nullptr;
  m_StagingBuffer = nullptr;
}

void Image::SetData(const void *data) {
  Device device = Application::Get()->GetDevice();
  // size_pp: RGBA8Unorm = 4 bytes = 32 bits
  size_t size_pp = Utils::BytesPerPixel(m_Format);
  size_t upload_size = m_Width * m_Height * size_pp;

#ifdef IMGUI_WGPU

  {
    WGPUImageCopyTexture dst_view = {};
    dst_view.texture = m_Image;
    dst_view.mipLevel = 0;
    dst_view.origin = {0, 0, 0};
    dst_view.aspect = WGPUTextureAspect_All;
    WGPUTextureDataLayout layout = {};
    layout.offset = 0;
    layout.bytesPerRow = m_Width * size_pp; // 列数 * 像素尺寸
    layout.rowsPerImage = m_Height;         // 行数
    WGPUExtent3D size = {(uint32_t)m_Width, (uint32_t)m_Height, 1}; // size here
    // NOTE: write data to texture with specific size and layout
    wgpuQueueWriteTexture(device.getQueue(), &dst_view, data, upload_size,
                          &layout, &size);
  }

#endif // IMGUI_WGPU
}

void Image::Resize(uint32_t width, uint32_t height) {
  if (m_Image && m_Width == width && m_Height == height)
    return;

  // TODO: max size?

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

void Image::InitModel(uint32_t width, uint32_t height) {
  auto app = Application::Get();
  Device device = app->GetDevice();
  TextureFormat swapChainFormat = app->GetSwapChainFormat();

  TextureView tex_id = nullptr;
  Sampler sampler = nullptr;

  ResourceManager::initTexture(width, height,
                               TextureUsage::RenderAttachment |
                                   TextureUsage::TextureBinding |
                                   TextureUsage::CopyDst,
                               swapChainFormat, device, &tex_id, &sampler);
  ImGuiID tex_id_hash = ImHashData(&tex_id, sizeof(tex_id));
  auto found = app->m_TextureStorage.GetVoidPtr(tex_id_hash);
  if (!found) {
    app->m_TextureStorage.SetVoidPtr(tex_id_hash, tex_id);
    app->m_TextureIdSet.insert(tex_id);
  }
}

} // namespace Walnut
