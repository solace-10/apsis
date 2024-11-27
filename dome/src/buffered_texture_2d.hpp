#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <webgpu/webgpu_cpp.h>

#include <core/smart_ptr.hpp>

namespace WingsOfSteel::Dome
{

DECLARE_SMART_PTR(BufferTexture2D);
class BufferedTexture2D
{
public:
    BufferedTexture2D(const std::string& label);
    ~BufferedTexture2D();

    bool Load(const std::filesystem::path& path);

    wgpu::TextureView GetTextureView() const;

    unsigned char* GetData(size_t& dataSize, int& width, int& height, int& channels);

private:
    bool LoadFromMemory(const char* pData, size_t dataSize);

    std::string m_Label;
    wgpu::Texture m_Texture;
    wgpu::TextureView m_TextureView;
    int m_Width{ 0 };
    int m_Height{ 0 };
    int m_Channels{ 0 };
    unsigned char* m_pTextureData{ nullptr };
};

} // namespace WingsOfSteel::Dome