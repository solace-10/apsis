#include "resources/resource_texture_2d.hpp"

#include "core/log.hpp"
#include "pandora.hpp"
#include "render/rendersystem.hpp"

// clang-format off
#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG // Better error messages.
#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#include <stb_image.h>
// clang-format on

namespace WingsOfSteel
{

ResourceTexture2D::ResourceTexture2D(const std::string& label, const unsigned char* pData, size_t dataSize)
{
    LoadFromMemory(label, pData, dataSize);
}

ResourceTexture2D::~ResourceTexture2D()
{
    if (m_Texture)
    {
        m_Texture.Destroy();
    }
}

void ResourceTexture2D::Load(const std::string& path)
{
    Resource::Load(path);

    GetVFS()->FileRead(path,
        [this](FileReadResult result, FileSharedPtr pFile) {
            this->LoadInternal(result, pFile);
        });
}

ResourceType ResourceTexture2D::GetResourceType() const
{
    return ResourceType::Texture2D;
}

wgpu::TextureView ResourceTexture2D::GetTextureView() const
{
    return m_TextureView;
}

void ResourceTexture2D::LoadInternal(FileReadResult result, FileSharedPtr pFile)
{
    if (result == FileReadResult::Ok)
    {
        LoadFromMemory(pFile->GetPath(), reinterpret_cast<const unsigned char*>(pFile->GetData().data()), pFile->GetData().size());
    }
    else
    {
        SetState(ResourceState::Error);
    }
}

void ResourceTexture2D::LoadFromMemory(const std::string& label, const unsigned char* pData, size_t dataSize)
{
    m_Channels = 4; // Setting desired channels to 4 as WGPU has no RGB8, just RGBA8.
    int channelsInMemory;
    unsigned char* pTextureData = stbi_load_from_memory(
        reinterpret_cast<const stbi_uc*>(pData),
        dataSize,
        &m_Width,
        &m_Height,
        &channelsInMemory,
        m_Channels);
    const size_t textureDataSize = m_Width * m_Height * m_Channels;

    if (pTextureData)
    {
        // TODO: Convert to GPU texture here.
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
        GetRenderSystem()->GetDevice().GetQueue().WriteTexture(&destination, pTextureData, textureDataSize, &sourceLayout, &textureDescriptor.size);

        wgpu::TextureViewDescriptor textureViewDescriptor{
            .label = label.c_str(),
            .format = textureDescriptor.format,
            .dimension = wgpu::TextureViewDimension::e2D,
            .mipLevelCount = textureDescriptor.mipLevelCount,
            .arrayLayerCount = 1
        };
        m_TextureView = m_Texture.CreateView(&textureViewDescriptor);

        stbi_image_free(pTextureData);
        SetState(ResourceState::Loaded);
    }
    else
    {
        SetState(ResourceState::Error);
        Log::Error() << "Failed to load texture '" << label << "': " << stbi_failure_reason();
    }
}

} // namespace WingsOfSteel
