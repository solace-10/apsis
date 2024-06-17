#if defined(TARGET_PLATFORM_WEB)

#include "vfs/private/web/vfs_web.hpp"

#include <cassert>
#include <vector>

#include <emscripten/fetch.h>

#include "core/log.hpp"
#include "vfs/file.hpp"

namespace WingsOfSteel::Pandora::Private
{

void downloadSucceeded(emscripten_fetch_t *fetch) 
{
    Log::Info() << "Finished downloading " << fetch->numBytes << " bytes from URL " << fetch->url;
    // The data is now available at fetch->data[0] through fetch->data[fetch->numBytes-1];
    emscripten_fetch_close(fetch); // Free data associated with the fetch.
}

void downloadFailed(emscripten_fetch_t *fetch) 
{
    Log::Info() << fetch->url << "Downloading %s failed, HTTP failure status code: " << fetch->status;
    emscripten_fetch_close(fetch); // Also free data on failure.
}

VFSWeb::VFSWeb()
{

}

VFSWeb::~VFSWeb()
{

}

bool VFSWeb::FileExists(const std::string& path) const
{
    return false;
}
    
void VFSWeb::FileRead(const std::string& path, FileReadCallback onFileReadCompleted)
{
    Log::Info() << "This: " << this;
    // emscripten_fetch_attr_t attr;
    // emscripten_fetch_attr_init(&attr);
    // strcpy(attr.requestMethod, "GET");
    // attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    // attr.onsuccess = downloadSucceeded;
    // attr.onerror = downloadFailed;
    // emscripten_fetch(&attr, "https://solace-10.dev/thebrighteststar/manifest.json");
}

} // namespace WingsOfSteel::Pandora::Private

#endif // TARGET_PLATFORM_WEB