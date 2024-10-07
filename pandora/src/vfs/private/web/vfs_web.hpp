#pragma once

#if defined(TARGET_PLATFORM_WEB)

#include <list>
#include <memory>
#include <optional>

#include "vfs/private/vfs_impl.hpp"

namespace WingsOfSteel::Pandora::Private
{

class Manifest;
class ManifestEntry;

class VFSWeb : public VFSImpl
{
public:
    VFSWeb();
    ~VFSWeb();

    void Initialize() override;
    void Update() override;
    void FileRead(const std::string& path, FileReadCallback onFileReadCompleted) override;
    bool FileWrite(const std::string& path, const std::vector<uint8_t>& bytes) override;

private:
    std::unique_ptr<Manifest> m_pManifest;

    struct QueuedFile
    {
        std::string path;
        ManifestEntry* pManifestEntry;
        FileReadCallback onFileReadCompleted;
    };
    std::list<QueuedFile> m_Queue;
    std::optional<QueuedFile> m_InProgress;
};

} // namespace WingsOfSteel::Pandora::Private

#endif // TARGET_PLATFORM_WEB