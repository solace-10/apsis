#if defined(TARGET_PLATFORM_WEB)

#include <sstream>

#include <emscripten/fetch.h>
#include <nlohmann/json.hpp>

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

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.userData = this;
    attr.onsuccess = [](emscripten_fetch_t* pFetch)
    {
        Manifest* pManifest = reinterpret_cast<Manifest*>(pFetch->userData);
        pManifest->OnDownloadSucceeded(pFetch->url, pFetch->data, pFetch->numBytes);
        emscripten_fetch_close(pFetch);
    };
    attr.onerror = [](emscripten_fetch_t* pFetch)
    {
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

}

void Manifest::OnDownloadFailed(const std::string& url, int statusCode)
{

}

} // namespace WingsOfSteel::Pandora::Private

#endif // TARGET_PLATFORM_WEB