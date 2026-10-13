#pragma once

#include <cstdint>
#include <optional>

#include <webgpu/webgpu_cpp.h>

namespace WingsOfSteel
{

// Describes how a texture format lays its texels out in memory.
//
// Uncompressed formats are a degenerate case of the same thing: a 1x1 block
// whose size is the size of a texel, which lets upload code compute row strides
// without caring whether it is dealing with blocks or pixels.
struct TextureFormatInfo
{
    uint32_t blockWidth{ 1 };
    uint32_t blockHeight{ 1 };
    uint32_t bytesPerBlock{ 4 };
    bool isCompressed{ false };
};

TextureFormatInfo GetTextureFormatInfo(wgpu::TextureFormat format);

// Maps the Vulkan format libktx reports to its WebGPU equivalent, returning
// Undefined for anything the engine has no format for.
wgpu::TextureFormat VkFormatToTextureFormat(uint32_t vkFormat);

// The device feature a format needs before a texture can be created with it, or
// nothing for the formats every device is required to support.
//
// A file carrying an already compressed payload has no transcode step to pick a
// supported format for it, so whoever loads one has to ask this first.
std::optional<wgpu::FeatureName> RequiredFeatureFor(wgpu::TextureFormat format);

} // namespace WingsOfSteel
