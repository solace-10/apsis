#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <webgpu/webgpu_cpp.h>

namespace WingsOfSteel
{

// A decoded KTX2 file, ready to be uploaded a mip level at a time.
//
// Basis Universal payloads have already been transcoded to a format the device
// supports by the time this is handed back, so the format is always one the GPU
// can take directly.
class Ktx2Image
{
public:
    Ktx2Image();
    ~Ktx2Image();
    Ktx2Image(Ktx2Image&&) noexcept;
    Ktx2Image& operator=(Ktx2Image&&) noexcept;
    Ktx2Image(const Ktx2Image&) = delete;
    Ktx2Image& operator=(const Ktx2Image&) = delete;

    wgpu::TextureFormat GetFormat() const { return m_Format; }
    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }
    uint32_t GetMipLevelCount() const { return static_cast<uint32_t>(m_Levels.size()); }

    // Level 0 is the full size image, each subsequent level half the size of the last.
    std::span<const uint8_t> GetLevel(uint32_t level) const { return m_Levels[level]; }

private:
    friend std::optional<Ktx2Image> LoadKtx2(const std::string&, const uint8_t*, size_t);

    struct Payload;

    std::unique_ptr<Payload> m_pPayload;
    std::vector<std::span<const uint8_t>> m_Levels;
    wgpu::TextureFormat m_Format{ wgpu::TextureFormat::Undefined };
    uint32_t m_Width{ 0 };
    uint32_t m_Height{ 0 };
};

// Whether the data begins with the KTX2 file identifier.
bool IsKtx2(const uint8_t* pData, size_t dataSize);

// Decodes a KTX2 file, transcoding it if it carries a Basis Universal payload.
// Returns nothing, having logged why, if the file cannot be represented on this device.
std::optional<Ktx2Image> LoadKtx2(const std::string& label, const uint8_t* pData, size_t dataSize);

} // namespace WingsOfSteel
