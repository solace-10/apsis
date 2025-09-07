#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <webgpu/webgpu_cpp.h>

#include <core/smart_ptr.hpp>

namespace WingsOfSteel
{

using TextureData = std::vector<unsigned char>;

DECLARE_SMART_PTR(BufferTexture2D);
class BufferedTexture2D
{
public:
    BufferedTexture2D(const std::string& label);
    BufferedTexture2D(const std::string& label, const BufferedTexture2D* pSourceTexture);
    ~BufferedTexture2D();

    bool Load(const std::filesystem::path& path);
    void Rebuild();

    wgpu::TextureView GetTextureView() const;

    int GetWidth() const;
    int GetHeight() const;
    int GetChannels() const;
    TextureData& GetTextureData();
    const TextureData& GetTextureData() const;

private:
    std::string m_Label;
    wgpu::Texture m_Texture;
    wgpu::TextureView m_TextureView;
    int m_Width{ 0 };
    int m_Height{ 0 };
    int m_Channels{ 0 };
    TextureData m_TextureData;
};

inline int BufferedTexture2D::GetWidth() const
{
    return m_Width;
}

inline int BufferedTexture2D::GetHeight() const
{
    return m_Height;
}

inline int BufferedTexture2D::GetChannels() const
{
    return m_Channels;
}

inline TextureData& BufferedTexture2D::GetTextureData()
{
    return m_TextureData;
}

inline const TextureData& BufferedTexture2D::GetTextureData() const
{
    return m_TextureData;
}

} // namespace WingsOfSteel