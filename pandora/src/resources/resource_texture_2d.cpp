#include "resources/resource_texture_2d.hpp"

#include <algorithm>
#include <optional>
#include <span>

#include "core/log.hpp"
#include "pandora.hpp"
#include "render/mip_level_generator.hpp"
#include "render/rendersystem.hpp"
#include "render/texture_format.hpp"
#include "resources/private/ktx2_loader.hpp"
#include "resources/resource_system.hpp"

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

ResourceTexture2D::ResourceTexture2D(const std::string& label, ColorSpace colorSpace, const unsigned char* pData, size_t dataSize)
{
    LoadFromMemoryCompressed(label, colorSpace, pData, dataSize);
}

ResourceTexture2D::ResourceTexture2D(const std::string& label, ColorSpace colorSpace, const unsigned char* pData, size_t dataSize, uint32_t width, uint32_t height, uint32_t channels, MipLevels mipLevels)
{
    LoadFromMemoryUncompressed(label, colorSpace, pData, dataSize, width, height, channels, mipLevels);
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
        const unsigned char* pData = reinterpret_cast<const unsigned char*>(pFile->GetData().data());
        const size_t dataSize = pFile->GetData().size();

        // Dispatching on the header rather than the extension means textures embedded in
        // other files, which arrive as bare bytes, take the same route.
        if (IsKtx2(pData, dataSize))
        {
            // KTX2 records its own transfer function, so unlike the paths below this one
            // does not have to assume anything about the colour space.
            LoadFromMemoryKTX2(pFile->GetPath(), pData, dataSize);
        }
        else
        {
            // We currently assume that all textures loaded through the VFS are in sRGB.
            // This will be true in most cases, but we really should check the file header for colorspace information.
            LoadFromMemoryCompressed(pFile->GetPath(), ColorSpace::sRGB, pData, dataSize);
        }
    }
    else
    {
        SetState(ResourceState::Error);
    }
}

void ResourceTexture2D::LoadFromMemoryCompressed(const std::string& label, ColorSpace colorSpace, const unsigned char* pData, size_t dataSize)
{
    m_Channels = 4; // Setting desired channels to 4 as WGPU has no RGB8, just RGBA8.
    int width = 0;
    int height = 0;
    int channelsInMemory = 0;
    unsigned char* pTextureData = stbi_load_from_memory(
        reinterpret_cast<const stbi_uc*>(pData),
        dataSize,
        &width,
        &height,
        &channelsInMemory,
        m_Channels);

    m_Width = static_cast<uint32_t>(width);
    m_Height = static_cast<uint32_t>(height);
    const size_t textureDataSize = m_Width * m_Height * m_Channels;

    if (pTextureData)
    {
        const uint32_t mipLevelCount = GetRenderSystem()->GetMipLevelGenerator()->GetMipLevelCount(m_Width, m_Height);
        CreateTextureAndView(label, GetTextureFormat(colorSpace), mipLevelCount, true);

        wgpu::TexelCopyTextureInfo destination{
            .texture = m_Texture,
            .aspect = wgpu::TextureAspect::All
        };

        wgpu::TexelCopyBufferLayout sourceLayout{
            .offset = 0,
            .bytesPerRow = m_Channels * m_Width,
            .rowsPerImage = m_Height
        };

        wgpu::Extent3D size{ m_Width, m_Height, 1 };
        GetRenderSystem()->GetDevice().GetQueue().WriteTexture(&destination, pTextureData, textureDataSize, &sourceLayout, &size);

        GetRenderSystem()->GetMipLevelGenerator()->GenerateMips(m_Texture);

        stbi_image_free(pTextureData);
        SetState(ResourceState::Loaded);
    }
    else
    {
        SetState(ResourceState::Error);
        Log::Error() << "Failed to load texture '" << label << "': " << stbi_failure_reason();
    }
}

void ResourceTexture2D::LoadFromMemoryUncompressed(const std::string& label, ColorSpace colorSpace, const unsigned char* pData, size_t dataSize, uint32_t width, uint32_t height, uint32_t channels, MipLevels mipLevels)
{
    m_Width = width;
    m_Height = height;
    m_Channels = channels;

    // The smaller levels are rendered into the texture, so it only needs to be an attachment when
    // there are any to generate.
    const bool generateMips = (mipLevels == MipLevels::Generate);
    const uint32_t mipLevelCount = generateMips ? GetRenderSystem()->GetMipLevelGenerator()->GetMipLevelCount(width, height) : 1;
    CreateTextureAndView(label, GetTextureFormat(colorSpace), mipLevelCount, generateMips);

    wgpu::TexelCopyTextureInfo destination{
        .texture = m_Texture,
        .aspect = wgpu::TextureAspect::All
    };

    wgpu::TexelCopyBufferLayout sourceLayout{
        .offset = 0,
        .bytesPerRow = m_Channels * m_Width,
        .rowsPerImage = m_Height
    };

    wgpu::Extent3D size{ m_Width, m_Height, 1 };
    GetRenderSystem()->GetDevice().GetQueue().WriteTexture(&destination, pData, dataSize, &sourceLayout, &size);

    if (generateMips)
    {
        GetRenderSystem()->GetMipLevelGenerator()->GenerateMips(m_Texture);
    }

    SetState(ResourceState::Loaded);
}

void ResourceTexture2D::LoadFromMemoryKTX2(const std::string& label, const unsigned char* pData, size_t dataSize)
{
    std::optional<Ktx2Image> image = LoadKtx2(label, pData, dataSize);
    if (!image.has_value())
    {
        SetState(ResourceState::Error);
        return;
    }

    m_Width = image->GetWidth();
    m_Height = image->GetHeight();

    // m_Channels is left alone: a block compressed format has no per-texel channel
    // count, and nothing on this path needs one.
    const wgpu::TextureFormat format = image->GetFormat();
    const TextureFormatInfo formatInfo = GetTextureFormatInfo(format);

    // The mip chain comes out of the file, so there is nothing to render into the texture
    // and it must not be an attachment - block compressed formats cannot be one.
    CreateTextureAndView(label, format, image->GetMipLevelCount(), false);

    for (uint32_t level = 0; level < image->GetMipLevelCount(); level++)
    {
        const uint32_t levelWidth = std::max(1u, m_Width >> level);
        const uint32_t levelHeight = std::max(1u, m_Height >> level);

        // Strides are counted in blocks. For an uncompressed format the block is a single
        // texel, so the same arithmetic gives the usual bytes per row.
        const uint32_t blocksPerRow = (levelWidth + formatInfo.blockWidth - 1) / formatInfo.blockWidth;
        const uint32_t blockRows = (levelHeight + formatInfo.blockHeight - 1) / formatInfo.blockHeight;

        wgpu::TexelCopyTextureInfo destination{
            .texture = m_Texture,
            .mipLevel = level,
            .origin = { 0, 0, 0 },
            .aspect = wgpu::TextureAspect::All
        };

        wgpu::TexelCopyBufferLayout sourceLayout{
            .offset = 0,
            .bytesPerRow = blocksPerRow * formatInfo.bytesPerBlock,
            .rowsPerImage = blockRows
        };

        // The copy covers whole blocks, so the extent is the level's size rounded up to the
        // block grid. Mip levels of a compressed texture are addressed by that padded size,
        // not their logical one, and the file stores the padding to match.
        wgpu::Extent3D levelSize{
            blocksPerRow * formatInfo.blockWidth,
            blockRows * formatInfo.blockHeight,
            1
        };

        const std::span<const uint8_t> levelData = image->GetLevel(level);
        GetRenderSystem()->GetDevice().GetQueue().WriteTexture(&destination, levelData.data(), levelData.size(), &sourceLayout, &levelSize);
    }

    SetState(ResourceState::Loaded);
}

void ResourceTexture2D::CreateTextureAndView(const std::string& label, wgpu::TextureFormat format, uint32_t mipLevelCount, bool renderAttachment)
{
    wgpu::TextureUsage usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
    if (renderAttachment)
    {
        usage |= wgpu::TextureUsage::RenderAttachment;
    }

    wgpu::TextureDescriptor textureDescriptor{
        .label = label.c_str(),
        .usage = usage,
        .dimension = wgpu::TextureDimension::e2D,
        .size = { m_Width, m_Height, 1 },
        .format = format,
        .mipLevelCount = mipLevelCount,
        .sampleCount = 1
    };

    m_Texture = GetRenderSystem()->GetDevice().CreateTexture(&textureDescriptor);

    wgpu::TextureViewDescriptor textureViewDescriptor{
        .label = label.c_str(),
        .format = format,
        .dimension = wgpu::TextureViewDimension::e2D,
        .mipLevelCount = mipLevelCount,
        .arrayLayerCount = 1
    };
    m_TextureView = m_Texture.CreateView(&textureViewDescriptor);
}

wgpu::TextureFormat ResourceTexture2D::GetTextureFormat(ColorSpace colorSpace) const
{
    return (colorSpace == ColorSpace::Linear) ? wgpu::TextureFormat::RGBA8Unorm : wgpu::TextureFormat::RGBA8UnormSrgb;
}

} // namespace WingsOfSteel
