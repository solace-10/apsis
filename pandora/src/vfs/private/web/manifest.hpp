#pragma once

#if defined(TARGET_PLATFORM_WEB)

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace WingsOfSteel::Pandora::Private
{

class ManifestEntry;

class Manifest
{
public:
    Manifest();
    ~Manifest();
    
    void Initialize();

    bool IsValid() const;
    ManifestEntry* GetEntry(const std::string& path) const;

    void OnDownloadSucceeded(const std::string& url, const char* pData, size_t dataSize);
    void OnDownloadFailed(const std::string& url, int statusCode);

private:
    bool m_IsValid;
    using ManifestEntryUniquePtr = std::unique_ptr<ManifestEntry>;
    using ManifestData = std::unordered_map<std::string, ManifestEntryUniquePtr>;
    ManifestData m_ManifestData;
};

} // namespace WingsOfSteel::Pandora::Private

#endif // TARGET_PLATFORM_WEB