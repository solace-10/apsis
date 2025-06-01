#if defined(TARGET_PLATFORM_WEB)

#include <sstream>

#include <emscripten/fetch.h>
#include <nlohmann/json.hpp>

#include "core/log.hpp"
#include "vfs/private/web/manifest.hpp"
#include "vfs/private/web/manifest_entry.hpp"

namespace WingsOfSteel::Pandora::Private
{

Manifest::Manifest()
    : m_IsValid(false)
{
}

Manifest::~Manifest()
{
}

void Manifest::Initialize()
{
    std::stringstream url;
    url << VFS_WEB_HOST << "/data/core/manifest.json";
    Log::Info() << "Downloading manifest from '" << url.str() << "'.";

    emscripten_fetch_attr_t attr;

    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_REPLACE;
    attr.userData = this;
    attr.onsuccess = [](emscripten_fetch_t* pFetch) {
        Manifest* pManifest = reinterpret_cast<Manifest*>(pFetch->userData);
        pManifest->OnDownloadSucceeded(pFetch->url, pFetch->data, pFetch->numBytes);
        emscripten_fetch_close(pFetch);
    };
    attr.onerror = [](emscripten_fetch_t* pFetch) {
        Manifest* pManifest = reinterpret_cast<Manifest*>(pFetch->userData);
        pManifest->OnDownloadFailed(pFetch->url, pFetch->status);
        emscripten_fetch_close(pFetch);
    };
    emscripten_fetch(&attr, url.str().c_str());
}

bool Manifest::IsValid() const
{
    return m_IsValid;
}

ManifestEntry* Manifest::GetEntry(const std::string& path) const
{
    auto it = m_ManifestData.find(path);
    if (it == m_ManifestData.end())
    {
        return nullptr;
    }
    else
    {
        return it->second.get();
    }
}

void Manifest::OnDownloadSucceeded(const std::string& url, const char* pData, size_t dataSize)
{
    using namespace nlohmann;

    Log::Info() << "Manifest download completed (" << dataSize << " bytes)";

    const std::string contents = std::string(pData, dataSize);
    const json jsonContents = json::parse(contents);

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
                Log::Warning() << "Manifest serialization: failed to load an element's path.";
            }

            auto hashIt = element.find("hash");
            if (hashIt != element.end() && hashIt->is_string())
            {
                hash = hashIt->get<std::string>();
            }
            else
            {
                Log::Warning() << "Manifest serialization: failed to load hash for file '" << path.value_or("UNKNOWN") << "'.";
            }

            auto sizeIt = element.find("size");
            if (sizeIt != element.end() && sizeIt->is_number_integer())
            {
                if (sizeIt->get<int64_t>() < 0)
                {
                    Log::Warning() << "Manifest serialization: negative size for file '" << path.value_or("UNKNOWN") << "'.";
                }
                else
                {
                    size = sizeIt->get<uint64_t>();
                }
            }
            else
            {
                Log::Warning() << "Manifest serialization: failed to load size for file '" << path.value_or("UNKNOWN") << "'.";
            }

            if (path.has_value() && hash.has_value() && size.has_value())
            {
                m_ManifestData[path.value()] = std::make_unique<ManifestEntry>(path.value(), hash.value(), size.value());
            }
            else
            {
                Log::Error() << "Manifest serialization failed for file '" << path.value_or("UNKNOWN") << "'.";
                exit(-1);
            }
        }
    }

    Log::Info() << "Manifest loaded (" << m_ManifestData.size() << " entries).";
    m_IsValid = true;
}

void Manifest::OnDownloadFailed(const std::string& url, int statusCode)
{
    Log::Error() << "Failed to download manifest: " << statusCode;
}

} // namespace WingsOfSteel::Pandora::Private

#endif // TARGET_PLATFORM_WEB