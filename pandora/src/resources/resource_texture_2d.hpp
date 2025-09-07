#pragma once

#include "resources/resource.hpp"

#include <webgpu/webgpu_cpp.h>

namespace WingsOfSteel
{

class ResourceTexture2D : public Resource
{
public:
    ResourceTexture2D() {}
    ResourceTexture2D(const std::string& label, const unsigned char* pData, size_t dataSize);
    ~ResourceTexture2D() override;

    void Load(const std::string& path) override;
    ResourceType GetResourceType() const override;

    wgpu::TextureView GetTextureView() const;

    int GetWidth() const;
    int GetHeight() const;

private:
    void LoadInternal(FileReadResult result, FileSharedPtr pFile);
    void LoadFromMemory(const std::string& label, const unsigned char* pData, size_t dataSize);

    wgpu::Texture m_Texture;
    wgpu::TextureView m_TextureView;
    int m_Width{ 0 };
    int m_Height{ 0 };
    int m_Channels{ 0 };
};

inline int ResourceTexture2D::GetWidth() const
{
    return m_Width;
}

inline int ResourceTexture2D::GetHeight() const
{
    return m_Height;
}

} // namespace WingsOfSteel