#if defined(TARGET_PLATFORM_WEB)

#include "vfs/private/web/manifest_entry.hpp"

namespace WingsOfSteel::Pandora::Private
{

ManifestEntry::ManifestEntry(const std::string& path, const std::string& hash, uint64_t size)
    : m_Path(path)
    , m_Hash(hash)
    , m_Size(size)
{
}

const std::string& ManifestEntry::GetPath() const
{
    return m_Path;
}

const std::string& ManifestEntry::GetHash() const
{
    return m_Hash;
}

const uint64_t ManifestEntry::GetSize() const
{
    return m_Size;
}

} // namespace WingsOfSteel::Pandora::Private

#endif // TARGET_PLATFORM_WEB