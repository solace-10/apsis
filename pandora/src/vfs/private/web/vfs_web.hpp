#pragma once

#if defined(TARGET_PLATFORM_WEB)

#include "vfs/private/vfs_impl.hpp"

namespace WingsOfSteel::Pandora::Private
{

class VFSWeb : public VFSImpl
{
public:
    VFSWeb();
    ~VFSWeb();

    virtual bool FileExists(const std::string& path) const override;
    virtual void FileRead(const std::string& path, FileReadCallback onFileReadCompleted) override;
};

} // namespace WingsOfSteel::Pandora::Private

#endif // TARGET_PLATFORM_WEB