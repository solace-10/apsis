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

    virtual void Initialize() override;
    virtual void Update() override;
    virtual void FileRead(const std::string& path, FileReadCallback onFileReadCompleted) override;

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