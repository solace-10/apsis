#pragma once

#if defined(TARGET_PLATFORM_WEB)

#include <string>

namespace WingsOfSteel::Pandora::Private
{

class ManifestEntry
{
public:
    ManifestEntry(const std::string& path, const std::string& hash, uint64_t size);
    ~ManifestEntry() {}

    const std::string& GetPath() const;
    const std::string& GetHash() const;
    const uint64_t GetSize() const;

private:
    std::string m_Path;
    std::string m_Hash;
    uint64_t m_Size;
};

} // namespace WingsOfSteel::Pandora::Private

#endif // TARGET_PLATFORM_WEB