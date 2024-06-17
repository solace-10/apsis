#include "vfs/vfs.hpp"
#include "core/log.hpp"

#if defined(TARGET_PLATFORM_NATIVE)
    #include "vfs/private/native/vfs_native.hpp"
#elif defined(TARGET_PLATFORM_WEB)
    #include "vfs/private/web/vfs_web.hpp"
#endif

namespace WingsOfSteel::Pandora
{

VFS::VFS()
{
#if defined(TARGET_PLATFORM_NATIVE)
    m_pImpl = new Private::VFSNative();
#elif defined(TARGET_PLATFORM_WEB)
    m_pImpl = new Private::VFSWeb();
#endif
    Log::Info() << "Impl new: " << m_pImpl;
}
    
VFS::~VFS()
{
    delete m_pImpl;
    m_pImpl = nullptr;
}

bool VFS::FileExists(const std::string& path) const
{
    return m_pImpl->FileExists(path);
}
    
void VFS::FileRead(const std::string& path, FileReadCallback onFileReadCompleted)
{
    Log::Info() << "FileRead";
    Log::Info() << "This: " << this;
    Log::Info() << "Impl: " << m_pImpl;
    m_pImpl->FileRead(path, onFileReadCompleted);
}

} // namespace WingsOfSteel::Pandora