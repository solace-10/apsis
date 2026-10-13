#if defined(TARGET_PLATFORM_WEB)

#include <iostream>
#include <sstream>

#include <emscripten/fetch.h>
#include <nlohmann/json.hpp>

#include "core/log.hpp"
#include "vfs/private/web/manifest.hpp"
#include "vfs/private/web/manifest_entry.hpp"

namespace WingsOfSteel::Private
{

Manifest::Manifest()
{
}

Manifest::~Manifest()
{
}

bool Manifest::Initialize()
{
    using namespace nlohmann;

    std::ifstream manifestFile("manifest.json");
    if (!manifestFile.is_open())
    {
        Log::Error() << "Failed to open embedded manifest file.";
        return false;
    }

    Log::Info() << "Manifest file is present.";

    const json jsonContents = json::parse(manifestFile);
    for (const auto& element : jsonContents)
    {
        if (element.is_object())
        {
            std::optional<std::string> path;
            std::optional<std::string> hash;
            std::optional<int> size;

            auto pathIt = element.find("path");
            if (pathIt != element.end() && pathIt->is_string())
            {
                path = pathIt->get<std::string>();
            }
            else
            {
                Log::Error() << "Manifest serialization: failed to load an element's path.";
                return false;
            }

            auto hashIt = element.find("hash");
            if (hashIt != element.end() && hashIt->is_string())
            {
                hash = hashIt->get<std::string>();
            }
            else
            {
                Log::Error() << "Manifest serialization: failed to load hash for file '" << path.value_or("UNKNOWN") << "'.";
                return false;
            }

            auto sizeIt = element.find("size");
            if (sizeIt != element.end() && sizeIt->is_number_integer())
            {
                if (sizeIt->get<int64_t>() < 0)
                {
                    Log::Error() << "Manifest serialization: negative size for file '" << path.value_or("UNKNOWN") << "'.";
                    return false;
                }
                else
                {
                    size = sizeIt->get<uint64_t>();
                }
            }
            else
            {
                Log::Error() << "Manifest serialization: failed to load size for file '" << path.value_or("UNKNOWN") << "'.";
                return false;
            }

            if (path.has_value() && hash.has_value() && size.has_value())
            {
                m_ManifestData.emplace(path.value(), ManifestEntry(path.value(), hash.value(), size.value()));
            }
            else
            {
                Log::Error() << "Manifest serialization failed for file '" << path.value_or("UNKNOWN") << "'.";
                return false;
            }
        }
    }

    Log::Info() << "Manifest file loaded with " << m_ManifestData.size() << " entries.";
    return true;
}

bool Manifest::ContainsEntry(const std::string& path) const
{
    return (m_ManifestData.find(path) != m_ManifestData.end());
}

const ManifestEntry* Manifest::GetEntry(const std::string& path) const
{
    auto it = m_ManifestData.find(path);
    if (it == m_ManifestData.end())
    {
        return nullptr;
    }
    else
    {
        return &it->second;
    }
}

const ManifestData& Manifest::GetEntries() const
{
    return m_ManifestData;
}

} // namespace WingsOfSteel::Private

#endif // TARGET_PLATFORM_WEB
