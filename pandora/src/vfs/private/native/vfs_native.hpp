#pragma once

#if defined(TARGET_PLATFORM_NATIVE)

#include <filesystem>
#include <string>
#include <unordered_map>

#include "vfs/private/vfs_impl.hpp"

namespace WingsOfSteel::Pandora::Private
{

class VFSNative : public VFSImpl
{
public:
    VFSNative();
    ~VFSNative();

    void Initialize() override;
    void Update() override;
    void FileRead(const std::string& path, FileReadCallback onFileReadCompleted) override;
    bool Exists(const std::string& path) const override;
    const std::vector<std::string> List(const std::string& path) const override;

private:
    void BuildVFS();

    std::unordered_map<std::string, std::filesystem::path> m_VFS;
};

} // namespace WingsOfSteel::Pandora::Private

#endif // TARGET_PLATFORM_NATIVE