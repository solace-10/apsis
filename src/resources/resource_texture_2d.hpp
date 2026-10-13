#pragma once

#include "resources/resource.hpp"

#include <webgpu/webgpu_cpp.h>

#include "render/color_space.hpp"

namespace WingsOfSteel
{

// Whether a texture gets a full mip chain. Generating one costs a render pass per level, which is
// wasted on a texture only ever shown at a fixed size - a debug view, say.
enum class MipLevels
{
    Generate,
    None
};

class ResourceTexture2D : public Resource
{
public:
    ResourceTexture2D() {}
    ResourceTexture2D(const std::string& label, ColorSpace colorSpace, const unsigned char* pData, size_t dataSize); // Construct from compressed data.
    ResourceTexture2D(const std::string& label, ColorSpace colorSpace, const unsigned char* pData, size_t dataSize, uint32_t width, uint32_t height, uint32_t channels, MipLevels mipLevels = MipLevels::Generate); // Construct from uncompressed data.
    ~ResourceTexture2D() override;

    void Load(const std::string& path) override;
    ResourceType GetResourceType() const override;

    wgpu::TextureView GetTextureView() const;

    uint32_t GetWidth() const;
    uint32_t GetHeight() const;

private:
    void LoadInternal(FileReadResult result, FileSharedPtr pFile);
    void LoadFromMemoryCompressed(const std::string& label, ColorSpace colorSpace, const unsigned char* pData, size_t dataSize);
    void LoadFromMemoryUncompressed(const std::string& label, ColorSpace colorSpace, const unsigned char* pData, size_t dataSize, uint32_t width, uint32_t height, uint32_t channels, MipLevels mipLevels);
    void LoadFromMemoryKTX2(const std::string& label, const unsigned char* pData, size_t dataSize);
    wgpu::TextureFormat GetTextureFormat(ColorSpace colorSpace) const;
    void CreateTextureAndView(const std::string& label, wgpu::TextureFormat format, uint32_t mipLevelCount, bool renderAttachment);

    wgpu::Texture m_Texture;
    wgpu::TextureView m_TextureView;
    uint32_t m_Width{ 0 };
    uint32_t m_Height{ 0 };
    uint32_t m_Channels{ 0 };
};

inline uint32_t ResourceTexture2D::GetWidth() const
{
    return m_Width;
}

inline uint32_t ResourceTexture2D::GetHeight() const
{
    return m_Height;
}

} // namespace WingsOfSteel
