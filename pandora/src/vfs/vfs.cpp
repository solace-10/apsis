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
    m_pImpl = std::make_unique<Private::VFSNative>();
#elif defined(TARGET_PLATFORM_WEB)
    m_pImpl = std::make_unique<Private::VFSWeb>();
#endif
}
    
VFS::~VFS()
{
}

void VFS::Initialize()
{
    m_pImpl->Initialize();
}

void VFS::Update()
{
    m_pImpl->Update();
}

bool VFS::FileExists(const std::string& path) const
{
    return m_pImpl->FileExists(path);
}
    
void VFS::FileRead(const std::string& path, FileReadCallback onFileReadCompleted)
{
    m_pImpl->FileRead(path, onFileReadCompleted);
}

} // namespace WingsOfSteel::Pandora