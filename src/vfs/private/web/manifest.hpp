#pragma once

#if defined(TARGET_PLATFORM_WEB)

#include <optional>
#include <string>
#include <unordered_map>

namespace WingsOfSteel::Private
{

class ManifestEntry;
using ManifestData = std::unordered_map<std::string, ManifestEntry>;

class Manifest
{
public:
    Manifest();
    ~Manifest();

    bool Initialize();

    bool ContainsEntry(const std::string& path) const;
    const ManifestEntry* GetEntry(const std::string& path) const;
    const ManifestData& GetEntries() const;    

private:
    ManifestData m_ManifestData;
};

} // namespace WingsOfSteel::Private

#endif // TARGET_PLATFORM_WEB
