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

BufferedTexture2D::BufferedTexture2D(const std::string& label, const BufferedTexture2D* pSourceTexture)
: m_Label(label)
{
    m_Width = pSourceTexture->GetWidth();
    m_Height = pSourceTexture->GetHeight();
    m_Channels = pSourceTexture->GetChannels();
    m_TextureData = pSourceTexture->GetTextureData();
}

BufferedTexture2D::~BufferedTexture2D()
{
    if (m_Texture)
    {
        m_Texture.Destroy();
    }
}

bool BufferedTexture2D::Load(const std::filesystem::path& path)
{
    using namespace Pandora;

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

    m_Channels = 4; // Setting desired channels to 4 as WGPU has no RGB8, just RGBA8.
    int channelsInMemory;
    stbi_uc* pUncompressedData = stbi_load_from_memory(
        reinterpret_cast<const stbi_uc*>(buffer.data()),
        buffer.size(),
        &m_Width,
        &m_Height,
        &channelsInMemory,
        m_Channels 
    );
    
    if (pUncompressedData == nullptr)
    {
        Log::Error() << "Failed to load texture '" << m_Label << "': " << stbi_failure_reason();
        return false;
    }
    else
    {
        const size_t textureDataSize = m_Width * m_Height * m_Channels;
        m_TextureData.resize(textureDataSize);
        memcpy(m_TextureData.data(), pUncompressedData, textureDataSize);
        stbi_image_free(pUncompressedData);
        Rebuild();
        return true;
    }
}

wgpu::TextureView BufferedTexture2D::GetTextureView() const
{
    return m_TextureView;
}

void BufferedTexture2D::Rebuild()
{
    using namespace Pandora;

    if (m_Texture)
    {
        m_Texture.Destroy();
    }

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

    GetRenderSystem()->GetDevice().GetQueue().WriteTexture(&destination, m_TextureData.data(), m_TextureData.size(), &sourceLayout, &textureDescriptor.size);

    wgpu::TextureViewDescriptor textureViewDescriptor{
        .label = m_Label.c_str(),
        .format = textureDescriptor.format,
        .dimension = wgpu::TextureViewDimension::e2D,
        .mipLevelCount = textureDescriptor.mipLevelCount,
        .arrayLayerCount = 1
    };
    m_TextureView = m_Texture.CreateView(&textureViewDescriptor);
}

} // namespace WingsOfSteel::Dome
