#pragma once

#if defined(TARGET_PLATFORM_WEB)

#include <list>
#include <memory>
#include <optional>

#include "vfs/private/vfs_impl.hpp"

namespace WingsOfSteel::Pandora::Private
{

class Manifest;

class VFSWeb : public VFSImpl
{
public:
    VFSWeb();
    ~VFSWeb();

    virtual void Initialize() override;
    virtual void Update() override;
    virtual bool FileExists(const std::string& path) const override;
    virtual void FileRead(const std::string& path, FileReadCallback onFileReadCompleted) override;

private:
    void PumpQueue();

    std::unique_ptr<Manifest> m_pManifest;

    struct QueuedFile
    {
        std::string path;
        FileReadCallback onFileReadCompleted;
    };
    std::list<QueuedFile> m_Queue;
    std::optional<QueuedFile> m_InProgress;
};

} // namespace WingsOfSteel::Pandora::Private

#endif // TARGET_PLATFORM_WEB