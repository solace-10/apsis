#pragma once

#if defined(TARGET_PLATFORM_NATIVE)

#include "vfs/private/vfs_impl.hpp"

namespace WingsOfSteel::Pandora::Private
{

class VFSNative : public VFSImpl
{
public:
    VFSNative();
    ~VFSNative();

    virtual bool FileExists(const std::string& path) const override;
    virtual void FileRead(const std::string& path, FileReadCallback onFileReadCompleted) override;
};

} // namespace WingsOfSteel::Pandora::Private

#endif // TARGET_PLATFORM_NATIVE