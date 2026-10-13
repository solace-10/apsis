#if defined(TARGET_PLATFORM_WEB)

#include "vfs/private/web/vfs_web.hpp"

#include <cassert>
#include <iomanip>
#include <sstream>
#include <vector>

#include <emscripten/fetch.h>
#include <xxhash.h>

#include "core/log.hpp"
#include "vfs/file.hpp"
#include "vfs/private/web/manifest.hpp"
#include "vfs/private/web/manifest_entry.hpp"

namespace WingsOfSteel::Private
{

VFSWeb::VFSWeb()
{
    Log::Info() << "Creating VFS web backend...";
}

VFSWeb::~VFSWeb()
{
}

void VFSWeb::Initialize()
{
    Log::Info() << "Initializing VFS web backend.";
    m_pManifest = std::make_unique<Manifest>();
    m_pManifest->Initialize();
}

void VFSWeb::Update()
{
    if (!m_Queue.empty() && !m_InProgress.has_value())
    {
        QueuedFile& queuedFile = m_Queue.front();
        m_InProgress = queuedFile;
        m_Queue.pop_front();

        std::stringstream url;
        url << VFS_WEB_HOST << m_InProgress->pManifestEntry->GetHash();
        Log::Info() << "Downloading '" << m_InProgress->path << "' from '" << url.str() << "'...";

        emscripten_fetch_attr_t attr;
        emscripten_fetch_attr_init(&attr);
        strcpy(attr.requestMethod, "GET");
        attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_PERSIST_FILE;
        attr.userData = this;
        attr.onsuccess = [](emscripten_fetch_t* pFetch) {
            VFSWeb* pVFS = reinterpret_cast<VFSWeb*>(pFetch->userData);
            std::stringstream downloadHash;
            downloadHash << std::hex << std::setfill('0') << std::setw(16) << XXH3_64bits(pFetch->data, pFetch->numBytes * sizeof(char));
            const std::string& manifestHash = pVFS->m_InProgress->pManifestEntry->GetHash();

            const std::string& path = pVFS->m_InProgress->path;
            if (manifestHash == downloadHash.str())
            {
                FileData fileData;
                fileData.resize(pFetch->numBytes);
                std::memcpy(fileData.data(), pFetch->data, pFetch->numBytes * sizeof(char));

                Log::Info() << "Downloaded '" << path << "'.";
                pVFS->m_InProgress->onFileReadCompleted(FileReadResult::Ok, std::make_shared<File>(path, std::move(fileData)));
            }
            else
            {
                Log::Error() << "Download '" << path << "' failed due to mismatched hashes. Expected " << manifestHash << ", got " << downloadHash.str() << ".";
                pVFS->m_InProgress->onFileReadCompleted(FileReadResult::ErrorHashMismatch, nullptr);
            }

            pVFS->m_InProgress.reset();
            emscripten_fetch_close(pFetch);
        };
        attr.onerror = [](emscripten_fetch_t* pFetch) {
            VFSWeb* pVFS = reinterpret_cast<VFSWeb*>(pFetch->userData);
            assert(pVFS->m_InProgress.has_value());
            Log::Error() << "Failed to download " << pVFS->m_InProgress->path;
            pVFS->m_InProgress->onFileReadCompleted(FileReadResult::ErrorGeneric, nullptr);
            pVFS->m_InProgress.reset();
            emscripten_fetch_close(pFetch);
        };
        emscripten_fetch(&attr, url.str().c_str());
    }
}

void VFSWeb::FileRead(const std::string& path, FileReadCallback onFileReadCompleted)
{
    const ManifestEntry* pManifestEntry = m_pManifest->GetEntry(path);
    if (!pManifestEntry)
    {
        onFileReadCompleted(FileReadResult::ErrorFileNotFound, nullptr);
        return;
    }

    QueuedFile queuedFile;
    queuedFile.path = path;
    queuedFile.pManifestEntry = pManifestEntry;
    queuedFile.onFileReadCompleted = onFileReadCompleted;
    m_Queue.push_back(queuedFile);
}

bool VFSWeb::FileWrite(const std::string& path, const std::vector<uint8_t>& bytes)
{
    Log::Error() << "VFSWeb::FileWrite: Unsupported operation.";
    return false;
}

bool VFSWeb::Exists(const std::string& path) const
{
    return m_pManifest->ContainsEntry(path);
}

const std::vector<std::string> VFSWeb::List(const std::string& path) const
{
    const auto& manifestEntries = m_pManifest->GetEntries();
    std::vector<std::string> files;
    for (auto& entryPair : manifestEntries)
    {
        const std::string& entryKey = entryPair.first;
        if (entryKey.starts_with(path))
        {
            files.push_back(entryKey);
        }
    }
    return files;
}

} // namespace WingsOfSteel::Private

#endif // TARGET_PLATFORM_WEB
