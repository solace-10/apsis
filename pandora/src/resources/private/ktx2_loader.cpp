#include "resources/private/ktx2_loader.hpp"

#include <algorithm>
#include <cstring>

#include <ktx.h>
#include <magic_enum.hpp>

#include "core/log.hpp"
#include "pandora.hpp"
#include "render/rendersystem.hpp"
#include "render/texture_format.hpp"

namespace WingsOfSteel
{

// Owns the ktxTexture2 the level spans point into.
struct Ktx2Image::Payload
{
    ~Payload()
    {
        if (pTexture)
        {
            ktxTexture2_Destroy(pTexture);
        }
    }

    ktxTexture2* pTexture{ nullptr };
};

Ktx2Image::Ktx2Image() = default;
Ktx2Image::~Ktx2Image() = default;
Ktx2Image::Ktx2Image(Ktx2Image&&) noexcept = default;
Ktx2Image& Ktx2Image::operator=(Ktx2Image&&) noexcept = default;

bool IsKtx2(const uint8_t* pData, size_t dataSize)
{
    static const uint8_t identifier[12] = { 0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32,
        0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A };
    return pData != nullptr && dataSize >= sizeof(identifier) && memcmp(pData, identifier, sizeof(identifier)) == 0;
}

namespace
{

    // Picks what a Basis Universal payload should become on this device.
    //
    // The channel count matters as much as the device does. A Basis payload records
    // how many channels it actually carries, and transcoding a one or two channel
    // texture into a four channel format spends four times the memory on data that
    // is not there. Worse, it moves the channels: a two channel payload is stored
    // with X in the RGB slice and Y in the alpha slice, so a four channel target
    // hands back X in .rgb and Y in .a, and a shader reading .rg silently gets
    // (X, X). Only BC5 and EAC RG11 put them back where they belong.
    //
    // Within a channel count the order is quality first, with RGBA32 as a last
    // resort that gives up compression rather than failing to display at all.
    // Returns nothing when there is no target that would be correct.
    std::optional<ktx_transcode_fmt_e> SelectTranscodeFormat(uint32_t components)
    {
        RenderSystem* pRenderSystem = GetRenderSystem();
        const bool hasBC = pRenderSystem->HasFeature(wgpu::FeatureName::TextureCompressionBC);
        const bool hasASTC = pRenderSystem->HasFeature(wgpu::FeatureName::TextureCompressionASTC);
        const bool hasETC2 = pRenderSystem->HasFeature(wgpu::FeatureName::TextureCompressionETC2);

        if (components == 1)
        {
            if (hasBC)
                return KTX_TTF_BC4_R;
            if (hasETC2)
                return KTX_TTF_ETC2_EAC_R11;

            // RGBA32 replicates the single channel across RGB, so .r still reads
            // correctly. It costs eight times the memory of BC4, but it displays.
            return KTX_TTF_RGBA32;
        }
        else if (components == 2)
        {
            if (hasBC)
                return KTX_TTF_BC5_RG;
            if (hasETC2)
                return KTX_TTF_ETC2_EAC_RG11;

            // There is deliberately no fallback. libktx offers no uncompressed one
            // or two channel target - the CLI's r8 and rg8 are its own conversion,
            // not a transcode format - so the only option left would be RGBA32,
            // which moves Y into alpha. A normal map that fails to load is far
            // easier to diagnose than one that lights the world slightly wrong.
            return std::nullopt;
        }

        if (hasBC)
            return KTX_TTF_BC7_RGBA;
        if (hasASTC)
            return KTX_TTF_ASTC_4x4_RGBA;
        if (hasETC2)
            return KTX_TTF_ETC2_RGBA;

        return KTX_TTF_RGBA32;
    }

} // namespace

std::optional<Ktx2Image> LoadKtx2(const std::string& label, const uint8_t* pData, size_t dataSize)
{
    ktxTexture2* pTexture = nullptr;

    // LOAD_IMAGE_DATA_BIT reads the levels into memory and, if the file is
    // supercompressed, inflates them.
    KTX_error_code result = ktxTexture2_CreateFromMemory(
        pData,
        dataSize,
        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
        &pTexture);

    if (result != KTX_SUCCESS)
    {
        Log::Error() << "Failed to read KTX2 texture '" << label << "': " << ktxErrorString(result);
        return std::nullopt;
    }

    Ktx2Image image;
    image.m_pPayload = std::make_unique<Ktx2Image::Payload>();
    image.m_pPayload->pTexture = pTexture;

    image.m_Width = pTexture->baseWidth;
    image.m_Height = pTexture->baseHeight;

    if (ktxTexture2_NeedsTranscoding(pTexture))
    {
        const uint32_t components = ktxTexture2_GetNumComponents(pTexture);
        const std::optional<ktx_transcode_fmt_e> selected = SelectTranscodeFormat(components);
        if (!selected.has_value())
        {
            Log::Error() << "KTX2 texture '" << label << "' carries " << components
                         << " channels, which this device has no format for. It needs either "
                         << "TextureCompressionBC or TextureCompressionETC2.";
            return std::nullopt;
        }

        const ktx_transcode_fmt_e transcodeFormat = selected.value();

        // WebGPU requires a compressed texture's dimensions to be a multiple of the
        // block size, which a transcode cannot fix. Falling back to uncompressed keeps
        // an oddly sized texture displaying rather than failing device validation.
        // Two channel payloads are excluded: RGBA32 would move Y into alpha, so a
        // badly sized one is refused rather than quietly rearranged.
        const bool isBlockFormat = (transcodeFormat != KTX_TTF_RGBA32);
        if (isBlockFormat && (image.m_Width % 4 != 0 || image.m_Height % 4 != 0))
        {
            if (components == 2)
            {
                Log::Error() << "KTX2 texture '" << label << "' is " << image.m_Width << "x" << image.m_Height
                             << ", which is not a multiple of the 4x4 block size. A two channel texture has no "
                             << "uncompressed format to fall back to.";
                return std::nullopt;
            }

            Log::Warning() << "KTX2 texture '" << label << "' is " << image.m_Width << "x" << image.m_Height
                           << ", which is not a multiple of the 4x4 block size. Falling back to uncompressed.";
            result = ktxTexture2_TranscodeBasis(pTexture, KTX_TTF_RGBA32, 0);
        }
        else
        {
            result = ktxTexture2_TranscodeBasis(pTexture, transcodeFormat, 0);
        }

        if (result != KTX_SUCCESS)
        {
            Log::Error() << "Failed to transcode KTX2 texture '" << label << "': " << ktxErrorString(result);
            return std::nullopt;
        }
    }

    image.m_Format = VkFormatToTextureFormat(pTexture->vkFormat);
    if (image.m_Format == wgpu::TextureFormat::Undefined)
    {
        Log::Error() << "KTX2 texture '" << label << "' has VkFormat " << pTexture->vkFormat
                     << ", which has no WebGPU equivalent.";
        return std::nullopt;
    }

    // A file that carried its block compressed payload directly has had no transcode
    // step to pick a format the device can take, so the check happens here instead.
    // Without it an unsupported format reaches CreateTexture and fails as a device
    // error, well away from the file that caused it.
    const std::optional<wgpu::FeatureName> requiredFeature = RequiredFeatureFor(image.m_Format);
    if (requiredFeature.has_value() && !GetRenderSystem()->HasFeature(requiredFeature.value()))
    {
        Log::Error() << "KTX2 texture '" << label << "' is " << magic_enum::enum_name(image.m_Format)
                     << ", which this device cannot use: it lacks "
                     << magic_enum::enum_name(requiredFeature.value()) << ".";
        return std::nullopt;
    }

    const uint8_t* pLevelData = ktxTexture_GetData(ktxTexture(pTexture));
    const size_t totalSize = ktxTexture_GetDataSize(ktxTexture(pTexture));

    image.m_Levels.reserve(pTexture->numLevels);
    for (uint32_t level = 0; level < pTexture->numLevels; level++)
    {
        ktx_size_t offset = 0;
        result = ktxTexture_GetImageOffset(ktxTexture(pTexture), level, 0, 0, &offset);
        if (result != KTX_SUCCESS)
        {
            Log::Error() << "Failed to locate mip level " << level << " of KTX2 texture '" << label
                         << "': " << ktxErrorString(result);
            return std::nullopt;
        }

        const size_t levelSize = ktxTexture_GetImageSize(ktxTexture(pTexture), level);
        if (offset + levelSize > totalSize)
        {
            Log::Error() << "Mip level " << level << " of KTX2 texture '" << label << "' runs past the end of the file.";
            return std::nullopt;
        }

        image.m_Levels.emplace_back(pLevelData + offset, levelSize);
    }

    Log::Info() << "Loaded KTX2 texture '" << label << "': " << image.m_Width << "x" << image.m_Height
                << ", " << image.m_Levels.size() << " mip levels, " << magic_enum::enum_name(image.m_Format) << ".";

    return image;
}

} // namespace WingsOfSteel
