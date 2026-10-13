#include "render/texture_format.hpp"

namespace WingsOfSteel
{

TextureFormatInfo GetTextureFormatInfo(wgpu::TextureFormat format)
{
    using Format = wgpu::TextureFormat;

    auto block = [](uint32_t bytesPerBlock) {
        return TextureFormatInfo{ 4, 4, bytesPerBlock, true };
    };

    switch (format)
    {
    // BC1 and BC4 pack a 4x4 block into 8 bytes, the rest of the BC family into 16.
    case Format::BC1RGBAUnorm:
    case Format::BC1RGBAUnormSrgb:
    case Format::BC4RUnorm:
    case Format::BC4RSnorm:
        return block(8);

    case Format::BC2RGBAUnorm:
    case Format::BC2RGBAUnormSrgb:
    case Format::BC3RGBAUnorm:
    case Format::BC3RGBAUnormSrgb:
    case Format::BC5RGUnorm:
    case Format::BC5RGSnorm:
    case Format::BC7RGBAUnorm:
    case Format::BC7RGBAUnormSrgb:
        return block(16);

    case Format::ETC2RGB8Unorm:
    case Format::ETC2RGB8UnormSrgb:
    case Format::ETC2RGB8A1Unorm:
    case Format::ETC2RGB8A1UnormSrgb:
    case Format::EACR11Unorm:
    case Format::EACR11Snorm:
        return block(8);

    case Format::ETC2RGBA8Unorm:
    case Format::ETC2RGBA8UnormSrgb:
    case Format::EACRG11Unorm:
    case Format::EACRG11Snorm:
        return block(16);

    case Format::ASTC4x4Unorm:
    case Format::ASTC4x4UnormSrgb:
        return block(16);

    case Format::RGBA8Unorm:
    case Format::RGBA8UnormSrgb:
        return TextureFormatInfo{ 1, 1, 4, false };

    case Format::RG8Unorm:
        return TextureFormatInfo{ 1, 1, 2, false };

    case Format::R8Unorm:
        return TextureFormatInfo{ 1, 1, 1, false };

    default:
        // Anything else is uncompressed as far as callers are concerned. The 4 bytes
        // per texel default suits the RGBA8 formats the engine actually uses; a format
        // that needs otherwise should be given an entry above.
        return TextureFormatInfo{};
    }
}

wgpu::TextureFormat VkFormatToTextureFormat(uint32_t vkFormat)
{
    // libktx reports the format as a VkFormat. Only the ones it can transcode Basis
    // Universal payloads into are listed, plus the uncompressed formats for KTX2
    // files that carry plain texels. The values are from the Vulkan specification.
    switch (vkFormat)
    {
    case 9:
        return wgpu::TextureFormat::R8Unorm; // VK_FORMAT_R8_UNORM
    case 16:
        return wgpu::TextureFormat::RG8Unorm; // VK_FORMAT_R8G8_UNORM
    case 37:
        return wgpu::TextureFormat::RGBA8Unorm; // VK_FORMAT_R8G8B8A8_UNORM
    case 43:
        return wgpu::TextureFormat::RGBA8UnormSrgb; // VK_FORMAT_R8G8B8A8_SRGB

    // WebGPU has no BC1 without alpha, but the formats are bit-identical: BC1 always
    // decodes an alpha channel, and the RGB variant simply promises it is opaque.
    case 131:
        return wgpu::TextureFormat::BC1RGBAUnorm; // VK_FORMAT_BC1_RGB_UNORM_BLOCK
    case 132:
        return wgpu::TextureFormat::BC1RGBAUnormSrgb; // VK_FORMAT_BC1_RGB_SRGB_BLOCK
    case 133:
        return wgpu::TextureFormat::BC1RGBAUnorm; // VK_FORMAT_BC1_RGBA_UNORM_BLOCK
    case 134:
        return wgpu::TextureFormat::BC1RGBAUnormSrgb; // VK_FORMAT_BC1_RGBA_SRGB_BLOCK
    case 135:
        return wgpu::TextureFormat::BC2RGBAUnorm; // VK_FORMAT_BC2_UNORM_BLOCK
    case 136:
        return wgpu::TextureFormat::BC2RGBAUnormSrgb; // VK_FORMAT_BC2_SRGB_BLOCK
    case 137:
        return wgpu::TextureFormat::BC3RGBAUnorm; // VK_FORMAT_BC3_UNORM_BLOCK
    case 138:
        return wgpu::TextureFormat::BC3RGBAUnormSrgb; // VK_FORMAT_BC3_SRGB_BLOCK
    case 139:
        return wgpu::TextureFormat::BC4RUnorm; // VK_FORMAT_BC4_UNORM_BLOCK
    case 140:
        return wgpu::TextureFormat::BC4RSnorm; // VK_FORMAT_BC4_SNORM_BLOCK
    case 141:
        return wgpu::TextureFormat::BC5RGUnorm; // VK_FORMAT_BC5_UNORM_BLOCK
    case 142:
        return wgpu::TextureFormat::BC5RGSnorm; // VK_FORMAT_BC5_SNORM_BLOCK
    case 145:
        return wgpu::TextureFormat::BC7RGBAUnorm; // VK_FORMAT_BC7_UNORM_BLOCK
    case 146:
        return wgpu::TextureFormat::BC7RGBAUnormSrgb; // VK_FORMAT_BC7_SRGB_BLOCK

    case 147:
        return wgpu::TextureFormat::ETC2RGB8Unorm; // VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK
    case 148:
        return wgpu::TextureFormat::ETC2RGB8UnormSrgb; // VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK
    case 149:
        return wgpu::TextureFormat::ETC2RGB8A1Unorm; // VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK
    case 150:
        return wgpu::TextureFormat::ETC2RGB8A1UnormSrgb; // VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK
    case 151:
        return wgpu::TextureFormat::ETC2RGBA8Unorm; // VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK
    case 152:
        return wgpu::TextureFormat::ETC2RGBA8UnormSrgb; // VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK
    case 153:
        return wgpu::TextureFormat::EACR11Unorm; // VK_FORMAT_EAC_R11_UNORM_BLOCK
    case 154:
        return wgpu::TextureFormat::EACR11Snorm; // VK_FORMAT_EAC_R11_SNORM_BLOCK
    case 155:
        return wgpu::TextureFormat::EACRG11Unorm; // VK_FORMAT_EAC_R11G11_UNORM_BLOCK
    case 156:
        return wgpu::TextureFormat::EACRG11Snorm; // VK_FORMAT_EAC_R11G11_SNORM_BLOCK

    case 157:
        return wgpu::TextureFormat::ASTC4x4Unorm; // VK_FORMAT_ASTC_4x4_UNORM_BLOCK
    case 158:
        return wgpu::TextureFormat::ASTC4x4UnormSrgb; // VK_FORMAT_ASTC_4x4_SRGB_BLOCK

    default:
        return wgpu::TextureFormat::Undefined;
    }
}

std::optional<wgpu::FeatureName> RequiredFeatureFor(wgpu::TextureFormat format)
{
    // WebGPU gates each compressed family behind one feature covering all of its
    // formats, so a device with BC7 also has BC4 and BC5, and a device with ETC2
    // also has the EAC formats. Nothing here needs to test them individually.
    const TextureFormatInfo info = GetTextureFormatInfo(format);
    if (!info.isCompressed)
    {
        return std::nullopt;
    }

    using Format = wgpu::TextureFormat;
    switch (format)
    {
    case Format::ASTC4x4Unorm:
    case Format::ASTC4x4UnormSrgb:
        return wgpu::FeatureName::TextureCompressionASTC;

    case Format::ETC2RGB8Unorm:
    case Format::ETC2RGB8UnormSrgb:
    case Format::ETC2RGB8A1Unorm:
    case Format::ETC2RGB8A1UnormSrgb:
    case Format::ETC2RGBA8Unorm:
    case Format::ETC2RGBA8UnormSrgb:
    case Format::EACR11Unorm:
    case Format::EACR11Snorm:
    case Format::EACRG11Unorm:
    case Format::EACRG11Snorm:
        return wgpu::FeatureName::TextureCompressionETC2;

    default:
        // Everything else GetTextureFormatInfo reports as compressed is BC.
        return wgpu::FeatureName::TextureCompressionBC;
    }
}

} // namespace WingsOfSteel
