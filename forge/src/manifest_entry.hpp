#pragma once

#include <string>

#include "core/smart_ptr.hpp"

namespace WingsOfSteel
{

enum class ManifestEntryMode
{
    Background,
    Preload
};

DECLARE_SMART_PTR(ManifestEntry);
class ManifestEntry
{
public:
    ManifestEntry(const std::string& path, uint64_t size, const std::string& hash, ManifestEntryMode mode)
    : m_Path(path)
    , m_Size(size)
    , m_Hash(hash)
    , m_Mode(mode)
    {}

    ~ManifestEntry() {}

    const std::string& GetPath() const { return m_Path; }
    uint64_t GetSize() const { return m_Size; }
    const std::string& GetHash() const { return m_Hash; }
    ManifestEntryMode GetMode() const { return m_Mode; }
    void SetMode(ManifestEntryMode mode) { m_Mode = mode; }

private:
    std::string m_Path;
    uint64_t m_Size;
    std::string m_Hash;
    ManifestEntryMode m_Mode;
};

} // namespace WingsOfSteel