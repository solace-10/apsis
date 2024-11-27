#include "buffered_texture_2d.hpp"

#include <core/log.hpp>
#include <render/rendersystem.hpp>
#include <pandora.hpp>

#include <stb_image.h>

namespace WingsOfSteel::Dome
{

BufferedTexture2D::BufferedTexture2D(const std::string& label)
: m_Label(label)
{
}

BufferedTexture2D::~BufferedTexture2D()
{
    if (m_Texture)
    {
        m_Texture.Destroy();
    }

    if (m_pTextureData)
    {
        stbi_image_free(m_pTextureData);
    }
}

bool BufferedTexture2D::Load(const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path)) 
    {
        return false;
    }

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) 
    {
        return false;
    }

    std::streamsize size = file.tellg();
    if (size <= 0) 
    {
        return false;
    }

    std::vector<char> buffer;
    buffer.resize(static_cast<size_t>(size));
    file.seekg(0, std::ios::beg);
    if (!file.read(buffer.data(), size))
    {
        return false;
    }

    return LoadFromMemory(buffer.data(), buffer.size());
}

wgpu::TextureView BufferedTexture2D::GetTextureView() const
{
    return m_TextureView;
}

bool BufferedTexture2D::LoadFromMemory(const char* pData, size_t dataSize)
{
    using namespace Pandora;

    m_Channels = 4; // Setting desired channels to 4 as WGPU has no RGB8, just RGBA8.
    int channelsInMemory;
    m_pTextureData = stbi_load_from_memory(
        reinterpret_cast<const stbi_uc*>(pData),
        dataSize,
        &m_Width,
        &m_Height,
        &channelsInMemory,
        m_Channels 
    );
    const size_t textureDataSize = m_Width * m_Height * m_Channels;

    if (m_pTextureData)
    {
        wgpu::TextureDescriptor textureDescriptor{
            .usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst,
            .dimension = wgpu::TextureDimension::e2D,
            .size = { static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height), 1 },
            .format = wgpu::TextureFormat::RGBA8Unorm,
            .mipLevelCount = 1,
            .sampleCount = 1
        };

        m_Texture = GetRenderSystem()->GetDevice().CreateTexture(&textureDescriptor);

        wgpu::ImageCopyTexture destination{
            .texture = m_Texture,
            .aspect = wgpu::TextureAspect::All
        };

        wgpu::TextureDataLayout sourceLayout{
            .offset = 0,
            .bytesPerRow = m_Channels * static_cast<uint32_t>(m_Width),
            .rowsPerImage = static_cast<uint32_t>(m_Height)
        };

        const size_t textureDataSize = m_Width * m_Height * m_Channels;
        GetRenderSystem()->GetDevice().GetQueue().WriteTexture(&destination, m_pTextureData, textureDataSize, &sourceLayout, &textureDescriptor.size);

        wgpu::TextureViewDescriptor textureViewDescriptor{
            .label = m_Label.c_str(),
            .format = textureDescriptor.format,
            .dimension = wgpu::TextureViewDimension::e2D,
            .mipLevelCount = textureDescriptor.mipLevelCount,
            .arrayLayerCount = 1
        };
        m_TextureView = m_Texture.CreateView(&textureViewDescriptor);
        return true;
    }
    else
    {
        Log::Error() << "Failed to load texture '" << m_Label << "': " << stbi_failure_reason();
        return false;
    }
}

unsigned char* BufferedTexture2D::GetData(size_t& dataSize, int& width, int& height, int& channels)
{
    dataSize = m_Width * m_Height * m_Channels;
    width = m_Width;
    height = m_Height;
    channels = m_Channels;
    return m_pTextureData;
}

} // namespace WingsOfSteel::Dome
